#ifndef VOICES_H
#define VOICES_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include <stdbool.h>
#include <stddef.h>

//**************************************************
//MIDI
#define MIDI_RINGBUFFER_SIZE 256
#define MIDI_UART_BAUD_RATE 31250
#define SAMPLE_RATE 32768u
#define NUM_VOICES 16 //size of voice array




typedef enum {
    MIDI_STATE_WAIT_FOR_STATUS,
    MIDI_STATE_WAIT_FOR_DATA1,
    MIDI_STATE_WAIT_FOR_DATA2,
    MIDI_STATE_WAIT_FOR_SYSEX, //unused
    MIDI_STATE_RUNNING_STATUS, //unused
    MIDI_STATE_ERROR,
} MidiFSMState_t;

extern MidiFSMState_t g_midi_fsm;


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

enum {
  ENV_REQUEST_NONE = 0,
  ENV_REQUEST_ATTACK = 1u << 0,
  ENV_REQUEST_RELEASE = 1u << 1
};



//*************************************************
//Voice Allocation
typedef struct {
  /*oscillator parameters */
  const uint16_t *osc_table; //pointer to wavetable
  volatile uint32_t phase_acc; //phase accumulator
  volatile uint32_t phase_inc; //phase increment
  uint8_t velocity; //velocity of the note

  //envelope parameters
  volatile uint16_t env_idx; //envelope index
  uint8_t a_inc, d_inc, r_inc; //attack, decay, release increments
  uint32_t s_level; //sustain level
  volatile EnvState_t env_state;
  volatile uint8_t env_requests; // consumed by the envelope ISR on the same core

  //audio/additonal env params
  volatile uint16_t env_amp_latch; //latch for envelope amplitude, used for release state
  uint16_t env_base_level; // envelope level before velocity scaling
  volatile uint16_t env_level; // final output of a voice, amplitude after envelope applied to oscillator output

  //voice allocation
  uint32_t age; //age of the voice, used for voice stealing

  //boolen flags
  volatile bool pressed; //true if note is pressed
} Voice_t;

//*************************************************
//Oscillator
extern int16_t g_audio; //final audio waveform to be sent to DMA
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

#endif