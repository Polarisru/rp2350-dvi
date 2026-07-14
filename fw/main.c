// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sem.h"
#include "dvi.h"
#include "gui.h"
#include "fonts.h"

#include "nature_rgb332.h"

int main(void) {
  
    set_sys_clock_khz(200000, true);
    
    clock_configure(clk_peri,
    0,
    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
    200 * MHZ,
    200 * MHZ);  
    
    stdio_init_all();
    
    gui_init();
    gui_fill_screen(GUI_WHITE);
    gui_draw_picture(0, 0, nature_pic, GUI_WIDTH, GUI_HEIGHT);
    gui_draw_text_centered(GUI_WIDTH / 2, 500, "TestTestTest", FONT_64, GUI_BLUE);
    
    dvi_init(gui_get_framebuf());
    
    while (1)
        __wfi();
}
