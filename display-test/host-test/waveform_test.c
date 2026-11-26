#include "display.h"

#include <stdio.h>
#include <stdlib.h>

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void write_image(const char *path)
{
    const uint8_t (*pixels)[DISPLAY_WIDTH] = display_get_framebuffer();
    FILE *file = fopen(path, "wb");
    check(file != NULL, path);
    check(fprintf(file, "P5\n%d %d\n255\n", DISPLAY_WIDTH, DISPLAY_HEIGHT) > 0, path);

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        uint8_t row[DISPLAY_WIDTH];
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            row[x] = pixels[y][x] ? 255 : 0;
        }
        check(fwrite(row, 1, sizeof row, file) == sizeof row, path);
    }
    check(fclose(file) == 0, path);
    printf("PASS: %s\n", path);
}

int main(void)
{
    const uint8_t (*pixels)[DISPLAY_WIDTH] = display_get_framebuffer();
    int16_t samples[DISPLAY_WIDTH] = {0};

    display_draw_waveform(samples, DISPLAY_WIDTH);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            check(pixels[y][x] == (y == DISPLAY_HEIGHT / 2), "flat center line");
        }
    }
    write_image("silence.pgm");

    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        samples[x] = INT16_MIN + (int32_t)x * UINT16_MAX / (DISPLAY_WIDTH - 1);
    }
    display_draw_waveform(samples, DISPLAY_WIDTH);
    int previous_y = DISPLAY_HEIGHT - 1;
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        int y = 0;
        while (y < DISPLAY_HEIGHT && !pixels[y][x]) {
            y++;
        }
        check(y <= previous_y && previous_y - y <= 1, "continuous rising ramp");
        previous_y = y;
    }
    check(pixels[DISPLAY_HEIGHT - 1][0] && pixels[0][DISPLAY_WIDTH - 1],
          "ramp reaches the bottom-left and top-right pixels");
    write_image("ramp.pgm");

    const int period_half = DISPLAY_WIDTH / 4;
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        samples[x] = (x / period_half) % 2 ? INT16_MAX : INT16_MIN;
    }
    display_draw_waveform(samples, DISPLAY_WIDTH);
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        int plateau = (x / period_half) % 2 ? 0 : DISPLAY_HEIGHT - 1;
        check(pixels[plateau][x], "square wave plateau reaches every column");
    }
    for (int x = period_half; x < DISPLAY_WIDTH; x += period_half) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            check(pixels[y][x - 1] || pixels[y][x], "square wave transition has no missing rows");
        }
    }
    write_image("square.pgm");

    return EXIT_SUCCESS;
}
