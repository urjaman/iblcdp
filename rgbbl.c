#include "main.h"
#include <avr/io.h>
#include "rgbbl.h"

/* 0bRGB */
#define MAX_RED		255
#define MAX_GREEN	200
#define MAX_BLUE 	200

static uint8_t rgbbl_color = C_MAGENTA; /* lol */
static uint8_t rgbbl_intensity = 255;

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
    rgbbl_set_intensity(rgbbl_intensity);
}

void rgbbl_off(void)
{
    rgbbl_intensity = 0;
    rgbbl_set(0,0,0);
    DDRD &= ~_BV(4);
    DDRD &= ~_BV(5);
    DDRD &= ~_BV(7);
}

void rgbbl_set_intensity(uint8_t in)
{
	uint8_t r,g,b;
	uint16_t scaler;
	if (rgbbl_color & C_RED) {
		scaler = (in * MAX_RED) >> 8;
		r = scaler;
	} else {
		r = 0;
	}

	if (rgbbl_color & C_GREEN) {
		scaler = (in * MAX_GREEN) >> 8;
		g = scaler;
	} else {
		g = 0;
	}

	if (rgbbl_color & C_BLUE) {
		scaler = (in * MAX_BLUE) >> 8;
		b = scaler;
	} else {
		b = 0;
	}
	rgbbl_intensity = in;
	rgbbl_set(r,g,b);
}



void rgbbl_set_color(enum colors color) {
	rgbbl_color = color;
	if (rgbbl_intensity) {
		rgbbl_set_intensity(rgbbl_intensity);
	}
}

