#include <stdio.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "lcd.h"

static void init_output(uint pin, bool level)
{
    gpio_init(pin);
    gpio_put(pin, level);
    gpio_set_dir(pin, GPIO_OUT);
}

int main(void)
{
    stdio_init_all();

    init_output(LCD_PIN_CS, true);
    init_output(LCD_PIN_DC, false);
    init_output(LCD_PIN_RESET, true);

    uint spi_hz = spi_init(LCD_SPI, 2 * 1000 * 1000);
    spi_set_format(LCD_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(LCD_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_PIN_MOSI, GPIO_FUNC_SPI);

    sleep_ms(100);
    LCD_Setup();
    printf("TFT: %u x %u, SPI0: %u Hz\n",
           (unsigned)lcddev.width, (unsigned)lcddev.height, spi_hz);

    if (lcddev.width == DISPLAY_WIDTH && lcddev.height == DISPLAY_HEIGHT) {
#ifdef TFT_RAMP_TEST
        int16_t samples[DISPLAY_WIDTH];
        for (int32_t x = 0; x < DISPLAY_WIDTH; x++) {
            samples[x] = INT16_MIN + x * UINT16_MAX / (DISPLAY_WIDTH - 1);
        }
        display_draw_waveform(samples, DISPLAY_WIDTH);
        LCD_FlushFramebuffer(display_get_framebuffer());
#else
        const u16 right = lcddev.width - 1;
        const u16 bottom = lcddev.height - 1;
        const u16 mark = 11;

        LCD_Clear(DARKBLUE);
        LCD_DrawLine(0, 0, right, bottom, WHITE);
        LCD_DrawFillRectangle(0, 0, mark, mark, RED);
        LCD_DrawFillRectangle(right - mark, 0, right, mark, GREEN);
        LCD_DrawFillRectangle(0, bottom - mark, mark, bottom, CYAN);
        LCD_DrawFillRectangle(right - mark, bottom - mark, right, bottom, YELLOW);
#endif
    }

    for (;;) {
        tight_loop_contents();
    }
}
