#include "main.h"
#include "adc.h"
#include "timer.h"
#include "tui.h"
#include "backlight.h"
#include "relay.h"

#define RLY_THINKSTACKDEPTH 4
static uint8_t relay_mode;
static uint8_t relay_auto_thinkstack[RLY_THINKSTACKDEPTH];
static uint16_t relay_auto_voltage;
static uint32_t relay_last_act_sec;
static uint8_t relay_auto_keep_on;
static uint8_t relay_last_autodecision;

static void relay_clr_thinkstack(void) {
	uint8_t i;
	for(i=0;i<RLY_THINKSTACKDEPTH;i++) {
		relay_auto_thinkstack[i] = 255;
	}
}

void relay_init(void) {
	PORTC &= ~_BV(3);
	DDRC |= _BV(3);
	relay_clr_thinkstack();
	relay_mode = RLY_MODE_OFF;
	relay_last_act_sec  = 0;
	relay_auto_voltage = 858*4; // 13.41V
	relay_last_autodecision = RLY_MODE_OFF;
	relay_auto_keep_on = 90;
}

void relay_set(uint8_t mode) {
	switch (mode) {
		default:
		case RLY_MODE_OFF:
			relay_mode = RLY_MODE_OFF;
			PORTC &= ~_BV(3);
			break;
		case RLY_MODE_ON:
			relay_mode = RLY_MODE_ON;
			PORTC |= _BV(3);
			break;
		case RLY_MODE_AUTO:
			relay_mode = RLY_MODE_AUTO;
			relay_clr_thinkstack();
			break;
	}
}

void relay_set_autovoltage(uint16_t v) {
	relay_auto_voltage = v;
	relay_clr_thinkstack(); // No hasty decisions, please ;)
}

void relay_set_keepon(uint8_t v) {
	relay_auto_keep_on = v;
}


uint8_t relay_get(void) {
	return (PINC&_BV(3)) ? RLY_MODE_ON : RLY_MODE_OFF;
}

uint8_t relay_get_mode(void) {
	return relay_mode;
}

uint16_t relay_get_autovoltage(void) {
	return relay_auto_voltage;
}

uint8_t relay_get_autodecision(void) {
	return relay_last_autodecision;
}

uint8_t relay_get_keepon(void) {
	return relay_auto_keep_on;
}

static uint8_t relay_auto_think(void) {
	uint8_t decision = RLY_MODE_OFF;
	uint32_t now = timer_get();
	uint16_t mbv;
	uint8_t i;
	if (!timer_get_1hzp()) return 0; // 1 Hz speed limit here
	if ((relay_last_autodecision == RLY_MODE_ON)&&((now - relay_last_act_sec) < (uint32_t)relay_auto_keep_on)) return 0;
	mbv = adc_read_mb();
	if (mbv >= relay_auto_voltage) decision = RLY_MODE_ON;
	for (i=1;i<RLY_THINKSTACKDEPTH;i++) relay_auto_thinkstack[i-1] = relay_auto_thinkstack[i];
	relay_auto_thinkstack[RLY_THINKSTACKDEPTH-1] = decision;
	for (i=0;i<RLY_THINKSTACKDEPTH;i++) if (relay_auto_thinkstack[i] != decision) return 0; // Decision not 100% sure, yet...
	relay_last_autodecision = decision;
	return 1;
}

void relay_run(void) {
	uint8_t r = relay_auto_think();
	if ((r)&&(relay_mode == RLY_MODE_AUTO)) {
		if (relay_last_autodecision != relay_get()) {
			backlight_activate(); // Something to show off
			tui_activate(); // refresh data onscreen when possible
			relay_last_act_sec = timer_get();
			if (relay_last_autodecision == RLY_MODE_ON) {
				PORTC |= _BV(3);
			} else {
				PORTC &= ~_BV(3);
			}
		}
	}
}
