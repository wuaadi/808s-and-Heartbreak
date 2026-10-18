#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

//**************************************************
//MIDI
typedef enum {
    MIDI_STATE_WAIT_FOR_STATUS,
    MIDI_STATE_WAIT_FOR_DATA1,
    MIDI_STATE_WAIT_FOR_DATA2,
    MIDI_STATE_WAIT_FOR_SYSEX,
    MIDI_STATE_RUNNING_STATUS,
    MIDI_STATE_ERROR,
} e_midi_fsm_state_t;

//*************************************************
//Voice Allocation


//*************************************************
//Oscillator


//*************************************************
//Envelope
typedef enum {
  ENV_STATE_OFF,
  ENV_STATE_ATTACK,
  ENV_STATE_DECAY,
  ENV_STATE_SUSTAIN,
  ENV_STATE_RELEASE
} e_env_fsm_state_t;


//*************************************************
// Oscillator Wavetables
