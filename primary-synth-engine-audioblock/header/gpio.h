#ifndef GPIO_H
#define GPIO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


// ---- Top Side: LCD Display (SPI0) -----------------------------------
#define MCU1_LCD_SPI            spi0
#define MCU1_LCD_SCK_PIN        18  // SPI0 SCK
#define MCU1_LCD_MOSI_PIN       19  // SPI0 TX
#define MCU1_LCD_CS_PIN         17  // Chip Select
#define MCU1_LCD_MISO_PIN       16  // SPI0 RX
#define MCU1_LCD_RST_PIN        15  // Reset

// ---- Bottom Side: Envelope Controls (ADC) ---------------------------
#define MCU1_WAVETABLE_BTN_PIN  39  // Wavetable cycle button

#define MCU1_ATTACK_POT_PIN     40  // ADC0
#define MCU1_ATTACK_POT_ADC     0
#define MCU1_DECAY_POT_PIN      41  // ADC1
#define MCU1_DECAY_POT_ADC      1
#define MCU1_SUSTAIN_POT_PIN    42  // ADC2
#define MCU1_SUSTAIN_POT_ADC    2
#define MCU1_RELEASE_POT_PIN    43  // ADC3
#define MCU1_RELEASE_POT_ADC    3

// ---- Left Side: MIDI Input (UART0) ----------------------------------
#define MCU1_MIDI_UART          uart1
#define MCU1_MIDI_RX_PIN        5   // From optocoupler output
#define MCU1_MIDI_BAUD          31250

// ---- Right Side: Inter-MCU I2S Output (PIO) -------------------------
#define MCU1_I2S_BCLK_PIN       29  // Bit clock
#define MCU1_I2S_LRCLK_PIN      30  // Frame clock
#define MCU1_I2S_DOUT_PIN       31  // Data out

#endif // MIDI_H