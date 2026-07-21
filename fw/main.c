// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

#include <stdio.h>
#include <inttypes.h>
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pico/sem.h"
#include "pico/multicore.h"
#include "dvi.h"
#include "gui.h"
#include "fonts.h"
#include "uart_rx.h"

#define APP_UART              uart0
#define APP_UART_TX_PIN       0
#define APP_UART_RX_PIN       1
#define APP_UART_BAUD         250000
#define UART_RX_BUFFER_SIZE   128

typedef struct
{
    uint8_t max_lines;
    uint8_t font;
} logic_context_t;

static void logic_on_uart_message(const uart_rx_message_t *message, void *user_ctx)
{
    logic_context_t *logic = (logic_context_t *)user_ctx;

    switch (message->type)
    {
        case UART_RX_LINE_TYPE_HEADER:
            logic->max_lines  = message->data.header.number_of_lines;
            logic->font = message->data.header.font;
            break;

        case UART_RX_LINE_TYPE_TEXT:
            if (message->line_index <= logic->max_lines)
            {
                draw_text(10,
                      10 + (message->line_index - 1) * get_font_height((font_id_t)logic->font),
                      message->data.text.text,
                      (font_id_t)logic->font,
                      message->data.draw_text.color);
            }
            break;

        case UART_RX_CMD_FILL:
            fill_screen(message->data.fill.color);
            break;

        case UART_RX_CMD_RECT:
            draw_rect(message->data.rect.x1,
                      message->data.rect.y1,
                      message->data.rect.x2,
                      message->data.rect.y2,
                      message->data.rect.color);
            break;
            
        case UART_RX_CMD_BOX:
            draw_box(message->data.box.x1,
                       message->data.box.y1,
                       message->data.box.x2,
                       message->data.box.y2,
                       message->data.box.width,
                       message->data.box.color);
            break;            

        case UART_RX_CMD_TEXT:
            draw_text(message->data.draw_text.x,
                      message->data.draw_text.y,
                      message->data.draw_text.text,
                      (font_id_t)message->data.draw_text.font,
                      message->data.draw_text.color);
            break;

        default:
            break;
    }
}

static void logic_on_uart_error(const char *raw_line, const char *reason, void *user_ctx)
{
    (void)raw_line;
    (void)reason;
    (void)user_ctx;
}

/* #include "nature_rgb332.h"

#define PERIOD_US 20000u

void core1_main() {
    uint64_t next_task = time_us_64() + PERIOD_US;

    while (1) {
        uint64_t now = time_us_64();

        if (now >= next_task) {
            char str[32];
            uint32_t sec = (uint32_t)(now / 1000000UL);
            uint32_t ms  = (uint32_t)((now / 1000UL) % 1000UL);

            snprintf(str, sizeof str, "Time: %d.%03d sec", sec, ms);

            while (!dvi_is_vblank())
                tight_loop_contents();
              
            gui_draw_rect(100, 380, GUI_WIDTH - 100, 470, GUI_BLACK);
            gui_draw_text_centered(GUI_WIDTH / 2, 400,
                                  str, FONT_64, GUI_BLUE);
            next_task += PERIOD_US;
        }

        tight_loop_contents();
    }
} */

void core1_main() {
    static char rx_buffer[UART_RX_BUFFER_SIZE];
    static uart_rx_t uart_rx;
    static logic_context_t logic = {0};  
    
    uart_init(APP_UART, APP_UART_BAUD);
    gpio_set_function(APP_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(APP_UART_RX_PIN, GPIO_FUNC_UART);

    uart_rx_init(&uart_rx,
                 APP_UART,
                 rx_buffer,
                 sizeof(rx_buffer),
                 logic_on_uart_message,
                 logic_on_uart_error,
                 &logic);

    while (1) {
        uart_rx_poll(&uart_rx);
    }
}

int main(void) {
  
    //set_sys_clock_khz(200000, true);
    set_sys_clock_khz(135000, true);
    
    clock_configure(clk_peri,
    0,
    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
    135 * MHZ,//200 * MHZ,
    135 * MHZ);//200 * MHZ);  
    
    stdio_init_all();
    
    gui_init();
    gui_fill_screen(GUI_WHITE);
    //gui_draw_picture(0, 0, nature_pic, GUI_WIDTH, GUI_HEIGHT);
    //gui_draw_text_centered(GUI_WIDTH / 2, 500, "TestTestTest", FONT_64, GUI_BLUE);
    
    multicore_launch_core1(core1_main);
    
    dvi_init(gui_get_framebuf());
    
    while (1) {
        tight_loop_contents();
    }
}
