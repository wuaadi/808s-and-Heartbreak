#include "pot.h"
#include "gpio.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/uart.h"



//************************************************************* */
//fx declarations

void init_POT();
static uint8_t pot_to_increment(uint16_t reading);
static bool button_timer_cb(repeating_timer_t *rt);
EnvParams_t pot_decode(uint16_t a, uint16_t d, uint16_t s, uint16_t r);
void pot_adc_init(void);
bool debounce_wavetable();
void cycle_wavetable();

//************************************************************* */
//isr 
void ISR_pot(void) { //COMPLETE
    //TIMER ISR ACK
    hw_clear_bits(&timer0_hw->intr, 1u << ALARM_NUM);

    //handle button
    cycle_wavetable();

    //handle pots
    adc_select_input(MCU0_ATTACK_POT_ADC);
    uint16_t attack = adc_read();
    adc_select_input(MCU0_DECAY_POT_ADC);
    uint16_t decay = adc_read();
    adc_select_input(MCU0_SUSTAIN_POT_ADC);
    uint16_t sustain = adc_read();
    adc_select_input(MCU0_RELEASE_POT_ADC);
    uint16_t release = adc_read();
    g_env_params = pot_decode(attack, decay, sustain, release);
}

//************************************************************* */
//fx definitons
void init_GPIO(void) {
    gpio_set_function(MCU0_LCD_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MCU0_LCD_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MCU0_LCD_MISO_PIN, GPIO_FUNC_SPI);

    gpio_init(MCU0_LCD_CS_PIN);
    gpio_put(MCU0_LCD_CS_PIN, 1);
    gpio_set_dir(MCU0_LCD_CS_PIN, GPIO_OUT);
    gpio_init(MCU0_LCD_RST_PIN);
    gpio_put(MCU0_LCD_RST_PIN, 1);
    gpio_set_dir(MCU0_LCD_RST_PIN, GPIO_OUT);

    gpio_init(MCU0_WAVETABLE_BTN_PIN);
    gpio_set_dir(MCU0_WAVETABLE_BTN_PIN, GPIO_IN);
    gpio_pull_down(MCU0_WAVETABLE_BTN_PIN);

    pot_adc_init();

    gpio_set_function(MCU0_MIDI_RX_PIN,
                      UART_FUNCSEL_NUM(MCU0_MIDI_UART, MCU0_MIDI_RX_PIN));

    pio_gpio_init(pio0, MCU0_I2S_BCLK_PIN);
    pio_gpio_init(pio0, MCU0_I2S_LRCLK_PIN);
    pio_gpio_init(pio0, MCU0_I2S_DOUT_PIN);
}

void init_POT() { //COMPLETE
    pot_adc_init();
    //use timer0 alarm0
    hw_set_bits(&timer0_hw->inte, 1u << ALARM_NUM);
    irq_set_exclusive_handler(POT_IRQ, ISR_pot);
    irq_set_enabled(POT_IRQ, true);
    //set delay
    uint64_t target = timer0_hw->timerawl + POT_PERIOD_US;
    //arm it
    timer0_hw->alarm[ALARM_NUM] = (uint32_t) target;
} //init_POT

static uint8_t pot_to_increment(uint16_t reading) { //INCOMPLETE
    // can't be zero, will tweak pot->increment scaling after testing
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

void pot_adc_init(void) { //COMPLETE
    adc_init();
    adc_gpio_init(MCU0_ATTACK_POT_PIN);
    adc_gpio_init(MCU0_DECAY_POT_PIN);
    adc_gpio_init(MCU0_SUSTAIN_POT_PIN);
    adc_gpio_init(MCU0_RELEASE_POT_PIN);

}

bool debounce_wavetable() { //COMPLETE
    //handles debouncing for cycle_wtable_button
    //pull down wiring implies pressed == 1
    uint8_t sample = gpio_get(MCU0_WAVETABLE_BTN_PIN) ? 1 : 0;
    //newpress will be either 0 or 1
    cycle_wtable_matrix = (uint8_t)((cycle_wtable_matrix << 1) | sample);
    //return true if the matrix is a rising edge
    return ((cycle_wtable_matrix & 0xFF) == 0x0F);
}
void cycle_wavetable() { //COMPLETE
    //increment through pointer array of wavetables, should wrap around at end
    if (!debounce_wavetable())
        return;
    wtable_mode = (WavetableMode_t)((wtable_mode + 1) % WTABLE_MODE_COUNT);
    
    //assign wavetables depending on mode
    //envelope state does not affect wavetable selection
    const uint16_t *table = wavetables[wtable_mode];
    for (int i = 0; i < NUM_VOICES; i++)
        g_voices[i].osc_table = table;
   return;
}

static bool button_timer_cb(repeating_timer_t *rt) { //UNUSED, COMPLETE
    (void)rt;
    cycle_wavetable();
    return true;
}
