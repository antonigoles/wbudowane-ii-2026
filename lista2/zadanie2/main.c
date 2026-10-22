#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <stdio.h>

#define BAUD 9600
#define UBRR_VALUE ((F_CPU)/16/(BAUD)-1)

void uart_init()
{
  UBRR0 = UBRR_VALUE;
  UCSR0A = 0;
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

int uart_transmit(char data, FILE *stream)
{
  while(!(UCSR0A & _BV(UDRE0)));
  UDR0 = data;
  return 0;
}

int uart_receive(FILE *stream)
{
  while (!(UCSR0A & _BV(RXC0)));
  return UDR0;
}

FILE uart_file;

#define LED PC2
#define LED_DDR DDRC
#define LED_PORT PORTC

#define BTN PC5
#define BTN_PIN PINC
#define BTN_PORT PORTC

#define DEBOUNCE_TIME 10

#define LONG_SIGNAL_THRESHOLD 500 // in ms
#define NEXT_SIGNAL_THRESHOLD 500 // in ms
#define NEXT_CHARACTER_THRESHOLD 1000 // in ms
#define NEXT_WORD_THRESHOLD 4000 // in ms

uint32_t TIMER = 0; // time passed in us

// 0 - failed
// 1 - success
// 2 - waiting
uint8_t debounce(void) {
    static uint32_t debounce_start = 0;
    static uint8_t is_filtering = 0;

    if (bit_is_clear(BTN_PIN, BTN)) {
        if (!is_filtering) {
            debounce_start = TIMER;
            is_filtering = 1;
        }
        
        if (TIMER - debounce_start < DEBOUNCE_TIME) {
            return 2;
        }
        return 1;
    }

    is_filtering = 0;
    return 0;
}

uint8_t MORSE_TABLE[36][7] = {
    // Litery A-Z
    {'A', 1, 2, 0},
    {'B', 2, 1, 1, 1, 0},
    {'C', 2, 1, 2, 1, 0},
    {'D', 2, 1, 1, 0},
    {'E', 1, 0},
    {'F', 1, 1, 2, 1, 0},
    {'G', 2, 2, 1, 0},
    {'H', 1, 1, 1, 1, 0},
    {'I', 1, 1, 0},
    {'J', 1, 2, 2, 2, 0},
    {'K', 2, 1, 2, 0},
    {'L', 1, 2, 1, 1, 0},
    {'M', 2, 2, 0},
    {'N', 2, 1, 0},
    {'O', 2, 2, 2, 0},
    {'P', 1, 2, 2, 1, 0},
    {'Q', 2, 2, 1, 2, 0},
    {'R', 1, 2, 1, 0},
    {'S', 1, 1, 1, 0},
    {'T', 2, 0},
    {'U', 1, 1, 2, 0},
    {'V', 1, 1, 1, 2, 0},
    {'W', 1, 2, 2, 0},
    {'X', 2, 1, 1, 2, 0},
    {'Y', 2, 1, 2, 2, 0},
    {'Z', 2, 2, 1, 1, 0},

    // Cyfry 0-9
    {'0', 2, 2, 2, 2, 2, 0},
    {'1', 1, 2, 2, 2, 2, 0},
    {'2', 1, 1, 2, 2, 2, 0},
    {'3', 1, 1, 1, 2, 2, 0},
    {'4', 1, 1, 1, 1, 2, 0},
    {'5', 1, 1, 1, 1, 1, 0},
    {'6', 2, 1, 1, 1, 1, 0},
    {'7', 2, 2, 1, 1, 1, 0},
    {'8', 2, 2, 2, 1, 1, 0},
    {'9', 2, 2, 2, 2, 1, 0}
};

uint8_t compare_chars(uint8_t *character, uint8_t *morse) {
    uint8_t failed = 0;
    uint8_t ptr = 0;
    while (morse[ptr] != 0) {
        if (character[ptr] != morse[ptr+1]) {
            return 0;
        }
        ptr++;
    }
    return 1;
}

void emit_character(uint8_t *current_character) {
    // 1. find matching
    for (int i = 0; i<36; i++) {
        int ptr = 0;
        if (compare_chars(current_character, MORSE_TABLE[i]) == 1) {
            printf("%c", MORSE_TABLE[i][0]);
            return;
        }
    }

    printf("?");
}

int main() {
    uint8_t button_was_pressed = 0;
    BTN_PORT |= _BV(BTN);
    LED_DDR |= _BV(LED);

    LED_PORT &= ~(1 << LED);

    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    uint32_t last_signal_timestamp = 0;
    uint8_t current_character_index = 0;
    uint8_t current_character[6] = {0,0,0,0,0,0};
    uint8_t pending_char = 0;
    uint8_t pending_space = 0;
    printf("UART ready\n");

    while (1) {
        // simple timer
        TIMER += 1UL;
        _delay_ms(1UL);

        uint8_t debounce_result = debounce();
        if (debounce_result == 1) {
            if (button_was_pressed == 0) {
                last_signal_timestamp = TIMER;
                button_was_pressed = 1;
            }

            uint32_t delta = TIMER - last_signal_timestamp;
            if (delta >= LONG_SIGNAL_THRESHOLD) {
                LED_PORT |= _BV(LED);
            }

        } else if (debounce_result == 0) {
            if (button_was_pressed == 1) {
                // button was released -> we should check signal length
                uint32_t delta = TIMER - last_signal_timestamp;

                if (delta >= LONG_SIGNAL_THRESHOLD) {
                    current_character[current_character_index] = 2; // long signal
                    // inform with LED
                } else {
                    current_character[current_character_index] = 1; // short signal
                }
                current_character_index = (current_character_index + 1) % 6;

                last_signal_timestamp = TIMER;

                pending_char = 1;
                pending_space = 1;

                LED_PORT &= ~_BV(LED);
            }
            button_was_pressed = 0;
        }

        // check if we should submit
        uint32_t delta = TIMER - last_signal_timestamp;
        if (delta >= NEXT_WORD_THRESHOLD && pending_space == 1) {
            // emit space
            printf(" ");
            pending_space = 0;
        } else if (delta >= NEXT_CHARACTER_THRESHOLD && pending_char == 1) {
            // emit the character and reset
            emit_character(current_character);
            pending_char = 0;

            current_character_index = 0;
            current_character[0] = 0;
            current_character[1] = 0;
            current_character[2] = 0;
            current_character[3] = 0;
            current_character[4] = 0;
            current_character[5] = 0;
        }
    }
}
