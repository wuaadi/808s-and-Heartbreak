#include "voices.h"
#include "osc.h"
#include "midi.h"

int main (void) {
    //initialize DMA
    //initalize i2S
    //initalize pot adc
    //initalize midi
    //initialize oscillators
    //initialize envelope

    for ( ; ; ) {
        voice_allocation();
    }
}

void decode_midinote (uint32_t *phase_inc, uint32_t *phase)

void voice_allocation () {
    //check if MIDI ring buffer has any pending messages; if not, return
    if (g_midi_rb.head == g_midi_rb_tail)
        return;
    //dequeue the ring buffer
    MidiNote_t note = dq_midi_rb(g_midi_rb);
    //decode note number into frequency & phase inc
    
    
}