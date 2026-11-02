#include "display.h"

static uint8_t framebuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

void waveform_to_pixels(uint16_t *samples, size_t count, uint8_t y[DISPLAY_WIDTH])
{
    if (samples == NULL || y == NULL || count == 0)
    {
        return;
    }

    for (size_t i = 0; i < DISPLAY_WIDTH; i++)
    {
        size_t index = i * (count - 1) / (DISPLAY_WIDTH - 1);

        uint32_t height = (uint32_t) samples[index] * (DISPLAY_HEIGHT - 1) / UINT16_MAX;

        y[i] = (DISPLAY_HEIGHT - 1) - height;
    }
}

void display_clear(void)
{
    for (int x = 0; x < DISPLAY_WIDTH; x++)
    {
        for (int y = 0; y < DISPLAY_HEIGHT; y++)
        {
            framebuffer[y][x] = 0;
        }
    }
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

void display_draw_waveform(uint16_t *samples, size_t count)
{
    if (samples == NULL || count == 0)
    {
        return;
    }

    uint8_t y[DISPLAY_WIDTH];
    waveform_to_pixels(samples, count, y);

    display_clear();

    for (int x = 0; x < DISPLAY_WIDTH; x++)
    {
        display_set_pixel(x, y[x]);
    }
}