#ifndef VOICES_H
#define VOICES_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"

//******************************************** */


#define ENV_WAVETABLE_SIZE 256
#define NUM_VOICES 16 //size of voice array
#define AUDIOBLOCK_SIZE 64

typedef struct {
    //
    bool pressed,
    //osc wavetable
    uint16t *osc_wt,
    uint32_t velocity,
    //osc params
    uint32_t phase_acc,
    uint32_t phase_inc,
    //envelope parameters
    uint32_t env_idx,
    uint8_t a_inc, d_inc, r_inc,
    uint32_t s_level,
    //more envelope stuff
    EnvState_t env_state,
    uint16_t env_level,
    uint16_t env_amp_latch,
    //voice stealing
    uint32_t age
    
} Voice_t;

extern Voice_t g_v [NUM_VOICES];
extern int16_t g_audioblock_A [AUDIOBLOCK_SIZE];
extern int16_t g_audioblock_B [AUDIOBLOCK_SIZE];
extern int16_t * g_audioblock_CURR;
extern int16_t g_MIDInote_to_freq [127];

//*************************************************
//Envelope
typedef enum {
  ENV_STATE_OFF,
  ENV_STATE_ATTACK,
  ENV_STATE_DECAY,
  ENV_STATE_SUSTAIN,
  ENV_STATE_RELEASE
} EnvState_t;

//*************************************************

#endif