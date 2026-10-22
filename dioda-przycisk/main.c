#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define LED PD5
#define LED_DDR DDRD
#define LED_PORT PORTD

#define BTN PD7
#define BTN_PIN PIND
#define BTN_PORT PORTD

#define DEBOUNCE_TIME 1000

// Source: "Make: AVR Programming"
uint8_t debounce(void) {
    if (bit_is_clear(BTN_PIN, BTN)) { /* button is pressed now */
        _delay_us(DEBOUNCE_TIME);
        if (bit_is_clear(BTN_PIN, BTN)) { /* still pressed */
            return (1);
        }
    }
    return (0);
}

int main() {
    uint8_t button_was_pressed = 0;
    BTN_PORT |= _BV(BTN);
    LED_DDR |= _BV(LED);
    while (1) {
        if (debounce()) {
            if (button_was_pressed == 0) {
                LED_PORT |= (1 << LED);
                button_was_pressed = 1;
            }
        } else {
            LED_PORT &= ~(1 << LED);
            button_was_pressed = 0;
        }
    }
}
