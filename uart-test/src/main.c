#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/stdio_uart.h"

enum {
    DEBUG_TX_PIN = 0,
    DEBUG_RX_PIN = 1,
    DEBUG_BAUD = 115200,
    MIDI_TX_PIN = 24,
    MIDI_RX_PIN = 25,
    MIDI_BAUD = 31250,
    MIDI_MESSAGE_SIZE = 3,
    RX_TIMEOUT_US = 20000,
    TEST_DELAY_MS = 500
};

static const uint8_t NOTE_ON[MIDI_MESSAGE_SIZE] = {0x90, 0x3C, 0x64};
static const uint8_t NOTE_OFF[MIDI_MESSAGE_SIZE] = {0x80, 0x3C, 0x00};

static void test_message(const uint8_t message[MIDI_MESSAGE_SIZE])
{
    uint8_t received[MIDI_MESSAGE_SIZE];
    bool matches = true;

    // Discard bytes queued before this transmission, including startup noise.
    while (uart_is_readable(uart1)) {
        (void)uart_getc(uart1);
    }

    // Send the raw bytes on GP24; the jumper feeds them back into GP25.
    uart_write_blocking(uart1, message, MIDI_MESSAGE_SIZE);
    for (int i = 0; i < MIDI_MESSAGE_SIZE; ++i) {
        if (!uart_is_readable_within_us(uart1, RX_TIMEOUT_US)) {
            printf("FAIL: UART1 RX timed out after %d of %d bytes. Check GP24 -> GP25.\n",
                   i, MIDI_MESSAGE_SIZE);
            return;
        }
        received[i] = (uint8_t)uart_getc(uart1);
        if (received[i] != message[i]) {
            matches = false;
        }
    }

    // Print readable text on the separate Lab 7 debugger console (UART0).
    printf("%s | TX: %02X %02X %02X | RX: %02X %02X %02X\n",
           matches ? "PASS" : "FAIL",
           (unsigned)message[0], (unsigned)message[1], (unsigned)message[2],
           (unsigned)received[0], (unsigned)received[1], (unsigned)received[2]);
}

int main(void)
{
    stdio_uart_init_full(uart0, DEBUG_BAUD, DEBUG_TX_PIN, DEBUG_RX_PIN);

    uart_init(uart1, MIDI_BAUD);
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(uart1, false, false);
    gpio_pull_up(MIDI_RX_PIN); // Keep RX idle high if the jumper is absent.
    gpio_set_function(MIDI_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MIDI_RX_PIN, GPIO_FUNC_UART);
    // Let a frame detected during pin setup finish before draining the FIFO.
    sleep_us((10 * 1000000 + MIDI_BAUD - 1) / MIDI_BAUD);

    puts("MIDI UART1 loopback: GP24 TX -> GP25 RX, 31250 baud, 8N1");

    for (;;) {
        test_message(NOTE_ON);
        sleep_ms(TEST_DELAY_MS);
        test_message(NOTE_OFF);
        sleep_ms(TEST_DELAY_MS);
    }
}
