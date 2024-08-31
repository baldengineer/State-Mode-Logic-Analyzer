#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "tusb.h"

#include "piodelay.pio.h"
#include "piotimestamps.pio.h"

void piotimestamps_make_it_happen(PIO pio, uint sm, uint offset) {
    piotimestamps_program_init(pio, sm, offset);
    pio_sm_set_enabled(pio, sm, true);
}

void clkdly_make_it_happen(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    pio_gpio_init(pio, 16);
    piodelay_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);
   
    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    //pio->txf[sm] = (125000000 / (2 * freq)) - 3;
    pio->txf[sm] = (20000);
}

static inline uint32_t millis() {
    return (time_us_32() / 1000);
}


int main() {
    stdio_init_all();
   // while (!tud_cdc_connected()) { sleep_ms(100);  }

    printf("Configuring PIO\n");
    // PIO Blinking example
    PIO pio_dly = pio0;
    uint offset_dly = pio_add_program(pio_dly, &piodelay_program);
    printf("Loaded clkdely program at %d\n", offset_dly);  
    clkdly_make_it_happen(pio_dly, 0, offset_dly, 17, 3);

    PIO pio_tstamps = pio1;
    uint offset_tstamp = pio_add_program(pio_tstamps, &piotimestamps_program);
    printf("Loaded timestamp program at %d\n", offset_tstamp);  
    piotimestamps_make_it_happen(pio_tstamps, 0, offset_tstamp);
    
        

    // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    printf("Looping forever\n");
    uint32_t delay=10000;
    pio_dly->txf[0] = delay;

    printf("Going into forever loop!\n");
    
    while (true) {    
        static uint32_t previous_counter_value = 0xFFFFFFFF;
        uint32_t counter_value = pio_tstamps->rxf[0];
        uint32_t counter_difference = previous_counter_value - counter_value;
        uint32_t current_millis = millis();
        printf("[%zu] counter value: [%zu], diff: [%zu]\n", current_millis, counter_value, counter_difference);
        previous_counter_value = counter_value;
        sleep_ms(500);
    }
}
