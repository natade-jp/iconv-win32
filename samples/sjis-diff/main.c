#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iconv.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
TSV columns:
code        bytes_hex
win_ok  win_cp   win_char   win_rt_ok  win_rt_hex
ic_ok   ic_cp    ic_char    ic_rt_ok   ic_rt_hex
diff_flags

diff_flags (bitset):
 1  decode_ok differs (win_ok != ic_ok)
 2  both ok but codepoint differs
 4  roundtrip differs (win_rt_ok vs ic_rt_ok)
*/

static int is_valid_sjis_2byte(uint8_t lead, uint8_t trail) {
    int lead_ok = ((0x81 <= lead && lead <= 0x9F) || (0xE0 <= lead && lead <= 0xFC));
    int trail_ok = ((0x40 <= trail && trail <= 0x7E) || (0x80 <= trail && trail <= 0xFC));
    return lead_ok && trail_ok;
}

static void bytes_to_hex(const uint8_t* b, size_t n, char* out, size_t out_cap) {
    size_t pos = 0;
    for (size_t i = 0; i < n; i++) {
        int w = snprintf(out + pos, (pos < out_cap ? out_cap - pos : 0),
            (i + 1 < n) ? "%02X " : "%02X", b[i]);
        if (w < 0) break;
        pos += (size_t)w;
        if (pos >= out_cap) break;
    }
    if (out_cap) out[out_cap - 1] = '\0';
}

static uint32_t utf16_to_codepoint(const WCHAR* w, int wlen, int* is_surrogate_like) {
    *is_surrogate_like = 0;
    if (wlen <= 0) return 0;
    if (wlen == 1) return (uint32_t)w[0];

    uint32_t hi = (uint32_t)w[0];
    uint32_t lo = (uint32_t)w[1];
    if (0xD800 <= hi && hi <= 0xDBFF && 0xDC00 <= lo && lo <= 0xDFFF) {
        *is_surrogate_like = 1;
        return (((hi - 0xD800) << 10) | (lo - 0xDC00)) + 0x10000;
    }
    return (uint32_t)w[0];
}

/* Windows: bytes(CP932) -> codepoint, strict */
static int win_decode_cp932(const uint8_t* bytes, int len, uint32_t* cp_out, int* is_surrogate_like) {
    WCHAR wbuf[4] = { 0 };
    int wlen = MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, (LPCCH)bytes, len, wbuf, 4);
    if (wlen <= 0) return 0;
    *cp_out = utf16_to_codepoint(wbuf, wlen, is_surrogate_like);
    return 1;
}

/* Windows: codepoint -> bytes(CP932), strict */
static int win_encode_cp932(uint32_t cp, uint8_t* out, int out_cap, int* out_len) {
    WCHAR wbuf[2] = { 0 };
    int wlen = 0;

    if (cp <= 0xFFFF) {
        wbuf[0] = (WCHAR)cp;
        wlen = 1;
    }
    else {
        uint32_t v = cp - 0x10000;
        wbuf[0] = (WCHAR)(0xD800 + (v >> 10));
        wbuf[1] = (WCHAR)(0xDC00 + (v & 0x3FF));
        wlen = 2;
    }

    char mb[8] = { 0 };
    BOOL usedDefault = FALSE;
    int mblen = WideCharToMultiByte(932, WC_NO_BEST_FIT_CHARS, wbuf, wlen, mb, (int)sizeof(mb),
        NULL, &usedDefault);

    if (mblen <= 0) return 0;
    if (usedDefault) return 0; // strict: treat best-fit/default as failure
    if (mblen > out_cap) return 0;

    memcpy(out, mb, mblen);
    *out_len = mblen;
    return 1;
}

