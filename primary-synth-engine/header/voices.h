#include <stdio.h>
#include <string.h>
#include <math.c>
#include "pico/stdlib.h"

//**************************************************
//MIDI
#define MIDI_RINGBUFFER_SIZE 256
#define ENV_WAVETABLE_SIZE 256
#define MIDI_UART_BAUD_RATE 31250
const uint32_t SAMPLE_RATE=  pow(2, 15); //32,768g
#define NUM_VOICES 16 //size of voice array

typedef uint8_t MidiByte_t;

typedef struct {
  MidiByte_t event;
  MidiByte_t velocity;
  MidiByte_t number;
} MidiNote_t;

typedef enum {
    MIDI_STATE_WAIT_FOR_STATUS,
    MIDI_STATE_WAIT_FOR_DATA1,
    MIDI_STATE_WAIT_FOR_DATA2,
    MIDI_STATE_WAIT_FOR_SYSEX,
    MIDI_STATE_RUNNING_STATUS,
    MIDI_STATE_ERROR,
} MidiFSMState_t;

typedef struct{
  volatile MidiNote_t fifo[MIDI_RINGBUFFER_SIZE];
  volatile uint8_t head;
  volatile uint8_t tail;
} MidiRingBuffer_t;

extern MidiRingBuffer_t g_midi_rb;


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
//Voice Allocation
typedef struct {
  /*oscillator parameters */
  uint16_t *osc_table; //pointer to wavetable
  volatile uint32_t phase_acc; //phase accumulator
  volatile uint32_t phase_inc; //phase increment
  uint8_t velocity; //velocity of the note

  //envelope parameters
  uint16_t *env_table; //pointer to envelope table
  volatile uint16_t env_idx; //envelope index
  uint8_t a_inc, d_inc, r_inc; //attack, decay, release increments
  uint32_t s_level; //sustain level
  volatile EnvState_t env_state;

  //audio/additonal env params
  volatile uint16_t env_amp_latch; //latch for envelope amplitude, used for release state
  volatile uint16_t env_level; // final output of a voice, amplitude after envelope applied to oscillator output

  //voice allocation
  uint32_t age; //age of the voice, used for voice stealing

  //boolen flags
  volatile bool pressed; //true if note is pressed
} Voice_t;

//*************************************************
//Oscillator
extern uint16_t g_audio; //final audio waveform to be sent to DMA
extern Voice_t g_voices[NUM_VOICES];
//*************************************************
//Potentiometers
typedef struct {
    uint8_t a_inc;
    uint8_t d_inc;
    uint8_t r_inc;
    uint32_t s_level;
} EnvParams_t;

extern volatile EnvParams_t g_env_params;

