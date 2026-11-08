#include "voices.h"

volatile EnvParams_t g_env_params;
MidiRingBuffer_t g_midi_rb;

MidiNote_t dq_midi_rb (MidiRingBuffer_t *g_midi_rb) {
    MidiNote_t event = fifo[g_midi_rb->tail];
    g_midi_rb->tail++;
    return event;
}

int eq_midi_rb (MidiRingBuffer_t *g_midi_rb, MidiNote_t new_note;) {
    //overwrites oldest byte in case of overflow
    g_midi_rb->head++;
    int overflow = ((g_midi_rb->head) == g_midi_rb->tail);
    g_midi_rb->fifo[g_midi_rb->head] = new_note;
    return overflow;
}

//button press cycles through wavetables
void cycle_wavetable() {
    //increment through pointer array of wavetables, should wrap around at end
}

