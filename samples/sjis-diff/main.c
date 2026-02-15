#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iconv.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nkf32.h"

/*
出力TSV列（日本語）:
入力コード
入力バイト列(16進)

Windows_CP932:デコード成功
Windows_CP932:Unicodeコードポイント
Windows_CP932:文字
Windows_CP932:元に戻る
Windows_CP932:再エンコード結果(16進)

ライブラリ利用:デコード成功
ライブラリ利用:Unicodeコードポイント
ライブラリ利用:文字
ライブラリ利用:元に戻る
ライブラリ利用:再エンコード結果(16進)

ライブラリ利用→Windows_CP932:エンコード可能
ライブラリ利用→Windows_CP932:元に戻る
ライブラリ利用→Windows_CP932:再エンコード結果(16進)

差分フラグ(ビット値)
差分内容

diff_flags (bitset):  ※Windows vs ライブラリ利用
 1  デコード可否差 (win_ok != lib_ok)
 2  Unicode差 (win_ok && lib_ok && win_cp != lib_cp)
 4  元に戻る差 (win_back != lib_back)
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

/* Windows: codepoint -> bytes(CP932), strict (no best-fit) */
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
    if (usedDefault) return 0;
    if (mblen > out_cap) return 0;

    memcpy(out, mb, mblen);
    *out_len = mblen;
    return 1;
}

/* codepoint -> UTF-8 string (for TSV) */
static int codepoint_to_utf8(uint32_t cp, char* out, size_t out_cap) {
    if (out_cap == 0) return 0;
    out[0] = '\0';

    // NULはTSVを壊すので空にする
    if (cp == 0) return 1;

    // Surrogates are invalid scalar values
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

/* UTF-8 bytes -> codepoint, strict */
static int win_decode_utf8(const uint8_t* bytes, int len, uint32_t* cp_out) {
    WCHAR wbuf[4] = { 0 };
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH)bytes, len, wbuf, 4);
    if (wlen <= 0) return 0;
    int dummy = 0;
    *cp_out = utf16_to_codepoint(wbuf, wlen, &dummy);
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

static void write_bom_utf8(FILE* fp) {
    fputc(0xEF, fp); fputc(0xBB, fp); fputc(0xBF, fp);
}

static void diff_to_text(int diff, char* out, size_t cap) {
    out[0] = '\0';
    int first = 1;

    if (diff & 1) {
        strncat(out, first ? "デコード可否差" : "|デコード可否差", cap - strlen(out) - 1);
        first = 0;
    }
    if (diff & 2) {
        strncat(out, first ? "Unicode差" : "|Unicode差", cap - strlen(out) - 1);
        first = 0;
    }
    if (diff & 4) {
        strncat(out, first ? "元に戻る差" : "|元に戻る差", cap - strlen(out) - 1);
        first = 0;
    }
    if (first) {
        strncat(out, "差分なし", cap - strlen(out) - 1);
    }
}

/* ---------------- iconv (library) ---------------- */

