#include <stdio.h>

#include "pico/stdlib.h"

#include "audio_pwm.h"
#include "effects.h"
#include "i2s_rx.h"

int main(void) {
    stdio_init_all();

    printf("Secondary effects engine starting...\n");

    init_effects();
    init_i2s_rx();
    init_audio();

    printf("PIO I2S receiver initialized\n");
    printf("PWM audio initialized on GP22\n");
    printf("Testing echo effect\n");

    while (true) {
        tight_loop_contents();
    }
}