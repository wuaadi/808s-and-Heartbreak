#include "voices.h"

volatile EnvParams_t g_env_params;

//decoding increments from pot levels
EnvParams_t pot_decode (uint16t a, uint16t d, uint16t s, uint16t r) {
    //args are pot levels from 0-4095
} 

//potentiometer isr
EnvParams_t pot_isr () {

    //decode levels into increments
    //pot_decode
}


//button press cycles through wavetables
void cycle_wavetable() {
    //increment through pointer array of wavetables, should wrap around at end
}