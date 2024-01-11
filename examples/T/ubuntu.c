/*******************************************************************************
 * Size: 8 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef UBUNTU
#define UBUNTU 1
#endif

#if UBUNTU

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0021 "!" */
    0xf8,

    /* U+0022 "\"" */
    0xc0,

    /* U+0023 "#" */
    0x53, 0x95, 0xf6, 0x0,

    /* U+0024 "$" */
    0x4e, 0x23, 0x50,

    /* U+0025 "%" */
    0xe2, 0xc6, 0x8d, 0x1c,

    /* U+0026 "&" */
    0x63, 0x19, 0x2f, 0x80,

    /* U+0027 "'" */
    0x80,

    /* U+0028 "(" */
    0x6a, 0xa4,

    /* U+0029 ")" */
    0xa5, 0x58,

    /* U+002A "*" */
    0x4c, 0x80,

    /* U+002B "+" */
    0x27, 0xc8, 0x40,

    /* U+002C "," */
    0xc0,

    /* U+002D "-" */
    0xc0,

    /* U+002E "." */
    0x0,

    /* U+002F "/" */
    0x5, 0x25, 0x24,

    /* U+0030 "0" */
    0x69, 0x99, 0x60,

    /* U+0031 "1" */
    0x55, 0x40,

    /* U+0032 "2" */
    0x71, 0x24, 0xe0,

    /* U+0033 "3" */
    0xe5, 0x1e,

    /* U+0034 "4" */
    0x32, 0x95, 0xf1, 0x0,

    /* U+0035 "5" */
    0x66, 0x11, 0xe0,

    /* U+0036 "6" */
    0x6e, 0x99, 0x60,

    /* U+0037 "7" */
    0xf2, 0x24, 0x40,

    /* U+0038 "8" */
    0xe9, 0x69, 0xf0,

    /* U+0039 "9" */
    0xe9, 0x71, 0x60,

    /* U+003A ":" */
    0x0,

    /* U+003B ";" */
    0x18,

    /* U+003C "<" */
    0x1e, 0x60,

    /* U+003D "=" */
    0xf0, 0xf0,

    /* U+003E ">" */
    0x87, 0x60,

    /* U+003F "?" */
    0xc5, 0x4,

    /* U+0040 "@" */
    0x7d, 0x77, 0x2e, 0x57, 0xc7, 0x0,

    /* U+0041 "A" */
    0x22, 0x94, 0xf8, 0x80,

    /* U+0042 "B" */
    0xf9, 0xe9, 0xf0,

    /* U+0043 "C" */
    0x74, 0x21, 0x7, 0x0,

    /* U+0044 "D" */
    0xf4, 0x63, 0x1f, 0x0,

    /* U+0045 "E" */
    0xf8, 0xe8, 0xf0,

    /* U+0046 "F" */
    0xf8, 0xe8, 0x80,

    /* U+0047 "G" */
    0x78, 0x89, 0x70,

    /* U+0048 "H" */
    0x99, 0xf9, 0x90,

    /* U+0049 "I" */
    0xf8,

    /* U+004A "J" */
    0x11, 0x11, 0xf0,

    /* U+004B "K" */
    0x9a, 0xca, 0x90,

    /* U+004C "L" */
    0x88, 0x88, 0xf0,

    /* U+004D "M" */
    0xcf, 0x3b, 0x6d, 0x84,

    /* U+004E "N" */
    0x9d, 0xdb, 0x90,

    /* U+004F "O" */
    0x74, 0x63, 0x17, 0x0,

    /* U+0050 "P" */
    0xf9, 0xe8, 0x80,

    /* U+0051 "Q" */
    0x74, 0x63, 0x17, 0x18,

    /* U+0052 "R" */
    0xf9, 0xea, 0x90,

    /* U+0053 "S" */
    0xf8, 0x61, 0xf0,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x0,

    /* U+0055 "U" */
    0x99, 0x99, 0x60,

    /* U+0056 "V" */
    0x8c, 0x94, 0xa2, 0x0,

    /* U+0057 "W" */
    0x83, 0x26, 0xab, 0x64, 0x40,

    /* U+0058 "X" */
    0x93, 0x8, 0xa9, 0x0,

    /* U+0059 "Y" */
    0x8a, 0x88, 0x42, 0x0,

    /* U+005A "Z" */
    0xf2, 0x44, 0xf0,

    /* U+005B "[" */
    0xea, 0xaa,

    /* U+005C "\\" */
    0x12, 0x44, 0x89,

    /* U+005D "]" */
    0xd5, 0x55,

    /* U+005E "^" */
    0x26, 0x90,

    /* U+005F "_" */
    0xf0,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x6e, 0xf0,

    /* U+0062 "b" */
    0x88, 0xe9, 0x9e,

    /* U+0063 "c" */
    0x72, 0x30,

    /* U+0064 "d" */
    0x11, 0x79, 0x97,

    /* U+0065 "e" */
    0x6e, 0x86,

    /* U+0066 "f" */
    0xf3, 0x49, 0x0,

    /* U+0067 "g" */
    0x79, 0x97, 0x70,

    /* U+0068 "h" */
    0x93, 0xdb, 0x40,

    /* U+0069 "i" */
    0x3c,

    /* U+006A "j" */
    0x5, 0x5c,

    /* U+006B "k" */
    0x88, 0xac, 0xca,

    /* U+006C "l" */
    0xaa, 0xb0,

    /* U+006D "m" */
    0xfd, 0x6b, 0x50,

    /* U+006E "n" */
    0xf6, 0xd0,

    /* U+006F "o" */
    0x69, 0x96,

    /* U+0070 "p" */
    0xe9, 0x9e, 0x80,

    /* U+0071 "q" */
    0x79, 0x97, 0x10,

    /* U+0072 "r" */
    0xf2, 0x40,

    /* U+0073 "s" */
    0xf1, 0xf0,

    /* U+0074 "t" */
    0x9a, 0x4e,

    /* U+0075 "u" */
    0xb6, 0xf0,

    /* U+0076 "v" */
    0x9a, 0x64,

    /* U+0077 "w" */
    0xb6, 0xd7, 0x92,

    /* U+0078 "x" */
    0xa6, 0x69,

    /* U+0079 "y" */
    0x9a, 0x64, 0xc0,

    /* U+007A "z" */
    0x69, 0x60,

    /* U+007B "{" */
    0x69, 0x44, 0x98,

    /* U+007C "|" */
    0xff,

    /* U+007D "}" */
    0x49, 0x14, 0x90,

    /* U+007E "~" */
    0x4b,

    /* U+00A8 "¨" */
    0xa0,

    /* U+00B4 "´" */
    0x40,

    /* U+00C1 "Á" */
    0x11, 0x8, 0xa5, 0x3e, 0x20,

    /* U+00C2 "Â" */
    0x20, 0x88, 0xa5, 0x3e, 0x20,

    /* U+00C7 "Ç" */
    0x74, 0x21, 0x7, 0x0,

    /* U+00C9 "É" */
    0x20, 0xf8, 0xe8, 0xf0,

    /* U+00CD "Í" */
    0x6a, 0xa8,

    /* U+00D3 "Ó" */
    0x1, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00D4 "Ô" */
    0x20, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00DA "Ú" */
    0x20, 0x99, 0x99, 0x60,

    /* U+00E0 "à" */
    0x1, 0xbb, 0xc0,

    /* U+00E1 "á" */
    0x9, 0xbb, 0xc0,

    /* U+00E2 "â" */
    0x41, 0xbb, 0xc0,

    /* U+00E7 "ç" */
    0x72, 0x32,

    /* U+00E9 "é" */
    0x20, 0x6e, 0x86,

    /* U+00ED "í" */
    0x6a, 0xa0,

    /* U+00F3 "ó" */
    0x20, 0x69, 0x96,

    /* U+00F4 "ô" */
    0x0, 0x69, 0x96,

    /* U+00FA "ú" */
    0x2, 0xdb, 0xc0,

    /* U+01F4 "Ǵ" */
    0x0, 0x78, 0x89, 0x70
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 35, .box_w = 1, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 51, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 2, .adv_w = 84, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 72, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 9, .adv_w = 108, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 84, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 31, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 18, .adv_w = 40, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 20, .adv_w = 40, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 22, .adv_w = 61, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 24, .adv_w = 72, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 31, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 28, .adv_w = 37, .box_w = 2, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 29, .adv_w = 31, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 47, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 33, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 72, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 72, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 72, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 59, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 31, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 31, .box_w = 1, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 64, .adv_w = 72, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 66, .adv_w = 72, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 68, .adv_w = 72, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 70, .adv_w = 50, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 121, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 78, .adv_w = 83, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 81, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 78, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 90, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 67, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 85, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 89, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 33, .box_w = 1, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 63, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 78, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 65, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 111, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 91, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 99, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 77, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 99, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 133, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 67, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 71, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 87, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 81, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 118, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 79, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 74, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 72, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 41, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 168, .adv_w = 47, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 171, .adv_w = 41, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 173, .adv_w = 72, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 175, .adv_w = 63, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 176, .adv_w = 47, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 177, .adv_w = 66, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 60, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 71, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 49, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 74, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 195, .adv_w = 73, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 31, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 31, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 201, .adv_w = 65, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 34, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 110, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 73, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 75, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 216, .adv_w = 75, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 219, .adv_w = 49, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 56, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 50, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 73, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 62, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 99, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 65, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 62, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 237, .adv_w = 59, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 42, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 242, .adv_w = 34, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 243, .adv_w = 42, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 246, .adv_w = 72, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 247, .adv_w = 50, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 248, .adv_w = 47, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 249, .adv_w = 83, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 83, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 78, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 263, .adv_w = 72, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 33, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 87, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 66, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 66, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 66, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 60, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 294, .adv_w = 71, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 31, .box_w = 2, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 302, .adv_w = 75, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 73, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 85, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0xc, 0x19, 0x1a, 0x1f, 0x21, 0x25, 0x2b,
    0x2c, 0x32, 0x38, 0x39, 0x3a, 0x3f, 0x41, 0x45,
    0x4b, 0x4c, 0x52, 0x14c
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 33, .range_length = 94, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 168, .range_length = 333, .glyph_id_start = 95,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 20, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR >= 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR >= 8
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ubuntu = {
#else
lv_font_t ubuntu = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 9,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
    .fallback = NULL,
    .user_data = NULL
};



#endif /*#if UBUNTU*/