/* iconv: bytes(SHIFT_JIS) -> codepoint via UTF-32LE */
static int iconv_decode_sjis(const uint8_t* bytes, size_t len, uint32_t* cp_out) {
    iconv_t cd = iconv_open("UTF-32LE", "SHIFT_JIS");
    if (cd == (iconv_t)-1) return 0;

    uint8_t out[16] = { 0 };
    char* inbuf = (char*)bytes;
    char* outbuf = (char*)out;
    size_t inleft = len;
    size_t outleft = sizeof(out);

    iconv(cd, NULL, NULL, NULL, NULL);
    size_t r = iconv(cd, &inbuf, &inleft, &outbuf, &outleft);
    iconv_close(cd);

    if (r == (size_t)-1) return 0;

    size_t out_len = sizeof(out) - outleft;
    if (out_len < 4) return 0;

    uint32_t cp = (uint32_t)out[0] |
        ((uint32_t)out[1] << 8) |
        ((uint32_t)out[2] << 16) |
        ((uint32_t)out[3] << 24);

    *cp_out = cp;
    return 1;
}

/* iconv: codepoint -> bytes(SHIFT_JIS) via UTF-32LE */
static int iconv_encode_sjis(uint32_t cp, uint8_t* out, size_t out_cap, size_t* out_len) {
    iconv_t cd = iconv_open("SHIFT_JIS", "UTF-32LE");
    if (cd == (iconv_t)-1) return 0;

    uint8_t in32[4];
    in32[0] = (uint8_t)(cp & 0xFF);
    in32[1] = (uint8_t)((cp >> 8) & 0xFF);
    in32[2] = (uint8_t)((cp >> 16) & 0xFF);
    in32[3] = (uint8_t)((cp >> 24) & 0xFF);

    memset(out, 0, out_cap);
    char* inbuf = (char*)in32;
    char* outbuf = (char*)out;
    size_t inleft = 4;
    size_t outleft = out_cap;

    iconv(cd, NULL, NULL, NULL, NULL);
    size_t r = iconv(cd, &inbuf, &inleft, &outbuf, &outleft);
    iconv_close(cd);

    if (r == (size_t)-1) return 0;
    *out_len = out_cap - outleft;
    return 1;
}

/* codepoint -> UTF-8 string (for TSV) */
static int codepoint_to_utf8(uint32_t cp, char* out, size_t out_cap) {
    if (out_cap == 0) return 0;
    out[0] = '\0';

    // NUL‚ÍTSV‚É“ü‚ê‚é‚Æ‰ó‚ê‚é‚Ì‚Å‹ó‚É‚·‚é
    if (cp == 0) return 1;

    // Surrogate code points are invalid as scalar values
    if (0xD800 <= cp && cp <= 0xDFFF) return 0;
    if (cp > 0x10FFFF) return 0;

    WCHAR wbuf[2];
    int wlen = 0;

    if (cp <= 0xFFFF) {
        wbuf[0] = (WCHAR)cp;
        wlen = 1;
    }
    else {
        uint32_t v = cp - 0x10000;
        wbuf[0] = (WCHAR)(0xD800 + (v >> 10));
        wbuf[1] = (WCHAR)(0xDC00 + (v & 0x3FF));
        wlen = 2;
    }

    int n = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, out, (int)out_cap - 1, NULL, NULL);
    if (n <= 0) return 0;
    out[n] = '\0';
    return 1;
}

/* Minimal TSV escape: tab/newline/carriage return */
static void escape_tsv_inplace(char* s) {
    char* r = s;
    char* w = s;
    while (*r) {
        unsigned char c = (unsigned char)*r++;
        if (c == '\t') { *w++ = '\\'; *w++ = 't'; }
        else if (c == '\n') { *w++ = '\\'; *w++ = 'n'; }
        else if (c == '\r') { *w++ = '\\'; *w++ = 'r'; }
        else { *w++ = (char)c; }
    }
    *w = '\0';
}

