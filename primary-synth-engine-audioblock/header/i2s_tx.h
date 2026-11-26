#ifndef I2S_TX_H
#define I2S_TX_H

#include <stdint.h>

void init_i2s_tx(void);
void i2s_tx_write_sample(int16_t sample);

#endif
