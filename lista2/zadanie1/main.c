#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define LED PC2
#define LED_DDR DDRC
#define LED_PORT PORTC

#define BTN PC5
#define BTN_PIN PINC
#define BTN_PORT PORTC

#define DEBOUNCE_TIME 10

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

struct Queue {
    uint32_t head_index; // head is always the first item
    uint32_t tail_index; // tail points to the next free index
    uint32_t content[128];
};

void init_queue(struct Queue* q) {
    q->head_index = 0;
    q->tail_index = 0;
}

uint32_t queue_empty(struct Queue* q) {
    return q->tail_index == q->head_index;
}

uint32_t queue_front(struct Queue* q) {
    return q->content[q->head_index % 128];
}

// Warning: unsafe method
uint32_t queue_pop_front(struct Queue* q) {
    uint32_t val = q->content[q->head_index % 128];
    q->head_index++;
    return val;
}

void push_back(struct Queue* q, uint32_t value) {
    if ((q->tail_index - q->head_index) >= 128) return;
    q->content[q->tail_index % 128] = value;
    q->tail_index++;
}

int main() {
    uint8_t button_was_pressed = 0;
    BTN_PORT |= _BV(BTN);
    LED_DDR |= _BV(LED);

    LED_PORT &= ~(1 << LED);

    struct Queue q;
    init_queue(&q);

    while (1) {
        // simple timer
        TIMER += 1UL;
        _delay_ms(1UL);

        uint8_t debounce_result = debounce();
        if (debounce_result == 1) {
            if (button_was_pressed == 0) {
                push_back(&q, TIMER + 1000UL);
                button_was_pressed = 1;
            }
        } else if (debounce_result == 0) {
            if (button_was_pressed == 1) push_back(&q, TIMER + 1000UL);
            button_was_pressed = 0;
        }

        // clean up the queue
        while (queue_empty(&q) != 1) {
            uint32_t timestamp = queue_front(&q);
            if (timestamp >= TIMER) {
                break;
            }

            queue_pop_front(&q);
            LED_PORT ^= (1 << LED); // state switch
        }
    }
}
