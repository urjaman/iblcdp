#include "main.h"
#include "backlight.h"
#include "buttons.h"


void buttons_init(void) {
	PORTC |= _BV(2) | _BV(3) | _BV(4) | _BV(5); // enable pull-ups
}

static uint8_t buttons_get_v(void) {
	uint8_t rv = (uint8_t)~PINC;
	rv = (uint8_t)rv & (uint8_t)(_BV(2)|_BV(3)|_BV(4)|_BV(5));
	rv = (uint8_t)rv >> (uint8_t)2;
	return rv;
}

uint8_t buttons_get(void) {
	uint8_t v = buttons_get_v();
	if (!v) return 0;
	backlight_activate();
	_delay_ms(120);
	for(;;) {
		uint8_t sv;
		_delay_ms(30);
		sv = buttons_get_v();
		if (sv == v){
			return v;
		}
		v = sv;
	}
}
