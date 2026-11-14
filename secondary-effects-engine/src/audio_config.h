#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

// Audio config

// !!! confirm sample rate with Aadi
#define SAMPLE_RATE 24000
#define AUDIO_PWM_PIN 22
#define PWM_CARRIER_HZ 240000
#define TEST_FREQUENCY 440

// echo delay 100 ms
#define DELAY_SAMPLES (SAMPLE_RATE / 10)

// I2S pins from PCB schematic
#define I2S_BCLK_PIN 29
#define I2S_LRCLK_PIN 30
#define I2S_DATA_PIN 31

// Effects controls from PCB schematic
#define FX_BUTTON_PIN 39

#define HPF_POT_PIN 40
#define LPF_POT_PIN 41
#define VIB_POT_PIN 42
#define REV_POT_PIN 43
#define DLY_POT_PIN 44

#endif