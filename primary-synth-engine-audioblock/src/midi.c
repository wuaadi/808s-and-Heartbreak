#include "voices.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

volatile EnvParams_t g_env_params;
MidiRingBuffer_t g_midi_rb;
MidiFSMState_t g_midi_fsm;
WavetableMode_t wtable_mode;
uint8_t cycle_wtable_matrix;
MidiNote_t g_midi_note; //full MIDI note currently being built

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

//fx declarations
MidiNote_t dq_midi_rb (MidiRingBuffer_t *rb);
int eq_midi_rb (MidiRingBuffer_t *rb, MidiNote_t new_note);
void init_midi ();
void midi_uart_init ();

//************************************************************** */
//isr
void ISR_midi () {
    //acknowledge interrupt
    
    //read byte from uart
    if (!uart_is_readable(MCU0_MIDI_UART))
        return;
    uint8_t byte = uart_getc(MCU0_MIDI_UART);
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

//******************************************************************** */
//fx definitions
MidiNote_t dq_midi_rb (MidiRingBuffer_t *rb) { //COMPLETE
    MidiNote_t event = rb->fifo[rb->tail];
    rb->tail++;
    return event;
}

int eq_midi_rb (MidiRingBuffer_t *rb, MidiNote_t new_note) { //COMPLETE
    //overwrites oldest byte in case of overflow
    rb->head++;
    int overflow = ((rb->head) == rb->tail);
    if (overflow) rb->tail++;
    rb->fifo[rb->head] = new_note;
    return overflow;
}

void init_midi () { //COMPLETE
    cycle_wtable_matrix = 0;
    gpio_init(MCU0_WAVETABLE_BTN_PIN);
    gpio_set_dir(MCU0_WAVETABLE_BTN_PIN, GPIO_IN);
    gpio_pull_down(MCU0_WAVETABLE_BTN_PIN);

    midi_uart_init();
}

void midi_uart_init () { //INCOMPLETE
    gpio_set_function(MCU0_MIDI_RX_PIN, UART_FUNCSEL_NUM(MCU0_MIDI_UART, MCU0_MIDI_RX_PIN));
    
    uart_init(MCU0_MIDI_UART, MCU0_MIDI_BAUD);
    uart_set_format (MCU0_MIDI_UART, 8, 1, UART_PARITY_NONE);
    //set up isr_midi as handler
    irq_set_exclusive_handler(MIDI_IRQ, ISR_midi);
    irq_set_enabled(MIDI_IRQ, true);
}
