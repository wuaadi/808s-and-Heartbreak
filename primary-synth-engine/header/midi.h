#include <stdio.h>

typedef enum {
    //fill in with AKWF-FREE wavetable names

} WavetableMode_t;

//midi channel number
const uint8_t MIDI_CHANNEL = 0x1;
//midi channels codes (only top 4 bytes)
const uint8_t MIDI_NOTE_OFF 0x80 | (MIDI_CHANNEL & 0xF);
const uint8_t MIDI_NOTE_ON 0x90 | (MIDI_CHANNEL & 0xF);

//midi note struct
typedef struct {
    uint32_t
} MidiPhaseIncTable_t;