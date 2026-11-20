#include "voices.h"
#include <stdlib.h>
#include "osc.h"
#include "midi.h"
#include "gpio.h"
#include "hardware/sync.h"

//Data structures list for VOICE ALLOCATION
#define AUDIOBLOCK_SIZE 64
#define NUM_VOICES 16
int16_t g_audioblock_A [AUDIOBLOCK_SIZE];
int16_t g_audioblock_B [AUDIOBLOCK_SIZE];


int16_t * g_current_audioblock;



int16_t g_MIDInote_to_freq [127];

//*********************************************************
//Function list for VOICE ALLOCATION

void newpress (uint32_t phase_inc) {
    //variables to keep track of:
    int age_oldest = 0;
    int idx_target = -1;
    for (int i=0; i<NUM_VOICES; i++) {
        //a. if same note is already playing, restart it
        if (g_voices[i].phase_inc == phase_inc && g_voices[i].pressed == true) {
            idx_target = i;
            break;
        }
        //b. search for unused voice
        if (g_voices[i].env_state == ENV_STATE_OFF) {
            idx_target = i;
            break;
        }
        //c. if unable, use oldest voice
        if (g_voices[i].age >= age_oldest) {
            age_oldest = g_voices[i].age;
            idx_target = i;
        }
    } //for
    //turn on the target voice
    if (idx_target >= 0) {
        g_voices[idx_target].phase_inc = phase_inc;
        g_voices[idx_target].pressed = true;
        g_voices[idx_target].age = 0;
        g_voices[idx_target].env_state = ENV_STATE_ATTACK;
    }
}

void newrelease (uint32_t phase_inc) {
    //search for note with same notenumber, release it
    for (int i=0; i<NUM_VOICES; i++) {
        if (g_voices[i].phase_inc == phase_inc && g_voices[i].pressed == true) {
            g_voices[i].pressed = false;
            g_voices[i].env_state = ENV_STATE_RELEASE;
            break;
        }
    }
}

void voice_allocation() {
    //handles a single midi message
    IF buffer empty THEN return
    //1. dequeues MIDI buffer
    l_MIDIstatus, l_MIDInotenumber, l_MIDIvelocity = dequeue(MIDI_ring_buffer)
    //2. decodes phase increment
    l_freq = MIDInote_to_freq [MIDInotenumber]
    l_phase_inc = freq_to_phase(l_freq);
    if (MIDIstatus == MIDION)  
        newpress (MIDInotenumber, l_phase_inc);
    else if (MIDIstatus == MIDIOFF)
        newrelease (MIDInotenumber);
    return
}

void env_update (voice) {
    ///update voice according to FSM
    //new voice, new release already handled by voice_allocation() in main()
}

void init_midi_phase_table(uint32_t *table) {
    for (int n = 0; n < 128; n++) {
        // stndard MIDI note-to-frequency formula used A4 = note 69 = 440 hz
        double freq = 440.0f * powf(2.0f, (float)(n - 69) / 12.0f);
        
        // calculate 32-bit fixed-point phase increment
        // general formula: phase_inc = round(f * wavetable_size * 2^franctional precusion size)

        double phase_inc_double = (freq * AKWF_WAVETABLE_SIZE /** powf(2, 24)*/) / SAMPLE_RATE;
        //high 8 bits are index, lower for fractional interpolation (unused)
        table[n] = (uint32_t)phase_inc_double;
    }
}

//*******************************************************
//audio block generation ISR
void ISR_audioblock (void) {
    //Triggered by DMA IRQ flag
    clear DMA IRQ flag

    //swap which buffer DMA reads

    //generates 64-sample audio block on per-voice block basis
    int16_t *ab = g_current_audioblock
    for (i < 16)
    {
        v = g_v
        //update envelope
        update_env(&v)
        for (i<64) {
            //do all generation for each sample
        }
        //do audio summing & scaling for that sample
    }
}

//************************************************************
//main loop
int main (void) {
    init_DMA();
    init_MIDI():
    init_AUDIO();
    init_POTS();
    for (;;)
        voice_allocation();
}