#include "voices.h"

volatile EnvParams_t g_env_params;
MidiRingBuffer_t g_midi_rb;

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
    //read the gpio reg, add value to shift register
    uint8_t newpress = (sio_hw->gpio_in >> MCU1_WAVETABLE_BTN_PIN) & 0x1;
    //for pull-up resistor, invert the bit
    newpress = ~newpress;
    //newpress will be either 0 or 1
    cycle_wtable_matrix <<= 1;
    cycle_wtable_matrix |= newpress;
    //return true if the matrix is a rising edge
    return ((cycle_wtable_matrix & 0xFF) == 0x0F);
}

//button press cycles through wavetables
void cycle_wavetable() {
    //increment through pointer array of wavetables, should wrap around at end
    if (!debounce_wavetable())
        return;
    wtable_mode = (WavetableMode_t)((wtable_mode + 1) % WTABLE_MODE_COUNT);
    /*
    //assign wavetables depending on mode
    switch (wtable_mode) {
        WT_SINE: ...
        WT_SAW: ...
        ...
    }
    */
   return;
}

void init_midi () {
    cycle_wtable_matrix = 0;
    gpio_init(MCU1_WAVETABLE_BTN_PIN);
    gpio_set_dir(MCU1_WAVETABLE_BTN_PIN, GPIO_IN);
    gpio_pull_down(MCU1_WAVETABLE_BTN_PIN);
}

static bool button_timer_cb(repeating_timer_t *rt) {
    (void)rt;
    cycle_wavetable();
    return true;   // true = keep repeating
}