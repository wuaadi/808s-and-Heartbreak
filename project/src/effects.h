#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>

void init_effects(void);

int16_t clip_audio(int32_t value);

int16_t apply_delay(int16_t input);

#endif