#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define BAUD 9600
#define UBRR_VALUE ((F_CPU)/16/(BAUD)-1)

#define LED_DDR DDRD
#define LED_PORT PORTD

#define BTN_RESET PC5
#define BTN_NEXT PC3
#define BTN_PREV PC4
#define BTN_PIN PINC
#define BTN_PORT PORTC

#define DEBOUNCE_TIME 10

uint32_t TIMER = 0; // time passed in us

// 0 - failed
// 1 - success
// 2 - waiting
uint8_t debounce_RESET(void) {
    static uint32_t debounce_start = 0;
    static uint8_t is_filtering = 0;

    if (bit_is_clear(BTN_PIN, BTN_RESET)) {
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

uint8_t debounce_PREV(void) {
    static uint32_t debounce_start = 0;
    static uint8_t is_filtering = 0;

    if (bit_is_clear(BTN_PIN, BTN_PREV)) {
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

uint8_t debounce_NEXT(void) {
    static uint32_t debounce_start = 0;
    static uint8_t is_filtering = 0;

    if (bit_is_clear(BTN_PIN, BTN_NEXT)) {
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

static uint8_t gray_codes[8] = {};

struct StripState 
{
    uint8_t index;
};

// źródło: wikipedia
uint8_t BinaryToGray(uint8_t num)
{
    return num ^ (num >> 1);
}

void reset_strip(struct StripState* state) {
    state->index = 0;
}

void next_gray_code(struct StripState* state) {
    state->index++;
}

void prev_gray_code(struct StripState* state) {
    state->index--;
}

void display_strip_state(struct StripState* state) {
    LED_PORT = BinaryToGray(state->index);
}

int main() {
    uint8_t button_was_pressed[3] = {0,0,0};
    BTN_PORT |= _BV(BTN_RESET) | _BV(BTN_PREV) | _BV(BTN_NEXT);
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0); // Turn RX and TX to PD0 and PD1
    LED_DDR |= ~0;
    
    struct StripState strip_state;
    strip_state.index = 0;
    reset_strip(&strip_state);
    display_strip_state(&strip_state);

    while (1) {
        // simple timer
        TIMER += 1UL;
        _delay_ms(1UL);

        // HANDLE RESET
        {
            uint8_t debounce_result = debounce_RESET();
            if (debounce_result == 1) {
                if (button_was_pressed[0] == 0) {
                    button_was_pressed[0] = 1;
                    reset_strip(&strip_state);
                    display_strip_state(&strip_state);
                }
            } else if (debounce_result == 0) {
                button_was_pressed[0] = 0;
            }
        }
        

        // HANDLE PREV
        {
            uint8_t debounce_result = debounce_PREV();
            if (debounce_result == 1) {
                if (button_was_pressed[1] == 0) {
                    button_was_pressed[1] = 1;
                    prev_gray_code(&strip_state);
                    display_strip_state(&strip_state);
                }
            } else if (debounce_result == 0) {
                button_was_pressed[1] = 0;
            }
        }
        

        // HANDLE NEXT
        {
            uint8_t debounce_result = debounce_NEXT();
            if (debounce_result == 1) {
                if (button_was_pressed[2] == 0) {
                    button_was_pressed[2] = 1;
                    next_gray_code(&strip_state);
                    display_strip_state(&strip_state);
                }
            } else if (debounce_result == 0) {
                button_was_pressed[2] = 0;
            }
        }
        

    }
}
