#ifndef I2S_RX_H
#define I2S_RX_H
#include <stdint.h>
#include <stdbool.h>

#include <stdint.h>

void init_i2s_rx(void);

bool i2s_sample_available(void);

int16_t i2s_read_sample(void);

#endif