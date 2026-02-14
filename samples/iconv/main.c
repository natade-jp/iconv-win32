#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <iconv.h>

static void dump_hex(const unsigned char* p, size_t n)
{
    for (size_t i = 0; i < n; i++) printf("%02X ", p[i]);
    printf("\n");
}

int main(void)
{
    /* このソースが Shift_JIS(CP932) 保存なら、ここも CP932 のバイト列になる */
    const char input_cp932[] = "こんにちは";

    /* 変換先: UTF-8, 変換元: CP932(Shift_JIS互換) */
    iconv_t cd = iconv_open("UTF-8", "CP932");
    if (cd == (iconv_t)-1) {
        perror("iconv_open");
        return 1;
    }

    char output_utf8[256];
    memset(output_utf8, 0, sizeof(output_utf8));

    char* inbuf = (char*)input_cp932;
    char* outbuf = output_utf8;

    size_t inbytes = strlen(input_cp932);
    size_t outbytes = sizeof(output_utf8);

    if (iconv(cd, &inbuf, &inbytes, &outbuf, &outbytes) == (size_t)-1) {
        perror("iconv");
        iconv_close(cd);
        return 1;
    }
    iconv_close(cd);

    printf("CP932 bytes: ");
    dump_hex((const unsigned char*)input_cp932, strlen(input_cp932));

    printf("UTF-8 bytes: ");
    dump_hex((const unsigned char*)output_utf8, strlen(output_utf8));

    /* UTF-8 を Windows コンソールで正しく表示させたい場合 */
    SetConsoleOutputCP(CP_UTF8);
    printf("UTF-8 text: %s\n", output_utf8);

    return 0;
}
