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


// PIO0: pio_stamper
// sm0: clock delay
// sm1: timestamp
const int sm_dly = 0;
const int sm_tstamp = 1;
PIO pio_stamper = pio0;
uint32_t clk_delay_value=5;

/// this has some interrupt examples to consider
// https://github.com/zenups/ZipZap/blob/master/main.cpp

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

void pio_timestamps_setup(PIO pio, uint sm, uint offset) {
    piotimestamps_program_init(pio, sm, offset);
    pio_sm_set_enabled(pio, sm, true);
}

volatile uint32_t value_from_pio_irq = 0;

static void __not_in_flash_func(pio_irq0_handler)(void) {
    // og example from arg001 on pi forums
    // while (!pio_sm_is_rx_fifo_empty(PIO_PWM_CAPTURE, MY_SM))
    // {
    //     	uint32_t fv;

    //     	fv = pio_sm_get(PIO_PWM_CAPTURE, MY_SM);
	// 	do_something_with(fv);
	// }
    pico_set_led(false);
    uint32_t fv;
    fv = pio_sm_get(pio_stamper, sm_tstamp);
    value_from_pio_irq = fv;
    pio_stamper->txf[sm_dly] = clk_delay_value; // do we need to reset this?
    pio_interrupt_clear(pio_stamper, 0);
    return;
}

void pio_clkdelay_setup(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    pio_gpio_init(pio, 16);
    piodelay_program_init(pio, sm, offset, pin);

    printf("Configuring IRQ handler\n");
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_irq0_handler);
    //irq_add_shared_handler(pis_interrupt0, pio_irq0_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    // Enable FIFO interrupt in the PIO itself
    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
    printf("IRQ handler state: [%d]\n", irq_get_exclusive_handler(PIO0_IRQ_0));
            // getting 536871105 which is 0x2000 00C1
    // Enable IRQ in the NVIC
    irq_set_enabled(PIO0_IRQ_0, true);
   // irq_set_enabled(pis_interrupt0, true);

    printf("Enabling clkdelay PIO\n");
    pio_sm_set_enabled(pio, sm, true);
   
    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    //pio->txf[sm] = (125000000 / (2 * freq)) - 3;
    pio->txf[sm] = (200);
}

// old habits
static inline uint32_t millis() {
    return (time_us_32() / 1000);
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

int main() {
    stdio_init_all();
    wait_for_usb();
    pico_set_led(true);

    printf("Configuring PIO and SMs\n");
    uint offset_dly = pio_add_program(pio_stamper, &piodelay_program);
    printf("Loaded clkdely program at %d\n", offset_dly);  
    pio_clkdelay_setup(pio_stamper, sm_dly, offset_dly, 17, 3);

    uint offset_tstamp = pio_add_program(pio_stamper, &piotimestamps_program);
    printf("Loaded timestamp program at %d\n", offset_tstamp);  
    pio_timestamps_setup(pio_stamper, sm_tstamp, offset_tstamp);

    printf("Setting clock delay: [%zu]\n", clk_delay_value);
    pio_stamper->txf[sm_dly] = clk_delay_value;

   // printf("IRQ0_INTE: %d\n", pio_stamper->irq_ctrl);


    printf("Going into forever loop!\n");
    while (true) {    
        static uint32_t previous_counter_value = 0xFFFFFFFF;   // show difference between sleeps
        static uint32_t prev_irq_value = 0;

        uint32_t counter_value = pio_stamper->rxf[sm_tstamp];  // get current buffer value (though it is probably delayed because of fifo?)

        uint32_t current_millis = millis(); // display purposes
        uint32_t counter_difference = previous_counter_value - counter_value;  
        previous_counter_value = counter_value; // remember...
        printf("[%zu] counter value: [%zu], diff: [%zu]\n", current_millis, counter_value, counter_difference);

        if (prev_irq_value != value_from_pio_irq) {
            prev_irq_value = value_from_pio_irq;
            printf("!!! Value from IRQ: [%zu]\n", value_from_pio_irq);
            //pio_stamper->txf[sm_dly] = clk_delay_value; // moved to ISR
            pico_set_led(true);
        }

        sleep_ms(1000);
    }
}
