#include "display.h"

#include <stdlib.h>
#include <string.h>

static uint8_t framebuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

void waveform_to_pixels(const int16_t *samples, size_t count, uint8_t y[DISPLAY_WIDTH])
{
    if (samples == NULL || y == NULL || count == 0)
    {
        return;
    }

    const int32_t center = DISPLAY_HEIGHT / 2;
    const size_t intervals = DISPLAY_WIDTH - 1;
    const size_t step = (count - 1) / intervals;
    const size_t remainder = (count - 1) % intervals;

    for (size_t x = 0; x < DISPLAY_WIDTH; x++)
    {
        size_t index = x * step + x * remainder / intervals;
        int32_t sample = samples[index];
        int32_t offset = sample >= 0
            ? sample * center / INT16_MAX
            : sample * (DISPLAY_HEIGHT - 1 - center) / -(int32_t)INT16_MIN;

        y[x] = (uint8_t)(center - offset);
    }
}

const uint8_t (*display_get_framebuffer(void))[DISPLAY_WIDTH]
{
    return (const uint8_t (*)[DISPLAY_WIDTH])framebuffer;
}

void display_clear(void)
{
    memset(framebuffer, 0, sizeof framebuffer);
}

void display_set_pixel(int x, int y)
{
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT)
    {
        return;
    }
    framebuffer[y][x] = 1;
}

void display_draw_line(int x0, int y0, int x1, int y1)
{
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (1)
    {
        display_set_pixel(x0, y0);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int err2 = 2 * err;

        if (err2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (err2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void display_draw_waveform(const int16_t *samples, size_t count)
{
    if (samples == NULL || count == 0)
    {
        return;
    }

    uint8_t y[DISPLAY_WIDTH];
    waveform_to_pixels(samples, count, y);

    display_clear();

    for (int x = 0; x + 1 < DISPLAY_WIDTH; x++)
    {
        display_draw_line(x, y[x], x + 1, y[x + 1]);
    }
}
