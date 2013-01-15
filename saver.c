#include "main.h"
#include "backlight.h"
#include "relay.h"
#include "saver.h"
#include "tui.h"
#include "batlvl.h"
#include "adc.h"
#include <util/crc16.h>


extern int16_t tui_temprefresh_lp_temp;
extern int16_t tui_temprefresh_hp_temp;
extern uint8_t tui_temprefresh_lp_interval;
extern uint8_t tui_temprefresh_hp_interval;

#ifdef ALARMCLOCK
extern uint8_t tui_alarm_enabled; // 0/1 => relay_autovoltage bit 15
extern uint8_t tui_alarm_snooze_time; // 0-255 => relay mode
extern uint8_t tui_alarm_autosnooze_time; // 0-255 => relay_keepon
extern uint16_t tui_alarm_set_time; // 0-1439 => relay_autovoltage bit 0-14
extern uint16_t tui_alarm_time; // Not saved, but tui_alarm_set_time is restored to here.
#endif

#define SAVER_MAGIC1 0x8A6F // MAGic-Format

#ifdef ALARMCLOCK
#define SAVER_MAGIC1_O 0xA5C5
#else
#define SAVER_MAGIC1_O 0xA551
#endif
#define SAVER_MAGIC3 0xE5

// Common to each format: starts with magic1, ends with magic3, crc16.

struct __attribute__ ((__packed__)) sys_settings_old {
	uint16_t magic1;
	uint16_t relay_autovoltage;
	uint8_t relay_mode;
	uint8_t relay_keepon;
	uint8_t backlight_brightness;
	uint8_t backlight_timeout;
	uint8_t backlight_dv;
	uint8_t lcd_contrast;
	uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];
	batlvl_setting_t batlvl_settings;
	int16_t tui_temprefresh_lp_temp;
	int16_t tui_temprefresh_hp_temp;
	uint8_t tui_temprefresh_lp_interval;
	uint8_t tui_temprefresh_hp_interval;
	uint8_t magic3;
	uint16_t crc16;
};

// Format>=1 have a format and size parameters so they wont need special CRC checking code.
struct __attribute__ ((__packed__)) sys_settings_f1 {
	uint16_t magic1;
	uint16_t format;
	uint16_t size;
	uint16_t relay_autovoltage;
	uint8_t relay_mode;
	uint8_t relay_keepon;
	uint8_t backlight_brightness;
	uint8_t backlight_timeout;
	uint8_t backlight_dv;
	uint8_t lcd_contrast;
	uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];
	batlvl_setting_t batlvl_settings;
	int16_t tui_temprefresh_lp_temp;
	int16_t tui_temprefresh_hp_temp;
	uint8_t tui_temprefresh_lp_interval;
	uint8_t tui_temprefresh_hp_interval;
	int16_t adc_calibration_diff[ADC_MUX_CNT];
	uint8_t magic3;
	uint16_t crc16;
};

union sys_settings {
	struct sys_settings_old old;
	struct sys_settings_f1 f1;
	struct {
		uint16_t magic1;
		uint16_t format;
		uint16_t size;
	} c;
};

#define CURRENT_SAVE_FORMAT_NAME sys_settings_f1
#define CURRENT_SAVE_FORMAT_NUMBER 1


void saver_load_settings_f0(struct sys_settings_old *st) {
#ifdef ALARMCLOCK
	// Load alarm stuff from relay variables
	tui_alarm_enabled = (st->relay_autovoltage&0x8000)?1:0;
	tui_alarm_snooze_time = st->relay_mode;
	tui_alarm_autosnooze_time = st->relay_keepon;
	tui_alarm_set_time = st->relay_autovoltage&0x7FFF;
	tui_alarm_time = tui_alarm_set_time;
#else
	relay_set_autovoltage(st->relay_autovoltage);
	relay_set_keepon(st->relay_keepon);
	relay_set(st->relay_mode);
#endif
	backlight_set(st->backlight_brightness);
	backlight_set_to(st->backlight_timeout);
	backlight_set_dv(st->backlight_dv);
	backlight_set_contrast(st->lcd_contrast);
	memcpy(tui_mp_mods,st->tui_mp_mods,TUI_MODS_MAXDEPTH*4);
	batlvl_settings = st->batlvl_settings;
	tui_temprefresh_lp_temp = st->tui_temprefresh_lp_temp;
	tui_temprefresh_hp_temp = st->tui_temprefresh_hp_temp;
	tui_temprefresh_lp_interval = st->tui_temprefresh_lp_interval;
	tui_temprefresh_hp_interval = st->tui_temprefresh_hp_interval;
	return;
}

