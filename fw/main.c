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

#include "nature_rgb332.h"

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
