#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "nkf32.h"

static void dump_hex(const unsigned char* p, size_t n)
{
    for (size_t i = 0; i < n; i++) printf("%02X ", p[i]);
    printf("\n");
}

int main(void)
{
    /* 入力: UTF-8（ソースをUTF-8で保存している前提。念のため u8 を付ける） */
    const char input_utf8[] = u8"こんにちは";

    /* nkf オプション：入力 UTF-8(-w)、出力 Shift_JIS(-s)
       ※あなたの前提「nkfはcp932のみ受け付ける」を “出力がCP932相当” と解釈して、
          nkf側の指定は -s を使う（CP932=Shift_JIS系の想定） */
    if (SetNkfOption("-w -s") < 0) {
        printf("SetNkfOption failed\n");
        return 1;
    }

    char output_cp932[256];
    memset(output_cp932, 0, sizeof(output_cp932));

    DWORD outBytes = 0;
    DWORD inBytes = (DWORD)strlen(input_utf8); /* UTF-8のバイト数 */

    BOOL ok = NkfConvertSafe(
        output_cp932,
        (DWORD)sizeof(output_cp932),
        &outBytes,
        input_utf8,
        inBytes
    );

    if (!ok) {
        printf("NkfConvertSafe failed\n");
        return 1;
    }

    /* outBytes は NUL終端を含まない出力バイト数 */
    printf("UTF-8 bytes: ");
    dump_hex((const unsigned char*)input_utf8, strlen(input_utf8));

    printf("CP932 bytes: ");
    dump_hex((const unsigned char*)output_cp932, outBytes);

    /* CP932 を Windows コンソールで正しく表示させたい場合 */
    SetConsoleOutputCP(932);
    printf("CP932 text: %s\n", output_cp932);

    return 0;
}