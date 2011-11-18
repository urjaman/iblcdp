#include "main.h"
#include "timer.h"
#include "adc.h"
#include "relay.h"
#include "backlight.h"
#include "hd44780.h"

uint8_t bl_drv_value;
uint8_t bl_value;
uint8_t bl_to;
uint32_t bl_last_sec=0;

const uint8_t backlight_values[17] PROGMEM = {
	0, 1, 2, 3, 4, 8, 13, 21, 32, 45, 62, 83, 108, 137, 171, 210, 255
};

int8_t bl_v_now;
int8_t bl_v_fadeto;

static void backlight_simple_set(int8_t v) {
	if (v < 0) {
		OCR0A = 0;
		DDRD &= ~_BV(6);
		hd44780_wait_ready();
		hd44780_outcmd(HD44780_DISPCTL(0, 0, 0));
		bl_v_now = v;
		return;
	}
	if (bl_v_now<0) {
		hd44780_wait_ready();
		hd44780_outcmd(HD44780_DISPCTL(1, 0, 0));
	}
	uint8_t hwv;
	hwv = pgm_read_byte(&(backlight_values[v]));
	DDRD |= _BV(6);
	OCR0A = hwv;
	bl_v_now = v;
}

static void backlight_fader(void) {
	if (bl_v_fadeto < bl_v_now) {
		if (bl_v_now>4) {
			uint8_t v1,v2;
			v1 = pgm_read_byte(&(backlight_values[bl_v_now]));
			v2 = pgm_read_byte(&(backlight_values[bl_v_now-1]));
			v1 = ((v1-v2)/2)+v2;
			OCR0A = v1;
			_delay_ms(25);
			backlight_simple_set(bl_v_now-1);
			_delay_ms(25);
		} else {
			backlight_simple_set(bl_v_now-1);
			_delay_ms(50);
		}
		goto ret;
	}
	if (bl_v_fadeto > bl_v_now) {
		backlight_simple_set(bl_v_fadeto);
		goto ret;
	}
ret:
	if (bl_v_fadeto != bl_v_now) timer_set_waiting();
	return;

}

void backlight_init(void) {
	DDRD |= _BV(6);
	TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS00);
	bl_to = 15;
	backlight_set(16);
	backlight_simple_set(16);
	bl_v_fadeto = 16;
	bl_drv_value = 10;
}

void backlight_set(uint8_t v) {
	if (v>16) v=16;
	if ((bl_v_now == bl_value)&&(bl_v_fadeto == bl_value)) {
		backlight_simple_set(v);
		bl_v_fadeto = v;
	}
	bl_value = v;
	if (bl_drv_value>v) bl_drv_value = v;
}


void backlight_set_dv(uint8_t v) {
	if (v>16) v=16;
	if (v>bl_value) v=bl_value;
	bl_drv_value = v;
}

uint8_t backlight_get(void) {
	return bl_value;
}

uint8_t backlight_get_dv(void) {
	return bl_drv_value;
}

uint8_t backlight_get_to(void) {
	return bl_to;
}

void backlight_set_to(uint8_t to) {
	bl_to = to;
}

void backlight_activate(void) {
	bl_last_sec = timer_get();
	bl_v_fadeto = bl_value;
	backlight_fader();
}

void backlight_run(void) {
	uint32_t diff = timer_get() - bl_last_sec;
	if (diff >= bl_to) {
		if (relay_get_autodecision() == RLY_MODE_ON) {
			bl_v_fadeto = bl_drv_value;
		} else {
			bl_v_fadeto = -1;
		}
	} else {
		bl_v_fadeto = bl_value;
	}
	backlight_fader();
}