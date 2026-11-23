#ifndef F_CPU
#define F_CPU 16000000UL // 16 MHz (standard dla Arduino Uno / Nano)
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define BUFFER_SIZE 100

volatile uint8_t history[BUFFER_SIZE]; // rozdzielczosc zapisu eventów to 10ms
volatile uint8_t idx = 0;

ISR(TIMER1_COMPA_vect) {
    // sztuczka z opóźnieniem polega na tym że zapisujemy "za sobą" cały czas
    // i przez długość buffera (100) jeżeli natrafimy na jakiś eventy to znaczy że
    // został on wpisany dokładnie BUFFER_SIZE * 10ms czasu temu (czyli 1s)

    if (history[idx]) {
        PORTB |= (1 << PB5);   // event włącz -> zapal diode
    } else {
        PORTB &= ~(1 << PB5);  // event wyłącz -> zgaś diode
    }

    if (!(PIND & (1 << PD2))) {
        history[idx] = 1; // Przycisk wciśnięty
    } else {
        history[idx] = 0; // Przycisk puszczony
    }

    idx++;
    if (idx >= BUFFER_SIZE) {
        idx = 0;
    }
}

void io_and_timer_init(void) {
    DDRB |= (1 << PB5);
    PORTB &= ~(1 << PB5);

    DDRD &= ~(1 << PD2);
    PORTD |= (1 << PD2);

    cli(); // Wyłącz przerwania na czas konfiguracji

    // Zerowanie liczników w razie gdyby były tam jakieś śmieci po bootloaderze
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    // chcemy sprawdzac z freq 100 hz
    // scaled_freq / 100 = X 
    // zaczynamy od 0 wiec -1 czyli finalnie
    OCR1A = F_CPU / (64 * 100) - 1;

    // Włączenie przerwania od zrównania licznika z wartością OCR1A
    TIMSK1 = (1 << OCIE1A);

    // CTC i Preskaler na 64
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

    // tryb spania, który nie wyłączy nam licznika
    set_sleep_mode(SLEEP_MODE_IDLE);

    sei(); // przywracamy obsługe przerwań
}

int main(void) {
    io_and_timer_init();

    while (1) {
        sleep_mode();
    }
}