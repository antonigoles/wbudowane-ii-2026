#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h> 

// Tablica cyklu sinusa (przeskalowana)
static const uint8_t PROGMEM breath_sine[128] = {
      0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,   9,
     10,  11,  13,  14,  16,  17,  19,  21,  22,  24,  26,  28,  30,  32,  35,  37,
     39,  42,  44,  47,  49,  52,  55,  57,  60,  63,  66,  69,  72,  75,  78,  81,
     84,  87,  90,  94,  97, 100, 103, 107, 110, 113, 117, 120, 123, 127, 130, 133,
    137, 140, 143, 146, 150, 153, 156, 159, 163, 166, 169, 172, 175, 178, 181, 184,
    187, 190, 193, 196, 199, 201, 204, 207, 209, 212, 214, 217, 219, 221, 224, 226,
    228, 230, 232, 234, 236, 237, 239, 241, 242, 244, 245, 246, 247, 249, 250, 251,
    251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

// Tablica jasności z poprawioną gammą
static const uint8_t PROGMEM gamma_lut[256] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
      2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,
      6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,
     11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,
     19,  19,  20,  21,  21,  22,  22,  23,  23,  24,  25,  25,  26,  27,  27,  28,
     29,  29,  30,  31,  31,  32,  33,  34,  34,  35,  36,  37,  37,  38,  39,  40,
     40,  41,  42,  43,  44,  45,  46,  46,  47,  48,  49,  50,  51,  52,  53,  54,
     55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
     71,  72,  73,  74,  76,  77,  78,  79,  80,  81,  83,  84,  85,  86,  88,  89,
     90,  91,  93,  94,  95,  96,  98,  99, 100, 102, 103, 104, 106, 107, 109, 110,
    111, 113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 128, 129, 131, 132, 134,
    135, 137, 138, 140, 142, 143, 145, 146, 148, 150, 151, 153, 155, 157, 158, 160,
    162, 163, 165, 167, 169, 170, 172, 174, 176, 178, 179, 181, 183, 185, 187, 189,
    191, 193, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220,
    222, 224, 226, 229, 231, 233, 235, 237, 239, 241, 244, 246, 248, 250, 253, 255
};

static void pwm_init(void) {
    DDRB |= (1 << DDB1) | (1 << DDB2) | (1 << DDB3);

    // fast pwm, inverting, preskaler tak zeby ok 976hz bylo 
    TCCR1A = (1 << COM1A1) | (1 << COM1A0) | (1 << COM1B1) | (1 << COM1B0) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

    TCCR2A = (1 << COM2A1) | (1 << COM2A0) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS22);

    OCR1A = 0;
    OCR1B = 0;
    OCR2A = 0;
}

static void adc_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    DIDR0 |= (1 << ADC0D);
}

static uint16_t adc_read(void) {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

static void set_rgb_perceptual(uint8_t r, uint8_t g, uint8_t b) {
    // Pobranie rzeczywistego wypelnienia PWM po korekcji gamma
    uint8_t pwm_r = pgm_read_byte(&gamma_lut[r]);
    uint8_t pwm_g = pgm_read_byte(&gamma_lut[g]);
    uint8_t pwm_b = pgm_read_byte(&gamma_lut[b]);

    OCR1A = pwm_r;
    OCR1B = pwm_b;
    OCR2A = pwm_g;
}

// source: https://stackoverflow.com/questions/50974884/avr-atmega128-random-number-generator-from-range
void init_random() {
    uint16_t u_rand_val = 0;
    uint16_t u_seed_rand_val = 0;
    adc_init();
    for(uint8_t i=0; i<16; i++){
        u_seed_rand_val = u_seed_rand_val<<1 | (adc_read()&0b1);
    }
    srand(u_seed_rand_val);
}   

// daje kolor z H={hue}, S=255 i V=255 (jakas magia)
static void hue_to_rgb(uint16_t hue, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t sector = hue / 255;
    uint8_t offset = hue % 255;

    switch (sector) {
        case 0:  *r = 255;          *g = offset;       *b = 0;            break; // R -> Y
        case 1:  *r = 255 - offset; *g = 255;          *b = 0;            break; // Y -> G
        case 2:  *r = 0;            *g = 255;          *b = offset;       break; // G -> C
        case 3:  *r = 0;            *g = 255 - offset; *b = 255;          break; // C -> B
        case 4:  *r = offset;       *g = 0;            *b = 255;          break; // B -> M
        default: *r = 255;          *g = 0;            *b = 255 - offset; break; // M -> R
    }
}

int main(void) {
    pwm_init();
    init_random();

    uint8_t random_r = 0;
    uint8_t random_g = 0;
    uint8_t random_b = 0;
    hue_to_rgb((uint16_t)rand() % 1530, &random_r, &random_g, &random_b);
    set_rgb_perceptual(random_r, random_g, random_b);
    while (1) {

        // w jednom strone
        for (uint8_t i = 0; i < 128; i++) {
            uint8_t breath = pgm_read_byte(&breath_sine[i]);

            uint8_t r = (uint8_t)(((uint16_t)random_r * breath) / 255);
            uint8_t g = (uint8_t)(((uint16_t)random_g * breath) / 255);
            uint8_t b = (uint8_t)(((uint16_t)random_b * breath) / 255);

            set_rgb_perceptual(r, g, b);
            _delay_ms(12);
        }

        // i drugom
        for (int16_t i = 127; i >= 0; i--) {
            uint8_t breath = pgm_read_byte(&breath_sine[i]);

            uint8_t r = (uint8_t)(((uint16_t)random_r * breath) / 255);
            uint8_t g = (uint8_t)(((uint16_t)random_g * breath) / 255);
            uint8_t b = (uint8_t)(((uint16_t)random_b * breath) / 255);

            set_rgb_perceptual(r, g, b);
            _delay_ms(12);
        }
    }

    return 0;
}