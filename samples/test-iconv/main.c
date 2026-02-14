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
    /* 入力: UTF-8（ソースがUTF-8で保存されている前提。念のため u8 を付ける） */
    const char input_utf8[] = u8"こんにちは";

    /* 変換先: CP932, 変換元: UTF-8 */
    iconv_t cd = iconv_open("CP932", "UTF-8");
    if (cd == (iconv_t)-1) {
        perror("iconv_open");
        return 1;
    }

    char output_cp932[256];
    memset(output_cp932, 0, sizeof(output_cp932));

    char* inbuf = (char*)input_utf8;
    char* outbuf = output_cp932;

    size_t inbytes = strlen(input_utf8);
    size_t outbytes = sizeof(output_cp932);

    if (iconv(cd, &inbuf, &inbytes, &outbuf, &outbytes) == (size_t)-1) {
        perror("iconv");
        iconv_close(cd);
        return 1;
    }
    iconv_close(cd);

    printf("UTF-8 bytes: ");
    dump_hex((const unsigned char*)input_utf8, strlen(input_utf8));

    /* 出力はCP932なので、strlenはNUL終端まででOK（日本語でも0x00は基本出ない） */
    printf("CP932 bytes: ");
    dump_hex((const unsigned char*)output_cp932, strlen(output_cp932));

    /* CP932 を Windows コンソールで正しく表示させたい場合 */
    SetConsoleOutputCP(932);
    printf("CP932 text: %s\n", output_cp932);

    return 0;
}
