#include <avr/io.h>
#include <util/delay.h>

#define LED_DDR DDRD
#define LED_PORT PORTD

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);
    LED_DDR |= ~0;
    uint8_t pattern = 0b00000111;
    while (1) {
        LED_PORT = pattern;
        pattern <<= 1;
        if (pattern == 0b11000000) pattern = 0b11000001;
        if (pattern == 0b10000010) pattern = 0b10000011;
        if (pattern == 0b00000110) pattern = 0b00000111;
        _delay_ms(166);
    }
}


