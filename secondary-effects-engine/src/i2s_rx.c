#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"

#include "audio_config.h"
#include "i2s_rx.h"

// PIO config

#define I2S_PIO pio0
#define I2S_SM 0

// !!! confirm I2S format with Aadi

// PIO program instructions
static uint16_t i2s_instructions[11];

static const struct pio_program i2s_program = {
    .instructions = i2s_instructions,
    .length = 11,
    .origin = 0
};

// initialize I2S receiver

void init_i2s_rx(void) {
    PIO pio = I2S_PIO;
    uint sm = I2S_SM;

    // encode PIO instructions
    i2s_instructions[0] = pio_encode_wait_gpio(1, I2S_LRCLK_PIN);
    i2s_instructions[1] = pio_encode_wait_gpio(0, I2S_LRCLK_PIN);

    // skip I2S one-bit offset
    i2s_instructions[2] = pio_encode_wait_gpio(1, I2S_BCLK_PIN);
    i2s_instructions[3] = pio_encode_wait_gpio(0, I2S_BCLK_PIN);

    // read 16 bits
    i2s_instructions[4] = pio_encode_set(pio_x, 15);
    i2s_instructions[5] = pio_encode_wait_gpio(1, I2S_BCLK_PIN);
    i2s_instructions[6] = pio_encode_in(pio_pins, 1);
    i2s_instructions[7] = pio_encode_wait_gpio(0, I2S_BCLK_PIN);
    i2s_instructions[8] = pio_encode_jmp_x_dec(5);

    // push sample into RX FIFO
    i2s_instructions[9] = pio_encode_push(false, true);
    i2s_instructions[10] = pio_encode_jmp(0);

    // initialize pins
    pio_gpio_init(pio, I2S_BCLK_PIN);
    pio_gpio_init(pio, I2S_LRCLK_PIN);
    pio_gpio_init(pio, I2S_DATA_PIN);

    // configure as inputs
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_BCLK_PIN, 3, false);

    // pull down inputs while disconnected
    gpio_pull_down(I2S_BCLK_PIN);
    gpio_pull_down(I2S_LRCLK_PIN);
    gpio_pull_down(I2S_DATA_PIN);

    // load program into PIO
    // PIO0 and SM0 reserved for RX
    uint offset = pio_add_program_at_offset(
        pio, &i2s_program, 0
    );

    pio_sm_config cfg = pio_get_default_sm_config();

    // set input data pin
    sm_config_set_in_pins(&cfg, I2S_DATA_PIN);

    // msb first, shift left, manual push
    sm_config_set_in_shift(&cfg, false, false, 32);

    // use available FIFO space for RX
    sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_RX);

    // follow incoming BCLK through WAIT
    sm_config_set_clkdiv(&cfg, 1.0f);

    // set instruction loop boundaries
    sm_config_set_wrap(&cfg, offset, offset + 10);

    // initialize state machine
    pio_sm_init(pio, sm, offset, &cfg);

    // clear any existing data
    pio_sm_clear_fifos(pio, sm);

    // start receiver
    pio_sm_set_enabled(pio, sm, true);
}

// check if sample is available

bool i2s_sample_available(void) {
    return !pio_sm_is_rx_fifo_empty(I2S_PIO, I2S_SM);
}

// read one sample

int16_t i2s_read_sample(void) {
    // call only when a sample is available
    uint32_t word = pio_sm_get(I2S_PIO, I2S_SM);

    return (int16_t)(word & 0xFFFF);
}