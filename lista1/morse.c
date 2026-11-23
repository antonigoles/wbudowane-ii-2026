#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUD 9600                          // baudrate
#define UBRR_VALUE ((F_CPU)/16/(BAUD)-1)   // zgodnie ze wzorem

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
char text[512];

#define UNIT_TIME 150 // in MS
#define DOT_LENGTH 1 // # of units
#define DASH_LENGTH 3 // # of units
#define SPACE_BETWEEN_WORDS 7 // # of units
#define SPACE_BETWEEN_LETTERS 3 // # of units
#define SPACE_BETWEEN_DOTS_AND_DASHES 1 // # of units

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB


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

#define blink_for_ms(ms) \
    LED_PORT |= _BV(LED); \
    _delay_ms(ms); \
    LED_PORT &= ~_BV(LED); \


char emit_char(char c)
{
    // ignore non alphanumeric characters / emit space
    if (((c < '0' || c > '9') && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) || (c == ' ')) {
        // emit space
        _delay_ms(UNIT_TIME * SPACE_BETWEEN_DOTS_AND_DASHES);
        return ' ';
    }

    char normalized = c >= 'a' ? c - ('a' - 'A') : c;

    int8_t morse_index = 0;
    for (int8_t i = 0; i<36; i++) {
        if (MORSE_TABLE[i][0] == normalized) {
            morse_index = i;
            break;
        }
    }

    int j = 1;
    while (MORSE_TABLE[morse_index][j] != 0) {
        if (j != 1) {
            _delay_ms(SPACE_BETWEEN_DOTS_AND_DASHES * UNIT_TIME);
        }
        if (MORSE_TABLE[morse_index][j] == 1) {
            // short
            blink_for_ms(DOT_LENGTH * UNIT_TIME);
        } else {
            // long
            blink_for_ms(DASH_LENGTH * UNIT_TIME);
        }
        j++;
    }

    return normalized;
}

void scanline_uart(char *buffer, int limit) {
    int i = 0;
    char c;
    
    while(1) {
        c = getchar();
        if (c == '\r' || c == '\n') {
            buffer[i] = '\0';
            return;
        }
        if (i < limit - 1) {
            buffer[i] = c;
            i++;
        }
    }
}

int main()
{
    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;
    printf("Hello world!\r\n");
    LED_DDR |= _BV(LED);
    while(1) {
        scanline_uart(text, 512);
        printf("Emitowanie na morsa...\r\n");
        char *current = text;
        while (*current != '\0' && (text-current <= 511)) {
            if (current != text) {
                _delay_ms(SPACE_BETWEEN_LETTERS * UNIT_TIME);
            }
            char result = emit_char(*current);
            printf("%c", result);
            current++;
        }
        printf("\r\n");
        printf("Zakończono emisje morsa.\r\n");
    }
}

