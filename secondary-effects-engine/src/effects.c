#include <stdint.h>
#include <string.h>

#include "audio_config.h"
#include "effects.h"

// global variables

static int16_t delay_buffer[DELAY_SAMPLES] = {0};
static uint32_t delay_index = 0;

// audio effects

void init_effects(void) {
    memset(delay_buffer, 0, sizeof(delay_buffer));
    delay_index = 0;
}

int16_t clip_audio(int32_t value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}

int16_t apply_delay(int16_t input) {
    int16_t old_sample = delay_buffer[delay_index];

    // mixes current input with half of old echo
    int32_t mixed = input + old_sample / 2;

    int16_t output = clip_audio(mixed);

    // saves result to create repeating echoes
    delay_buffer[delay_index] = output;

    delay_index++;

    if (delay_index >= DELAY_SAMPLES) {
        delay_index = 0;
    }

    return output;
}