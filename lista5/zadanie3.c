#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdio.h>

#define BAUD 9600                          // baudrate
#define UBRR_VALUE ((F_CPU)/16/(BAUD)-1)   // zgodnie ze wzorem

#define SAMPLES 100UL                      // liczba pomiarow w serii

// --- Twój boilerplate UART ---
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

// --- Zmienna do zapisu wyniku z przerwania ADC ---
volatile uint16_t adc_nr_result = 0;

ISR(ADC_vect)
{
  adc_nr_result = ADC;
}

void adc_init()
{
  // REFS0 = 1 -> Aref jako AVcc (5V)
  // MUX3..1 = 1110 -> Pomiar wewnętrznego napięcia referencyjnego 1.1V
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  ADCSRB = 0;

  // Włączenie ADC + preskaler 128 (16MHz / 128 = 125 kHz)
  ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

  _delay_ms(20);
}

// 1. Pomiar bez Noise Reduction (aktywne czekanie w pętli)
uint16_t measure_polling()
{
  ADCSRA &= ~_BV(ADIE);          // wyłącz przerwanie ADC
  ADCSRA |= _BV(ADSC);           // start pomiaru
  while (ADCSRA & _BV(ADSC));    // aktywne czekanie w pętli
  ADCSRA |= _BV(ADIF);           // wyczyszczenie flagi przerwania ADC
  return ADC;
}

// 2. Pomiar z ADC Noise Reduction (SMCR + przerwanie + uśpienie)
uint16_t measure_noise_reduction()
{
  ADCSRA |= _BV(ADIF);           // wyczyść flagę przed włączeniem przerwania

  // Rejestr SMCR: SM0 = 1 (ADC Noise Reduction), SE = 1 (Sleep Enable)
  SMCR = _BV(SM0) | _BV(SE);
  ADCSRA |= _BV(ADIE);           // włącz przerwanie ADC

  sleep_cpu();                   // uśpienie procesora (automatycznie startuje pomiar ADC)

  SMCR = 0;                      // wyłącz uśpienie po wybudzeniu
  ADCSRA &= ~_BV(ADIE);          // wyłącz przerwanie ADC
  return adc_nr_result;
}

void print_variance(const char *name, uint32_t sum, uint32_t sum_sq)
{
  uint32_t mean_x100 = (sum * 100UL) / SAMPLES;

  // Wzór na wariancję:Var(X) = E(X^2) - (E(X))^2 = (N * sum_sq - sum^2) / N^2
  uint32_t num = SAMPLES * sum_sq - sum * sum;
  uint32_t denom = SAMPLES * SAMPLES;
  uint32_t var_x10000 = (num * 10000UL) / denom;

  printf("%s -> Srednia: %lu.%02lu | Wariancja: %lu.%04lu\r\n",
         name,
         mean_x100 / 100UL, mean_x100 % 100UL,
         var_x10000 / 10000UL, var_x10000 % 10000UL);
}

int main()
{
  uart_init();
  fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
  stdin = stdout = stderr = &uart_file;

  adc_init();
  sei();

  while (1) {
    // --- Seria 1: Bez Noise Reduction (aktywne czekanie) ---
    for (uint8_t i = 0; i < 50; i++) {
      measure_polling(); // rozgrzewka stabilizująca pobór prądu w trybie aktywnym
    }

    uint32_t sum_poll = 0, sum_sq_poll = 0;
    for (uint16_t i = 0; i < SAMPLES; i++) {
      uint16_t v = measure_polling();
      sum_poll += v;
      sum_sq_poll += (uint32_t)v * v;
    }

    for (uint8_t i = 0; i < 50; i++) {
      measure_noise_reduction(); // rozgrzewka stabilizująca napięcie przy niższym poborze prądu we śnie
    }

    uint32_t sum_nr = 0, sum_sq_nr = 0;
    for (uint16_t i = 0; i < SAMPLES; i++) {
      uint16_t v = measure_noise_reduction();
      sum_nr += v;
      sum_sq_nr += (uint32_t)v * v;
    }

    // --- Wypisanie wyników ---
    printf("--- Seria N = %lu ---\r\n", SAMPLES);
    print_variance("Bez Noise Reduction", sum_poll, sum_sq_poll);
    print_variance("ADC Noise Reduction", sum_nr, sum_sq_nr);
    printf("\r\n");

    _delay_ms(1000);
  }
}