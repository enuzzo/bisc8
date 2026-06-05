/*******************************************************************************
 * Size: 17 px
 * Bpp: 1
 * Opts: --no-compress --no-prefilter --bpp 1 --size 17 --font assets/fonts/PixelifySans-Regular.ttf -r 0x20-0xFF --format lvgl --lv-include lvgl.h -o firmware/bisc8_fortune/components/externlib/ui_res/bisc8_font_small.c --force-fast-kern-format
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
    0xff, 0xff, 0x3c,

    /* U+0022 "\"" */
    0xdb, 0x64, 0x52,

    /* U+0023 "#" */
    0x63, 0x31, 0x98, 0xdf, 0xff, 0xfb, 0x19, 0x8c,
    0xc6, 0xff, 0xff, 0xd8, 0xc0,

    /* U+0024 "$" */
    0x18, 0x18, 0x7c, 0xff, 0xc3, 0xc0, 0x7c, 0x7c,
    0x3, 0x3, 0xc3, 0xff, 0x7c, 0x18, 0x18,

    /* U+0025 "%" */
    0x60, 0x37, 0xc7, 0xbe, 0x30, 0xc3, 0x6, 0x78,
    0x3, 0x0, 0x31, 0x81, 0x8c, 0x30, 0xfb, 0x87,
    0xd8, 0x18,

    /* U+0026 "&" */
    0x78, 0xff, 0xc3, 0x6c, 0x78, 0x18, 0x68, 0x68,
    0xc7, 0xff, 0x79,

    /* U+0027 "'" */
    0xd8, 0xa0,

    /* U+0028 "(" */
    0x18, 0xc6, 0xc6, 0x33, 0x18, 0xc2, 0x18, 0xc6,
    0xc, 0x63,

    /* U+0029 ")" */
    0xc6, 0x30, 0xc6, 0x30, 0x63, 0x19, 0x18, 0xc6,
    0x63, 0x18,

    /* U+002A "*" */
    0xea, 0x80,

    /* U+002B "+" */
    0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18,

    /* U+002C "," */
    0xd8, 0xa0,

    /* U+002D "-" */
    0xff, 0xc0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x6, 0x8, 0x30, 0x41, 0x82, 0xc, 0x10, 0x60,
    0x83, 0x0,

    /* U+0030 "0" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+0031 "1" */
    0x67, 0xf6, 0x31, 0x8c, 0x63, 0x18, 0xc6,

    /* U+0032 "2" */
    0x7c, 0xff, 0xc3, 0x3, 0x7c, 0x7c, 0xc0, 0xc0,
    0xc3, 0xff, 0x7c,

    /* U+0033 "3" */
    0x7c, 0xff, 0xc3, 0x3, 0x7c, 0x7c, 0x3, 0x3,
    0xc3, 0xff, 0x7c,

    /* U+0034 "4" */
    0x7, 0x1f, 0x1b, 0x63, 0x63, 0xc3, 0xff, 0xff,
    0x3, 0x3, 0x3,

    /* U+0035 "5" */
    0x7c, 0xff, 0xc3, 0xc3, 0x60, 0x60, 0x1c, 0x1c,
    0xc3, 0xff, 0x7c,

    /* U+0036 "6" */
    0x7c, 0xff, 0xc3, 0xc0, 0xfc, 0xfc, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+0037 "7" */
    0x7c, 0xff, 0xc3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0x3, 0x3, 0x3,

    /* U+0038 "8" */
    0x7c, 0xff, 0xc3, 0xc3, 0x7c, 0x7c, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+0039 "9" */
    0x7c, 0xff, 0xc3, 0xc3, 0x7f, 0x7f, 0x3, 0x3,
    0xc3, 0xff, 0x7c,

    /* U+003A ":" */
    0xf3, 0xc0,

    /* U+003B ";" */
    0xd8, 0xd, 0xd0,

    /* U+003C "<" */
    0x3, 0x3, 0xc, 0x1c, 0x18, 0x60, 0xe0, 0xc0,
    0x60, 0x60, 0x18, 0x1c, 0xc, 0x3, 0x3,

    /* U+003D "=" */
    0xff, 0xf0, 0x3f, 0xfc,

    /* U+003E ">" */
    0xc0, 0xc0, 0x60, 0x78, 0x18, 0xc, 0xf, 0x3,
    0xc, 0xc, 0x18, 0x78, 0x60, 0xc0, 0xc0,

    /* U+003F "?" */
    0x7c, 0xff, 0xc3, 0xc3, 0x1c, 0x1c, 0x18, 0x18,
    0x0, 0x18, 0x18,

    /* U+0040 "@" */
    0x3c, 0x42, 0x82, 0xbd, 0xa7, 0x9a, 0x40, 0x38,

    /* U+0041 "A" */
    0x18, 0x7c, 0x6c, 0xc3, 0xc3, 0xc3, 0xff, 0xff,
    0xc3, 0xc3, 0xc3,

    /* U+0042 "B" */
    0x7c, 0xff, 0xc3, 0xc3, 0xdc, 0xdc, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+0043 "C" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc0, 0xc0, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+0044 "D" */
    0xf0, 0xfc, 0xcc, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xcc, 0xfc, 0xf0,

    /* U+0045 "E" */
    0x7c, 0xff, 0xc3, 0xc0, 0xf8, 0xf8, 0xc0, 0xc0,
    0xc3, 0xff, 0x7c,

    /* U+0046 "F" */
    0xfc, 0xff, 0xc3, 0xc0, 0xf8, 0xf8, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0,

    /* U+0047 "G" */
    0x7c, 0xff, 0xc3, 0xc3, 0xd8, 0xd8, 0xc7, 0xc7,
    0xc3, 0x7c, 0x7c,

    /* U+0048 "H" */
    0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3,

    /* U+0049 "I" */
    0xff, 0xd8, 0xc6, 0x31, 0x8c, 0x67, 0xfe,

    /* U+004A "J" */
    0x7c, 0x7f, 0x3, 0x3, 0x3, 0x3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+004B "K" */
    0x63, 0xef, 0xcc, 0xd8, 0xf8, 0xe0, 0xd8, 0xd8,
    0xcc, 0xcf, 0xc3,

    /* U+004C "L" */
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc3, 0xff, 0x7c,

    /* U+004D "M" */
    0x60, 0xdf, 0x7f, 0x6d, 0xed, 0xbc, 0xc7, 0x98,
    0xf3, 0x1e, 0x63, 0xcc, 0x78, 0xf, 0x1, 0x80,

    /* U+004E "N" */
    0xc3, 0xe3, 0xe3, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb,
    0xc7, 0xc7, 0xc3,

    /* U+004F "O" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+0050 "P" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xfc,
    0xc0, 0xc0, 0xc0,

    /* U+0051 "Q" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c, 0x18, 0xc, 0xc,

    /* U+0052 "R" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xfc,
    0xcc, 0xc6, 0xc6,

    /* U+0053 "S" */
    0x7c, 0xff, 0xc3, 0xc0, 0x7c, 0x7c, 0x3, 0x3,
    0xc3, 0xff, 0x7c,

    /* U+0054 "T" */
    0x7c, 0xff, 0xdb, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18,

    /* U+0055 "U" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xff, 0x7c,

    /* U+0056 "V" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x6c, 0x6c,
    0x6c, 0x18, 0x18,

    /* U+0057 "W" */
    0xc0, 0x79, 0x8f, 0x31, 0xe6, 0x3c, 0xc7, 0x98,
    0xf6, 0xde, 0xdb, 0xdb, 0x7f, 0x7d, 0x83, 0x0,

    /* U+0058 "X" */
    0xc3, 0xef, 0x6c, 0x6c, 0x18, 0x18, 0x6c, 0x6c,
    0x6c, 0xc3, 0xc3,

    /* U+0059 "Y" */
    0xc3, 0xc3, 0xc3, 0x6c, 0x6c, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18,

    /* U+005A "Z" */
    0xfc, 0xff, 0xc3, 0x3, 0x7c, 0x7c, 0xc0, 0xc3,
    0xc3, 0x7f, 0x7f,

    /* U+005B "[" */
    0x3f, 0xf1, 0x8c, 0x63, 0x18, 0xc6, 0x30, 0x73,
    0x80,

    /* U+005C "\\" */
    0xc0, 0x81, 0x81, 0x3, 0x2, 0x6, 0x4, 0xc,
    0x8, 0x18,

    /* U+005D "]" */
    0xe7, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc7, 0xce,
    0x0,

    /* U+005E "^" */
    0x54,

    /* U+005F "_" */
    0xff, 0xc0,

    /* U+0060 "`" */
    0x88, 0x80,

    /* U+0061 "a" */
    0x7c, 0x3f, 0xcc, 0x33, 0xc, 0xc3, 0x30, 0xc7,
    0xcd, 0xf3,

    /* U+0062 "b" */
    0xc0, 0xc0, 0xc0, 0xfc, 0xfc, 0xc3, 0xc3, 0xc3,
    0xc3, 0xff, 0x7c,

    /* U+0063 "c" */
    0x7c, 0xff, 0xc3, 0xc0, 0xc0, 0xc3, 0xff, 0x7c,

    /* U+0064 "d" */
    0x3, 0x3, 0x3, 0x7f, 0x7f, 0xc3, 0xc3, 0xc3,
    0xc3, 0xff, 0x7c,

    /* U+0065 "e" */
    0x7c, 0xff, 0xc3, 0xf8, 0xfb, 0xc3, 0x7c, 0x7c,

    /* U+0066 "f" */
    0x7c, 0xff, 0xc3, 0xc0, 0xf8, 0xf8, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0,

    /* U+0067 "g" */
    0x7c, 0xff, 0xc3, 0xc3, 0x7f, 0x7f, 0x3, 0x3,
    0xc3, 0xff, 0x7c,

    /* U+0068 "h" */
    0xc0, 0xc0, 0xc0, 0xdc, 0xdc, 0xe3, 0xe3, 0xc3,
    0xc3, 0xc3, 0xc3,

    /* U+0069 "i" */
    0xf3, 0xff, 0xfc,

    /* U+006A "j" */
    0xc, 0x30, 0xc3, 0xcf, 0x33, 0x8e,

    /* U+006B "k" */
    0xc0, 0xc0, 0xc0, 0xc3, 0xdf, 0xdc, 0xe0, 0xe0,
    0xdc, 0xdf, 0xc3,

    /* U+006C "l" */
    0xff, 0xff, 0xfc,

    /* U+006D "m" */
    0xf3, 0x9e, 0x73, 0x31, 0xe6, 0x3c, 0xc7, 0x98,
    0xf3, 0x1e, 0x63,

    /* U+006E "n" */
    0xfc, 0xfc, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,

    /* U+006F "o" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+0070 "p" */
    0x7c, 0x7c, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xfc,
    0xc0, 0xc0, 0xc0,

    /* U+0071 "q" */
    0x7c, 0x7c, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0x7f,
    0x3, 0x3, 0x3,

    /* U+0072 "r" */
    0x7c, 0x7c, 0xc3, 0xc3, 0xc0, 0xc0, 0xc0, 0xc0,

    /* U+0073 "s" */
    0x7f, 0xff, 0xc0, 0x7c, 0x7c, 0x3, 0xff, 0xfc,

    /* U+0074 "t" */
    0x60, 0x18, 0x6, 0x3, 0xfc, 0xff, 0x18, 0x6,
    0x1, 0x80, 0x60, 0xdf, 0xf1, 0xf0,

    /* U+0075 "u" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+0076 "v" */
    0xc3, 0xc3, 0xc3, 0x6c, 0x6c, 0x6c, 0x18, 0x18,

    /* U+0077 "w" */
    0xcc, 0x79, 0x8f, 0x31, 0xe6, 0x3c, 0xc7, 0x98,
    0xfc, 0xe7, 0x9c,

    /* U+0078 "x" */
    0xc3, 0xef, 0x6c, 0x18, 0x18, 0x6c, 0xef, 0xc3,

    /* U+0079 "y" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0x7f, 0x3, 0x3,
    0xc3, 0xff, 0x7c,

    /* U+007A "z" */
    0xff, 0xff, 0xc, 0x18, 0x18, 0x60, 0xff, 0xff,

    /* U+007B "{" */
    0x18, 0x66, 0xdb, 0x61, 0x8e, 0x30, 0x61, 0x86,
    0x1b, 0x6c, 0x61, 0x80,

    /* U+007C "|" */
    0xff, 0xff, 0xfc,

    /* U+007D "}" */
    0x61, 0x8d, 0xb6, 0x18, 0x61, 0xc3, 0x18, 0x61,
    0xb6, 0xd9, 0x86, 0x0,

    /* U+007E "~" */
    0xdb,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xf3, 0xff, 0xfc,

    /* U+00A2 "¢" */
    0x18, 0x18, 0x7c, 0x7c, 0xc3, 0xc3, 0xc0, 0xc3,
    0xff, 0x7c, 0x18, 0x18,

    /* U+00A3 "£" */
    0x1f, 0x1f, 0xf6, 0xd, 0x80, 0x7c, 0x1f, 0x6,
    0x1, 0x80, 0x60, 0xff, 0xff, 0xf0,

    /* U+00A4 "¤" */
    0xb5, 0x24, 0x8c, 0x84,

    /* U+00A5 "¥" */
    0xc3, 0xc3, 0xc3, 0x6c, 0x6c, 0x7c, 0x7c, 0x18,
    0x7c, 0x7c, 0x18,

    /* U+00A7 "§" */
    0x7c, 0xff, 0xc3, 0xc0, 0x7c, 0x7c, 0xff, 0xff,
    0xc0, 0x7f, 0x7f,

    /* U+00A8 "¨" */
    0xde, 0xc0,

    /* U+00A9 "©" */
    0x74, 0x73, 0x57, 0x0,

    /* U+00AA "ª" */
    0x64, 0xa4, 0xd6, 0x0,

    /* U+00AB "«" */
    0x3, 0x18, 0x18, 0xc3, 0xc, 0x39, 0xe1, 0x8c,
    0x30, 0xc3, 0x9e, 0x18, 0xc0, 0x61, 0x83, 0xc,
    0x6, 0x30, 0x39, 0xe0, 0xc3, 0x1, 0x8c, 0xc,
    0x60,

    /* U+00AC "¬" */
    0xff, 0xfc, 0x18, 0x30,

    /* U+00AE "®" */
    0x74, 0x6b, 0x17, 0x0,

    /* U+00AF "¯" */
    0xff, 0xc0,

    /* U+00B0 "°" */
    0xf5, 0x0,

    /* U+00B1 "±" */
    0x30, 0x63, 0xff, 0xf3, 0x6, 0x0, 0x7f, 0xfe,

    /* U+00B2 "²" */
    0x69, 0x1e, 0x96,

    /* U+00B3 "³" */
    0x69, 0x17, 0x96,

    /* U+00B4 "´" */
    0x2a, 0x0,

    /* U+00B5 "µ" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfc, 0xfc,
    0xc0, 0xc0, 0xc0,

    /* U+00B6 "¶" */
    0x3f, 0xff, 0xff, 0xfc, 0xff, 0x9f, 0xf3, 0xfe,
    0x4f, 0xc9, 0xf9, 0x1, 0x20, 0x24, 0x4, 0x80,
    0x90, 0x12, 0x2, 0x40,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0x21, 0xa2, 0xe0,

    /* U+00B9 "¹" */
    0x54, 0x92, 0x40,

    /* U+00BA "º" */
    0x6a, 0x99, 0x66,

    /* U+00BB "»" */
    0xc6, 0x6, 0x30, 0x18, 0x60, 0xf3, 0x81, 0x8c,
    0x6, 0x18, 0x3c, 0xe0, 0x63, 0xc, 0x30, 0x61,
    0x86, 0x30, 0xf3, 0x86, 0x18, 0x63, 0x3, 0x18,
    0x0,

    /* U+00BC "¼" */
    0xe0, 0xc1, 0xe, 0x8, 0x70, 0x43, 0x2, 0x30,
    0x7, 0x8c, 0x3c, 0xa1, 0x8d, 0x18, 0x78, 0xc0,
    0x46, 0x2,

    /* U+00BD "½" */
    0xe1, 0x82, 0x38, 0x23, 0x82, 0x30, 0x2c, 0x61,
    0xc9, 0x1c, 0x11, 0x86, 0x30, 0x93, 0x9, 0x30,
    0x60,

    /* U+00BE "¾" */
    0xf0, 0x60, 0x43, 0x86, 0xe, 0x4, 0x30, 0x93,
    0x1, 0x9c, 0x30, 0x71, 0x41, 0x8d, 0xc, 0x3c,
    0x30, 0x10, 0xc0, 0x40,

    /* U+00BF "¿" */
    0x18, 0x18, 0x0, 0x18, 0x78, 0x78, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+00C0 "À" */
    0x20, 0x10, 0x0, 0x0, 0x18, 0x7c, 0x6c, 0xc3,
    0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C1 "Á" */
    0x8, 0x10, 0x0, 0x0, 0x18, 0x7c, 0x6c, 0xc3,
    0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C2 "Â" */
    0x10, 0x28, 0x0, 0x0, 0x18, 0x7c, 0x6c, 0xc3,
    0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C3 "Ã" */
    0x74, 0x7c, 0x0, 0x0, 0x18, 0x7c, 0x6c, 0xc3,
    0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C4 "Ä" */
    0x66, 0x66, 0x0, 0x0, 0x18, 0x7c, 0x6c, 0xc3,
    0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C5 "Å" */
    0x18, 0x18, 0x0, 0x0, 0x18, 0x7c, 0x6c, 0xc3,
    0xc3, 0xc3, 0xff, 0xff, 0xc3, 0xc3, 0xc3,

    /* U+00C6 "Æ" */
    0x18, 0xf9, 0xff, 0xf6, 0x70, 0xf0, 0xc0, 0xc3,
    0xe3, 0xf, 0x8f, 0xf0, 0x3f, 0xc0, 0xc3, 0xf,
    0xf, 0xfc, 0x3f, 0x80,

    /* U+00C7 "Ç" */
    0x7c, 0xff, 0xc3, 0xc3, 0xc0, 0xc0, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c, 0x18, 0x24, 0x18,

    /* U+00C8 "È" */
    0x30, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc0,
    0xf8, 0xf8, 0xc0, 0xc0, 0xc3, 0xff, 0x7c,

    /* U+00C9 "É" */
    0x18, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc0,
    0xf8, 0xf8, 0xc0, 0xc0, 0xc3, 0xff, 0x7c,

    /* U+00CA "Ê" */
    0x10, 0x18, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc0,
    0xf8, 0xf8, 0xc0, 0xc0, 0xc3, 0xff, 0x7c,

    /* U+00CB "Ë" */
    0x66, 0x66, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc0,
    0xf8, 0xf8, 0xc0, 0xc0, 0xc3, 0xff, 0x7c,

    /* U+00CC "Ì" */
    0xc3, 0x0, 0xf, 0xfd, 0x8c, 0x63, 0x18, 0xc6,
    0x7f, 0xe0,

    /* U+00CD "Í" */
    0x64, 0x0, 0xf, 0xfd, 0x8c, 0x63, 0x18, 0xc6,
    0x7f, 0xe0,

    /* U+00CE "Î" */
    0x63, 0x80, 0xf, 0xfd, 0x8c, 0x63, 0x18, 0xc6,
    0x7f, 0xe0,

    /* U+00CF "Ï" */
    0xde, 0xc0, 0xf, 0xfc, 0x84, 0x21, 0x8, 0x42,
    0x7f, 0xe0,

    /* U+00D0 "Ð" */
    0xf8, 0x7f, 0x31, 0x98, 0x3c, 0x1f, 0x8f, 0xc7,
    0x83, 0xc6, 0x7f, 0x3e, 0x0,

    /* U+00D1 "Ñ" */
    0x34, 0x3c, 0x0, 0x0, 0xc3, 0xe3, 0xe3, 0xdb,
    0xdb, 0xdb, 0xdb, 0xdb, 0xc7, 0xc7, 0xc3,

    /* U+00D2 "Ò" */
    0x30, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00D3 "Ó" */
    0x18, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00D4 "Ô" */
    0x10, 0x18, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00D5 "Õ" */
    0x74, 0x7c, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00D6 "Ö" */
    0x66, 0x66, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00D7 "×" */
    0x85, 0x99, 0xe1, 0x87, 0x99, 0xa1, 0x0,

    /* U+00D8 "Ø" */
    0x3, 0x7e, 0xff, 0xcf, 0xcf, 0xdb, 0xd3, 0xf3,
    0xe3, 0xe3, 0x7c, 0xfc, 0x80,

    /* U+00D9 "Ù" */
    0x30, 0x10, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0x7c,

    /* U+00DA "Ú" */
    0x18, 0x10, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0x7c,

    /* U+00DB "Û" */
    0x10, 0x18, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0x7c,

    /* U+00DC "Ü" */
    0x66, 0x66, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0x7c,

    /* U+00DD "Ý" */
    0x18, 0x20, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0x6c,
    0x6c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,

    /* U+00DE "Þ" */
    0xc0, 0xfc, 0xfc, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xfc, 0xfc, 0xc0,

    /* U+00DF "ß" */
    0xfc, 0xff, 0xc3, 0xc3, 0xdc, 0xdc, 0xc3, 0xc3,
    0xc3, 0xdc, 0xdc,

    /* U+00E0 "à" */
    0x20, 0x6, 0x0, 0x0, 0x0, 0x7c, 0x3f, 0xcc,
    0x33, 0xc, 0xc3, 0x30, 0xc7, 0xcd, 0xf3,

    /* U+00E1 "á" */
    0x8, 0x4, 0x0, 0x0, 0x0, 0x7c, 0x3f, 0xcc,
    0x33, 0xc, 0xc3, 0x30, 0xc7, 0xcd, 0xf3,

    /* U+00E2 "â" */
    0x18, 0x0, 0x0, 0x1, 0xf0, 0xff, 0x30, 0xcc,
    0x33, 0xc, 0xc3, 0x1f, 0x37, 0xcc,

    /* U+00E3 "ã" */
    0x3c, 0x0, 0x0, 0x1, 0xf0, 0xff, 0x30, 0xcc,
    0x33, 0xc, 0xc3, 0x1f, 0x37, 0xcc,

    /* U+00E4 "ä" */
    0x66, 0x19, 0x80, 0x1, 0xf0, 0xff, 0x30, 0xcc,
    0x33, 0xc, 0xc3, 0x1f, 0x37, 0xcc,

    /* U+00E5 "å" */
    0x18, 0x6, 0x0, 0x1, 0xf0, 0xff, 0x30, 0xcc,
    0x33, 0xc, 0xc3, 0x1f, 0x37, 0xcc,

    /* U+00E6 "æ" */
    0x7c, 0x1f, 0x7f, 0xbf, 0xf0, 0xd8, 0x78, 0x6f,
    0x8c, 0x37, 0xde, 0x1b, 0xd, 0xf3, 0x7c, 0xf9,
    0xbe,

    /* U+00E7 "ç" */
    0x7c, 0xff, 0xc3, 0xc0, 0xc3, 0xc3, 0x7c, 0x7c,
    0x18, 0x24, 0x18,

    /* U+00E8 "è" */
    0x20, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xf8,
    0xfb, 0xc3, 0x7c, 0x7c,

    /* U+00E9 "é" */
    0x8, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xf8,
    0xfb, 0xc3, 0x7c, 0x7c,

    /* U+00EA "ê" */
    0x18, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xf8, 0xfb,
    0xc3, 0x7c, 0x7c,

    /* U+00EB "ë" */
    0x66, 0x66, 0x0, 0x7c, 0xff, 0xc3, 0xf8, 0xfb,
    0xc3, 0x7c, 0x7c,

    /* U+00EC "ì" */
    0x8c, 0x6, 0xdb, 0x6d, 0xb0,

    /* U+00ED "í" */
    0x38, 0x6, 0xdb, 0x6d, 0xb0,

    /* U+00EE "î" */
    0xc3, 0xff, 0xfc,

    /* U+00EF "ï" */
    0xff, 0x6, 0x66, 0x66, 0x66, 0x60,

    /* U+00F0 "ð" */
    0x1, 0x81, 0x9f, 0x9f, 0xec, 0xf0, 0x19, 0xfc,
    0xfe, 0xc3, 0x61, 0xb0, 0xcf, 0x87, 0xc0,

    /* U+00F1 "ñ" */
    0x3c, 0x0, 0x0, 0xfc, 0xfc, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3,

    /* U+00F2 "ò" */
    0x20, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00F3 "ó" */
    0x8, 0x10, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3,
    0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00F4 "ô" */
    0x18, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+00F5 "õ" */
    0x7c, 0x0, 0x0, 0x7c, 0xff, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+00F6 "ö" */
    0x66, 0x66, 0x0, 0x7c, 0xff, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+00F7 "÷" */
    0x30, 0xc0, 0x3f, 0xfc, 0x3, 0xc,

    /* U+00F8 "ø" */
    0x1, 0x80, 0xcf, 0xcf, 0xf6, 0x7b, 0x6d, 0xa6,
    0xf3, 0x3e, 0x1f, 0x10, 0x18, 0x0,

    /* U+00F9 "ù" */
    0x20, 0x10, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00FA "ú" */
    0x8, 0x10, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0x7c, 0x7c,

    /* U+00FB "û" */
    0x18, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+00FC "ü" */
    0x66, 0x66, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0x7c, 0x7c,

    /* U+00FD "ý" */
    0x8, 0x10, 0x0, 0x0, 0xc3, 0xc3, 0xc3, 0xc3,
    0xff, 0x7f, 0x3, 0x3, 0xc3, 0xff, 0x7c,

    /* U+00FE "þ" */
    0xc0, 0xc0, 0xfc, 0xff, 0xc3, 0xc3, 0xc3, 0xc3,
    0xfc, 0xfc, 0xc0,

    /* U+00FF "ÿ" */
    0x66, 0x66, 0x0, 0xc3, 0xc3, 0xc3, 0xc3, 0xff,
    0x7f, 0x3, 0x3, 0xc3, 0xff, 0x7c
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 60, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 126, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 7, .adv_w = 184, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 35, .adv_w = 236, .box_w = 13, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 184, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 73, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 66, .adv_w = 102, .box_w = 5, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 76, .adv_w = 102, .box_w = 5, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 86, .adv_w = 73, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 88, .adv_w = 163, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 96, .adv_w = 73, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 98, .adv_w = 91, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 100, .adv_w = 60, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 110, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 60, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 219, .adv_w = 73, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 163, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 237, .adv_w = 163, .box_w = 6, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 241, .adv_w = 163, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 256, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 275, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 110, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 381, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 209, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 419, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 159, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 466, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 499, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 510, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 209, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 548, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 570, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 579, .adv_w = 144, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 598, .adv_w = 73, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 599, .adv_w = 120, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 601, .adv_w = 72, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 603, .adv_w = 184, .box_w = 10, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 613, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 632, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 643, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 662, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 673, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 684, .adv_w = 60, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 687, .adv_w = 135, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 693, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 704, .adv_w = 60, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 707, .adv_w = 209, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 718, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 734, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 745, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 756, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 772, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 786, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 794, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 802, .adv_w = 209, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 813, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 832, .adv_w = 159, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 126, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 852, .adv_w = 60, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 855, .adv_w = 126, .box_w = 6, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 867, .adv_w = 163, .box_w = 4, .box_h = 2, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 868, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 869, .adv_w = 60, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 872, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 884, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 898, .adv_w = 123, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 902, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 913, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 924, .adv_w = 82, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 9},
    {.bitmap_index = 926, .adv_w = 111, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 930, .adv_w = 108, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 934, .adv_w = 237, .box_w = 13, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 959, .adv_w = 163, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 963, .adv_w = 111, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 967, .adv_w = 82, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 969, .adv_w = 85, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 971, .adv_w = 163, .box_w = 7, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 979, .adv_w = 99, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 982, .adv_w = 99, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 985, .adv_w = 82, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 987, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 998, .adv_w = 208, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1018, .adv_w = 60, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1019, .adv_w = 82, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 1022, .adv_w = 74, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1025, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1028, .adv_w = 232, .box_w = 13, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1053, .adv_w = 239, .box_w = 13, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1071, .adv_w = 222, .box_w = 12, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1088, .adv_w = 260, .box_w = 14, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1108, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1119, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1134, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1149, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1164, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1179, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1194, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1209, .adv_w = 258, .box_w = 14, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1229, .adv_w = 159, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1243, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1258, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1273, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1288, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1303, .adv_w = 110, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1313, .adv_w = 110, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1323, .adv_w = 110, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1333, .adv_w = 110, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1343, .adv_w = 159, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1356, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1371, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1386, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1401, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1416, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1431, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1446, .adv_w = 163, .box_w = 7, .box_h = 7, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1453, .adv_w = 169, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1466, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1481, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1496, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1511, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1526, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1541, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1552, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1563, .adv_w = 184, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1578, .adv_w = 184, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1593, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1607, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1621, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1635, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1649, .adv_w = 308, .box_w = 17, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1666, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1677, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1689, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1701, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1712, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1723, .adv_w = 60, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1728, .adv_w = 60, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1733, .adv_w = 60, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1736, .adv_w = 60, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1742, .adv_w = 165, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1757, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1768, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1780, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1792, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1803, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1814, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1825, .adv_w = 163, .box_w = 6, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 1831, .adv_w = 159, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1845, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1857, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1869, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1880, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1891, .adv_w = 159, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1906, .adv_w = 159, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1917, .adv_w = 159, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -3}
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
    0, 0, 0, 0, -14, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -14, 0, 0, 0, 0, 0, 0, 0,
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
    0, 0, 0, 0, 0, 0, 0, -11,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -11,
    -11, 0, -5, 0, -5, 0, 0, -5,
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
    0, -11, -8, 0, -8, -5, -5, -5,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, -11, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -16, 0,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, -11, -14,
    0, -5, 0, -5, 0, 0, 0, 0,
    0, 0, -5, 0, 0, 0, 0, -5,
    -5, -5, -5, -5, -5, -5, 0, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, -8, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, 0, 0,
    -14, 0, 0, 0, 0, 0, 0, -14,
    -5, 0, 0, -5, 0, 0, -5, 0,
    -14, -16, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, -14, 0, -8, 0, -8, -5, -5,
    0, 0, -24, 0, -5, 0, -3, -16,
    -5, -5, -5, -5, -8, -24, -11, -5,
    -8, -8, -5, -5, -5, -11, -8, -5,
    -27, -8, -27, -27, -11, -30, -8, 0,
    -54, -8, -11, -14, -27, -27, 0, -16,
    -19, -22, -24, -24, -24, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -14, 0, -14,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, -8, 0, 0, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, -8, -11,
    0, -5, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    -8, 0, -14, 0, -14, 0, 0, -11,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -8, -11, 0, 0, -8, -8, 0,
    0, 0, -22, 0, -33, 0, 0, -8,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -14, -33, 0, -8,
    -5, -5, -5, -5, 0, -11, 0, 0,
    -5, -5, 0, 0, -5, -27, 0, -27,
    0, -11, 0, 0, 0, 0, -14, 0,
    0, -5, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -5, -11, -8, 0, -8, -5, -8, -5,
    -5, 0, -5, 0, 0, -5, -5, -5,
    -8, -8, -5, -5, -8, -5, 0, -5,
    -5, 0, 0, -24, -8, -8, -16, -5,
    -11, -8, -8, -8, -8, 0, 0, -11,
    0, -11, -11, -8, -11, 0, -16, -11,
    0, -27, -5, -27, -27, -8, -27, -8,
    -8, 0, -8, -8, -11, -27, -27, 0,
    -14, 0, -16, 0, -22, -14, -27, -14,
    0, 0, 0, -8, 0, -8, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, -3, -8, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, -11, -5, -8, -8, 0,
    -8, 0, 0, 0, -5, 0, -5, -5,
    -8, 0, -5, -3, -5, -5, -5, -5,
    0, 0, 0, 0, 0, -11, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, 0, -5, 0,
    -8, 0, -5, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, -8, -5,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, -8, -11, -8, -11, -11, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, -19, 0, -16,
    0, 0, -8, 0, 0, 0, 0, 0,
    -5, -5, -8, 0, 0, 0, -5, -5,
    -5, -5, 0, 0, -14, -14, -5, 0,
    -5, -14, -8, 0, 0, -5, -5, 0,
    -8, -5, -5, -8, -5, -8, -5, -5,
    0, -5, -19, 0, -22, -22, 0, -24,
    0, 0, -44, -11, 0, -14, 0, -27,
    -14, -19, -19, -19, -19, -14, -16, 0,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, 0, -11,
    0, -11, 0, 0, 0, 0, 0, 0,
    0, 0, -5, -5, -5, -11, -5, -8,
    -5, -5, -5, -5, 0, 0, 0, 0,
    -19, -35, -8, -11, -19, 0, -14, 0,
    0, -16, 0, -16, -11, -11, -16, -27,
    -8, -14, -54, 0, -14, -14, -14, -14,
    -14, -14, -8, -8, -16, -11, -8, -8,
    0, -8, -30, -11, -24, -8, -11, -19,
    -8, 0, 0, 0, 0, 0, -33, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, -27,
    0, -5, 0, -5, 0, 0, -5, 0,
    0, 0, 0, -5, -3, -5, -5, -14,
    0, -11, -8, -8, -5, 0, 0, 0,
    0, 0, -8, -33, -5, 0, -8, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, -8, 0, -16, -27, 0, -8, -5,
    -8, -5, -5, -8, -5, 0, 0, -5,
    -5, 0, 0, -5, -14, -5, -8, 0,
    -8, -8, 0, 0, 0, 0, 0, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, -5, -5, -5, -5, -5,
    -5, -5, 0, -5, -5, -5, -5, -5,
    -5, -11, -5, -8, -5, -5, -5, -5,
    0, 0, 0, 0, -14, 0, -11, 0,
    -14, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -11, 0, 0, -19, 0,
    -27, -8, -27, -27, 0, -27, -8, 0,
    -52, -16, -14, -19, -30, -27, -24, -14,
    -19, -14, -22, -22, -16, 0, 0, 0,
    0, -5, -33, -5, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, -27, 0, -8, -5, -5,
    -5, -5, -5, -5, 0, -5, -5, -5,
    0, 0, -5, -11, -5, -11, 0, 0,
    0, 0, 0, 0, 0, 0, -8, -27,
    -5, 0, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    -27, 0, -5, 0, -8, -5, 0, -5,
    0, 0, 0, 0, 0, 0, 0, -5,
    -11, -5, -8, -5, -8, -5, 0, 0,
    0, 0, 0, -5, -8, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, 0, -8, 0, -5,
    -3, -5, 0, 0, -5, 0, 0, -5,
    0, 0, -5, 0, 0, -8, 0, 0,
    -5, 0, -5, -3, 0, 0, 0, 0,
    -5, -33, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -27, 0, 0, 0, 0, -5,
    0, -5, -8, 0, 0, 0, 0, 0,
    -5, 0, -11, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -16, -3, -14, -14, -5, -3, -5,
    0, 0, -5, -5, -5, -5, -5, -11,
    -5, -8, -5, 0, -5, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    -8, 0, 0, -5, 0, 0, 0, -8,
    0, -5, -3, -5, -8, -5, -8, -5,
    -5, -5, 0, 0, 0, 0, 0, -5,
    -33, -5, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -8, 0,
    0, -27, 0, -3, -5, -3, -5, -5,
    -5, -5, 0, -5, -5, -5, 0, 0,
    -5, -11, -5, -11, 0, 0, 0, 0,
    0, 0, -41, 0, -3, -33, -3, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -8, 0, 0, -27, 0,
    -8, -5, -5, -8, 0, -5, 0, 0,
    0, -5, -5, -8, -8, -5, 0, -5,
    -5, -5, -5, -5, -5, 0, 0, 0,
    0, -5, -33, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, -24, 0, -5, -8, -5,
    0, 0, -5, 0, 0, 0, 0, 0,
    0, -5, -5, 0, 0, -5, -3, -3,
    -5, -5, 0, 0, 0, 0, 0, -33,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    -27, 0, -8, -5, -5, -5, -5, -8,
    -5, 0, -5, -5, -5, -5, 0, -5,
    -19, -5, -19, -8, -8, 0, -5, 0,
    0, 0, 0, -5, -33, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, -16, -27, 0, -5,
    0, -3, -5, 0, -3, -8, 0, 0,
    0, 0, 0, -3, -3, -11, -5, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, -19, 0, 0, -5, 0, 0, 0,
    0, -11, 0, 0, 0, 0, 0, -5,
    0, 0, -27, 0, -11, -8, -8, -11,
    0, -5, -8, -5, -11, -3, -5, -5,
    0, -5, -8, -5, -5, -5, -5, -5,
    -5, 0, 0, 0, 0, -5, -33, 0,
    0, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, -27,
    0, -8, -5, -5, -5, -3, -3, -5,
    0, -5, -3, -3, 0, -5, -3, -8,
    -8, 0, 0, 0, 0, -3, 0, 0,
    0, 0, -5, -33, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, 0, -27, 0, -14, -5,
    -14, -5, -5, 0, -8, 0, 0, -5,
    -5, 0, 0, 0, -8, -5, -5, -5,
    0, -5, -5, 0, 0, 0, 0, -5,
    -33, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -27, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, -33, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -27, 0,
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
    .line_height = 19,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
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

