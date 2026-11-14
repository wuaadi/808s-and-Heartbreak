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
const uint8_t MIDI_CHANNEL = 0x1;
//midi channels codes (only top 4 bytes)
const uint8_t MIDI_NOTE_OFF 0x80 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_NOTE_ON 0x90 | (MIDI_CHANNEL & 0xF);

//midi note struct
typedef struct {
    uint32_t
} MidiPhaseIncTable_t;

#endif