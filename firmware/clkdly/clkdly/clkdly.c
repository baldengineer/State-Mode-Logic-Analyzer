#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "tusb.h"

#include "piodelay.pio.h"
#include "piotimestamps.pio.h"

#include "hardware/structs/systick.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/// this has some interrupt examples to consider
// https://github.com/zenups/ZipZap/blob/master/main.cpp

void pio_timestamps_setup(PIO pio, uint sm, uint offset) {
    piotimestamps_program_init(pio, sm, offset);
    pio_sm_set_enabled(pio, sm, true);
}

void pio_clkdelay_setup(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    pio_gpio_init(pio, 16);
    piodelay_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);
   
    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    //pio->txf[sm] = (125000000 / (2 * freq)) - 3;
    pio->txf[sm] = (20000);
}

// old habits
static inline uint32_t millis() {
    return (time_us_32() / 1000);
}

int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

void wait_for_usb() {
    static bool first_run = true;
    if (first_run) {
        pico_led_init();
        first_run = false;
    }
    while (!tud_cdc_connected()) { 
        static bool led_state = true;
        pico_set_led(led_state);
        led_state = !led_state;
        sleep_ms(100);  
    } // wait for usb to connect
}

    // systick_hw->csr = 0x5; //SysTick Control and Status Register
    // systick_hw->rvr = 0x00FFFFFF; //SysTick Reload Value Register

    

    // uint32_t new, old, t0, t1;
    // old=systick_hw->cvr;//SysTick Current Value Registe
    // // SYST_CALIB SysTick Calibration value Register

int main() {
    stdio_init_all();
    wait_for_usb();
    pico_set_led(true);

    printf("Systick status:\n");
    printf("systick_hw->csr: [%d]\n", systick_hw->csr);
    printf("systick_hw->cvr: [%zu]\n", systick_hw->cvr);
    printf("Setting csr to 0x5\n");
    systick_hw->csr = 0x5;

    printf("Configuring PIO and SMs\n");
    // PIO0: pio_stamper
    // sm0: clock delay
    // sm1: timestamp
    const int sm_dly = 0;
    const int sm_tstamp = 1;
    PIO pio_stamper = pio0;
    uint offset_dly = pio_add_program(pio_stamper, &piodelay_program);
    printf("Loaded clkdely program at %d\n", offset_dly);  
    pio_clkdelay_setup(pio_stamper, sm_dly, offset_dly, 17, 3);

    uint offset_tstamp = pio_add_program(pio_stamper, &piotimestamps_program);
    printf("Loaded timestamp program at %d\n", offset_tstamp);  
    pio_timestamps_setup(pio_stamper, sm_tstamp, offset_tstamp);

    uint32_t clk_delay_value=10000;
    printf("Setting clock delay: [%zu]\n", clk_delay_value);
    pio_stamper->txf[sm_dly] = clk_delay_value;

    printf("Going into forever loop!\n");
    while (true) {    
        static uint32_t previous_counter_value = 0xFFFFFFFF;   // show difference between sleeps
        static uint32_t old_systick = 0;

        uint32_t counter_value = pio_stamper->rxf[sm_tstamp];  // get current buffer value (though it is probably delayed because of fifo?)

        uint32_t current_millis = millis(); // display purposes
        uint32_t counter_difference = previous_counter_value - counter_value;  
        previous_counter_value = counter_value; // remember...
        printf("[%zu] counter value: [%zu], diff: [%zu]\n", current_millis, counter_value, counter_difference);
        uint32_t current_systick = systick_hw->cvr;
        uint32_t systick_difference = current_systick - old_systick;
        old_systick = current_systick;
        printf("systick_hw->cvr: [%zu], difference: [%zu]\n", current_systick, systick_difference);
        

        sleep_ms(100);
    }
}



