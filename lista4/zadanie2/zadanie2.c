#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>

#define ADC_MIN 150L
#define ADC_MAX 850L

static const uint8_t PROGMEM gamma_lut[32] = {
    0,   1,   1,   2,   3,   4,   6,   8,
   11,  14,  18,  22,  27,  33,  40,  47,
   56,  65,  76,  87, 100, 114, 129, 145,
  163, 182, 202, 224, 247, 250, 253, 255
};

// Inicjalizacja sprzętowego PWM na Timer0 (wyjście OC0A / pin PD6 / Arduino D6)
static void pwm_init(void) {
    // Ustaw pin PD6 (OC0A) jako wyjście
    DDRD |= (1 << DDD6);

    // Tryb Fast PWM (Mode 3): WGM01 = 1, WGM00 = 1, TOP = 0xFF
    // Non-inverting mode na kanale A: COM0A1 = 1, COM0A0 = 0
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);

    // Preskaler = 64: f_PWM = 16 MHz / (64 * 256) ~= 976.5 Hz
    TCCR0B = (1 << CS01) | (1 << CS00);

    OCR0A = 0; // Początkowo compare register na zero
}

// Inicjalizacja przetwornika ADC
static void adc_init(void) {
    // VREF = AVcc z zewnętrznym kondensatorem na AREF (REFS0 = 1)
    // Wejście: ADC0 (pin A0 / PC0) domyślnie wybrane przez bity MUX3:0 = 0000
    ADMUX = (1 << REFS0);

    // Włączenie ADC (ADEN), preskaler 128: 16 MHz / 128 = 125 kHz (optymalne 50-200 kHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Wyłączenie bufora cyfrowego na pinie PC0 (zmniejsza szumy i pobór prądu)
    DIDR0 |= (1 << ADC0D);
}

static uint16_t adc_read(void) {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

int main(void) {
    pwm_init();
    adc_init();

    while (1) {
        long val = (long)adc_read();

        // Ograniczenie wartości do zdefiniowanego zakresu
        if (val < ADC_MIN) val = ADC_MIN;
        if (val > ADC_MAX) val = ADC_MAX;

        // Odwrotna zależność:
        // Ciemno (ADC_MIN) -> index = 31 (maksymalna jasność)
        // Jasno (ADC_MAX)  -> index = 0  (dioda wygaszona)
        uint8_t index = (uint8_t)(((ADC_MAX - val) * 31L) / (ADC_MAX - ADC_MIN));

        // Pobranie skorygowanego wypełnienia PWM z tablicy w pamięci Flash
        OCR0A = pgm_read_byte(&gamma_lut[index]);

    }

    return 0;
}