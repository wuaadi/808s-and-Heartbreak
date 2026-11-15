#ifndef MIDI_H
#define MIDI_H

#include <stdio.h>
#include <stdlib.h>
typedef enum {
    //fill in with AKWF-FREE wavetable names
    SINE,
    SAW,
    TRIANGLE,

} WavetableMode_t;

WavetableMode_t wtable_mode;

uint8_t cycle_wtable_matrix;

//midi channel number
const uint8_t MIDI_CHANNEL = 0x0; //midi channel 1, zero-based indexing
//midi channels codes (only top 4 bytes)
const uint8_t MIDI_NOTE_OFF =  0x80 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_NOTE_ON = 0x90 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_POLY_PRESSURE = 0xA0 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_CTRL_CHANGE = 0xB0 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_PRG_CHANGE = 0xC0 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_CHAN_PRESSURE = 0xD0 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_PITCH_BEND = 0xE0 | (MIDI_CHANNEL & 0xF);
//

volatile MidiByte_t midi_event_last;

//midi note struct
typedef struct {
    uint32_t
} MidiPhaseIncTable_t;

//functions
void midi_uart_init ();

void ISR_midi ();

#endif