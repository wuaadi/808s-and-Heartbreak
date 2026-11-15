/*
 * Timer-driven alternative to main_prototype.c; not part of the firmware build.
 * The I2S and synth hooks below still need implementations.
 *
 * Reserve PWM slice 0 and DMA IRQ 0 for this prototype. I2S must output
 * exactly 40,000 stereo frames/second using a compatible clock. Each DMA word
 * below packs left in bits 31:16 and right in bits 15:0; the PIO program must
 * transmit that exact layout. Verify FIFO coverage of DMA restart latency.
 * Use the same core for control updates and audio interrupts. Keep control
 * critical sections short enough not to delay the 25 us sample deadline.
 * The existing voices.h and effects engine currently use different sample
 * rates; both and the phase table must be updated before integrating this.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#define SAMPLE_RATE_HZ 40000u
#define AUDIO_BLOCK_FRAMES 64u
#define AUDIO_BUFFER_COUNT 3u
#define ENVELOPE_PERIOD_SAMPLES 40u
#define SAMPLE_PWM_SLICE 0u

typedef enum {
    BUFFER_FREE,
    BUFFER_FILLING,
    BUFFER_READY,
    BUFFER_PLAYING
} BufferState;

typedef enum {
    AUDIO_OK,
    AUDIO_INIT_ERROR,
    AUDIO_TIMER_ERROR,
    AUDIO_UNDERRUN,
    AUDIO_OVERRUN,
    AUDIO_DMA_ERROR
} AudioFault;

/*
 * Configure the I2S PIO and return its TX FIFO address and TX DREQ.
 * Confirm the actual I2S rate and word layout before enabling playback.
 */
extern bool i2s_tx_init(uint32_t sample_rate_hz, volatile uint32_t **tx_fifo,
                        uint *tx_dreq);
extern void i2s_tx_stop(void);
extern bool synth_init(uint32_t sample_rate_hz);
extern int16_t render_sample(void); // advance and mix every voice once
extern void update_envelopes(void); // consume requests and advance ADSR
extern void service_one_control_event(void);
extern void report_audio_fault(AudioFault fault);

static uint32_t audio_buffers[AUDIO_BUFFER_COUNT][AUDIO_BLOCK_FRAMES];
static volatile BufferState buffer_states[AUDIO_BUFFER_COUNT];
static volatile uint32_t ready_order[AUDIO_BUFFER_COUNT];
static volatile uint32_t next_ready_order;
static volatile AudioFault audio_fault = AUDIO_OK;
static unsigned int playing_buffer;
static int filling_buffer = -1;
static unsigned int fill_index;
static unsigned int envelope_samples;
static int dma_channel;

static int find_free_buffer(void)
{
    for (unsigned int i = 0; i < AUDIO_BUFFER_COUNT; ++i) {
        if (buffer_states[i] == BUFFER_FREE) {
            return (int)i;
        }
    }
    return -1;
}

static int find_oldest_ready_buffer(void)
{
    int oldest = -1;
    for (unsigned int i = 0; i < AUDIO_BUFFER_COUNT; ++i) {
        if (buffer_states[i] == BUFFER_READY &&
            (oldest < 0 ||
             (int32_t)(ready_order[i] - ready_order[oldest]) < 0)) {
            oldest = (int)i;
        }
    }
    return oldest;
}

static void start_dma(unsigned int buffer)
{
    dma_channel_set_read_addr((uint)dma_channel, audio_buffers[buffer], false);
    dma_channel_set_trans_count((uint)dma_channel, AUDIO_BLOCK_FRAMES, true);
}

static void sample_timer_irq(void)
{
    pwm_clear_irq(SAMPLE_PWM_SLICE);
    if (audio_fault != AUDIO_OK) {
        return;
    }

    if (filling_buffer < 0) {
        uint32_t irq_state = save_and_disable_interrupts();
        filling_buffer = find_free_buffer();
        if (filling_buffer >= 0) {
            buffer_states[filling_buffer] = BUFFER_FILLING;
        }
        restore_interrupts(irq_state);
        if (filling_buffer < 0) {
            audio_fault = AUDIO_OVERRUN;
            return;
        }
    }

    if (envelope_samples == 0u) {
        update_envelopes();
    }
    int16_t sample = render_sample();
    uint32_t pcm = (uint16_t)sample;
    audio_buffers[filling_buffer][fill_index++] = (pcm << 16) | pcm;
    envelope_samples = (envelope_samples + 1u) % ENVELOPE_PERIOD_SAMPLES;

    if (fill_index == AUDIO_BLOCK_FRAMES) {
        uint32_t irq_state = save_and_disable_interrupts();
        ready_order[filling_buffer] = next_ready_order++;
        __dmb();
        buffer_states[filling_buffer] = BUFFER_READY;
        filling_buffer = -1;
        fill_index = 0u;
        restore_interrupts(irq_state);
    }

    /* A second wrap during this handler means at least one deadline was missed. */
    if (pwm_get_irq_status_mask() & (1u << SAMPLE_PWM_SLICE)) {
        audio_fault = AUDIO_TIMER_ERROR;
    }
}

