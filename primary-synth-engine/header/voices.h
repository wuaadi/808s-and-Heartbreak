#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

//**************************************************
//MIDI
#define MIDI_RINGBUFFER_SIZE = 256;
#define MIDI_UART_BAUD_RATE = 31250;
#define NUM_VOICES = 16; //size of voice array

typedef uint8_t MidiByte_t;

typedef enum {
    MIDI_STATE_WAIT_FOR_STATUS,
    MIDI_STATE_WAIT_FOR_DATA1,
    MIDI_STATE_WAIT_FOR_DATA2,
    MIDI_STATE_WAIT_FOR_SYSEX,
    MIDI_STATE_RUNNING_STATUS,
    MIDI_STATE_ERROR,
} MidiFSMState_t;

typedef struct{
  volatile MidiByte_t fifo[MIDI_RINGBUFFER_SIZE];
  volatile uint8_t head;
  volatile uint8_t tail;
} MidiRingBuffer_t

//*************************************************
//Voice Allocation


//*************************************************
//Oscillator

//*************************************************
//Potentiometers
typedef struct {
    uint8_t attack_inc;
    uint8_t decay_inc;
    uint8_t release_inc;
    uint32_t sustain_level;
} EnvParams_t;

extern volatile EnvParams_t g_env_params;

//*************************************************
//Envelope
typedef enum {
  ENV_STATE_OFF,
  ENV_STATE_ATTACK,
  ENV_STATE_DECAY,
  ENV_STATE_SUSTAIN,
  ENV_STATE_RELEASE
} EnvState_t;

