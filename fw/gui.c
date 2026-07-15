#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "fonts.h"
#include "gui.h"
#include "generated_fonts/vga_font_32.h"
#include "generated_fonts/vga_font_48.h"
#include "generated_fonts/vga_font_64.h"

static uint8_t framebuf[GUI_SIZE];

static __force_inline uint16_t colour_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)r & 0xf8) >> 3 | ((uint16_t)g & 0xfc) << 3 | ((uint16_t)b & 0xf8) << 8;
}

static __force_inline uint8_t colour_rgb332(uint8_t r, uint8_t g, uint8_t b) {
    return (r & 0xc0) >> 6 | (g & 0xe0) >> 3 | (b & 0xe0) >> 0;
}

// ---------------------------------------------------------------------------
// Font table
// ---------------------------------------------------------------------------

static const bitmap_font_t *get_font(font_id_t id)
{
    switch (id)
    {
        case FONT_64: return &vga_font_64;
        case FONT_48: return &vga_font_48;
        case FONT_32: return &vga_font_32;
        default:      return &vga_font_32;
    }
}

uint16_t get_font_height(font_id_t id)
{
    switch (id)
    {
        case FONT_64: return 64;
        case FONT_48: return 48;
        case FONT_32: return 32;
        default:      return 32;
    }
}

static int gui_get_text_width(const char *text, const bitmap_font_t *font)
{
    int width = 0;

    if (text == NULL || font == NULL)
        return 0;

    while (*text != '\0' && *text != '\n')
    {
        uint8_t c = (uint8_t)*text++;

        if (c == ' ')
        {
            width += font->height / 3;
        }
        else if (c >= font->first_char && c <= font->last_char)
        {
            uint16_t glyph_index = c - font->first_char;
            width += font->widths[glyph_index] + 1;
        }
    }

    return width;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static __force_inline void draw_pixel(int x, int y, char color)
{
    //if ((x < 0) || (x >= GUI_WIDTH) || 
    //    (y < 0) || (y >= GUI_HEIGHT))
    //  return;

    int pixel = (GUI_WIDTH * y) + x;

    framebuf[pixel] = color;
}

/* RGB332:
 * bits 7:5 = red   (0..7)
 * bits 4:2 = green (0..7)
 * bits 1:0 = blue  (0..3)
 */
static void draw_pixel_alpha_rgb332(int x, int y,
                                    uint8_t color, uint8_t alpha)
{
    if ((x < 0) || (x >= GUI_WIDTH) ||
        (y < 0) || (y >= GUI_HEIGHT) ||
        (alpha == 0))
    {
        return;
    }

    uint32_t pixel = (uint32_t)y * GUI_WIDTH + (uint32_t)x;
    uint8_t bg = framebuf[pixel];

    /* alpha is the 2-bpp glyph coverage: 0, 1, 2, or 3. */
    if (alpha >= 3)
    {
        framebuf[pixel] = color;
        return;
    }

    uint8_t bg_r = (bg    >> 5) & 0x07;
    uint8_t bg_g = (bg    >> 2) & 0x07;
    uint8_t bg_b =  bg          & 0x03;

    uint8_t fg_r = (color >> 5) & 0x07;
    uint8_t fg_g = (color >> 2) & 0x07;
    uint8_t fg_b =  color       & 0x03;

    /* Rounded blend: result = bg * (1-a) + fg * a */
    uint8_t out_r = (bg_r * (3 - alpha) + fg_r * alpha + 1) / 3;
    uint8_t out_g = (bg_g * (3 - alpha) + fg_g * alpha + 1) / 3;
    uint8_t out_b = (bg_b * (3 - alpha) + fg_b * alpha + 1) / 3;

    framebuf[pixel] = (out_r << 5) | (out_g << 2) | out_b;
}

static void draw_char(int x, int y, char c, const bitmap_font_t *font, uint8_t color)
{
    if (font == NULL) return;
    if ((uint8_t)c < font->first_char || (uint8_t)c > font->last_char) return;

    uint16_t glyph_index = (uint8_t)c - font->first_char;
    uint16_t byte_offset  = font->offsets[glyph_index];   // now a BYTE offset
    uint8_t  glyph_width  = font->widths[glyph_index];
    uint8_t  glyph_height = font->height;

    uint16_t row_bytes = (glyph_width + 7) / 8;

    for (uint8_t gy = 0; gy < glyph_height; gy++)
    {
        for (uint8_t gx = 0; gx < glyph_width; gx++)
        {
            uint32_t byte_index = byte_offset + (uint32_t)gy * row_bytes + (gx >> 3);
            uint8_t  byte       = font->bitmap[byte_index];
            uint8_t  bit        = (byte >> (7 - (gx & 7))) & 0x01;
            if (bit)
                draw_pixel(x + gx, y + gy, color);
        }
    }
}

/*static void draw_char(int x, int y, char c,
                      const bitmap_font_t *font, uint8_t color)
{
    if (font == NULL)
        return;

    uint8_t uc = (uint8_t)c;

    if ((uc < font->first_char) || (uc > font->last_char))
        return;

    uint16_t glyph_index = (uint16_t)(uc - font->first_char);
    uint16_t byte_offset = font->offsets[glyph_index];
    uint8_t glyph_width = font->widths[glyph_index];
    uint8_t glyph_height = font->height;

    // 2 bits/pixel = four pixels stored in each byte.
    uint16_t row_bytes = (glyph_width + 3u) / 4u;

    for (uint8_t gy = 0; gy < glyph_height; gy++)
    {
        for (uint8_t gx = 0; gx < glyph_width; gx++)
        {
            uint32_t byte_index =
                (uint32_t)byte_offset +
                (uint32_t)gy * row_bytes +
                (gx >> 2);

            uint8_t packed = font->bitmap[byte_index];

             // Packed byte:
             // bits 7:6 -> x + 0
             // bits 5:4 -> x + 1
             // bits 3:2 -> x + 2
             // bits 1:0 -> x + 3
            uint8_t alpha = (packed >> (6u - 2u * (gx & 3u))) & 0x03u;

            if (alpha != 0)
            {
                draw_pixel_alpha_rgb332(x + gx, y + gy, color, alpha);
            }
        }
    }
}*/

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void gui_fill_screen(uint8_t color)
{
    memset(framebuf, (color & 0x07) | ((color & 0x07) << 3), GUI_SIZE);
}

void gui_draw_rect(int x1, int y1, int x2, int y2, uint8_t color)
{
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= GUI_WIDTH)  x2 = GUI_WIDTH  - 1;
    if (y2 >= GUI_HEIGHT) y2 = GUI_HEIGHT - 1;
    int width  = x2 - x1 + 1;
    int height = y2 - y1 + 1;

    for (int y = y1; y <= y2; y++)
        (void)memset(&framebuf[y * GUI_WIDTH + x1], color, width);
}

