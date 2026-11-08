#include "voices.h"
#include "osc.h"

uint16_t g_audio; //final audio waveform to be sent to DMA

Voice_t g_voices[NUM_VOICES-1:0];
void ISR_oscillator (void) {
  // This function is called by the timer interrupt to generate audio samples
  // It will iterate through all active voices and sum their outputs
  g_audio = 0;
  for (int i=0; i<NUM_VOICES; i++) {
    //increment phase
    g_voices[i].phase_acc += phase_inc;
    //envelope ISR handles scaling amplitude
    //sum outputs to be sent to I2S
    g_audio += g_voices[i].amp
  }

}

void voice_init(Voice_t *voice) {
  if (voice == NULL) return;

  voice->osc_table = NULL; //change to sinewave once get osc.h header setup
  voice->phase_acc = 0;
  voice->phase_inc = 0;
  voice->velocity = 0;

  voice->env_table = NULL;
  voice->env_idx = 0;
  voice->a_inc = 0;
  voice->d_inc = 0;
  voice->r_inc = 0;
  voice->s_level = 0;
  voice->env_state = ENV_STATE_OFF;

  voice->env_amp_latch = 0.0f;
  voice->amp = 0.0f;

  voice->age = 0;
  voice->pressed = false;
}

// Initializes every voice in the global array
void voices_init_all(Voice_t *g_voices, size_t count) {
  for (int i = 0; i < NUM_VOICES; i++) {
    voice_init(&g_voices[i]);
  }
}

void ISR_envelope(void) {
  //envelope progression
  for (int i=0; i<NUM_VOICES; i++) {
    
  }
}