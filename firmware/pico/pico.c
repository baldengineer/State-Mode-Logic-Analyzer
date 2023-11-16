#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"

#include "smla.pio.h"

#define CLK_IN 5
#define TS_CNTR_CLK 2

// PIO 1 of ?
PIO pio;
uint pio_offset;
uint pio_sm;
uint la_dma_chan=10;

// Buffer for data capture
#define SAMPLE_COUNT 4000
uint32_t cap_buf[SAMPLE_COUNT]; // 128 kilobytes, i think?

// shortcut for setting up output pins (there isn't a SDK call for this?)
void out_init(uint8_t pin, bool state) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, state);
}

// incoming data is only 24 bits wide, but I think
// I need to align things to a 32-bit boundry?
void fill_buffer(uint32_t *cap_buf) {
    for (uint16_t x=0; x<SAMPLE_COUNT; x++) {
        cap_buf[x] = 0xFFFF;
    }
}

// borrowed from mega's vga2040
void print_hex_buf(const uint32_t *buf, uint word_count, uint line_count) {
    printf("---[START]---\n");
    for (int x=0; x < (word_count * line_count); x++) {   // 
        if (x % 10 == 0)
            printf("\n");
        uint32_t current_word = buf[x];
        printf("%08x", current_word);
    }
    printf("\n---[END]---\n");

}

void print_capture_buf(const uint32_t *buf, uint word_count, uint offset) {
   // printf("Captured: Hex, Binary, RGBx[4 Words, right to left]\n");
    for (int x=0; x < word_count; x++) {
       // printf("%d:%08x, b%b\n", x, buf[x], buf[x]); // lol, %b works
       uint32_t current_buf = buf[x+(word_count*offset)];
        printf("%d: 0x%02x\n", x+(word_count*offset), current_buf); // lol, %b works

        // for (int j=0; j<8; j++) {
        //     uint offset = 4 * j;
        //     printf((current_buf & (0x1<<0+(offset))) ? "1" : "0");
        //     printf((current_buf & (0x1<<1+(offset))) ? "1" : "0");
        //     printf((current_buf & (0x1<<2+(offset))) ? "1" : "0");
        //     printf((current_buf & (0x1<<3+(offset))) ? "1" : "0");
        //     printf((j!=7) ? ", " : "");
        // } 
        // printf("\n");
    }
   // printf("\n");
}


void smla_pio_setup() {
    pio = pio0;
    pio_offset = pio_add_program(pio, &SMLA_program);

    pio_sm = pio_claim_unused_sm(pio, true);
    pio_sm_config c = SMLA_program_get_default_config(pio_offset);
    pio_sm_set_enabled(pio, pio_sm, false);

    pio_gpio_init(pio, CLK_IN);

    sm_config_set_in_pins(&c, 8);
    pio_sm_set_consecutive_pindirs(pio, pio_sm, 8, 8, GPIO_IN);
    sm_config_set_clkdiv(&c, 1); 

    // From C SDK PDF:
    // sm_config_set_in_shift sets the shift direction to rightward, enables autopush, and sets the autopush threshold to 32.
    // The state machine keeps an eye on the total amount of data shifted into the ISR, and on the in which reaches or
    // breaches a total shift count of 24 (or whatever number you have configured), the ISR contents, along with the new data
    // from the in. goes straight to the RX FIFO. The ISR is cleared to zero in the same operation.
    sm_config_set_in_shift(&c, false, true, 8); // i've setup 8 bits to make this boundry clean, i think
    //sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_NONE); // PIO_FIFO_JOIN_NONE, _RX, _TX
    
    pio_sm_init(pio, pio_sm, pio_offset, &c);
}

void smla_pio_stop() {
    pio_sm_set_enabled(pio, pio_sm, false);
    pio_sm_clear_fifos(pio, pio_sm);  
}

void smla_pio_run() {
    pio_sm_set_enabled(pio, pio_sm, false);
    pio_sm_restart(pio, pio_sm);
    pio_sm_clear_fifos(pio, pio_sm);  


    // ------------------
   // la_dma_chan = 10;// dma_claim_unused_channel(false);
    dma_channel_config c = dma_channel_get_default_config(la_dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
	//channel_config_set_bswap(&c,true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, pio_sm, false));

    dma_channel_configure(
        la_dma_chan,            // dma channel
        &c,                     // configuration
        cap_buf,                // Destination pointer
        &pio->rxf[pio_sm],      // Source pointer
        32,           // Number of transfers
        true                    // Start immediately
    );
    //------------------

    // will the real PIO reset, please stand up.
   //    
    pio_sm_exec(pio, pio_sm, pio_encode_jmp(SMLA_offset_start)); // why is this locking up the PIO now?
    pio_sm_set_enabled(pio, pio_sm, true);
    // Load our configraution, and jump to program start
    // pio_sm_init(pio, pio_sm, pio_offset, &c);

    // set the state machine running
    // pio_sm_set_enabled(pio, pio_sm, true);
}

inline void check_for_usb_input() {
    //     if (stdio_usb_connected()) {
    //     // do we need to dump a buffer?
    //     uint8_t incoming_char = getchar_timeout_us(0);
            // if ((incoming_char == '!'))
            //     print_hex_buf((uint32_t*)Box, WIDTHWORDS, LINE_COUNT);
            // if ((incoming_char == '@'))
            //     print_capture_buf((uint32_t*)Box, WIDTHWORDS, 0);
            // if ((incoming_char == '#'))
            //     print_capture_buf((uint32_t*)Box, WIDTHWORDS, (HEIGHT-1)); 
            // if ((incoming_char == '?'))
            //     printf("\nHi\n");
    // }
    if (stdio_usb_connected()) {
        uint8_t incoming_char = getchar_timeout_us(0);
        if ((incoming_char == '!')) {  
            puts("Running State Machine");
            smla_pio_run();
            puts("Waiting for DMA to finish...");
           // busy_wait_ms(100);
            dma_channel_wait_for_finish_blocking(la_dma_chan);
           // print_capture_buf(cap_buf,SAMPLE_COUNT,0);
            puts("Disabling State Machine");
            smla_pio_stop();
            print_capture_buf((uint32_t*)cap_buf,32,0);
        }
    }
}

#define LED_PIN 25

void setup() {
    //setup_default_uart();
    stdio_usb_init();

    smla_pio_setup();

    out_init(LED_PIN, true);
    printf("Hello, world!\n");
}
int main() {
    setup();
    //fill_buffer(cap_buf);
    volatile uint32_t blink_interval = 500000;
    for(;;) {
        static uint32_t previous_ticks=0;
        static bool led_state=true;
    
        if (time_us_32()-previous_ticks >= blink_interval) {
            previous_ticks = time_us_32();
            led_state = !led_state;
            gpio_put(LED_PIN, led_state);
            //printf("pin 2: %d\n",gpio_get(2));
        }
        check_for_usb_input();

        // restart the state machines ... 
        // TEST_CAP_pio_arm((uint32_t*)cap_buf, SAMPLE_COUNT);
		// dma_channel_wait_for_finish_blocking(10);
    }
    return 0;
}