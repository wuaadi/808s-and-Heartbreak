#include <stdint.h>

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"

#include "gpio.h"
#include "i2s_tx.h"

#define I2S_PIO pio0
#define I2S_SM 0

// Two 32-bit channel slots plus PIO setup cycles between slots.
#define I2S_FRAME_INSTRUCTIONS 133.0f

static uint16_t i2s_tx_instructions[9];

static const struct pio_program i2s_tx_program = {
    .instructions = i2s_tx_instructions,
    .length = 9,
    .origin = 0
};

void init_i2s_tx(void) {
    PIO pio = I2S_PIO;
    uint sm = I2S_SM;

    i2s_tx_instructions[0] = pio_encode_set(pio_x, 31);
    i2s_tx_instructions[1] = pio_encode_set(pio_pins, 0);
    i2s_tx_instructions[2] = pio_encode_out(pio_pins, 1);
    i2s_tx_instructions[3] =
        pio_encode_jmp_x_dec(2) | pio_encode_sideset(1, 1);
    i2s_tx_instructions[4] = pio_encode_set(pio_x, 31);
    i2s_tx_instructions[5] = pio_encode_set(pio_pins, 1);
    i2s_tx_instructions[6] = pio_encode_out(pio_pins, 1);
    i2s_tx_instructions[7] =
        pio_encode_jmp_x_dec(6) | pio_encode_sideset(1, 1);
    i2s_tx_instructions[8] = pio_encode_set(pio_pins, 0);

    pio_gpio_init(pio, MCU1_I2S_BCLK_PIN);
    pio_gpio_init(pio, MCU1_I2S_LRCLK_PIN);
    pio_gpio_init(pio, MCU1_I2S_DOUT_PIN);

    uint offset = pio_add_program_at_offset(pio, &i2s_tx_program, 0);

    pio_sm_config cfg = pio_get_default_sm_config();
    sm_config_set_out_pins(&cfg, MCU1_I2S_DOUT_PIN, 1);
    sm_config_set_set_pins(&cfg, MCU1_I2S_LRCLK_PIN, 1);
    sm_config_set_sideset(&cfg, 1, false, false);
    sm_config_set_sideset_pins(&cfg, MCU1_I2S_BCLK_PIN);
    sm_config_set_out_shift(&cfg, false, true, 32);
    sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_TX);

    float divider = (float)clock_get_hz(clk_sys) /
        ((float)SAMPLE_RATE * I2S_FRAME_INSTRUCTIONS);
    sm_config_set_clkdiv(&cfg, divider);

    sm_config_set_wrap(&cfg, offset, offset + 8);

    pio_sm_set_consecutive_pindirs(pio, sm, MCU1_I2S_BCLK_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, MCU1_I2S_LRCLK_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, MCU1_I2S_DOUT_PIN, 1, true);

    pio_sm_init(pio, sm, offset, &cfg);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_enabled(pio, sm, true);
}

void i2s_tx_write_sample(int16_t sample) {
    uint32_t channel_word = (uint32_t)(uint16_t)sample << 15;

    // Queue the same mono sample for the left and right 32-bit slots.
    pio_sm_put_blocking(I2S_PIO, I2S_SM, channel_word);
    pio_sm_put_blocking(I2S_PIO, I2S_SM, channel_word);
}
