/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --no-compress --no-prefilter --bpp 1 --size 16 --font assets/fonts/PixelifySans-Regular.ttf -r 0x20-0xFF --format lvgl --lv-include lvgl.h -o firmware/bisc8_fortune/components/externlib/ui_res/bisc8_font_small.c --force-fast-kern-format
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef BISC8_FONT_SMALL
#define BISC8_FONT_SMALL 1
#endif

#if BISC8_FONT_SMALL

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xfc, 0x30,

    /* U+0022 "\"" */
    0xd9, 0x30,

    /* U+0023 "#" */
    0x63, 0x31, 0x98, 0xdf, 0xf6, 0x33, 0x19, 0x8d,
    0xff, 0x63, 0x31, 0x80,

    /* U+0024 "$" */
    0x18, 0x18, 0x7c, 0xc3, 0xc0, 0xc0, 0x7c, 0x3,
    0x3, 0xc3, 0x44, 0x7c, 0x18, 0x18,

    /* U+0025 "%" */
    0x60, 0x66, 0x4, 0xf8, 0xc6, 0x30, 0x6, 0x0,
    0x40, 0xc, 0x63, 0x1f, 0x20, 0x66, 0x6,

    /* U+0026 "&" */
    0x7c, 0x22, 0x30, 0xcd, 0x81, 0x81, 0x41, 0xb1,
    0x86, 0x45, 0x3e, 0xc0,

    /* U+0027 "'" */
    0xcc,

    /* U+0028 "(" */
    0x18, 0xc6, 0xc6, 0x73, 0x18, 0x63, 0x18, 0x31,
    0x8c,

    /* U+0029 ")" */
    0xc6, 0x30, 0xc6, 0x3c, 0x63, 0x63, 0x19, 0x8c,
    0x60,

    /* U+002A "*" */
    0xdc,

    /* U+002B "+" */
    0x18, 0x30, 0x67, 0xf1, 0x83, 0x6, 0x0,

    /* U+002C "," */
    0xcc,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xc0,

    /* U+002F "/" */
    0x4, 0x30, 0x86, 0x10, 0xc2, 0x18, 0x43, 0x0,

    /* U+0030 "0" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+0031 "1" */
    0x63, 0x36, 0x31, 0x8c, 0x63, 0x18, 0xc0,

    /* U+0032 "2" */
    0x7c, 0xc3, 0x3, 0x3, 0x7c, 0xc0, 0xc0, 0xc3,
    0x44, 0x7c,

    /* U+0033 "3" */
    0x7c, 0xc3, 0x3, 0x3, 0x7c, 0x3, 0x3, 0xc3,
    0x44, 0x7c,

    /* U+0034 "4" */
    0x7, 0xb, 0x1b, 0x63, 0xc3, 0xc3, 0xff, 0x3,
    0x3, 0x3,

    /* U+0035 "5" */
    0x7c, 0xc3, 0xc3, 0xc3, 0x60, 0x20, 0x1c, 0xc3,
    0x44, 0x7c,

    /* U+0036 "6" */
    0x7c, 0x44, 0xc3, 0xc0, 0xfc, 0xc4, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+0037 "7" */
    0x7c, 0x44, 0xc3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3,

    /* U+0038 "8" */
    0x7c, 0xc3, 0xc3, 0xc3, 0x7c, 0x44, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+0039 "9" */
    0x7c, 0xc3, 0xc3, 0xc3, 0x7f, 0x3, 0x3, 0xc3,
    0x44, 0x7c,

    /* U+003A ":" */
    0xc3,

    /* U+003B ";" */
    0xc3, 0x14,

    /* U+003C "<" */
    0x3, 0xc, 0x8, 0x18, 0x60, 0xc0, 0x40, 0x60,
    0x18, 0x8, 0xc, 0x3,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0xc0, 0x60, 0x20, 0x18, 0xc, 0x3, 0x4, 0xc,
    0x18, 0x20, 0x60, 0xc0,

    /* U+003F "?" */
    0x7c, 0xc3, 0xc3, 0xc3, 0x1c, 0x18, 0x18, 0x0,
    0x0, 0x18,

    /* U+0040 "@" */
    0x38, 0x44, 0x82, 0x91, 0xaa, 0xd4, 0x30,

    /* U+0041 "A" */
    0x18, 0x28, 0x6c, 0xc3, 0xc3, 0xc3, 0xff, 0xc3,
    0xc3, 0xc3,

    /* U+0042 "B" */
    0x7c, 0xc3, 0xc3, 0xc3, 0xdc, 0xc4, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+0043 "C" */
    0x7c, 0xc3, 0xc3, 0xc3, 0xc0, 0xc0, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+0044 "D" */
    0xf0, 0xd0, 0xcc, 0xc3, 0xc3, 0xc3, 0xc3, 0xcc,
    0xd0, 0xf0,

    /* U+0045 "E" */
    0x7c, 0x44, 0xc3, 0xc0, 0xf8, 0xc0, 0xc0, 0xc3,
    0x44, 0x7c,

    /* U+0046 "F" */
    0xfc, 0xc4, 0xc3, 0xc0, 0xf8, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0,

    /* U+0047 "G" */
    0x7c, 0x44, 0xc3, 0xc3, 0xdb, 0xc8, 0xc7, 0xc3,
    0xc3, 0x7c,

    /* U+0048 "H" */
    0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3,

    /* U+0049 "I" */
    0xfb, 0x18, 0xc6, 0x31, 0x8c, 0x67, 0xc0,

    /* U+004A "J" */
    0x7c, 0x3, 0x3, 0x3, 0x3, 0x3, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+004B "K" */
    0x63, 0x44, 0xcc, 0xd8, 0xe0, 0xe0, 0xd8, 0xcc,
    0xc4, 0xc3,

    /* U+004C "L" */
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc3,
    0x44, 0x7c,

    /* U+004D "M" */
    0x61, 0x8c, 0x33, 0x6d, 0xed, 0xbd, 0xf7, 0x98,
    0xf3, 0x1e, 0x63, 0xc0, 0x78, 0xc,

    /* U+004E "N" */
    0xc3, 0xc3, 0xe3, 0xdb, 0xdb, 0xdb, 0xdb, 0xc7,
    0xc3, 0xc3,

    /* U+004F "O" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+0050 "P" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xc0,
    0xc0, 0xc0,

    /* U+0051 "Q" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x18, 0x18, 0xc,

    /* U+0052 "R" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xd8,
    0xd8, 0xc6,

    /* U+0053 "S" */
    0x7c, 0xc3, 0xc0, 0xc0, 0x7c, 0x3, 0x3, 0xc3,
    0x44, 0x7c,

    /* U+0054 "T" */
    0x7c, 0x5c, 0xdb, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18,

    /* U+0055 "U" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0x44, 0x7c,

    /* U+0056 "V" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x6c, 0x6c,
    0x6c, 0x18,

    /* U+0057 "W" */
    0xc0, 0x78, 0xf, 0x31, 0xe6, 0x3c, 0xc7, 0xbe,
    0xf6, 0xde, 0xdb, 0x61, 0x8c, 0x30,

    /* U+0058 "X" */
    0xc3, 0x6c, 0x6c, 0x6c, 0x18, 0x28, 0x6c, 0x6c,
    0x6c, 0xc3,

    /* U+0059 "Y" */
    0xc3, 0xc3, 0xc3, 0x6c, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18,

    /* U+005A "Z" */
    0xfc, 0xc3, 0xc3, 0x3, 0x7c, 0x40, 0xc0, 0xc3,
    0xc3, 0x7f,

    /* U+005B "[" */
    0x3e, 0x31, 0x8c, 0x63, 0x18, 0xc1, 0xce,

    /* U+005C "\\" */
    0xc1, 0x6, 0x8, 0x30, 0x41, 0x82, 0xc, 0x10,

    /* U+005D "]" */
    0xe0, 0xc6, 0x31, 0x8c, 0x63, 0x1f, 0x38,

    /* U+005E "^" */
    0x70,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0xd0,

    /* U+0061 "a" */
    0x7c, 0x61, 0xb0, 0xd8, 0x6c, 0x32, 0x29, 0xf6,

    /* U+0062 "b" */
    0xc0, 0xc0, 0xc0, 0xfc, 0xc3, 0xc3, 0xc3, 0xc3,
    0x44, 0x7c,

    /* U+0063 "c" */
    0x7c, 0xc3, 0xc0, 0xc0, 0xc3, 0x44, 0x7c,

    /* U+0064 "d" */
    0x3, 0x3, 0x3, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3,
    0x44, 0x7c,

    /* U+0065 "e" */
    0x7c, 0xc3, 0xc0, 0xf8, 0xc3, 0x44, 0x7c,

    /* U+0066 "f" */
    0x7c, 0x44, 0xc3, 0xc0, 0xf8, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0,

    /* U+0067 "g" */
    0x7c, 0xc3, 0xc3, 0xc3, 0x7f, 0x3, 0x3, 0xc3,
    0x44, 0x7c,

    /* U+0068 "h" */
    0xc0, 0xc0, 0xc0, 0xdc, 0xe3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3,

    /* U+0069 "i" */
    0xc3, 0xff, 0xf0,

    /* U+006A "j" */
    0xc, 0x30, 0xc3, 0xf, 0x33, 0x80,

    /* U+006B "k" */
    0xc0, 0xc0, 0xc0, 0xc3, 0xdc, 0xe0, 0xe0, 0xdc,
    0xc4, 0xc3,

    /* U+006C "l" */
    0xff, 0xff, 0xf0,

    /* U+006D "m" */
    0xf3, 0x99, 0x8f, 0x31, 0xe6, 0x3c, 0xc7, 0x98,
    0xf3, 0x18,

    /* U+006E "n" */
    0xfc, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,

    /* U+006F "o" */
    0x7c, 0xc3, 0xc3, 0xc3, 0xc3, 0x44, 0x7c,

    /* U+0070 "p" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xc0,
    0xc0, 0xc0,

    /* U+0071 "q" */
    0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x3,
    0x3, 0x3,

    /* U+0072 "r" */
    0x7c, 0xc3, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,

    /* U+0073 "s" */
    0x7f, 0xc0, 0x40, 0x7c, 0x3, 0x4, 0xfc,

    /* U+0074 "t" */
    0x60, 0x30, 0x18, 0x1f, 0xe6, 0x3, 0x1, 0x80,
    0xc3, 0x21, 0xf, 0x80,

    /* U+0075 "u" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c,

    /* U+0076 "v" */
    0xc3, 0xc3, 0xc3, 0x6c, 0x6c, 0x6c, 0x18,

    /* U+0077 "w" */
    0xcc, 0x79, 0x8f, 0x31, 0xe6, 0x3c, 0xc7, 0x98,
    0xfc, 0xe0,

    /* U+0078 "x" */
    0xc3, 0x6c, 0x28, 0x18, 0x6c, 0x44, 0xc3,

    /* U+0079 "y" */
    0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x3, 0x3, 0xc3,
    0x44, 0x7c,

    /* U+007A "z" */
    0xfe, 0x18, 0x30, 0xc6, 0xc, 0x3f, 0x80,

    /* U+007B "{" */
    0x19, 0xb6, 0x18, 0x63, 0x4, 0x18, 0x61, 0x86,
    0xc6,

    /* U+007C "|" */
    0xff, 0xff, 0xf0,

    /* U+007D "}" */
    0x63, 0x61, 0x86, 0x18, 0x30, 0x86, 0x18, 0x6d,
    0x98,

    /* U+007E "~" */
    0xdb,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xc3, 0xff, 0xf0,

    /* U+00A2 "¢" */
    0x18, 0x18, 0x18, 0x7c, 0xc3, 0xc0, 0xc0, 0xc3,
    0x7c, 0x18, 0x18,

    /* U+00A3 "£" */
    0x1f, 0x10, 0x98, 0x6c, 0x7, 0xc3, 0x1, 0x80,
    0xc3, 0x61, 0x7f, 0x80,

    /* U+00A4 "¤" */
    0xb5, 0x25, 0xc, 0x84,

    /* U+00A5 "¥" */
    0xc3, 0xc3, 0xc3, 0x6c, 0x18, 0x7c, 0x18, 0x7c,
    0x18, 0x18,

    /* U+00A7 "§" */
    0x7c, 0xc3, 0xc0, 0xc0, 0x7c, 0x7c, 0xff, 0xc0,
    0xc0, 0x7f,

    /* U+00A8 "¨" */
    0xd8,

    /* U+00A9 "©" */
    0x6f, 0x66,

    /* U+00AA "ª" */
    0x64, 0xa4, 0xd6, 0x0,

    /* U+00AB "«" */
    0x3, 0x18, 0x63, 0x2, 0x10, 0x31, 0x86, 0x30,
    0x63, 0x1, 0x8, 0xc, 0x60, 0x18, 0xc0, 0x42,
    0x3, 0x18, 0x6, 0x30,

    /* U+00AC "¬" */
    0xfe, 0xc, 0x18,

    /* U+00AE "®" */
    0x69, 0x66,

    /* U+00AF "¯" */
    0xf8,

    /* U+00B0 "°" */
    0x55, 0x0,

    /* U+00B1 "±" */
    0x30, 0xcf, 0xcc, 0x30, 0x0, 0x3f,

    /* U+00B2 "²" */
    0xf1, 0xe9, 0x60,

    /* U+00B3 "³" */
    0xf1, 0x79, 0x60,

    /* U+00B4 "´" */
    0x60,

    /* U+00B5 "µ" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xc0,
    0xc0, 0xc0,

    /* U+00B6 "¶" */
    0x3f, 0xe7, 0xcb, 0xf9, 0x7f, 0x2f, 0xe5, 0xfc,
    0x8f, 0x90, 0x12, 0x2, 0x40, 0x48, 0x9, 0x1,
    0x20, 0x24,

    /* U+00B7 "·" */
    0xc0,

    /* U+00B8 "¸" */
    0x57, 0x80,

    /* U+00B9 "¹" */
    0xd5, 0x40,

    /* U+00BA "º" */
    0x69, 0x96, 0x60,

    /* U+00BB "»" */
    0xc6, 0x3, 0x18, 0x8, 0x40, 0x31, 0x80, 0xc6,
    0x1, 0x8c, 0x10, 0x81, 0x8c, 0x18, 0xc1, 0x8,
    0x18, 0xc1, 0x8c, 0x0,

    /* U+00BC "¼" */
    0xe0, 0xc1, 0xc, 0x9, 0x80, 0x4c, 0x0, 0x62,
    0x6, 0x1c, 0x61, 0x23, 0xf, 0x18, 0x8,

    /* U+00BD "½" */
    0xe1, 0x84, 0x60, 0xb0, 0x16, 0x20, 0xcc, 0x39,
    0xcc, 0x21, 0x85, 0x30, 0x40,

    /* U+00BE "¾" */
    0xf0, 0x63, 0xc, 0x4, 0x41, 0xe6, 0x0, 0x31,
    0x3, 0x14, 0x30, 0xa1, 0x87, 0xc, 0x8,

    /* U+00BF "¿" */
    0x18, 0x0, 0x0, 0x18, 0x78, 0x40, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+00C0 "À" */
    0x20, 0x10, 0x0, 0x18, 0x28, 0x6c, 0xc3, 0xc3,
    0xc3, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C1 "Á" */
    0x18, 0x20, 0x0, 0x18, 0x28, 0x6c, 0xc3, 0xc3,
    0xc3, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C2 "Â" */
    0x10, 0x28, 0x0, 0x18, 0x28, 0x6c, 0xc3, 0xc3,
    0xc3, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C3 "Ã" */
    0x74, 0x7c, 0x0, 0x18, 0x28, 0x6c, 0xc3, 0xc3,
    0xc3, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C4 "Ä" */
    0x64, 0x0, 0x18, 0x28, 0x6c, 0xc3, 0xc3, 0xc3,
    0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C5 "Å" */
    0x18, 0x18, 0x0, 0x18, 0x28, 0x6c, 0xc3, 0xc3,
    0xc3, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C6 "Æ" */
    0x19, 0xf1, 0x48, 0x99, 0xc7, 0x86, 0xc, 0x3c,
    0x61, 0x83, 0xfc, 0x18, 0x63, 0xc3, 0x16, 0x1f,
    0x80,

    /* U+00C7 "Ç" */
    0x7c, 0xc3, 0xc3, 0xc3, 0xc0, 0xc0, 0xc3, 0xc3,
    0xff, 0x7c, 0x10, 0x2c, 0x18,

    /* U+00C8 "È" */
    0x30, 0x10, 0x0, 0x7c, 0x44, 0xc3, 0xc0, 0xf8,
    0xc0, 0xc0, 0xc3, 0x44, 0x7c,

    /* U+00C9 "É" */
    0x18, 0x10, 0x0, 0x7c, 0x44, 0xc3, 0xc0, 0xf8,
    0xc0, 0xc0, 0xc3, 0x44, 0x7c,

    /* U+00CA "Ê" */
    0x10, 0x18, 0x0, 0x7c, 0x44, 0xc3, 0xc0, 0xf8,
    0xc0, 0xc0, 0xc3, 0x44, 0x7c,

    /* U+00CB "Ë" */
    0x6c, 0x0, 0x7c, 0x44, 0xc3, 0xc0, 0xf0, 0xc0,
    0xc0, 0xc3, 0x44, 0x7c,

    /* U+00CC "Ì" */
    0xc3, 0x1, 0xf6, 0x31, 0x8c, 0x63, 0x18, 0xcf,
    0x80,

    /* U+00CD "Í" */
    0x64, 0x1, 0xf6, 0x31, 0x8c, 0x63, 0x18, 0xcf,
    0x80,

    /* U+00CE "Î" */
    0x63, 0x81, 0xf6, 0x31, 0x8c, 0x63, 0x18, 0xcf,
    0x80,

    /* U+00CF "Ï" */
    0xd8, 0x3e, 0x42, 0x10, 0x84, 0x21, 0x9, 0xf0,

    /* U+00D0 "Ð" */
    0xf8, 0xc8, 0xc6, 0xc3, 0xc3, 0xf3, 0xc3, 0xc6,
    0xc8, 0xf8,

    /* U+00D1 "Ñ" */
    0x34, 0x3c, 0x0, 0xc3, 0xc3, 0xe3, 0xdb, 0xdb,
    0xdb, 0xdb, 0xc7, 0xc3, 0xc3,

    /* U+00D2 "Ò" */
    0x30, 0x10, 0x0, 0x7c, 0x44, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0x7c,

    /* U+00D3 "Ó" */
    0x18, 0x10, 0x0, 0x7c, 0x44, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0x7c,

    /* U+00D4 "Ô" */
    0x10, 0x18, 0x0, 0x7c, 0x44, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0x7c,

    /* U+00D5 "Õ" */
    0x74, 0x7c, 0x0, 0x7c, 0x44, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0x7c,

    /* U+00D6 "Ö" */
    0x6c, 0x0, 0x7c, 0x44, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0x7c,

    /* U+00D7 "×" */
    0x8b, 0x77, 0x9c, 0xfb, 0x20,

    /* U+00D8 "Ø" */
    0x3, 0x7f, 0x46, 0xcf, 0xcf, 0xdb, 0xd3, 0xf3,
    0xe3, 0xc3, 0xfc, 0x80,

    /* U+00D9 "Ù" */
    0x30, 0x10, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0x44, 0x7c,

    /* U+00DA "Ú" */
    0x18, 0x10, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0x44, 0x7c,

    /* U+00DB "Û" */
    0x10, 0x18, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0x44, 0x7c,

    /* U+00DC "Ü" */
    0x6c, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x44, 0x7c,

    /* U+00DD "Ý" */
    0x18, 0x20, 0x0, 0xc3, 0xc3, 0xc3, 0x6c, 0x18,
    0x18, 0x18, 0x18, 0x18, 0x18,

    /* U+00DE "Þ" */
    0xc0, 0xc0, 0xfc, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc,
    0xc0, 0xc0,

    /* U+00DF "ß" */
    0xfc, 0xc3, 0xc3, 0xc3, 0xdc, 0xc4, 0xc3, 0xc3,
    0xc3, 0xdc,

    /* U+00E0 "à" */
    0x20, 0x8, 0x2, 0x0, 0x7, 0xc6, 0x1b, 0xd,
    0x86, 0xc3, 0x22, 0x9f, 0x60,

    /* U+00E1 "á" */
    0x8, 0x8, 0x4, 0x0, 0x7, 0xc6, 0x1b, 0xd,
    0x86, 0xc3, 0x22, 0x9f, 0x60,

    /* U+00E2 "â" */
    0x8, 0xc, 0x0, 0xf, 0x8c, 0x36, 0x1b, 0xd,
    0x86, 0x45, 0x3e, 0xc0,

    /* U+00E3 "ã" */
    0x34, 0x16, 0x0, 0xf, 0x8c, 0x36, 0x1b, 0xd,
    0x86, 0x45, 0x3e, 0xc0,

    /* U+00E4 "ä" */
    0x66, 0x0, 0x0, 0xf, 0x8c, 0x36, 0x1b, 0xd,
    0x86, 0x45, 0x3e, 0xc0,

    /* U+00E5 "å" */
    0x18, 0xc, 0x0, 0xf, 0x8c, 0x36, 0x1b, 0xd,
    0x86, 0x45, 0x3e, 0xc0,

    /* U+00E6 "æ" */
    0x7c, 0x3e, 0xc3, 0x63, 0xc3, 0x60, 0xc3, 0x78,
    0xc3, 0x63, 0x45, 0xa2, 0x7d, 0xbe,

    /* U+00E7 "ç" */
    0x7c, 0x44, 0xc3, 0xc0, 0xc3, 0x7c, 0x7c, 0x10,
    0x2c, 0x18,

    /* U+00E8 "è" */
    0x20, 0x10, 0x10, 0x0, 0x7c, 0xc3, 0xc0, 0xf8,
    0xc3, 0x44, 0x7c,

    /* U+00E9 "é" */
    0x8, 0x10, 0x10, 0x0, 0x7c, 0xc3, 0xc0, 0xf8,
    0xc3, 0x44, 0x7c,

    /* U+00EA "ê" */
    0x10, 0x18, 0x0, 0x7c, 0xc3, 0xc0, 0xf8, 0xc3,
    0x44, 0x7c,

    /* U+00EB "ë" */
    0x6c, 0x0, 0x0, 0x7c, 0xc3, 0xc0, 0xf0, 0xc3,
    0x44, 0x7c,

    /* U+00EC "ì" */
    0x89, 0x86, 0xdb, 0x6d, 0x80,

    /* U+00ED "í" */
    0x2a, 0x6, 0xdb, 0x6d, 0x80,

    /* U+00EE "î" */
    0xf3, 0xff, 0xf0,

    /* U+00EF "ï" */
    0xf0, 0x6, 0x66, 0x66, 0x66,

    /* U+00F0 "ð" */
    0x3, 0x3f, 0x11, 0x19, 0xe0, 0x33, 0xf9, 0xd,
    0x86, 0xc3, 0x61, 0x9f, 0x0,

    /* U+00F1 "ñ" */
    0x34, 0x3c, 0x0, 0xfc, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3,

    /* U+00F2 "ò" */
    0x20, 0x10, 0x10, 0x0, 0x7c, 0xc3, 0xc3, 0xc3,
    0xc3, 0x44, 0x7c,

    /* U+00F3 "ó" */
    0x8, 0x10, 0x10, 0x0, 0x7c, 0xc3, 0xc3, 0xc3,
    0xc3, 0x44, 0x7c,

    /* U+00F4 "ô" */
    0x10, 0x18, 0x0, 0x7c, 0xc3, 0xc3, 0xc3, 0xc3,
    0x44, 0x7c,

    /* U+00F5 "õ" */
    0x74, 0x7c, 0x0, 0x7c, 0xc3, 0xc3, 0xc3, 0xc3,
    0x44, 0x7c,

    /* U+00F6 "ö" */
    0x6c, 0x0, 0x0, 0x7c, 0xc3, 0xc3, 0xc3, 0xc3,
    0x44, 0x7c,

    /* U+00F7 "÷" */
    0x30, 0xf, 0xc0, 0x0, 0xc0,

    /* U+00F8 "ø" */
    0x1, 0x9f, 0x99, 0xec, 0xb6, 0xdb, 0xcc, 0xc8,
    0x7c, 0x40, 0x0,

    /* U+00F9 "ù" */
    0x20, 0x10, 0x10, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x7c,

    /* U+00FA "ú" */
    0x8, 0x10, 0x10, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x7c,

    /* U+00FB "û" */
    0x10, 0x18, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+00FC "ü" */
    0x6c, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c,

    /* U+00FD "ý" */
    0x18, 0x10, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f,
    0x3, 0x3, 0xc3, 0x44, 0x7c,

    /* U+00FE "þ" */
    0xc0, 0xc0, 0xfc, 0xc4, 0xc3, 0xc3, 0xc3, 0xc3,
    0xfc, 0xc0,

    /* U+00FF "ÿ" */
    0x6c, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f,
    0x3, 0x3, 0xc3, 0x44, 0x7c
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 51, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 119, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 6, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 18, .adv_w = 150, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 32, .adv_w = 222, .box_w = 12, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 68, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 60, .adv_w = 96, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 69, .adv_w = 96, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 78, .adv_w = 68, .box_w = 2, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 79, .adv_w = 154, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 86, .adv_w = 68, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 87, .adv_w = 85, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 88, .adv_w = 57, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 135, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 103, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 57, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 195, .adv_w = 68, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 154, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 209, .adv_w = 154, .box_w = 6, .box_h = 4, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 212, .adv_w = 154, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 224, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 241, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 261, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 311, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 103, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 196, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 372, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 415, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 425, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 465, .adv_w = 196, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 479, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 499, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 516, .adv_w = 135, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 96, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 531, .adv_w = 68, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 532, .adv_w = 113, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 533, .adv_w = 68, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 534, .adv_w = 173, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 552, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 569, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 586, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 596, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 127, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 615, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 628, .adv_w = 196, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 638, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 652, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 662, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 672, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 679, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 686, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 698, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 705, .adv_w = 147, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 196, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 722, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 729, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 739, .adv_w = 150, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 746, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 755, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 758, .adv_w = 119, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 767, .adv_w = 154, .box_w = 4, .box_h = 2, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 768, .adv_w = 51, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 769, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 772, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 783, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 116, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 799, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 809, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 819, .adv_w = 77, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = 8},
    {.bitmap_index = 820, .adv_w = 105, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 822, .adv_w = 102, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 826, .adv_w = 223, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 846, .adv_w = 154, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 849, .adv_w = 105, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 851, .adv_w = 77, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 852, .adv_w = 80, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 854, .adv_w = 154, .box_w = 6, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 860, .adv_w = 93, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 863, .adv_w = 93, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 866, .adv_w = 77, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 867, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 877, .adv_w = 196, .box_w = 11, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 895, .adv_w = 57, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 896, .adv_w = 77, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 898, .adv_w = 69, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 900, .adv_w = 90, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 903, .adv_w = 218, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 923, .adv_w = 225, .box_w = 13, .box_h = 9, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 938, .adv_w = 209, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 951, .adv_w = 245, .box_w = 13, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 966, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 976, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 989, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1002, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1015, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1028, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1053, .adv_w = 243, .box_w = 13, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1070, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1083, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1096, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1109, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1122, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1134, .adv_w = 103, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1143, .adv_w = 103, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1152, .adv_w = 103, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1161, .adv_w = 103, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1169, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1179, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1192, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1205, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1218, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1231, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1244, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1256, .adv_w = 154, .box_w = 6, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1261, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1273, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1286, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1299, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1312, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1337, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1347, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1357, .adv_w = 173, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1370, .adv_w = 173, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1383, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1395, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1407, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1419, .adv_w = 173, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1431, .adv_w = 290, .box_w = 16, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1445, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1455, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1466, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1477, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1487, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1497, .adv_w = 57, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1502, .adv_w = 57, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1507, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1510, .adv_w = 57, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1515, .adv_w = 155, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1528, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1538, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1549, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1560, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1570, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1580, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1590, .adv_w = 154, .box_w = 6, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1595, .adv_w = 150, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1606, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1617, .adv_w = 150, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1628, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1638, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1648, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1661, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1671, .adv_w = 150, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint8_t glyph_id_ofs_list_1[] = {
    0, 1, 2, 3, 4, 5, 0, 6,
    7, 8, 9, 10, 11
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 13, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = glyph_id_ofs_list_1, .list_length = 13, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
    },
    {
        .range_start = 174, .range_length = 82, .glyph_id_start = 108,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 2,
    0, 3, 4, 0, 5, 6, 0, 7,
    4, 5, 3, 0, 0, 0, 0, 0,
    0, 0, 8, 5, 9, 10, 0, 11,
    0, 6, 12, 3, 13, 14, 4, 6,
    3, 15, 3, 0, 7, 16, 17, 18,
    19, 20, 21, 22, 0, 0, 0, 0,
    2, 0, 23, 24, 25, 26, 0, 27,
    28, 29, 30, 31, 32, 33, 34, 28,
    0, 0, 28, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 0, 6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 9, 0, 0,
    0, 0, 0, 0, 0, 0, 10, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 25, 0, 0,
    0, 0, 0, 30, 30, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 2, 1,
    0, 3, 4, 0, 0, 0, 0, 3,
    0, 0, 5, 0, 0, 0, 0, 0,
    0, 0, 6, 3, 7, 8, 3, 9,
    3, 10, 11, 12, 13, 14, 15, 8,
    3, 16, 3, 13, 5, 4, 17, 18,
    19, 20, 21, 22, 0, 0, 0, 0,
    1, 0, 23, 24, 25, 26, 23, 27,
    28, 29, 30, 31, 32, 33, 34, 34,
    23, 0, 23, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 0, 8, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 6, 3, 0, 0,
    0, 0, 0, 0, 0, 0, 8, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 8, 27, 44, 44,
    44, 44, 44, 44, 23, 23, 45, 45,
    45, 45, 30, 0, 30, 0, 26, 0,
    45, 45, 45, 45, 45, 0, 0, 45,
    45, 45, 45, 0, 27, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -13, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -13, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -8, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -10,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -10,
    -10, 0, -5, 0, -5, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -5,
    0, -5, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, -10, -8, 0, -8, -5, -5, -5,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, -10, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -15, 0,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, -10, -13,
    0, -5, 0, -5, 0, 0, 0, 0,
    0, 0, -5, 0, 0, 0, 0, -5,
    -5, -5, -5, -5, -5, -5, 0, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, -8, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, 0, 0,
    -13, 0, 0, 0, 0, 0, 0, -13,
    -5, 0, 0, -5, 0, 0, -5, 0,
    -13, -15, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, -13, 0, -8, 0, -8, -5, -5,
    0, 0, -23, 0, -5, 0, -3, -15,
    -5, -5, -5, -5, -8, -23, -10, -5,
    -8, -8, -5, -5, -5, -10, -8, -5,
    -26, -8, -26, -26, -10, -28, -8, 0,
    -51, -8, -10, -13, -26, -26, 0, -15,
    -18, -20, -23, -23, -23, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -13, 0, -13,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, -8, 0, 0, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, -8, -10,
    0, -5, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    -8, 0, -13, 0, -13, 0, 0, -10,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -8, -10, 0, 0, -8, -8, 0,
    0, 0, -20, 0, -31, 0, 0, -8,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -13, -31, 0, -8,
    -5, -5, -5, -5, 0, -10, 0, 0,
    -5, -5, 0, 0, -5, -26, 0, -26,
    0, -10, 0, 0, 0, 0, -13, 0,
    0, -5, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -5, -10, -8, 0, -8, -5, -8, -5,
    -5, 0, -5, 0, 0, -5, -5, -5,
    -8, -8, -5, -5, -8, -5, 0, -5,
    -5, 0, 0, -23, -8, -8, -15, -5,
    -10, -8, -8, -8, -8, 0, 0, -10,
    0, -10, -10, -8, -10, 0, -15, -10,
    0, -26, -5, -26, -26, -8, -26, -8,
    -8, 0, -8, -8, -10, -26, -26, 0,
    -13, 0, -15, 0, -20, -13, -26, -13,
    0, 0, 0, -8, 0, -8, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, -3, -8, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, -10, -5, -8, -8, 0,
    -8, 0, 0, 0, -5, 0, -5, -5,
    -8, 0, -5, -3, -5, -5, -5, -5,
    0, 0, 0, 0, 0, -10, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, 0, -5, 0,
    -8, 0, -5, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, -8, -5,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, -8, -10, -8, -10, -10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, -18, 0, -15,
    0, 0, -8, 0, 0, 0, 0, 0,
    -5, -5, -8, 0, 0, 0, -5, -5,
    -5, -5, 0, 0, -13, -13, -5, 0,
    -5, -13, -8, 0, 0, -5, -5, 0,
    -8, -5, -5, -8, -5, -8, -5, -5,
    0, -5, -18, 0, -20, -20, 0, -23,
    0, 0, -41, -10, 0, -13, 0, -26,
    -13, -18, -18, -18, -18, -13, -15, 0,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, 0, -10,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, -5, -5, -5, -10, -5, -8,
    -5, -5, -5, -5, 0, 0, 0, 0,
    -18, -33, -8, -10, -18, 0, -13, 0,
    0, -15, 0, -15, -10, -10, -15, -26,
    -8, -13, -51, 0, -13, -13, -13, -13,
    -13, -13, -8, -8, -15, -10, -8, -8,
    0, -8, -28, -10, -23, -8, -10, -18,
    -8, 0, 0, 0, 0, 0, -31, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, -26,
    0, -5, 0, -5, 0, 0, -5, 0,
    0, 0, 0, -5, -3, -5, -5, -13,
    0, -10, -8, -8, -5, 0, 0, 0,
    0, 0, -8, -31, -5, 0, -8, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, -8, 0, -15, -26, 0, -8, -5,
    -8, -5, -5, -8, -5, 0, 0, -5,
    -5, 0, 0, -5, -13, -5, -8, 0,
    -8, -8, 0, 0, 0, 0, 0, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, -5, -5, -5, -5, -5,
    -5, -5, 0, -5, -5, -5, -5, -5,
    -5, -10, -5, -8, -5, -5, -5, -5,
    0, 0, 0, 0, -13, 0, -10, 0,
    -13, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -10, 0, 0, -18, 0,
    -26, -8, -26, -26, 0, -26, -8, 0,
    -49, -15, -13, -18, -28, -26, -23, -13,
    -18, -13, -20, -20, -15, 0, 0, 0,
    0, -5, -31, -5, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, -26, 0, -8, -5, -5,
    -5, -5, -5, -5, 0, -5, -5, -5,
    0, 0, -5, -10, -5, -10, 0, 0,
    0, 0, 0, 0, 0, 0, -8, -26,
    -5, 0, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    -26, 0, -5, 0, -8, -5, 0, -5,
    0, 0, 0, 0, 0, 0, 0, -5,
    -10, -5, -8, -5, -8, -5, 0, 0,
    0, 0, 0, -5, -8, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, 0, -8, 0, -5,
    -3, -5, 0, 0, -5, 0, 0, -5,
    0, 0, -5, 0, 0, -8, 0, 0,
    -5, 0, -5, -3, 0, 0, 0, 0,
    -5, -31, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -26, 0, 0, 0, 0, -5,
    0, -5, -8, 0, 0, 0, 0, 0,
    -5, 0, -10, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -15, -3, -13, -13, -5, -3, -5,
    0, 0, -5, -5, -5, -5, -5, -10,
    -5, -8, -5, 0, -5, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    -8, 0, 0, -5, 0, 0, 0, -8,
    0, -5, -3, -5, -8, -5, -8, -5,
    -5, -5, 0, 0, 0, 0, 0, -5,
    -31, -5, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -8, 0,
    0, -26, 0, -3, -5, -3, -5, -5,
    -5, -5, 0, -5, -5, -5, 0, 0,
    -5, -10, -5, -10, 0, 0, 0, 0,
    0, 0, -38, 0, -3, -31, -3, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -8, 0, 0, -26, 0,
    -8, -5, -5, -8, 0, -5, 0, 0,
    0, -5, -5, -8, -8, -5, 0, -5,
    -5, -5, -5, -5, -5, 0, 0, 0,
    0, -5, -31, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, -23, 0, -5, -8, -5,
    0, 0, -5, 0, 0, 0, 0, 0,
    0, -5, -5, 0, 0, -5, -3, -3,
    -5, -5, 0, 0, 0, 0, 0, -31,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    -26, 0, -8, -5, -5, -5, -5, -8,
    -5, 0, -5, -5, -5, -5, 0, -5,
    -18, -5, -18, -8, -8, 0, -5, 0,
    0, 0, 0, -5, -31, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, -15, -26, 0, -5,
    0, -3, -5, 0, -3, -8, 0, 0,
    0, 0, 0, -3, -3, -10, -5, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, -18, 0, 0, -5, 0, 0, 0,
    0, -10, 0, 0, 0, 0, 0, -5,
    0, 0, -26, 0, -10, -8, -8, -10,
    0, -5, -8, -5, -10, -3, -5, -5,
    0, -5, -8, -5, -5, -5, -5, -5,
    -5, 0, 0, 0, 0, -5, -31, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, -26,
    0, -8, -5, -5, -5, -3, -3, -5,
    0, -5, -3, -3, 0, -5, -3, -8,
    -8, 0, 0, 0, 0, -3, 0, 0,
    0, 0, -5, -31, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, 0, -26, 0, -13, -5,
    -13, -5, -5, 0, -8, 0, 0, -5,
    -5, 0, 0, 0, -8, -5, -5, -5,
    0, -5, -5, 0, 0, 0, 0, -5,
    -31, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -26, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, -31, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -26, 0,
    -5, 0, -8, -5, 0, -8, 0, 0,
    0, 0, 0, 0, 0, -8, 0, -8,
    -5, -5, 0, 0, -3, 0, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 43,
    .right_class_cnt     = 45,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t bisc8_font_small = {
#else
lv_font_t bisc8_font_small = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0)
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if BISC8_FONT_SMALL*/

