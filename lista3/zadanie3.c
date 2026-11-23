#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>


#define LED PD6
#define LED_DDR DDRD
#define LED_PORT PORTD

// inicjalizacja ADC
void adc_init()
{
  ADMUX   = _BV(REFS0); // referencja AVcc, wejście ADC0
  DIDR0   = _BV(ADC0D); // wyłącz wejście cyfrowe na ADC0
  // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
  ADCSRA  = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
  ADCSRA |= _BV(ADEN); // włącz ADC
}

static const uint16_t led_curve[33] = {
       0,    0,    1,    3,    7,   12,   20,   29,
      41,   56,   73,   94,  118,  146,  177,  212,
     252,  295,  343,  396,  454,  516,  584,  657,
     735,  819,  909, 1005, 1010, 1015, 1019, 1022,
    1024
};

uint16_t get_led_pwm(uint16_t brightness) {
    if (brightness == 0) return 0;
    if (brightness >= 1024) return 1024;

    uint32_t idx  = brightness >> 5;
    uint32_t frac = brightness & 31;

    uint32_t y0 = led_curve[idx];
    uint32_t y1 = led_curve[idx + 1];

    return (uint16_t)(y0 + (((y1 - y0) * frac) >> 5));
}

inline static void stupid_delay_us(uint32_t us) {
    while (us--) {
        _delay_us(10);
    }
}

inline static void led_linear_power_tick(uint16_t strength) {
    // strength is 0 to 1024
    // Według standardu IEEE 1789-2015 żeby oczy nie bolały!!
    const uint32_t FREQ = 3000;

    // scaling trzeba zaczac jakos od 900
    uint32_t scaled_strength = get_led_pwm(strength);
    uint32_t total_tick = 1000000 / (FREQ);
    uint32_t time_on = (total_tick * scaled_strength) >> 10;
    uint32_t time_off = total_tick - time_on;

    stupid_delay_us(time_off);
    LED_PORT |= _BV(LED);
    stupid_delay_us(time_on);
    LED_PORT &= ~_BV(LED);
}

FILE uart_file;

void start_adc_read() {
    ADCSRA |= _BV(ADSC);
}

uint8_t check_if_read_is_running() {
    return (ADCSRA & _BV(ADSC));
}

int main()
{
    LED_DDR |= _BV(LED);
    LED_PORT = 0;
    adc_init();
    
    start_adc_read(); // first read
    uint16_t led_strength = 0;

    // mierz napięcie
    while(1) {
        if (!check_if_read_is_running()) {
            led_strength = ADC;
            // setup next read
            start_adc_read();
        }
        led_linear_power_tick(led_strength);   
    }
}

