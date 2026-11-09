#include "voices.h"
#include "osc.h"

int16_t g_audio; //final audio waveform to be sent to DMA

Voice_t g_voices[NUM_VOICES];
void ISR_oscillator (void) {
  // This function is called by the timer interrupt to generate audio samples
  // It will iterate through all active voices and sum their outputs
  int32_t mix = 0;
  for (int i=0; i<NUM_VOICES; i++) {
    Voice_t *v = &g_voices[i];
    if (v->env_state == ENV_STATE_OFF)
      continue;
    //increment phase
    v->phase_acc += v->phase_inc;
    int32_t s = (int32_t)v->osc_table[v->phase_acc >> 24] - 32768; //subtract dc offset; choose only top 8 index bits from phase
    mix += (s * (int32_t)(v->env_level >> 1)) >> 15; //total right-shift = 16
    //sum outputs to be sent to I2S
    
  }
  if (mix > INT16_MAX) mix = INT16_MAX;
  if (mix < INT16_MIN) mix = INT16_MIN;
  g_audio = (int16_t)(mix >> 4); //final g_audio is signed for i2s compatibility
}

void voice_init(Voice_t *voice) {
  if (voice == NULL) return;

  voice->osc_table = zero_table; //change to sinewave once get osc.h header setup
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

  voice->env_amp_latch = 0;
  voice->env_level = 0;

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
  for (int i = 0; i < NUM_VOICES; i++) {
    Voice_t *v = &g_voices[i];
    uint32_t level = 0;
    //handles a->d->s->r transitions, voice allocator handles special cases

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
    }

    v->env_level = (level * v->velocity) >> 7;
    // >> 7 same as /128, scaling by max velocity value
  }
}

/*
void ISR_envelope_OLD(void) {
  //envelope progression
  for (int i=0; i<NUM_VOICES; i++) {
    Voice_t *voice;
    voice = &g_voices[i];
    
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
        voice->s_level = voice->s_level;
        voice->env_table = env_d_table;
        break;
      case ENV_STATE_RELEASE:
        voice->env_table = env_r_table;
        voice->env_idx += voice->r_inc;
        break;
    }

    //state machine transitions
    if (voice->env_idx >= ENV_WAVETABLE_SIZE) {
      voice->env_idx = 0;
      voice->env_state = (voice->env_state+1);
      if (voice->env_state > 4)
        voice->env_state = ENV_STATE_OFF;
    }

    //amplitude scaling
    voice->env_level = voice->env_table[voice->env_idx];
    voice->env_level *= voice->velocity;
    voice->env_level >>= 7; //equivalent to dividing by 128
  }
}
*/