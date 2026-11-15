#include "pot.h"
#include "gpio.h"
#include "hardware/adc.h"

#define POT_ADC_MAX 4095u

// can't be zero, will tweak pot->increment scaling after testing
static uint8_t pot_to_increment(uint16_t reading) {
    return (uint8_t)(UINT8_MAX - ((uint32_t)reading * (UINT8_MAX - 1u) / POT_ADC_MAX));
}

EnvParams_t pot_decode(uint16_t a, uint16_t d, uint16_t s, uint16_t r) {
    EnvParams_t params;
    params.a_inc = pot_to_increment(a);
    params.d_inc = pot_to_increment(d);
    params.s_level = (uint32_t)s * UINT16_MAX / POT_ADC_MAX;
    params.r_inc = pot_to_increment(r);
    return params;
}

void pot_isr(void) {
    adc_select_input(MCU1_ATTACK_POT_ADC);
    uint16_t attack = adc_read();
    adc_select_input(MCU1_DECAY_POT_ADC);
    uint16_t decay = adc_read();
    adc_select_input(MCU1_SUSTAIN_POT_ADC);
    uint16_t sustain = adc_read();
    adc_select_input(MCU1_RELEASE_POT_ADC);
    uint16_t release = adc_read();

    g_env_params = pot_decode(attack, decay, sustain, release);
}

void pot_adc_init(void) {
    adc_init();
    adc_gpio_init(MCU1_ATTACK_POT_PIN);
    adc_gpio_init(MCU1_DECAY_POT_PIN);
    adc_gpio_init(MCU1_SUSTAIN_POT_PIN);
    adc_gpio_init(MCU1_RELEASE_POT_PIN);

}

void pot_init() {
    pot_adc_init();

    //use timer0
}