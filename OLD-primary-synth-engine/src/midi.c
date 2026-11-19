#include "voices.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

volatile EnvParams_t g_env_params;
MidiRingBuffer_t g_midi_rb;
MidiFSMState_t g_midi_fsm;
WavetableMode_t wtable_mode;
uint8_t cycle_wtable_matrix;

const uint16_t *const wavetables[WTABLE_MODE_COUNT] = {
    [PLAIN_SINE]    = PLAIN_sin,
    [PLAIN_SAW]     = PLAIN_saw,
    [PLAIN_TRI]     = PLAIN_tri,
    [PLAIN_SQU]     = PLAIN_squ,
    [AKWF_BW_SIN]   = AKWF_sin_0001,
    [AKWF_BW_SAW]   = AKWF_saw_0001,
    [AKWF_BW_TRI]   = AKWF_tri_0001,
    [AKWF_BW_SQU]   = AKWF_sq_0001,
    [AKWF_LINEAR]   = AKWF_linear_0001,
    [AKWF_PIANO]    = AKWF_piano_0001,
    [AKWF_VIOLIN]   = AKWF_violin_0001,
    [AKWF_THEREMIN] = AKWF_theremin_0001,
    [AKWF_OBOE]     = AKWF_oboe_0001,
    [AKWF_OVERTONE] = AKWF_overtone_0001,
    [AKWF_HVOICE]   = AKWF_hvoice_0001,
    [AKWF_EBASS]    = AKWF_ebass_0001,
};



MidiNote_t dq_midi_rb (MidiRingBuffer_t *rb) {
    MidiNote_t event = rb->fifo[rb->tail];
    rb->tail++;
    return event;
}

int eq_midi_rb (MidiRingBuffer_t *rb, MidiNote_t new_note) {
    //overwrites oldest byte in case of overflow
    rb->head++;
    int overflow = ((rb->head) == rb->tail);
    if (overflow) rb->tail++;
    rb->fifo[rb->head] = new_note;
    return overflow;
}

bool debounce_wavetable() {
    //handles debouncing for cycle_wtable_button
    //pull down wiring implies pressed == 1
    uint8_t sample = gpio_get(MCU1_WAVETABLE_BTN_PIN) ? 1 : 0;
    //newpress will be either 0 or 1
    cycle_wtable_matrix = (uint8_t)((cycle_wtable_matrix << 1) | sample);
    //return true if the matrix is a rising edge
    return ((cycle_wtable_matrix & 0xFF) == 0x0F);
}

//button press cycles through wavetables
void cycle_wavetable() {
    //increment through pointer array of wavetables, should wrap around at end
    if (!debounce_wavetable())
        return;
    wtable_mode = (WavetableMode_t)((wtable_mode + 1) % WTABLE_MODE_COUNT);
    
    //assign wavetables depending on mode
    //envelope state does not affect wavetable selection
    const uint16_t *table = wavetables[wtable_mode];
    for (int i = 0; i < NUM_VOICES; i++)
        g_voices[i].osc_table = table;
    
   return;
}

void init_midi () {
    cycle_wtable_matrix = 0;
    gpio_init(MCU1_WAVETABLE_BTN_PIN);
    gpio_set_dir(MCU1_WAVETABLE_BTN_PIN, GPIO_IN);
    gpio_pull_down(MCU1_WAVETABLE_BTN_PIN);
}


void midi_uart_init () {
    gpio_set_function(MCU1_MIDI_RX_PIN, UART_FUNCSEL_NUM(MCU1_MIDI_UART, MCU1_MIDI_RX_PIN));
    
    uart_init(MCU1_MIDI_UART, MCU1_MIDI_BAUD);
    uart_set_format (MCU1_MIDI_UART, 8, 1, 0);
    //set up isr_midi as handler

}


void ISR_midi () {
    //acknowledge interrupt
    
    //read byte from uart
    if (!uart_is_readable(MCU1_MIDI_UART))
        return;
    uint8_t byte = uart_getc(MCU1_MIDI_UART);
    //feed into state machine
    if (byte & 0x80) {
        //if MSB != 0, is status byte

        if (byte == MIDI_NOTE_ON || byte == MIDI_NOTE_OFF)
            midi_event_last = byte;
    }
    else {
        //channel bytes, ignore any byte not within 0 - 127
        
    }

}

