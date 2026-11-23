#include <avr/io.h>
#include <util/delay.h>

#define LED_DDR DDRD
#define LED_PORT PORTD

uint8_t numbers[10] = {
 // 0b0gfedcba
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
};

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);
    LED_DDR |= ~0;
    uint8_t pattern = 0b00000111;
    int i = 0;
    while (1) {
        PORTD = ~numbers[i];
        i = (i + 1) % 10;
        _delay_ms(500);
    }
}


