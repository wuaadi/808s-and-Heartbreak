#include "voices.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

volatile EnvParams_t g_env_params;
MidiRingBuffer_t g_midi_rb;
MidiFSMState_t g_midi_fsm;
WavetableMode_t wtable_mode;
MidiByte_t g_midi_running_status;
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
report_midi_overflow();
void midi_fsm(uint8_t byte);

//************************************************************** */
//isr
void ISR_midi () { //COMPLETE
    //acknowledge interrupt; copied from lab 7
    uart_get_hw(MCU0_MIDI_UART)->icr = UART_UARTICR_RXIC_BITS;
    //read byte from uart
    if (!uart_is_readable(MCU0_MIDI_UART))
        return;
    //feed into state machine
    midi_fsm(uart_getc(MCU0_MIDI_UART));
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

void report_midi_overflow() { //INCOMPLETE
    return;
}

void init_midi () { //COMPLETE
    cycle_wtable_matrix = 0;
    gpio_init(MCU0_WAVETABLE_BTN_PIN);
    gpio_set_dir(MCU0_WAVETABLE_BTN_PIN, GPIO_IN);
    gpio_pull_down(MCU0_WAVETABLE_BTN_PIN);

    g_midi_fsm = MIDI_STATE_WAIT_FOR_STATUS;

    midi_uart_init();
}

void midi_uart_init () { //COMPLETE
    gpio_set_function(MCU0_MIDI_RX_PIN, UART_FUNCSEL_NUM(MCU0_MIDI_UART, MCU0_MIDI_RX_PIN));
    
    uart_init(MCU0_MIDI_UART, MCU0_MIDI_BAUD);
    uart_set_format (MCU0_MIDI_UART, 8, 1, UART_PARITY_NONE);
    //set up isr_midi as handler
    irq_set_exclusive_handler(MIDI_IRQ, ISR_midi);
    uart_set_irq_enables(MCU0_MIDI_UART, true, false);
    irq_set_enabled(MIDI_IRQ, true);
}

void midi_fsm(uint8_t byte) { //COMPLETE
    if (byte >= 0xF8) {
        // isgnore system real time messages
        return;
    }
    if (byte >= 0xF0) {
        // system common and SysEx cancel running stats
        g_midi_running_status = 0;
        g_midi_fsm = MIDI_STATE_WAIT_FOR_STATUS;
        return;
    }
    if (byte & 0x80) {
        if (byte == MIDI_NOTE_ON || byte == MIDI_NOTE_OFF) {
            g_midi_running_status = byte;
            g_midi_fsm = MIDI_STATE_WAIT_FOR_DATA1;
        } else {
            // ignore other channels and channel voice mesg
            g_midi_running_status = 0;
            g_midi_fsm = MIDI_STATE_WAIT_FOR_STATUS;
        }
        return;
    }

    if (g_midi_fsm == MIDI_STATE_WAIT_FOR_DATA1) {
        //note number
        g_midi_note.number = byte;
        g_midi_fsm = MIDI_STATE_WAIT_FOR_DATA2;
    } else if (g_midi_fsm == MIDI_STATE_WAIT_FOR_DATA2) {
        //velocity
        g_midi_note.velocity = byte;
        g_midi_note.event = g_midi_running_status;
        if (g_midi_note.event == MIDI_NOTE_ON && byte == 0) {
            //zero velocity => turn off note
            g_midi_note.event = MIDI_NOTE_OFF;
        }
        if (eq_midi_rb(&g_midi_rb, g_midi_note)) {
            report_midi_overflow();
        }
        g_midi_fsm = MIDI_STATE_WAIT_FOR_DATA1;
    }
} //midi_fsm