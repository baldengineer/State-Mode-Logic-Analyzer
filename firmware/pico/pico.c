#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

#include "smla.pio.h"

// PIO 1 of ?
PIO pio;
uint pio_offset;
uint pio_sm;

// Buffer for data capture
#define SAMPLE_COUNT 4000
uint32_t cap_buf[SAMPLE_COUNT]; // 128 kilobytes, i think?

// shortcut for setting up output pins (there isn't a SDK call for this?)
void out_init(uint8_t pin, bool state) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, state);
}

void smla_pio_setup() {
    pio = pio0;
    pio_offset = pio_add_program(pio, &SMLA_program);
    pio_sm = pio_claim_unused_sm(pio, true);

    pio_sm_config c = SMLA_program_get_default_config(pio_offset);
    //pio_gpio_init()
    //sm_config_set_out_pins()
    //sm_config_set_set_pins()
    //sm_config_set_in_pins()
    //pio_sm_set_consecutive_pindirs()

   // ------------------
    // uint la_dma_chan;
    // la_dma_chan = 10;// dma_claim_unused_channel(false);
    // dma_channel_config c = dma_channel_get_default_config(la_dma_chan);
    // channel_config_set_read_increment(&c, false);
    // channel_config_set_write_increment(&c, true);
	// channel_config_set_bswap(&c,true);
    // channel_config_set_dreq(&c, pio_get_dreq(pio, pio_sm, false));

    // dma_channel_configure(la_dma_chan, &c,
    //     cap_buf,                // Destination pointer
    //     &pio->rxf[pio_sm],      // Source pointer
    //     SAMPLE_COUNT,           // Number of transfers
    //     true                    // Start immediately
    // );
    //------------------

    // Load our configraution, and jump to program start
    pio_sm_init(pio, pio_sm, pio_offset, &c);

    // the actual way to reset pio
    //pio_sm_exec(pio, pio_sm, pio_encode_jmp(_offset_start));

    // set the state machine running
    pio_sm_set_enabled(pio, pio_sm, true);
}


void setup() {
    setup_default_uart();
    smla_pio_setup();
    printf("Hello, world!\n");
}

int main() {
    setup();
    for(;;) {

        // dma_channel_wait_for_finish_blocking(10);
    }
    return 0;
}