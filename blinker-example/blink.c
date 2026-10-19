#include <avr/io.h>
#include <util/delay.h>

// #define LED PB5
#define LED PD5
// #define LED_DDR DDRB
#define LED_DDR DDRD
// #define LED_PORT PORTB
#define LED_PORT PORTB


int main() {
  LED_DDR |= _BV(LED);
  while (1) {
    LED_PORT |= _BV(LED);
    _delay_ms(166);
    LED_PORT &= ~_BV(LED);
    _delay_ms(166);
  }
}


