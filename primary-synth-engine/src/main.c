#include "voices.h"
#include <stdlib.h>
#include "osc.h"
#include "midi.h"

uint32_t midi_to_phase_inc[128];

int main (void) {
    //initialize DMA
    //initalize i2S
    //initalize pot adc
    //initalize midi
    //initialize oscillators
    //initialize envelope
    init_midi_phase_table(&midi_to_phase_inc);

    for ( ; ; ) {
        //voice allocation
        voice_allocation();
    }
}


void voice_allocation () {
    //check if MIDI ring buffer has any pending messages; if not, return
    if (g_midi_rb.head == g_midi_rb.tail)
        return;
    //dequeue the ring buffer
    MidiNote_t note = dq_midi_rb(&g_midi_rb);
    //decode note number into phase inc
    uint32_t note_phase_inc = midi_to_phase_inc[note.number];

    if (note.event == MIDI_NOTE_OFF || note.velocity == 0) {
        //search for an existing voice that's pressed and unpress it
        for (int i=0; i<NUM_VOICES; i++)
            if (g_voices[i].phase_inc == note_phase_inc && g_voices[i].pressed == true) {
                release_voice(&g_voices[i]);
                return;
            }
    }
    else if (note.event == MIDI_NOTE_ON) {
        //increment age
        
        //see if the array contains that note already
        //if so, restart the voice
        for (int i=0; i<NUM_VOICES; i++) {
            g_voices[i].age++;
            if (g_voices[i].phase_inc == note_phase_inc && g_voices[i].pressed == true) {
                press_voice (&g_voices[i], note_phase_inc, note.velocity);
            }
        }
        //if not, choose an empty voice
        uint32_t age_oldest=0;
        uint32_t idx_oldest=0;
        for (int i=0; i<NUM_VOICES; i++) {
            // Q** : does it matter if pressed == true?
            if (g_voices[i].env_state == ENV_STATE_OFF) {
                press_voice (&g_voices[i], note_phase_inc, note.velocity);
                return;
            }
                
            //keep track of oldest voice (index)
            if (g_voices[i].age >= age_oldest) {
                age_oldest = g_voices[i].age;
                idx_oldest = i;
            }
        }
        //if not, replace the oldest voice
        press_voice (&g_voices[idx_oldest], note_phase_inc, note.velocity);
    }
    //all other commands are ignored
    return;
}



void init_midi_phase_table(uint32_t *table) {
    for (int n = 0; n < 128; n++) {
        // stndard MIDI note-to-frequency formula used A4 = note 69 = 440 hz
        float freq = 440.0f * powf(2.0f, (float)(n - 69) / 12.0f);
        
        // calculate 32-bit fixed-point phase increment
        // general formula: phase_inc = round(f * wavetable_size * 2^franctional precusion size)

        double phase_inc_double = (freq * AKWF_WAVETABLE_SIZE * powf(2, 24)) / SAMPLE_RATE;
        //high 8 bits are index, lower for fractional interpolation (unused)
        table[n] = (uint32_t)phase_inc_double;
    }
}

void press_voice (Voice_t *voice, uint32_t phase_inc, uint8_t velocity) {
    //voice->osc_table = ; fill in when wavetable cycling fx finished
    voice->env_state = ENV_STATE_OFF;
    voice->phase_acc = 0;
    voice->phase_inc = phase_inc;
    voice->velocity = velocity;

    //latch ADSR potentiometer readings
    voice->a_inc = g_env_params.a_inc;
    voice->d_inc = g_env_params.d_inc;
    voice->r_inc = g_env_params.r_inc;
    voice->s_level = g_env_params.s_level;
    voice->age = 0;
    voice->pressed = true;
    voice->env_state = ENV_STATE_ATTACK;
}
void release_voice (Voice_t *voice) {
    voice->env_idx = 0;
    voice->env_amp_latch = voice->env_level;
    voice->env_state = ENV_STATE_RELEASE;

    voice->pressed = false;
}