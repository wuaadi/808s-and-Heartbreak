#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"

#include "audio_config.h"
#include "audio_pwm.h"
#include "effects.h"

// global variables

static uint audio_slice;
static uint audio_channel;
static uint sample_slice = 1;
static uint32_t pwm_top;

static uint32_t tone_phase = 0;

// test audio generator

int16_t generate_test_sample(void) {
    tone_phase += TEST_FREQUENCY;

    if (tone_phase >= SAMPLE_RATE) {
        tone_phase -= SAMPLE_RATE;
    }

    // square wave
    if (tone_phase < SAMPLE_RATE / 2) {
        return 8000;
    }

    return -8000;
}

// PWM output

void output_audio(int16_t sample) {
    // converts signed audio to unsigned PWM duty
    uint32_t unsigned_sample = (int32_t)sample + 32768;

    uint32_t duty =
        (unsigned_sample * (pwm_top + 1)) / 65536;

    pwm_set_chan_level(audio_slice, audio_channel, duty);
}

// audio sample interrupt

void audio_irq_handler(void) {
    pwm_clear_irq(sample_slice);

    // !!! replace generated audio with samples received
    // through PIO/I2S (check with Aadi)
    int16_t input = generate_test_sample();

    int16_t output = apply_delay(input);

    output_audio(output);
}

// hardware init

void init_audio(void) {
    uint32_t clock_hz = clock_get_hz(clk_sys);

    gpio_set_function(AUDIO_PWM_PIN, GPIO_FUNC_PWM);

    audio_slice = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);
    audio_channel = pwm_gpio_to_channel(AUDIO_PWM_PIN);

    pwm_top = clock_hz / PWM_CARRIER_HZ - 1;

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_wrap(&cfg, pwm_top);

    pwm_init(audio_slice, &cfg, false);

    // start at 50% duty (zero audio level)
    pwm_set_chan_level(
        audio_slice,
        audio_channel,
        (pwm_top + 1) / 2
    );

    pwm_set_enabled(audio_slice, true);

    pwm_config timer_cfg = pwm_get_default_config();

    uint32_t timer_top = clock_hz / SAMPLE_RATE - 1;

    pwm_config_set_wrap(&timer_cfg, timer_top);

    pwm_init(sample_slice, &timer_cfg, false);

    pwm_clear_irq(sample_slice);
    pwm_set_irq_enabled(sample_slice, true);

    irq_set_exclusive_handler(
        PWM_DEFAULT_IRQ_NUM(),
        audio_irq_handler
    );

    irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);

    pwm_set_enabled(sample_slice, true);
}