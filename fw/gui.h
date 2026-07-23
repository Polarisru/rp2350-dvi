#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "fonts.h"

#define GUI_WIDTH     720//800
#define GUI_HEIGHT    480//600
#define GUI_SIZE      (GUI_WIDTH * GUI_HEIGHT)

// Colors (8-bit RGB)
#define GUI_BLACK     0x00
#define GUI_RED       0xE0
#define GUI_GREEN     0x1C
#define GUI_YELLOW    0xFC
#define GUI_BLUE      0x03
#define GUI_MAGENTA   0xE3
#define GUI_CYAN      0x1F
#define GUI_WHITE     0xFF

uint16_t get_font_height(font_id_t id);
uint8_t gui_get_old_color(uint8_t color);
void gui_fill_screen(uint8_t color);
void gui_draw_rect(int x1, int y1, int x2, int y2, uint8_t color);
void gui_draw_box(int x1, int y1, int x2, int y2, uint8_t width, uint8_t color);
void gui_draw_picture(int dst_x, int dst_y, const uint8_t *pic, int pic_width, int pic_height);
void gui_draw_text(int x, int y, const char *text, font_id_t font_id, uint8_t color);
void gui_draw_text_centered(int x, int y, const char *text,
                            font_id_t font_id, uint8_t color);
uint8_t *gui_get_framebuf(void);
void gui_init(void);

#endif // GUI_H
