/*******************************************************************************
 * Size: 18 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef ACE
#define ACE 1
#endif

#if ACE

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0021 "!" */
    0xfd,

    /* U+0022 "\"" */
    0xf0,

    /* U+0023 "#" */
    0x42, 0xff, 0x42, 0x42, 0x42, 0x42, 0xff, 0x42,

    /* U+0024 "$" */
    0x24, 0x24, 0xff, 0xa4, 0xa4, 0xff, 0x25, 0x25,
    0x25, 0xff, 0x24,

    /* U+0025 "%" */
    0xe1, 0xd1, 0xb9, 0x81, 0x81, 0xb9, 0x95, 0x8b,
    0x87,

    /* U+0026 "&" */
    0x7c, 0x22, 0x11, 0x1f, 0xa8, 0x74, 0x32, 0x1d,
    0xfa,

    /* U+0027 "'" */
    0xc0,

    /* U+0028 "(" */
    0xf2, 0x49, 0x27,

    /* U+0029 ")" */
    0xd5, 0x57,

    /* U+002A "*" */
    0x80,

    /* U+002B "+" */
    0x20, 0x82, 0x3f, 0x20, 0x80,

    /* U+002C "," */
    0xc0,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x1, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x80,

    /* U+0030 "0" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0031 "1" */
    0xc9, 0x24, 0x97,

    /* U+0032 "2" */
    0xff, 0x1, 0x1, 0xff, 0x80, 0x80, 0x80, 0xff,

    /* U+0033 "3" */
    0x1f, 0x1, 0x1, 0x3f, 0x1, 0x1, 0x1, 0xff,

    /* U+0034 "4" */
    0x84, 0x84, 0x84, 0xff, 0x4, 0x4, 0x4, 0x4,

    /* U+0035 "5" */
    0xfc, 0x80, 0x80, 0xff, 0x1, 0x1, 0x1, 0xff,

    /* U+0036 "6" */
    0xfc, 0x80, 0x80, 0xff, 0x81, 0x81, 0x81, 0xff,

    /* U+0037 "7" */
    0xfe, 0x4, 0x8, 0x10, 0x20, 0x40, 0x81,

    /* U+0038 "8" */
    0x7e, 0x42, 0x42, 0xff, 0x81, 0x81, 0x81, 0xff,

    /* U+0039 "9" */
    0xff, 0x81, 0x81, 0xff, 0x1, 0x1, 0x1, 0x3f,

    /* U+003A ":" */
    0xa0,

    /* U+003B ";" */
    0xb0,

    /* U+003C "<" */
    0x2f, 0x44, 0x40,

    /* U+003D "=" */
    0xfc, 0xf, 0xc0,

    /* U+003E ">" */
    0x99, 0x95, 0x0,

    /* U+003F "?" */
    0xff, 0x4, 0x8, 0xf1, 0x0, 0x0, 0x8,

    /* U+0040 "@" */
    0xff, 0x81, 0xbd, 0xa5, 0xbf, 0x80, 0x80, 0xfc,

    /* U+0041 "A" */
    0xff, 0x81, 0x81, 0xff, 0x81, 0x81, 0x81, 0x81,

    /* U+0042 "B" */
    0xfc, 0x84, 0x84, 0xff, 0x81, 0x81, 0x81, 0xff,

    /* U+0043 "C" */
    0xff, 0x2, 0x4, 0x8, 0x10, 0x20, 0x7f,

    /* U+0044 "D" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0045 "E" */
    0xe0, 0x80, 0x80, 0xfc, 0x80, 0x80, 0x80, 0xff,

    /* U+0046 "F" */
    0xff, 0x2, 0x7, 0x88, 0x10, 0x20, 0x40,

    /* U+0047 "G" */
    0xfc, 0x80, 0x80, 0x9e, 0x81, 0x81, 0x81, 0xfe,

    /* U+0048 "H" */
    0x81, 0x81, 0x81, 0xff, 0x81, 0x81, 0x81, 0x81,

    /* U+0049 "I" */
    0xe9, 0x24, 0x97,

    /* U+004A "J" */
    0x1, 0x1, 0x1, 0x1, 0x1, 0x81, 0x81, 0xff,

    /* U+004B "K" */
    0x84, 0x84, 0x84, 0xff, 0x81, 0x81, 0x81, 0x81,

    /* U+004C "L" */
    0x81, 0x2, 0x4, 0x8, 0x10, 0x20, 0x7f,

    /* U+004D "M" */
    0xff, 0x26, 0x4c, 0x99, 0x32, 0x60, 0xc1,

    /* U+004E "N" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,

    /* U+004F "O" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0050 "P" */
    0xff, 0x81, 0x81, 0xff, 0x80, 0x80, 0x80, 0x80,

    /* U+0051 "Q" */
    0xff, 0x6, 0xc, 0x19, 0x32, 0x64, 0xff, 0x10,
    0x20,

    /* U+0052 "R" */
    0xfc, 0x84, 0x84, 0xff, 0x81, 0x81, 0x81, 0x81,

    /* U+0053 "S" */
    0xfc, 0x80, 0x80, 0xff, 0x1, 0x1, 0x1, 0xff,

    /* U+0054 "T" */
    0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

    /* U+0055 "U" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0056 "V" */
    0x83, 0x6, 0xc, 0x18, 0x28, 0x8a, 0x8,

    /* U+0057 "W" */
    0x83, 0x6, 0xc, 0x99, 0x32, 0x64, 0xff,

    /* U+0058 "X" */
    0x81, 0x81, 0xc3, 0x7e, 0x42, 0x81, 0x81, 0x81,

    /* U+0059 "Y" */
    0x83, 0x6, 0xf, 0xf1, 0x2, 0x4, 0x8,

    /* U+005A "Z" */
    0xff, 0x1, 0x1, 0xff, 0x80, 0x80, 0x80, 0xff,

    /* U+005B "[" */
    0xea, 0xab,

    /* U+005C "\\" */
    0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x30, 0xc,
    0x3,

    /* U+005D "]" */
    0xd5, 0x57,

    /* U+005E "^" */
    0x66, 0x80,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x88, 0x80,

    /* U+0061 "a" */
    0x3f, 0x1, 0xff, 0x81, 0x81, 0xff,

    /* U+0062 "b" */
    0x80, 0x80, 0xff, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0063 "c" */
    0xff, 0x2, 0x4, 0x8, 0x1f, 0xc0,

    /* U+0064 "d" */
    0x1, 0x1, 0xff, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0065 "e" */
    0xff, 0x81, 0xff, 0x80, 0x80, 0xfc,

    /* U+0066 "f" */
    0x6b, 0xa4, 0x92,

    /* U+0067 "g" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0xff, 0x1, 0x3f,

    /* U+0068 "h" */
    0x80, 0x80, 0xff, 0x81, 0x81, 0x81, 0x81, 0x81,

    /* U+0069 "i" */
    0xbf,

    /* U+006A "j" */
    0x4d, 0x55, 0x70,

    /* U+006B "k" */
    0x80, 0x80, 0x84, 0x84, 0xff, 0x81, 0x81, 0x81,

    /* U+006C "l" */
    0x92, 0x49, 0x27,

    /* U+006D "m" */
    0xff, 0x26, 0x4c, 0x99, 0x30, 0x40,

    /* U+006E "n" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0x81,

    /* U+006F "o" */
    0xff, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0070 "p" */
    0xff, 0x6, 0xc, 0x18, 0x3f, 0xe0, 0x40,

    /* U+0071 "q" */
    0xff, 0x6, 0xc, 0x18, 0x3f, 0xc0, 0x81,

    /* U+0072 "r" */
    0xff, 0x2, 0x4, 0x8, 0x10, 0x0,

    /* U+0073 "s" */
    0xfc, 0x80, 0xff, 0x1, 0x1, 0xff,

    /* U+0074 "t" */
    0x41, 0xf9, 0x2, 0x4, 0x8, 0x1f, 0x80,

    /* U+0075 "u" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0xff,

    /* U+0076 "v" */
    0x83, 0x6, 0xa, 0x22, 0x82, 0x0,

    /* U+0077 "w" */
    0x83, 0x26, 0x4c, 0x99, 0x3f, 0xc0,

    /* U+0078 "x" */
    0x81, 0xc3, 0x7e, 0x42, 0x81, 0x81,

    /* U+0079 "y" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0xff, 0x1, 0x3f,

    /* U+007A "z" */
    0xff, 0x1, 0xff, 0x80, 0x80, 0xff,

    /* U+007B "{" */
    0x74, 0x48, 0x84, 0x47,

    /* U+007C "|" */
    0xff,

    /* U+007D "}" */
    0xc9, 0x12, 0x96,

    /* U+007E "~" */
    0x5e,

    /* U+00A8 "¨" */
    0xa0,

    /* U+00B4 "´" */
    0x2b, 0x0,

    /* U+00C1 "Á" */
    0x8, 0x10, 0x20, 0x0, 0xff, 0x81, 0x81, 0xff,
    0x81, 0x81, 0x81, 0x81,

    /* U+00C2 "Â" */
    0x3c, 0x24, 0x24, 0x0, 0xff, 0x81, 0x81, 0xff,
    0x81, 0x81, 0x81, 0x81,

    /* U+00C3 "Ã" */
    0x72, 0x4e, 0x0, 0x0, 0xff, 0x81, 0x81, 0xff,
    0x81, 0x81, 0x81, 0x81,

    /* U+00C7 "Ç" */
    0xff, 0x2, 0x4, 0x8, 0x10, 0x20, 0x7f, 0x10,
    0x60,

    /* U+00C9 "É" */
    0x8, 0x10, 0x20, 0x0, 0xe0, 0x80, 0x80, 0xfc,
    0x80, 0x80, 0x80, 0xff,

    /* U+00CD "Í" */
    0x2a, 0xe, 0x92, 0x49, 0x70,

    /* U+00D2 "Ò" */
    0x10, 0x8, 0x4, 0x0, 0xff, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0xff,

    /* U+00D3 "Ó" */
    0x8, 0x10, 0x20, 0x0, 0xff, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0xff,

    /* U+00D4 "Ô" */
    0x3c, 0x24, 0x24, 0x0, 0xff, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0xff,

    /* U+00D5 "Õ" */
    0x72, 0x4e, 0x0, 0x0, 0xff, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0xff,

    /* U+00DA "Ú" */
    0x8, 0x10, 0x20, 0x0, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0xff,

    /* U+00E0 "à" */
    0x20, 0x10, 0x8, 0x0, 0x3f, 0x1, 0xff, 0x81,
    0x81, 0xff,

    /* U+00E1 "á" */
    0x4, 0x8, 0x10, 0x0, 0x3f, 0x1, 0xff, 0x81,
    0x81, 0xff,

    /* U+00E2 "â" */
    0x3c, 0x24, 0x24, 0x0, 0x3f, 0x1, 0xff, 0x81,
    0x81, 0xff,

    /* U+00E3 "ã" */
    0x72, 0x52, 0x4c, 0x0, 0x3f, 0x1, 0xff, 0x81,
    0x81, 0xff,

    /* U+00E7 "ç" */
    0xff, 0x2, 0x4, 0x8, 0x1f, 0xc4, 0x18,

    /* U+00E9 "é" */
    0x4, 0x8, 0x10, 0x0, 0xff, 0x81, 0xff, 0x80,
    0x80, 0xfc,

    /* U+00ED "í" */
    0x2a, 0x4, 0x92, 0x48,

    /* U+00F3 "ó" */
    0x4, 0x8, 0x10, 0x0, 0xff, 0x81, 0x81, 0x81,
    0x81, 0xff,

    /* U+00F4 "ô" */
    0x3c, 0x24, 0x24, 0x0, 0xff, 0x81, 0x81, 0x81,
    0x81, 0xff,

    /* U+00F5 "õ" */
    0x0, 0xe, 0x40, 0x2, 0x70, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x3f, 0xc0, 0x8, 0x10, 0x2,
    0x4, 0x0, 0x81, 0x0, 0x20, 0x40, 0xf, 0xf0,
    0x0,

    /* U+00FA "ú" */
    0x4, 0x8, 0x10, 0x0, 0x81, 0x81, 0x81, 0x81,
    0x81, 0xff
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 36, .box_w = 1, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 54, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 2, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 10, .adv_w = 144, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 21, .adv_w = 162, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 162, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 36, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 40, .adv_w = 54, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 54, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 36, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 46, .adv_w = 108, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 51, .adv_w = 36, .box_w = 1, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 52, .adv_w = 90, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 53, .adv_w = 36, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 162, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 72, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 36, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 138, .adv_w = 36, .box_w = 1, .box_h = 4, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 139, .adv_w = 72, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 108, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 145, .adv_w = 72, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 72, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 282, .adv_w = 144, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 291, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 345, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 54, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 162, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 371, .adv_w = 54, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 373, .adv_w = 90, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 375, .adv_w = 108, .box_w = 7, .box_h = 1, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 72, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 378, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 126, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 398, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 72, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 415, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 423, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 36, .box_w = 1, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 432, .adv_w = 54, .box_w = 2, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 435, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 54, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 144, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 458, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 471, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 478, .adv_w = 126, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 126, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 126, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 144, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 529, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 72, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 36, .box_w = 1, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 72, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 72, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 544, .adv_w = 72, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 545, .adv_w = 72, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 547, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 571, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 126, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 592, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 72, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 621, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 633, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 657, .adv_w = 144, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 669, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 679, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 689, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 699, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 126, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 716, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 36, .box_w = 3, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 730, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 740, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 750, .adv_w = 306, .box_w = 18, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 775, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0xc, 0x19, 0x1a, 0x1b, 0x1f, 0x21, 0x25,
    0x2a, 0x2b, 0x2c, 0x2d, 0x32, 0x38, 0x39, 0x3a,
    0x3b, 0x3f, 0x41, 0x45, 0x4b, 0x4c, 0x4d, 0x52
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 33, .range_length = 94, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 168, .range_length = 83, .glyph_id_start = 95,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 24, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
const lv_font_t ace = {
#else
lv_font_t ace = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
    .fallback = NULL,
    .user_data = NULL
};



#endif /*#if ACE*/

