#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64

void waveform_to_pixels(uint16_t *samples, size_t count, uint8_t y[DISPLAY_WIDTH]);
void display_clear(void);
void display_set_pixel(int x, int y);
void display_draw_line(int x0, int y0, int x1, int y1);
void display_draw_waveform(uint16_t *samples, size_t count);

#endif