static void audio_dma_irq(void)
{
    uint32_t channel_mask = 1u << (uint)dma_channel;
    if ((dma_hw->ints0 & channel_mask) == 0u) {
        return;
    }
    dma_hw->ints0 = channel_mask;

    uint32_t irq_state = save_and_disable_interrupts();
    if (audio_fault != AUDIO_OK) {
        restore_interrupts(irq_state);
        return;
    }
    if (dma_channel_hw_addr((uint)dma_channel)->al1_ctrl &
        (DMA_CH0_CTRL_TRIG_READ_ERROR_BITS | DMA_CH0_CTRL_TRIG_WRITE_ERROR_BITS)) {
        audio_fault = AUDIO_DMA_ERROR;
        restore_interrupts(irq_state);
        return;
    }
    buffer_states[playing_buffer] = BUFFER_FREE;
    int next = find_oldest_ready_buffer();
    if (next < 0) {
        audio_fault = AUDIO_UNDERRUN;
        restore_interrupts(irq_state);
        return;
    }
    playing_buffer = (unsigned int)next;
    buffer_states[playing_buffer] = BUFFER_PLAYING;
    start_dma(playing_buffer);
    restore_interrupts(irq_state);
}

static bool init_audio_interrupts(void)
{
    volatile uint32_t *tx_fifo;
    uint tx_dreq;
    uint32_t clock_hz = clock_get_hz(clk_sys);
    if (clock_hz % SAMPLE_RATE_HZ != 0u ||
        clock_hz / SAMPLE_RATE_HZ == 0u ||
        clock_hz / SAMPLE_RATE_HZ > 65536u) {
        return false;
    }
    if (!i2s_tx_init(SAMPLE_RATE_HZ, &tx_fifo, &tx_dreq)) {
        return false;
    }
    dma_channel = dma_claim_unused_channel(false);
    if (dma_channel < 0) {
        i2s_tx_stop();
        return false;
    }

    dma_channel_config dma_cfg = dma_channel_get_default_config((uint)dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);
    channel_config_set_dreq(&dma_cfg, tx_dreq);
    dma_channel_configure((uint)dma_channel, &dma_cfg, tx_fifo,
                          audio_buffers[0], AUDIO_BLOCK_FRAMES, false);

    pwm_config timer_cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&timer_cfg, 1.0f);
    pwm_config_set_wrap(&timer_cfg, clock_hz / SAMPLE_RATE_HZ - 1u);
    pwm_init(SAMPLE_PWM_SLICE, &timer_cfg, false);
    pwm_clear_irq(SAMPLE_PWM_SLICE);
    irq_set_exclusive_handler(PWM_DEFAULT_IRQ_NUM(), sample_timer_irq);
    irq_set_priority(PWM_DEFAULT_IRQ_NUM(), 0x00);
    pwm_set_irq_enabled(SAMPLE_PWM_SLICE, true);
    irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);

    dma_hw->ints0 = 1u << (uint)dma_channel;
    irq_set_exclusive_handler(DMA_IRQ_0, audio_dma_irq);
    irq_set_priority(DMA_IRQ_0, 0x40);
    dma_channel_set_irq0_enabled((uint)dma_channel, true);
    irq_set_enabled(DMA_IRQ_0, true);
    return true;
}

int main(void)
{
    if (!synth_init(SAMPLE_RATE_HZ) || !init_audio_interrupts()) {
        report_audio_fault(AUDIO_INIT_ERROR);
        return 1;
    }

    /* Two initial blocks of silence give the timer time to fill the third. */
    buffer_states[0] = BUFFER_PLAYING;
    buffer_states[1] = BUFFER_READY;
    buffer_states[2] = BUFFER_FREE;
    ready_order[1] = next_ready_order++;
    playing_buffer = 0u;
    start_dma(0u);
    pwm_set_enabled(SAMPLE_PWM_SLICE, true);

    while (audio_fault == AUDIO_OK) {
        service_one_control_event();
    }

    pwm_set_enabled(SAMPLE_PWM_SLICE, false);
    pwm_set_irq_enabled(SAMPLE_PWM_SLICE, false);
    dma_channel_set_irq0_enabled((uint)dma_channel, false);
    dma_channel_abort((uint)dma_channel);
    i2s_tx_stop();
    report_audio_fault(audio_fault);
    return 1;
}
