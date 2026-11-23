#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <stdio.h>

#define BAUD 9600                          // baudrate
#define UBRR_VALUE ((F_CPU)/16/(BAUD)-1)   // zgodnie ze wzorem

FILE uart_file;

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

#define DEFINE_BENCHMARK(TYPE, C_VAL) \
    volatile TYPE some_number_##TYPE = C_VAL; \
    \
    uint8_t measure_add_##TYPE() { \
        volatile TYPE experimental = 10; \
        TCNT0 = 0; \
        experimental += some_number_##TYPE; \
        return TCNT0; \
    } \
    uint8_t measure_mult_##TYPE() { \
        volatile TYPE experimental = 10; \
        TCNT0 = 0; \
        experimental *= some_number_##TYPE; \
        return TCNT0; \
    } \
    uint8_t measure_div_##TYPE() { \
        volatile TYPE experimental = 10; \
        TCNT0 = 0; \
        experimental /= some_number_##TYPE; \
        return TCNT0; \
    } \

DEFINE_BENCHMARK(int8_t, 17)
DEFINE_BENCHMARK(int16_t, 47)
DEFINE_BENCHMARK(int32_t, 193)
DEFINE_BENCHMARK(int64_t, 736)
DEFINE_BENCHMARK(float, 19.3f)

int main() 
{
    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    // no prescaler -> same as CPU
    TCCR0B = _BV(CS00);
    
    printf("measure_add_int8_t: %d\n\r", measure_add_int8_t());
    printf("measure_mult_int8_t: %d\n\r", measure_mult_int8_t());
    printf("measure_div_int8_t: %d\n\r", measure_div_int8_t());
    
    printf("\n\r");

    printf("measure_add_int16_t: %d\n\r", measure_add_int16_t());
    printf("measure_mult_int16_t: %d\n\r", measure_mult_int16_t());
    printf("measure_div_int16_t: %d\n\r", measure_div_int16_t());

    printf("\n\r");

    printf("measure_add_int32_t: %d\n\r", measure_add_int32_t());
    printf("measure_mult_int32_t: %d\n\r", measure_mult_int32_t());
    printf("measure_div_int32_t: %d\n\r", measure_div_int32_t());

    printf("\n\r");

    printf("measure_add_int64_t: %d\n\r", measure_add_int64_t());
    printf("measure_mult_int64_t: %d\n\r", measure_mult_int64_t());
    printf("measure_div_int64_t: %d\n\r", measure_div_int64_t());

    printf("\n\r");

    printf("measure_add_float: %d\n\r", measure_add_float());
    printf("measure_mult_float: %d\n\r", measure_mult_float());
    printf("measure_div_float: %d\n\r", measure_div_float());
}
