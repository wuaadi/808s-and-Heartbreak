#include "voices.h"
#include "osc.h"

uint16_t g_audio; //final audio waveform to be sent to DMA

Voice_t g_voices[NUM_VOICES];
void ISR_oscillator (void) {
  // This function is called by the timer interrupt to generate audio samples
  // It will iterate through all active voices and sum their outputs
  int32_t mix = 0;
  for (int i=0; i<NUM_VOICES; i++) {
    Voice_t *v = &g_voices[i];
    //increment phase
    v->phase_acc += v->phase_inc;
    int32_t s = (int32_t)v->osc_table[v->phase_acc >> 24] - 32768; //subtract dc offset; choose only top 8 index bits from phase
    mix += (s * (int32_t)v->env_level) >> 16;
    //sum outputs to be sent to I2S
    
  }
  g_audio = (uint16_t)((mix >> 4) + 32768);  // scale for 16 voices, restore DC offset
}

void voice_init(Voice_t *voice) {
  if (voice == NULL) return;

  voice->osc_table = NULL; //change to sinewave once get osc.h header setup
  voice->phase_acc = 0;
  voice->phase_inc = 0;
  voice->velocity = 0;

  voice->env_table = &env_a_table;
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
void voices_init_all(Voice_t *voices, size_t count) {
  for (int i = 0; i < NUM_VOICES; i++) {
    voice_init(&voices[i]);
  }
}

void ISR_envelope(void) {
  //envelope progression
  for (int i=0; i<NUM_VOICES; i++) {
    Voice_t *voice;
    voice = &g_voices[i];
    //state machine transitions
    if (voice->env_idx >= ENV_WAVETABLE_SIZE) {
      voice->env_idx = 0;
      voice->env_state = (voice->env_state+1);
      if (voice->env_state > 4)
        voice->env_state = ENV_STATE_OFF;
    }
    //special cases handled by voice allocation
    //state machine outputs
    switch (voice->env_state) {
      case ENV_STATE_OFF:
        voice->env_table = zero_table;
        voice->env_idx = 0;
        break;
      case ENV_STATE_ATTACK:
        voice->env_table = env_a_table;
        voice->env_idx += voice->a_inc;
        break;
      case ENV_STATE_DECAY:
        voice->env_table = env_d_table;
        voice->env_idx += voice->d_inc;
        break;
      case ENV_STATE_SUSTAIN:
        voice->s_level = env_d_table[ENV_WAVETABLE_SIZE-1];
        voice->env_table = env_d_table;
        break;
      case ENV_STATE_RELEASE:
        voice->env_table = &env_r_table;
        voice->env_idx += voice->r_inc;
        break;
    }
    //amplitude scaling
    voice->amp = voice->osc_table[voice->phase_acc >> 24];
    voice->amp *= voice->env_table[voice->env_idx];
    voice->amp >>= 16;
    voice->amp *= velocity / 127.0f;
    if (voice->env_state==ENV_STATE_OFF) voice->amp = 0;
  }
}