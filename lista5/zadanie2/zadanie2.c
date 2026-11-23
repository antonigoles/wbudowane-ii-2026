#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdio.h>

#define R_REF 10000UL

#define BAUD 9600                          // baudrate
#define UBRR_VALUE ((F_CPU)/16/(BAUD)-1)   // zgodnie ze wzorem

void uart_init()
{
  UBRR0 = UBRR_VALUE;
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

int uart_transmit(char data, FILE *stream)
{
  while(!(UCSR0A & _BV(UDRE0)));
  UDR0 = data;
  return 0;
}

int uart_receive(FILE *stream)
{
  while (!(UCSR0A & _BV(RXC0)));
  return UDR0;
}

FILE uart_file;

volatile uint16_t last_adc = 0;

// Puste obsługa INT0 - dzięki temu sprzęt automatycznie zeruje INTF0
// co pozwala układowi ADC Auto Trigger wykryć kolejne naciśnięcie.
ISR(INT0_vect) {}

// Obłusga ADC
ISR(ADC_vect) {
    last_adc = ADC; // Zapisz zmierzoną wartość (0..1023)
}

void io_int0_adc_init(void) {
    DDRD &= ~(1 << PD2);
    PORTD |= (1 << PD2);

    // Wyłączenie bufora cyfrowego na pinie A0 (PC0) - zmniejsza szumy i pobór prądu przy pomiarze ADC
    DIDR0 |= (1 << ADC0D);

    // Ustawienie tak żeby spadek napięcia generował przerwanie (czyli odbicie guzika)
    EICRA = (1 << ISC01);
    EIMSK = (1 << INT0);

    // Konfiguracja ADC z Auto Triggerem wyzwalanym przez INT0
    ADMUX = (1 << REFS0);  // Napięcie referencyjne AVcc (5V), kanał ADC0 (pin A0)
    ADCSRB = (1 << ADTS1); // Źródło Auto Triggera: External Interrupt Request 0 (INT0)

    // Włączenie ADC (ADEN), Auto Triggera (ADATE), przerwania ADC (ADIE) oraz preskalera 128 (ADPS2..0)
    ADCSRA = (1 << ADEN) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

int main()
{
    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    cli();
    io_int0_adc_init();
    sei(); // Globalne włączenie przerwań

    while (1) {
        uint16_t adc_copy;

        // Bezpieczne skopiowanie 16-bitowej zmiennej zapisywanej w przerwaniu
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            adc_copy = last_adc;
        }

        // matematyka jak policzyc ohmy tutaj: (ze voltage divider formula)
        // U_a0 = U_cc * (R_ref / (R_foto + R_Ref))
        // i mamy tez ADC = 1023 * U_a0 / U_cc
        // Po przkeształceniu dostajemy R_foto = R_ref * (1023 - ADC) / ADC
        if (adc_copy == 0) {
            printf("Brak pomiaru lub obwod otwarty (ADC = 0)\r\n");
        } else {
            uint32_t resistance = (R_REF * (1023UL - adc_copy)) / adc_copy;
            printf("Ostatnia rezystancja: %lu Ohm (ADC: %u)\r\n", resistance, adc_copy);
        }

        _delay_ms(500);
    }
}