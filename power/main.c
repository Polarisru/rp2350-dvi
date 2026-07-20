#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/powman.h"
#include "hardware/sync.h"      // __wfi()

#define LED_PIN     10
#define BUTTON_PIN  11

static void enter_deep_sleep(void)
{
    // LED off, then make it high impedance.
    gpio_put(LED_PIN, 0);
    gpio_set_dir(LED_PIN, GPIO_IN);
    gpio_disable_pulls(LED_PIN);

    // Assumption: push-button shorts GPIO11 to GND when pressed.
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    //gpio_pull_up(BUTTON_PIN);
    gpio_disable_pulls(BUTTON_PIN);

    /*
     * Arguments:
     *   0           POWMAN GPIO wake source slot (0..3)
     *   BUTTON_PIN  GPIO number
     *   true        edge-triggered wakeup
     *   false       falling edge (wake when GPIO goes low)
     */
    powman_enable_gpio_wakeup(0, BUTTON_PIN, true, false);

    /*
     * Power off the switched core domain. Wake on GPIO11 performs a
     * reboot, so firmware starts from main() again.
     *
     * POWMAN_POWER_STATE_NONE is the fully-off switched-core state;
     * P0 is not an SDK enum name.
     */
    powman_set_power_state(POWMAN_POWER_STATE_NONE);

    // The power transition occurs on WFI.
    __wfi();

    // A valid wake source resets/restarts the device; normally unreachable.
    while (true) {
        __wfi();
    }
}

int main(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // 1 Hz blink for 10 seconds.
    for (unsigned i = 0; i < 10; ++i) {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);

        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }

    enter_deep_sleep();
}
