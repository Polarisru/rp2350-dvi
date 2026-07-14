#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint8_t height;
    uint8_t first_char;
    uint8_t last_char;
    uint8_t bpp;
    const uint16_t *offsets;
    const uint8_t  *widths;
    const uint8_t  *advances;
    const uint8_t  *bitmap;
} bitmap_font_t;

#define BITMAP_FONT_T_DEFINED

typedef enum
{
    FONT_64 = 0,
    FONT_48,
    FONT_32
} font_id_t;

#endif