int main(void) {
    FILE* fp = fopen("diff_sjis_cp932.tsv", "wb");
    if (!fp) { perror("fopen"); return 1; }

    // UTF-8 BOM (Excel‘ÎôB•s—v‚È‚çí‚Á‚ÄOK)
    fputc(0xEF, fp); fputc(0xBB, fp); fputc(0xBF, fp);

    fprintf(fp,
        "code\tbytes_hex\t"
        "win_ok\twin_cp\twin_char\twin_rt_ok\twin_rt_hex\t"
        "ic_ok\tic_cp\tic_char\tic_rt_ok\tic_rt_hex\t"
        "diff_flags\n"
    );

    for (uint32_t code = 1; code <= 0xFFFF; code++) {
        uint8_t bytes[2];
        int blen = 0;

        if (code < 0x100) {
            if (code <= 0x20) continue; // like HSP
            bytes[0] = (uint8_t)code;
            blen = 1;
        }
        else {
            uint8_t lead = (uint8_t)(code >> 8);
            uint8_t trail = (uint8_t)(code & 0xFF);
            if (!is_valid_sjis_2byte(lead, trail)) continue;
            bytes[0] = lead;
            bytes[1] = trail;
            blen = 2;
        }

        char bytes_hex[32] = { 0 };
        bytes_to_hex(bytes, (size_t)blen, bytes_hex, sizeof(bytes_hex));

        // Windows decode/encode
        int win_ok = 0, win_sur = 0;
        uint32_t win_cp = 0;
        uint8_t win_rt[8] = { 0 };
        int win_rt_len = 0;
        int win_rt_ok = 0;

        win_ok = win_decode_cp932(bytes, blen, &win_cp, &win_sur);
        if (win_ok) {
            win_rt_ok = win_encode_cp932(win_cp, win_rt, (int)sizeof(win_rt), &win_rt_len);
        }

        // iconv decode/encode
        int ic_ok = 0;
        uint32_t ic_cp = 0;
        uint8_t ic_rt[8] = { 0 };
        size_t ic_rt_len = 0;
        int ic_rt_ok = 0;

        ic_ok = iconv_decode_sjis(bytes, (size_t)blen, &ic_cp);
        if (ic_ok) {
            ic_rt_ok = iconv_encode_sjis(ic_cp, ic_rt, sizeof(ic_rt), &ic_rt_len);
        }

        char win_rt_hex[32] = { 0 };
        char ic_rt_hex[32] = { 0 };
        if (win_rt_ok) bytes_to_hex(win_rt, (size_t)win_rt_len, win_rt_hex, sizeof(win_rt_hex));
        if (ic_rt_ok)  bytes_to_hex(ic_rt, ic_rt_len, ic_rt_hex, sizeof(ic_rt_hex));

        // Unicode character columns (UTF-8)
        char win_char[32] = { 0 };
        char ic_char[32] = { 0 };

        if (win_ok) {
            if (!codepoint_to_utf8(win_cp, win_char, sizeof(win_char))) win_char[0] = '\0';
            escape_tsv_inplace(win_char);
        }
        if (ic_ok) {
            if (!codepoint_to_utf8(ic_cp, ic_char, sizeof(ic_char))) ic_char[0] = '\0';
            escape_tsv_inplace(ic_char);
        }

        int diff = 0;
        if (win_ok != ic_ok) diff |= 1;
        if (win_ok && ic_ok && win_cp != ic_cp) diff |= 2;

        int win_back = (win_rt_ok && win_rt_len == blen && memcmp(win_rt, bytes, blen) == 0);
        int ic_back = (ic_rt_ok && ic_rt_len == (size_t)blen && memcmp(ic_rt, bytes, blen) == 0);
        if (win_back != ic_back) diff |= 4;

        fprintf(fp,
            "0x%04X\t%s\t"
            "%d\t0x%08X\t%s\t%d\t%s\t"
            "%d\t0x%08X\t%s\t%d\t%s\t"
            "%d\n",
            (unsigned)code,
            bytes_hex,
            win_ok, (unsigned)win_cp, win_ok ? win_char : "", win_back, win_rt_ok ? win_rt_hex : "",
            ic_ok, (unsigned)ic_cp, ic_ok ? ic_char : "", ic_back, ic_rt_ok ? ic_rt_hex : "",
            diff
        );
    }

    fclose(fp);
    printf("Saved: diff_sjis_cp932.tsv\n");
    return 0;
}