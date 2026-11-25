#ifndef POT_H
#define POT_H

#include "voices.h"

//*******************************************************************************
#define POT_ADC_MAX 4095u
#define POT_IRQ TIMER0_IRQ_0
#define ALARM_NUM TIMER0_IRQ_0
//microseconds between interrupts
#define POT_PERIOD_US 50000

#define WTABLE_MODE_COUNT 16

typedef struct {
    uint8_t a_inc;
    uint8_t d_inc;
    uint8_t r_inc;
    uint32_t s_level;
} EnvParams_t;

extern volatile EnvParams_t g_env_params;

extern const uint16_t *const wavetables[WTABLE_MODE_COUNT];


//***********************************************************************
//functions list

void pot_to_inc(uint16_t reading);

EnvParams_t pot_decode(uint16_t a, uint16_t d, uint16_t s, uint16_t r);

void pot_adc_init(void);

//Initialization functions
void init_POT();

bool debounce_wavetable(void);
void cycle_wavetable(void);
void init_button();

#endif // POT_H
