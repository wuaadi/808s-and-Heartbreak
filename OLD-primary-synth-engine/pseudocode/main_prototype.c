/*
 * Design prototype, not part of the PlatformIO build.
 *
 * The platform and synth functions declared below still need implementations.
 * The proposed I2S format is 24 kHz, stereo, 16-bit slots, with the same
 * synthesized sample in both channels. Confirm this format with the receiver.
 *
 * DMA completion starts the next block in the IRQ. The I2S FIFO must cover
 * the restart latency; measure this on hardware. If it cannot, use a
 * hardware-paced handoff with equivalent buffer-ownership and underrun checks.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SAMPLE_RATE_HZ 24000u
#define AUDIO_BLOCK_FRAMES 64u
#define AUDIO_CHANNELS 2u
#define ENVELOPE_RATE_HZ 1000u
#define ENVELOPE_PERIOD_SAMPLES (SAMPLE_RATE_HZ / ENVELOPE_RATE_HZ)

typedef enum {
    BUFFER_FREE,
    BUFFER_FILLING,
    BUFFER_READY,
    BUFFER_PLAYING
} BufferState;

typedef enum {
    AUDIO_OK,
    AUDIO_SYNTH_INIT_ERROR,
    AUDIO_PLATFORM_INIT_ERROR,
    AUDIO_UNDERRUN,
    AUDIO_DMA_ERROR
} AudioFault;

typedef uint32_t IrqState;

/*
 * Platform contracts:
 * - audio_platform_init configures I2S, DMA and audio_dma_irq_handler, but
 *   does not start DMA. It must configure the stated frame rate and format.
 * - audio_dma_start sends frame_count stereo frames from the given buffer.
 *   DMA must not access the buffer after its completion interrupt.
 * - audio_dma_acknowledge clears the completion IRQ and reports transfer errors.
 * - audio_stop stops output and prevents further DMA access to the buffers.
 * - irq_save_and_disable / irq_restore protect state shared with the DMA ISR.
 * - report_audio_fault reports the error and must not silently restart output.
 */
extern bool audio_platform_init(void);
extern bool audio_dma_start(const int16_t *samples, size_t frame_count);
extern bool audio_dma_acknowledge(void);
extern void audio_stop(void);
extern IrqState irq_save_and_disable(void);
extern void irq_restore(IrqState state);
extern void report_audio_fault(AudioFault fault);

/*
 * Synth contracts:
 * - synth_init initializes voices, tables and MIDI input using SAMPLE_RATE_HZ.
 * - render_sample advances and mixes all voices once, returning signed PCM.
 * - update_envelopes advances the ADSR state once at ENVELOPE_RATE_HZ.
 * - service_one_control_event does bounded work and returns; MIDI IRQs only
 *   enqueue events, so the renderer and allocator both own voices in main.
 *   Synchronize access to the MIDI queue and other ISR-produced controls.
 */
extern bool synth_init(uint32_t sample_rate_hz);
extern int16_t render_sample(void);
extern void update_envelopes(void);
extern void service_one_control_event(void);

static int16_t audio_buffers[2][AUDIO_BLOCK_FRAMES][AUDIO_CHANNELS];
static volatile BufferState buffer_states[2] = { BUFFER_FREE, BUFFER_FREE };
static volatile unsigned int playing_buffer;
static volatile AudioFault audio_fault = AUDIO_OK;
static unsigned int envelope_samples;

static void fill_audio_buffer(unsigned int buffer)
{
    for (size_t frame = 0; frame < AUDIO_BLOCK_FRAMES; ++frame) {
        if (envelope_samples == 0u) {
            update_envelopes();
        }

        int16_t sample = render_sample();
        audio_buffers[buffer][frame][0] = sample;
        audio_buffers[buffer][frame][1] = sample;

        ++envelope_samples;
        if (envelope_samples == ENVELOPE_PERIOD_SAMPLES) {
            envelope_samples = 0u;
        }
    }
}

/* Install this as the DMA completion handler before starting playback. */
void audio_dma_irq_handler(void)
{
    if (!audio_dma_acknowledge()) {
        audio_fault = AUDIO_DMA_ERROR;
        audio_stop();
        return;
    }

    unsigned int completed = playing_buffer;
    unsigned int next = completed ^ 1u;
    buffer_states[completed] = BUFFER_FREE;

    if (buffer_states[next] != BUFFER_READY) {
        audio_fault = AUDIO_UNDERRUN;
        audio_stop();
        return;
    }

    buffer_states[next] = BUFFER_PLAYING;
    playing_buffer = next;
    if (!audio_dma_start(&audio_buffers[next][0][0], AUDIO_BLOCK_FRAMES)) {
        audio_fault = AUDIO_DMA_ERROR;
        audio_stop();
    }
}

int main(void)
{
    if (!synth_init(SAMPLE_RATE_HZ)) {
        report_audio_fault(AUDIO_SYNTH_INIT_ERROR);
        return 1;
    }
    if (!audio_platform_init()) {
        report_audio_fault(AUDIO_PLATFORM_INIT_ERROR);
        return 1;
    }

    /* Prime both blocks before enabling playback and its completion IRQ. */
    fill_audio_buffer(0u);
    fill_audio_buffer(1u);
    buffer_states[0] = BUFFER_PLAYING;
    buffer_states[1] = BUFFER_READY;
    playing_buffer = 0u;

    if (!audio_dma_start(&audio_buffers[0][0][0], AUDIO_BLOCK_FRAMES)) {
        audio_stop();
        report_audio_fault(AUDIO_DMA_ERROR);
        return 1;
    }

    for (;;) {
        IrqState irq_state = irq_save_and_disable();
        AudioFault fault = audio_fault;
        int free_buffer = -1;
        if (fault == AUDIO_OK) {
            for (unsigned int i = 0; i < 2u; ++i) {
                if (buffer_states[i] == BUFFER_FREE) {
                    buffer_states[i] = BUFFER_FILLING;
                    free_buffer = (int)i;
                    break;
                }
            }
        }
        irq_restore(irq_state);

        if (fault != AUDIO_OK) {
            report_audio_fault(fault);
            return 1;
        }

        if (free_buffer >= 0) {
            fill_audio_buffer((unsigned int)free_buffer);
            irq_state = irq_save_and_disable();
            if (audio_fault == AUDIO_OK) {
                buffer_states[free_buffer] = BUFFER_READY;
            }
            irq_restore(irq_state);
        } else {
            service_one_control_event();
        }
    }
}