/* iconv: bytes(ENC) -> codepoint via UTF-32LE */
static int iconv_decode_any(const char* enc, const uint8_t* bytes, size_t len, uint32_t* cp_out) {
    iconv_t cd = iconv_open("UTF-32LE", enc);
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

/* iconv: codepoint -> bytes(ENC) via UTF-32LE */
static int iconv_encode_any(const char* enc, uint32_t cp, uint8_t* out, size_t out_cap, size_t* out_len) {
    iconv_t cd = iconv_open(enc, "UTF-32LE");
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

/* ---------------- nkf (library, --ic/--oc) ---------------- */

/*
  nkf convert wrapper:
    --ic=<in_codeset> --oc=<out_codeset>
  ※nkf32.dll が長いオプションを受け付ける前提
*/
static int nkf_convert_ic_oc(
    const char* ic, const char* oc,
    const uint8_t* in, int in_len,
    uint8_t* out, DWORD out_cap,
    DWORD* out_len
) {
    char opt[128];
    snprintf(opt, sizeof(opt), "--ic=%s --oc=%s", ic, oc);

    if (SetNkfOption(opt) < 0) return 0;

    DWORD produced = 0;
    BOOL ok = NkfConvertSafe((LPSTR)out, out_cap, &produced, (LPCSTR)in, (DWORD)in_len);
    if (!ok) return 0;
    if (produced >= out_cap) return 0;

    *out_len = produced;
    return 1;
}

/* nkf: bytes(<enc>) -> codepoint (via UTF-8 -> Windows) */
static int nkf_decode_to_codepoint(const char* nkf_enc, const uint8_t* bytes, int len, uint32_t* cp_out) {
    uint8_t u8buf[64] = { 0 };
    DWORD u8len = 0;

    if (!nkf_convert_ic_oc(nkf_enc, "UTF-8", bytes, len, u8buf, (DWORD)sizeof(u8buf), &u8len)) return 0;
    if (u8len == 0) return 0;

    return win_decode_utf8(u8buf, (int)u8len, cp_out);
}

/* nkf: codepoint -> bytes(<enc>) (via UTF-8 string -> nkf) */
static int nkf_encode_from_codepoint(const char* nkf_enc, uint32_t cp, uint8_t* out, int out_cap, int* out_len) {
    char u8str[32] = { 0 };
    if (!codepoint_to_utf8(cp, u8str, sizeof(u8str))) return 0;

    uint8_t buf[32] = { 0 };
    DWORD blen = 0;
    int in_len = (int)strlen(u8str);

    if (!nkf_convert_ic_oc("UTF-8", nkf_enc, (const uint8_t*)u8str, in_len, buf, (DWORD)sizeof(buf), &blen)) return 0;

    if ((int)blen > out_cap) return 0;
    memcpy(out, buf, (size_t)blen);
    *out_len = (int)blen;
    return 1;
}

/* ---------------- common TSV header ---------------- */

static void write_tsv_header(FILE* fp) {
    fprintf(fp,
        "入力コード\t入力バイト列(16進)\t"
        "Windows_CP932:デコード成功\tWindows_CP932:Unicodeコードポイント\tWindows_CP932:文字\tWindows_CP932:元に戻る\tWindows_CP932:再エンコード結果(16進)\t"
        "ライブラリ利用:デコード成功\tライブラリ利用:Unicodeコードポイント\tライブラリ利用:文字\tライブラリ利用:元に戻る\tライブラリ利用:再エンコード結果(16進)\t"
        "ライブラリ利用→Windows_CP932:エンコード可能\tライブラリ利用→Windows_CP932:元に戻る\tライブラリ利用→Windows_CP932:再エンコード結果(16進)\t"
        "差分フラグ(ビット値)\t差分内容\n"
    );
}

/* ---------------- case runners ---------------- */

static void run_case_iconv(const char* iconv_enc, const char* out_path) {
    FILE* fp = fopen(out_path, "wb");
    if (!fp) { perror("fopen"); return; }

    write_bom_utf8(fp);
    fprintf(fp, "# Windows: CP932, Library: iconv (%s)\n", iconv_enc);
    write_tsv_header(fp);

    int lib_available = 1;
    {
        iconv_t cd = iconv_open("UTF-32LE", iconv_enc);
        if (cd == (iconv_t)-1) lib_available = 0;
        else iconv_close(cd);
    }

    for (uint32_t code = 1; code <= 0xFFFF; code++) {
        uint8_t bytes[2];
        int blen = 0;

        if (code < 0x100) {
            if (code <= 0x20) continue;
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

        // Windows decode/encode (CP932)
        int win_ok = 0, win_sur = 0;
        uint32_t win_cp = 0;
        uint8_t win_rt[8] = { 0 };
        int win_rt_len = 0;
        int win_rt_ok = 0;

        win_ok = win_decode_cp932(bytes, blen, &win_cp, &win_sur);
        if (win_ok) win_rt_ok = win_encode_cp932(win_cp, win_rt, (int)sizeof(win_rt), &win_rt_len);
        int win_back = (win_rt_ok && win_rt_len == blen && memcmp(win_rt, bytes, blen) == 0);

        char win_rt_hex[32] = { 0 };
        if (win_rt_ok) bytes_to_hex(win_rt, (size_t)win_rt_len, win_rt_hex, sizeof(win_rt_hex));

        // Library (iconv) decode/encode
        int lib_ok = 0;
        uint32_t lib_cp = 0;
        uint8_t lib_rt[8] = { 0 };
        size_t lib_rt_len = 0;
        int lib_rt_ok = 0;

        if (lib_available) {
            lib_ok = iconv_decode_any(iconv_enc, bytes, (size_t)blen, &lib_cp);
            if (lib_ok) lib_rt_ok = iconv_encode_any(iconv_enc, lib_cp, lib_rt, sizeof(lib_rt), &lib_rt_len);
        }

        int lib_back = (lib_rt_ok && lib_rt_len == (size_t)blen && memcmp(lib_rt, bytes, blen) == 0);

        char lib_rt_hex[32] = { 0 };
        if (lib_rt_ok) bytes_to_hex(lib_rt, lib_rt_len, lib_rt_hex, sizeof(lib_rt_hex));

        // Library Unicode -> Windows CP932 encode
        int lib_win_ok = 0;
        uint8_t lib_win_rt[8] = { 0 };
        int lib_win_rt_len = 0;

        if (lib_ok) lib_win_ok = win_encode_cp932(lib_cp, lib_win_rt, (int)sizeof(lib_win_rt), &lib_win_rt_len);
        int lib_win_back = (lib_win_ok && lib_win_rt_len == blen && memcmp(lib_win_rt, bytes, blen) == 0);

        char lib_win_rt_hex[32] = { 0 };
        if (lib_win_ok) bytes_to_hex(lib_win_rt, (size_t)lib_win_rt_len, lib_win_rt_hex, sizeof(lib_win_rt_hex));

        // Unicode character columns (UTF-8)
        char win_char[32] = { 0 };
        char lib_char[32] = { 0 };

        if (win_ok) {
            if (!codepoint_to_utf8(win_cp, win_char, sizeof(win_char))) win_char[0] = '\0';
            escape_tsv_inplace(win_char);
        }
        if (lib_ok) {
            if (!codepoint_to_utf8(lib_cp, lib_char, sizeof(lib_char))) lib_char[0] = '\0';
            escape_tsv_inplace(lib_char);
        }

        int diff = 0;
        if (win_ok != lib_ok) diff |= 1;
        if (win_ok && lib_ok && win_cp != lib_cp) diff |= 2;
        if (win_back != lib_back) diff |= 4;

        char diff_text[64];
        diff_to_text(diff, diff_text, sizeof(diff_text));

        fprintf(fp,
            "0x%04X\t%s\t"
            "%d\t0x%08X\t%s\t%d\t%s\t"
            "%d\t0x%08X\t%s\t%d\t%s\t"
            "%d\t%d\t%s\t"
            "%d\t%s\n",
            (unsigned)code, bytes_hex,

            win_ok, (unsigned)win_cp, win_ok ? win_char : "", win_back, win_rt_ok ? win_rt_hex : "",

            lib_ok, (unsigned)lib_cp, lib_ok ? lib_char : "", lib_back, lib_rt_ok ? lib_rt_hex : "",

            lib_win_ok, lib_win_back, lib_win_ok ? lib_win_rt_hex : "",

            diff, diff_text
        );
    }

    fclose(fp);

    if (!lib_available) {
        printf("[WARN] iconv encoding not available: %s (file generated but library側は常に失敗になります)\n", iconv_enc);
    }
    printf("Saved: %s\n", out_path);
}

static void run_case_nkf(const char* nkf_enc, const char* out_path) {
    FILE* fp = fopen(out_path, "wb");
    if (!fp) { perror("fopen"); return; }

    write_bom_utf8(fp);
    fprintf(fp, "# Windows: CP932, Library: nkf (--ic/--oc) (%s)\n", nkf_enc);
    write_tsv_header(fp);

    int lib_available = 1;
    {
        // 軽いプローブ：'A' を <nkf_enc> -> UTF-8 に変換できるか
        const uint8_t in[1] = { 0x41 };
        uint8_t out[8] = { 0 };
        DWORD outLen = 0;
        if (!nkf_convert_ic_oc(nkf_enc, "UTF-8", in, 1, out, (DWORD)sizeof(out), &outLen)) lib_available = 0;
    }

    for (uint32_t code = 1; code <= 0xFFFF; code++) {
        uint8_t bytes[2];
        int blen = 0;

        if (code < 0x100) {
            if (code <= 0x20) continue;
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

        // Windows decode/encode (CP932)
        int win_ok = 0, win_sur = 0;
        uint32_t win_cp = 0;
        uint8_t win_rt[8] = { 0 };
        int win_rt_len = 0;
        int win_rt_ok = 0;

        win_ok = win_decode_cp932(bytes, blen, &win_cp, &win_sur);
        if (win_ok) win_rt_ok = win_encode_cp932(win_cp, win_rt, (int)sizeof(win_rt), &win_rt_len);
        int win_back = (win_rt_ok && win_rt_len == blen && memcmp(win_rt, bytes, blen) == 0);

        char win_rt_hex[32] = { 0 };
        if (win_rt_ok) bytes_to_hex(win_rt, (size_t)win_rt_len, win_rt_hex, sizeof(win_rt_hex));

        // Library (nkf) decode/encode
        int lib_ok = 0;
        uint32_t lib_cp = 0;
        uint8_t lib_rt[8] = { 0 };
        int lib_rt_len = 0;
        int lib_rt_ok = 0;

        if (lib_available) {
            lib_ok = nkf_decode_to_codepoint(nkf_enc, bytes, blen, &lib_cp);
            if (lib_ok) lib_rt_ok = nkf_encode_from_codepoint(nkf_enc, lib_cp, lib_rt, (int)sizeof(lib_rt), &lib_rt_len);
        }

        int lib_back = (lib_rt_ok && lib_rt_len == blen && memcmp(lib_rt, bytes, blen) == 0);

        char lib_rt_hex[32] = { 0 };
        if (lib_rt_ok) bytes_to_hex(lib_rt, (size_t)lib_rt_len, lib_rt_hex, sizeof(lib_rt_hex));

        // Library Unicode -> Windows CP932 encode
        int lib_win_ok = 0;
        uint8_t lib_win_rt[8] = { 0 };
        int lib_win_rt_len = 0;

        if (lib_ok) lib_win_ok = win_encode_cp932(lib_cp, lib_win_rt, (int)sizeof(lib_win_rt), &lib_win_rt_len);
        int lib_win_back = (lib_win_ok && lib_win_rt_len == blen && memcmp(lib_win_rt, bytes, blen) == 0);

        char lib_win_rt_hex[32] = { 0 };
        if (lib_win_ok) bytes_to_hex(lib_win_rt, (size_t)lib_win_rt_len, lib_win_rt_hex, sizeof(lib_win_rt_hex));

        // Unicode character columns (UTF-8)
        char win_char[32] = { 0 };
        char lib_char[32] = { 0 };

        if (win_ok) {
            if (!codepoint_to_utf8(win_cp, win_char, sizeof(win_char))) win_char[0] = '\0';
            escape_tsv_inplace(win_char);
        }
        if (lib_ok) {
            if (!codepoint_to_utf8(lib_cp, lib_char, sizeof(lib_char))) lib_char[0] = '\0';
            escape_tsv_inplace(lib_char);
        }

        int diff = 0;
        if (win_ok != lib_ok) diff |= 1;
        if (win_ok && lib_ok && win_cp != lib_cp) diff |= 2;
        if (win_back != lib_back) diff |= 4;

        char diff_text[64];
        diff_to_text(diff, diff_text, sizeof(diff_text));

        fprintf(fp,
            "0x%04X\t%s\t"
            "%d\t0x%08X\t%s\t%d\t%s\t"
            "%d\t0x%08X\t%s\t%d\t%s\t"
            "%d\t%d\t%s\t"
            "%d\t%s\n",
            (unsigned)code, bytes_hex,

            win_ok, (unsigned)win_cp, win_ok ? win_char : "", win_back, win_rt_ok ? win_rt_hex : "",

            lib_ok, (unsigned)lib_cp, lib_ok ? lib_char : "", lib_back, lib_rt_ok ? lib_rt_hex : "",

            lib_win_ok, lib_win_back, lib_win_ok ? lib_win_rt_hex : "",

            diff, diff_text
        );
    }

    fclose(fp);

    if (!lib_available) {
        printf("[WARN] nkf probe failed for enc=%s (file generated but library側は常に失敗になります)\n", nkf_enc);
    }
    printf("Saved: %s\n", out_path);
}

/* ---------------- main ---------------- */

int main(void) {
    // iconv 系
    run_case_iconv("SHIFT_JIS", "diff_win_cp932_vs_iconv_shift_jis.tsv");
    run_case_iconv("CP932", "diff_win_cp932_vs_iconv_cp932.tsv");
    run_case_iconv("SHIFT_JISX0213", "diff_win_cp932_vs_iconv_shift_jisx0213.tsv");

    // nkf 系
    run_case_nkf("Shift_JIS", "diff_win_cp932_vs_nkf_shift_jis.tsv");
    run_case_nkf("CP932", "diff_win_cp932_vs_nkf_cp932.tsv");
    run_case_nkf("Shift_JIS-2004", "diff_win_cp932_vs_nkf_shift_jis_2004.tsv");

    return 0;
}
