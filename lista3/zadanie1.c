#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define BUZZER PD5
#define BUZZER_DDR DDRD
#define BUZZER_PORT PORTD

uint32_t TIMER = 0; // time passed in us


// wszystko będzie w pseudo szóstej oktawie

// static const uint16_t NOTE_HALF_PERIOD[] = {
//     //    A,   B,   C,   D,   E,   F,   G,   H (pause)
//     284, 253, 478, 426, 379, 358, 319,   0
// };

static const uint16_t NOTE_HALF_PERIOD[] = {
    //    A,    B,    C,    D,    E,    F,    G,   H (pauza)
    1136, 1012, 1911, 1703, 1517, 1432, 1276,    0
};

#define BPM 40
#define NOTE_LENGTH ((uint32_t)60 * 1000000)/BPM // 240 BPM is makes 1 sec per note

void delay_us_var(uint32_t us) {
    while (us--) {
        _delay_us(1);
    }
}

void play_note_for(char note, uint32_t length, uint32_t padding) {
    // uint32_t tick = NOTE_TO_TICK[note-'A'];
    // if (tick == 0) {
    //     delay_us_var(length);
    //     return;
    // }
    // uint32_t tick_count = (uint32_t)length / tick;
    // for (uint32_t i = 0; i < tick_count; i++) {
    //     BUZZER_PORT ^= _BV(BUZZER);
    //     delay_us_var(tick);
    // }

    uint8_t idx = (note >= 'A' && note <= 'H') ? (note - 'A') : 7;
    uint16_t half_period = NOTE_HALF_PERIOD[idx];
    
    if (half_period > 0) {
        uint32_t cycles = length / (half_period * 2);
        
        for (uint32_t i = 0; i < cycles; i++) {
            BUZZER_PORT ^= _BV(BUZZER);
            delay_us_var(half_period);
            BUZZER_PORT ^= _BV(BUZZER);
            delay_us_var(half_period);
        }
    } else {
        delay_us_var(length);
    }
    delay_us_var(padding);
}

#define STD_PAUSE 4
#define NOTE_PADDING 10000

static const uint8_t SONG[][2] = {
    {'E', 4}, {'E', 4}, {'E', 2},
    {'E', 4}, {'E', 4}, {'E', 2},
    {'E', 4}, {'G', 4}, {'C', 4}, {'D', 4},
    {'E', 1},

    {'F', 4}, {'F', 4}, {'F', 4}, {'F', 4},
    {'F', 4}, {'E', 4}, {'E', 4}, {'E', 4},
    {'E', 4}, {'D', 4}, {'D', 4}, {'E', 4},
    {'D', 2}, {'G', 2},

    {'E', 4}, {'E', 4}, {'E', 2},
    {'E', 4}, {'E', 4}, {'E', 2},
    {'E', 4}, {'G', 4}, {'C', 4}, {'D', 4},
    {'E', 1},

    {'F', 4}, {'F', 4}, {'F', 4}, {'F', 4},
    {'F', 4}, {'E', 4}, {'E', 4}, {'E', 4},
    {'E', 4}, {'G', 4}, {'G', 4}, {'F', 4},
    {'D', 2}, {'C', 1}
};

int main() {
    BUZZER_DDR |= _BV(BUZZER);
    for (int i = 0; i<sizeof(SONG) / 2; i++) {
        play_note_for(SONG[i][0], NOTE_LENGTH/SONG[i][1], NOTE_PADDING);
    }
}
