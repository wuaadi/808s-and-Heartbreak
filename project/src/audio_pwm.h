#ifndef AUDIO_PWM_H
#define AUDIO_PWM_H

#include <stdint.h>

void init_audio(void);

int16_t generate_test_sample(void);

void output_audio(int16_t sample);

void audio_irq_handler(void);

#endif