void gui_draw_box(int x1, int y1, int x2, int y2, uint8_t width, uint8_t color)
{
    if (width == 0) return;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    int rect_w = x2 - x1 + 1;
    int rect_h = y2 - y1 + 1;
    if (rect_w <= 0 || rect_h <= 0) return;

    // Clamp thickness so opposite strips never overlap/cross
    int wv = width;
    if (wv * 2 > rect_h) wv = (rect_h + 1) / 2;

    int wh = width;
    if (wh * 2 > rect_w) wh = (rect_w + 1) / 2;

    // Top and bottom strips (full width)
    gui_draw_rect(x1, y1, x2, y1 + wv - 1, color);
    gui_draw_rect(x1, y2 - wv + 1, x2, y2, color);

    // Left and right strips (only the middle region, between top/bottom strips)
    gui_draw_rect(x1, y1 + wv, x1 + wh - 1, y2 - wv, color);
    gui_draw_rect(x2 - wh + 1, y1 + wv, x2, y2 - wv, color);
}

void gui_draw_picture(int dst_x, int dst_y,
                     const uint8_t *pic,
                     int pic_width,
                     int pic_height)
{
    if (pic == NULL) 
      return;
    if (pic_width <= 0 || pic_height <= 0) 
      return;
    if (dst_x < 0 || dst_y < 0) 
      return;
    if ((dst_x + pic_width)  > GUI_WIDTH) 
      return;
    if ((dst_y + pic_height) > GUI_HEIGHT) 
      return;
    if ((dst_x & 1) != 0) 
      return;
    if ((pic_width & 1) != 0) 
      return;

    const int src_bytes_per_row = pic_width;
    const int dst_bytes_per_row = GUI_WIDTH;

    for (int y = 0; y < pic_height; y++)
    {
        uint8_t       *dst = &framebuf[(dst_y + y) * dst_bytes_per_row + dst_x];
        const uint8_t *src = &pic[y * src_bytes_per_row];
        memcpy(dst, src, (size_t)src_bytes_per_row);
    }
}

void gui_draw_text(int x, int y, const char *text, font_id_t font_id, uint8_t color)
{
    const bitmap_font_t *font = get_font(font_id);
    int cursor_x = x;

    if (text == NULL || font == NULL) return;

    while (*text != '\0')
    {
        char c = *text++;

        if (c == '\n')
        {
            cursor_x = x;
            y += font->height;
            continue;
        }

        if (c == ' ')
        {
            cursor_x += font->height / 3;
            continue;
        }

        if ((uint8_t)c >= font->first_char && (uint8_t)c <= font->last_char)
        {
            draw_char(cursor_x, y, c, font, color);
            //cursor_x += font->widths[(uint8_t)c - font->first_char] + 1;
            cursor_x += font->advances[(uint8_t)c - font->first_char];
        }
    }
}

void gui_draw_text_centered(int x, int y, const char *text,
                            font_id_t font_id, uint8_t color)
{
    const bitmap_font_t *font = get_font(font_id);

    if (text == NULL || font == NULL)
        return;

    int text_width = gui_get_text_width(text, font);

    gui_draw_text(x - (text_width / 2), y, text, font_id, color);
}

uint8_t *gui_get_framebuf(void)
{
  return framebuf;
}

void gui_init(void)
{
    // Fill black
    memset(framebuf, 0x00, sizeof(framebuf));
}

