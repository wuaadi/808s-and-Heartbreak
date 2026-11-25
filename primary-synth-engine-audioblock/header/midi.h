#ifndef MIDI_H
#define MIDI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "pot.h"
#include "gpio.h"
#include "voices.h"

#define MIDI_RINGBUFFER_SIZE 256
#define MIDI_IRQ UART1_IRQ
#define DONNER_N_25

//Custom types
//midi channel number
#define MIDI_CHANNEL        0x0 //midi channel 1, zero-based indexing

//Channel Voice Messages (cancel running status)
#define MIDI_NOTE_OFF       (0x80 | (MIDI_CHANNEL & 0xF))
#define MIDI_NOTE_ON        (0x90 | (MIDI_CHANNEL & 0xF))
#define MIDI_POLY_PRES      (0xA0 | (MIDI_CHANNEL & 0xF))
#define MIDI_CTRL_CHANGE    (0xB0 | (MIDI_CHANNEL & 0xF))
#define MIDI_PRG_CHANGE     (0xC0 | (MIDI_CHANNEL & 0xF))
#define MIDI_CHAN_PRESSURE  (0xD0 | (MIDI_CHANNEL & 0xF))
#define MIDI_PITCH_BEND     (0xE0 | (MIDI_CHANNEL & 0xF))

//System Common Messages (cancel running status)

//System Real Time Messages (does not cancel running status)

//******************************************************************** */
typedef struct {
  MidiByte_t event;
  MidiByte_t velocity;
  MidiByte_t number;
} MidiNote_t;

typedef struct{
  volatile MidiNote_t fifo[MIDI_RINGBUFFER_SIZE];
  volatile uint8_t head;
  volatile uint8_t tail;
} MidiRingBuffer_t;

extern MidiRingBuffer_t g_midi_rb;

typedef uint32_t WavetableMode_t;

typedef uint8_t MidiByte_t;

extern MidiRingBuffer_t g_midi_rb;
extern MidiFSMState_t g_midi_fsm;
extern WavetableMode_t wtable_mode;
extern uint8_t cycle_wtable_matrix;
extern MidiNote_t g_midi_note; //full MIDI note currently being built
extern MidiByte_t g_midi_running_status; //current status byte for midi


#ifdef DONNER_N_25
typedef enum {
    MIDI_STATE_WAIT_FOR_STATUS,
    MIDI_STATE_WAIT_FOR_DATA1,
    MIDI_STATE_WAIT_FOR_DATA2,
} MidiFSMState_t;

extern MidiFSMState_t g_midi_fsm;
#endif

//**************************************************************************
//function list
MidiNote_t dq_midi_rb(MidiRingBuffer_t *rb);
int eq_midi_rb(MidiRingBuffer_t *rb, MidiNote_t new_note);

void init_midi(void);
void midi_uart_init(void);
void ISR_midi(void);


#endif