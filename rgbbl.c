#include "main.h"
#include <avr/io.h>
#include "rgbbl.h"


void rgbbl_set(uint8_t r, uint8_t g, uint8_t b)
{
    OCR1AH = 0;
    OCR1AL = g;
    OCR1BL = r;
    OCR2A = b;
    DDRD |= _BV(4); // OC0A, red
    DDRD |= _BV(5); // OC0B, green
    DDRD |= _BV(7); // OC2A, blue
}

void rgbbl_init(void) {
    OCR1AH = 0;
    OCR1AL = 0;
    OCR1BL = 0;
    TCCR1A = 0xA1;
    TCCR1B = 0x09;
    DDRD |= _BV(4); // OC1B, red
    DDRD |= _BV(5); // OC1A, green
    TCCR2A = 0x83; /* OC2A (OCR2A) only, Fast PWM. */
    TCCR2B = 0x01; /* Go (No div). */
    OCR2A = 0;
    DDRD |= _BV(7); // OC2A, blue
}

void rgbbl_off(void)
{
    rgbbl_set(0,0,0);
    DDRD &= ~_BV(4);
    DDRD &= ~_BV(5);
    DDRD &= ~_BV(7);
}
