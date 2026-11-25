#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/sync.h"

#include "voices.h"
#include "midi.h"
#include "gpio.h"


//Data structures list for VOICE ALLOCATION

//audio blocks
uint32_t g_block_A [AUDIOBLOCK_SIZE];
uint32_t g_block_B [AUDIOBLOCK_SIZE];
uint32_t * g_block_CPU; //the block that is currently being generated
uint32_t * g_block_I2S; //the block that is currently being processed by I2S/DMA

uint32_t g_MIDInote_to_phase [127];

//*********************************************************
//Function declarations
void newpress (uint8_t notenum);
void newrelease (uint8_t notenum);
void voice_allocation();
uint32_t freq_to_phase(uint32_t freq);
void env_update (Voice_t *v);
void init_midi_phase_table(uint32_t *table);

void render_block (int16_t * block);
void format_for_pio(int32_t * arr);
void init_set_priorities(void);

//*******************************************************
//audio block generation ISR
void ISR_audioblock (void) { //COMPLETE
    //Triggered by DMA IRQ flag
    dma_channel_acknowledge_irq0(DMA_CHANNEL);

    //swap which buffer DMA reads
    swap_audioblocks();
    //reconfig DMA write address after swapping
    dma_channel_set_read_addr
    (
        DMA_CHANNEL, //same dma channel 0
        g_block_I2S, //reassign pointer after swap
        true //trigger DMA on new block
    );
    //generates 64-sample audio block on per-voice block basis
    render_block(g_block_CPU);
}

//************************************************************
//main loop
int main (void) {
    init_MIDI():
    init_POT();
    init_AUDIO();
    for (;;) {
        voice_allocation();
    }
} //main()

//************************************************************** */
//FUNCTION LIST FOR VOICE alloca

void newpress (uint32_t notenum) { //complete
    //variables to keep track of:
    int age_oldest = 0;
    int idx_target = -1;
    for (int i=0; i<NUM_VOICES; i++) {
        g_v[i].age++; //unconditionally increment all voice ages
        //a. if same note is already playing, restart it
        if (g_v[i].phase_inc == g_MIDInote_to_phase [notenum] && g_v[i].pressed == true) {
            idx_target = i;
            break;
        }
        //b. search for unused voice
        if (g_v[i].env_state == ENV_STATE_OFF) {
            idx_target = i;
            break;
        }
        //c. if unable, use oldest voice
        if (g_v[i].age >= age_oldest) {
            age_oldest = g_v[i].age;
            idx_target = i;
        }
    } //for
    //turn on the target voice
    if (idx_target >= 0) {
        g_v[idx_target].phase_inc = g_MIDInote_to_phase [notenum];;
        g_v[idx_target].pressed = true;
        g_v[idx_target].age = 0;
        g_v[idx_target].env_state = ENV_STATE_ATTACK;
    }
} //newpress

void newrelease (uint8_t notenum) { //complete
    //search for note with same notenumber, release it
    for (int i=0; i<NUM_VOICES; i++) {
        if (g_v[i].notenum == notenum && g_v[i].pressed == true) {
            g_v[i].pressed = false;
            g_v[i].env_state = ENV_STATE_RELEASE;
            break;
        }
    }
} //newrelease

void voice_allocation() { //COMPLETE
    //handles a single midi message
    if (g_midi_rb.head == g_midi_rb.tail) 
        return;
    //1. dequeues MIDI buffer
    MidiNote_t l_midinote = dq_midi_rb(&g_midi_rb);
    MidiByte_t l_MIDIstatus = l_midinote.event;
    MidiByte_t l_MIDInotenumber = l_midinote.number;
    MidiByte_t l_MIDIvelocity = l_midinote.velocity;
    //2. decodes phase increment
    if (l_MIDIstatus == MIDION)  
        newpress (MIDInotenumber);
    else if (l_MIDIstatus == MIDIOFF)
        newrelease (MIDInotenumber);
    return;
}


void env_update (Voice_t *v) { //complete
    ///update voice according to FSM
    //new voice, new release already handled by voice_allocation() in main()
    uint16_t level;
    switch (v->env_state) {
        case ENV_STATE_OFF:
            v->env_idx = 0;
            break;
        case ENV_STATE_ATTACK:
            level = env_a_table[v->env_idx];
            v->env_idx += v->a_inc;
            if (v->env_idx >= ENV_WAVETABLE_SIZE) {
            v->env_idx = 0;
            v->env_state = ENV_STATE_DECAY;
            }
            break;
        case ENV_STATE_DECAY: {
            uint32_t drop = UINT16_MAX - v->s_level;
            level = UINT16_MAX - ((drop * (UINT16_MAX - env_d_table[v->env_idx])) >> 16);
            v->env_idx += v->d_inc;
            if (v->env_idx >= ENV_WAVETABLE_SIZE) {
            v->env_idx = 0;
            v->env_state = ENV_STATE_SUSTAIN;
            }
            break;
        }
        case ENV_STATE_SUSTAIN:
            level = v->s_level;
            break;
        case ENV_STATE_RELEASE:
            level = ((uint32_t)v->env_amp_latch * env_r_table[v->env_idx]) >> 16;
            v->env_idx += v->r_inc;
            if (v->env_idx >= ENV_WAVETABLE_SIZE) {
            v->env_idx = 0;
            v->env_state = ENV_STATE_OFF;
            }
            break;
    } //switch

    v->env_base_level = (uint16_t)level;
    v->env_level = (level * v->velocity) >> 7;
    // >> 7 same as /128, scaling by max velocity value
} //env_update

void init_midi_phase_table(uint32_t *table) { //complete
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

//***************************************************************** */
//FX LIST FOR AUDIOBLOCK ISR

void render_block (int16_t * block) { //COMPLETE
    uint32_t *ab = g_block_CPU;
    //clear block
    for (int k=0; k<AUDIOBLOCK_SIZE; k++) 
        ab[k] = 0;
    //sum 
    for (int j=0;  j< NUM_VOICES; j++) {
        Voice_t *v = &g_v[j];
        //update envelope
        env_update(&v);
        if (v->env_state == ENV_STATE_OFF) continue;
        for (int i=0; i<AUDIOBLOCK_SIZE ; i++) {
            //do all generation for each sample
            v->phase_acc += v->phase_inc;
            int32_t s = (int32_t)v->osc_table[v->phase_acc >> 24] - 32768; //subtract dc offset; choose only top 8 index bits from phase
            //aduio summing
            ab[i] += (s * (int32_t)(v->env_level >> 1)) >> 15; //total right-shift = 16      
        } //if
    }//for
    format_for_pio (ab);
} //render_block

void format_for_pio(int32_t * arr) { //COMPLETE
    //handles scaling for sum, shifting for pio output
    for (int i=0; i<NUM_VOICES; i++)
        if (arr[i] > INT16_MAX) arr[i] = INT16_MAX;
        else if (mix < INT16_MIN) arr[i] = INT16_MIN;
        arr[i] >>= 4; //shift down to make it fit
        arr[i] <<= 15;
} //format_for_pio

void init_set_priorities(void) { //COMPLETE
    init_midi_phase_table();
    irq_set_priority(MIDI_IRQ, 0x00); // MIDI is most important
    irq_set_priority(DMA_IRQ,   0x40); //then audio block generation (takes a while)
    irq_set_priority(POT_IRQ,     0xC0); // pot least important
}
