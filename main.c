/**
 * pico_hdmi Bouncing Box Example
 *
 * Demonstrates basic usage of the pico_hdmi library:
 * - 640x480 @ 60Hz HDMI output
 * - Scanline callback for rendering
 * - Simple animation
 *
 * Target: RP2350 (Raspberry Pi Pico 2)
 */

#include "pico_hdmi/hstx_data_island_queue.h"
#include "pico_hdmi/hstx_packet.h"
#include "pico_hdmi/video_output.h"

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/clocks.h"
#include "hardware/vreg.h"

#include <math.h>
#include <string.h>

// ============================================================================
// Configuration
// ============================================================================

#ifdef VIDEO_MODE_1280x720
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720
#define BOX_SIZE 64
#else
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define BOX_SIZE 32
#endif
#define BG_COLOR 0x0010  // Dark blue (RGB565)
#define BOX_COLOR 0xFFE0 // Yellow (RGB565)

// ============================================================================
// Animation State
// ============================================================================

static volatile int box_x = 50, box_y = 50;
#ifdef VIDEO_MODE_1280x720
static int box_dx = 4, box_dy = 2;
#else
static int box_dx = 2, box_dy = 1;
#endif

// ============================================================================
// Scanline Callback (runs on Core 1)
// ============================================================================

static void __scratch_x("") scanline_callback(uint32_t v_scanline, uint32_t active_line, uint32_t *dst)
{
    (void)v_scanline;

    int fb_line = active_line;

    // Read current box position
    int bx = box_x;
    int by = box_y;

    uint32_t bg = BG_COLOR | (BG_COLOR << 16);
    uint32_t box = BOX_COLOR | (BOX_COLOR << 16);

    // Check if this line intersects the box vertically
    if (fb_line >= by && fb_line < by + BOX_SIZE) {
        // Three regions: before box, box, after box
        int i = 0;

        // Region 1: before box
        // Note: iterating by 2 pixels at a time (1 uint32_t)
        for (; i < bx / 2; i++) {
            dst[i] = bg;
        }

        // Region 2: box
        for (; i < (bx + BOX_SIZE) / 2 && i < FRAME_WIDTH / 2; i++) {
            dst[i] = box;
        }

        // Region 3: after box
        for (; i < FRAME_WIDTH / 2; i++) {
            dst[i] = bg;
        }
    } else {
        // Fast path: entire line is background
        for (int i = 0; i < FRAME_WIDTH / 2; i++) {
            dst[i] = bg;
        }
    }
}

// ============================================================================
// Main (Core 0)
// ============================================================================

static void update_box(void)
{
    int x = box_x + box_dx;
    int y = box_y + box_dy;

    if (x <= 0 || x + BOX_SIZE >= FRAME_WIDTH) {
        box_dx = -box_dx;
        x = box_x + box_dx;
    }
    if (y <= 0 || y + BOX_SIZE >= FRAME_HEIGHT) {
        box_dy = -box_dy;
        y = box_y + box_dy;
    }

    box_x = x;
    box_y = y;
}

int main(void)
{
#ifdef VIDEO_MODE_1280x720
    // 720p60: 372 MHz at 1.3V. Closest achievable to 371.25 MHz with 12 MHz XOSC
    // (0.2% high -> 74.4 MHz pixel clock, within HDMI tolerance for 720p60).
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(10);
    set_sys_clock_khz(372000, true);
#else
    // 480p60: set system clock to 126 MHz for HSTX timing (25.2 MHz pixel clock).
    set_sys_clock_khz(126000, true);
#endif

    stdio_init_all();

    // Initialize LED for heartbeat
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    sleep_ms(1000);

    // Initialize HDMI output
    hstx_di_queue_init();
    video_output_init(FRAME_WIDTH, FRAME_HEIGHT);
#ifdef BOUNCING_BOX_DVI_ONLY
    video_output_set_dvi_mode(true);
#endif

    // Register scanline callback
    video_output_set_scanline_callback(scanline_callback);

    // Launch Core 1 for HSTX output
    multicore_launch_core1(video_output_core1_run);
    sleep_ms(100);

    // Main loop - animation
    uint32_t last_frame = 0;
    bool led_state = false;

    while (1) {

        while (video_frame_count == last_frame) {
            tight_loop_contents();
        }
        last_frame = video_frame_count;

        // Update animation
        update_box();
    }

    return 0;
}
