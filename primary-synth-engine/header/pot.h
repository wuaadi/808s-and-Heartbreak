#ifndef POT_H
#define POT_H

#include "voices.h"

// Inputs are 12-bit adc readings 0-4095.
// Higher A/D/R readings produce slower stage n sustain spans 0 to 65535.
EnvParams_t pot_decode(uint16_t a, uint16_t d, uint16_t s, uint16_t r);


void pot_adc_init(void);
void pot_isr(void);

#endif // POT_H
