#include "main.h"
#include "timer.h"
#include "lcd.h"
#include "relay.h"
#include "backlight.h"
#include "buttons.h"
#include "tui.h"

#ifdef ALARMCLOCK
static uint8_t tui_alarm_enabled = 0;
static uint8_t tui_alarm_active = 0;
static uint16_t tui_alarm_set_time = 0;
static uint16_t tui_alarm_time = 0;
static uint8_t tui_alarm_snooze_time = 8;
static uint8_t tui_alarm_autosnooze_time = 30;

static uint16_t tui_get_mindaytime(void) {
	struct mtm tm;
	timer_get_time(&tm);
	return (tm.hour*60+tm.min);
}

static void tui_alarm_active_f(uint8_t init) {
	static uint8_t alarm_state = 0;
	if (init) {
		backlight_lock(1);
		buttons_lock(1);
		alarm_state = 0;
		tui_alarm_active = 1;
	}
	if (!timer_get_5hzp()) return;
	if (!alarm_state) {
		relay_set(RLY_MODE_ON);
		backlight_simple_set(16);
	} else {
		relay_set(RLY_MODE_OFF);
		backlight_simple_set(0);
	}
	alarm_state ^= 1;
}

static void tui_alarm_deactivate(void) {
	buttons_lock(0);
	backlight_lock(0);
	relay_set(RLY_MODE_OFF);
	tui_alarm_active = 0;
}

static void tui_alarm_snooze(void) {
	if (tui_alarm_active) {
		tui_alarm_deactivate();
		tui_alarm_enabled = 1;
		tui_alarm_time = tui_get_mindaytime() + tui_alarm_snooze_time;
	}
}

void tui_alarm_run(void) {
	if (tui_alarm_active) {
		if (tui_get_mindaytime() >= (tui_alarm_time + tui_alarm_autosnooze_time)) {
			tui_alarm_snooze();
			return;
		}
		buttons_lock(0);
		if (buttons_get_v()) {
			tui_alarm_snooze();
		} else {
			buttons_lock(1);
			tui_alarm_active_f(0);
		}
	} else {
		if (!timer_get_5hzp()) return;
		if (tui_alarm_enabled) {
			if (tui_get_mindaytime() == tui_alarm_time) {
				tui_alarm_active_f(1);
			}
		}
	}
}

// *** Menu subsysten start **** /


const unsigned char tui_am_name[] PROGMEM = "ALARM MENU";
const unsigned char tui_am_s1_off[] PROGMEM = "SET ALARM";
const unsigned char tui_am_s1_on[] PROGMEM = "DISABLE ALARM";
const unsigned char tui_am_s2[] PROGMEM = "SET SNOOZE";
const unsigned char tui_am_s3[] PROGMEM = "SET AUTOSNOOZE";

PGM_P const tui_am_off_table[] PROGMEM = {
    (PGM_P)tui_am_s1_off,
    (PGM_P)tui_am_s2,
    (PGM_P)tui_am_s3,
    (PGM_P)tui_exit_menu,
};

PGM_P const tui_am_on_table[] PROGMEM = {
    (PGM_P)tui_am_s1_on,
    (PGM_P)tui_am_s2,
    (PGM_P)tui_am_s3,
    (PGM_P)tui_exit_menu,
};

static uint8_t tui_alarm_time_printer(unsigned char* buf, int32_t val) {
	tui_num_helper(buf,val/60);
	buf[2] = ':';
	tui_num_helper(buf+3,val%60);
	buf[5] = 0;
	return 5;
}

static uint16_t tui_alarm_time_pick(uint16_t start) {
	start = tui_gen_adjmenu(PSTR("SET HOURS"),tui_alarm_time_printer,0,1439,start,60);
	start = tui_gen_adjmenu(PSTR("SET MINUTES"),tui_alarm_time_printer,0,1439,start,1);
	return start;
}

void tui_alarm_menu(void) {
	PGM_P alarm_is = PSTR("ALARM IS");
	uint8_t sel = 0;
	for (;;) {
		if (tui_alarm_enabled) {
			sel = tui_gen_listmenu((PGM_P)tui_am_name, tui_am_on_table, 4, sel);
		} else {
			sel = tui_gen_listmenu((PGM_P)tui_am_name, tui_am_off_table, 4, sel);
		}
		switch (sel) {
			case 0:
				if (tui_alarm_enabled) {
					tui_alarm_enabled = 0;
					tui_gen_message(alarm_is,PSTR("DISABLED"));
				} else {
					tui_alarm_set_time = tui_alarm_time_pick(tui_alarm_set_time);
					tui_alarm_time = tui_alarm_set_time;
					tui_alarm_enabled = 1;
					tui_gen_message(alarm_is,PSTR("ENABLED"));
				}
				break;
			case 1:
				tui_alarm_snooze_time = tui_gen_adjmenu((PGM_P)tui_am_s2,tui_alarm_time_printer,2,30,tui_alarm_snooze_time,1);
				break;
			case 2:
				tui_alarm_autosnooze_time = tui_gen_adjmenu((PGM_P)tui_am_s3,tui_alarm_time_printer,2,60,tui_alarm_autosnooze_time,1);
				break;
			default:
				return;
		}
	}
}

uint8_t tui_alarm_mod_str(uint8_t *buf) {
	buf[0] = 'A';
	buf[1] = ':';
	if (tui_alarm_enabled) {
		return tui_alarm_time_printer(buf+2,tui_alarm_time)+2;
	} else {
		buf[2] = 'O';
		buf[3] = 'F';
		buf[4] = 'F';
		return 5;
	}
}

#else
void tui_alarm_run(void) { }
void tui_alarm_menu(void { }
uint8_t tui_alarm_mod_str(uint8_t *buf) {
	return 0;
}
#endif