void saver_load_settings_f1(struct sys_settings_f1 *st) {
	uint8_t i;
	if (st->format&0x8000) {
		#ifdef ALARMCLOCK
		// Load alarm stuff from relay variables
		tui_alarm_enabled = (st->relay_autovoltage&0x8000)?1:0;
		tui_alarm_snooze_time = st->relay_mode;
		tui_alarm_autosnooze_time = st->relay_keepon;
		tui_alarm_set_time = st->relay_autovoltage&0x7FFF;
		tui_alarm_time = tui_alarm_set_time;
		#endif
	} else {
		#ifndef ALARMCLOCK
		relay_set_autovoltage(st->relay_autovoltage);
		relay_set_keepon(st->relay_keepon);
		relay_set(st->relay_mode);
		#endif
	}
	for (i=0;i<ADC_MUX_CNT;i++) adc_calibration_diff[i] = st->adc_calibration_diff[i];
	backlight_set(st->backlight_brightness);
	backlight_set_to(st->backlight_timeout);
	backlight_set_dv(st->backlight_dv);
	backlight_set_contrast(st->lcd_contrast);
	memcpy(tui_mp_mods,st->tui_mp_mods,TUI_MODS_MAXDEPTH*4);
	batlvl_settings = st->batlvl_settings;
	tui_temprefresh_lp_temp = st->tui_temprefresh_lp_temp;
	tui_temprefresh_hp_temp = st->tui_temprefresh_hp_temp;
	tui_temprefresh_lp_interval = st->tui_temprefresh_lp_interval;
	tui_temprefresh_hp_interval = st->tui_temprefresh_hp_interval;
	return;
}


void saver_load_settings(void) {
	uint16_t format;
	uint16_t size;
	uint16_t crc=0xFFFF;
	uint8_t i;
	union sys_settings st;
	eeprom_read_block(&st,(void*)0, sizeof(union sys_settings));
	if (st.c.magic1==SAVER_MAGIC1_O) {
		size = sizeof(struct sys_settings_old);
		format = 0;
	} else if (st.c.magic1==SAVER_MAGIC1) {
		format = st.c.format&0x7FFF;
		size = st.c.size;
		if (size>sizeof(union sys_settings)) return; // Formats bigger than biggest known, not supported.
		if (format>CURRENT_SAVE_FORMAT_NUMBER) return; // Formats from the future are not supported.
		if (!format) return; // So that we dont call load_f0 with f>=1 magic ... wut.
	} else { 
		return;
	}
	uint8_t * stb = (uint8_t*)&st;
	// M3 check
	if (stb[size-3] != SAVER_MAGIC3) return;
	// CRC Calculation
	for(i=0;i<(size-2);i++) {
		crc = _crc16_update(crc, stb[i] );
	}
	uint16_t stcrc = *((uint16_t*)&(stb[size-2]));
	if (crc != stcrc) return;
	switch (format) {
		case 0: saver_load_settings_f0(&st.old); return;
		case 1: saver_load_settings_f1(&st.f1); return;
		default: return;
	}
}

void saver_save_settings(void) {
	uint16_t crc=0xFFFF;
	uint8_t i;
	struct CURRENT_SAVE_FORMAT_NAME st;
	st.magic1 = SAVER_MAGIC1;
	st.format = CURRENT_SAVE_FORMAT_NUMBER; // Later this will be used to detect which data to load.
	st.size = sizeof(struct CURRENT_SAVE_FORMAT_NAME); // This will be used to automate CRC check for any format.
#ifdef ALARMCLOCK
	st.format |= 0x8000; // Highest bit of format is DALARMCLOCK flag.
	//Save alarm stuff in relay variables
	st.relay_autovoltage = tui_alarm_set_time;
	if (tui_alarm_enabled) st.relay_autovoltage |= 0x8000;
	st.relay_keepon = tui_alarm_autosnooze_time;
	st.relay_mode = tui_alarm_snooze_time;
#else
	st.relay_autovoltage = relay_get_autovoltage();
	st.relay_keepon = relay_get_keepon();
	st.relay_mode = relay_get_mode();
#endif
	st.backlight_brightness = backlight_get();
	st.backlight_timeout = backlight_get_to();
	st.backlight_dv = backlight_get_dv();
	st.lcd_contrast = backlight_get_contrast();
	memcpy(st.tui_mp_mods,tui_mp_mods,TUI_MODS_MAXDEPTH*4);
	st.batlvl_settings = batlvl_settings;
	st.tui_temprefresh_lp_temp = tui_temprefresh_lp_temp;
	st.tui_temprefresh_hp_temp = tui_temprefresh_hp_temp;
	st.tui_temprefresh_lp_interval = tui_temprefresh_lp_interval;
	st.tui_temprefresh_hp_interval = tui_temprefresh_hp_interval;
	for (i=0;i<ADC_MUX_CNT;i++) st.adc_calibration_diff[i] = adc_calibration_diff[i];
	st.magic3 = SAVER_MAGIC3;
	for(i=0;i<(sizeof(struct CURRENT_SAVE_FORMAT_NAME)-2);i++) {
		crc = _crc16_update(crc, (((uint8_t*)&st)[i]) );
	}
	st.crc16 = crc;
	eeprom_update_block(&st,(void*)0, sizeof(struct CURRENT_SAVE_FORMAT_NAME));
}
