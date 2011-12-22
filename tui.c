#include "main.h"
#include "relay.h"
#include "adc.h"
#include "buttons.h"
#include "timer.h"
#include "lcd.h"
#include "lib.h"
#include "backlight.h"
#include "saver.h"
#include "tui.h"

uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];
static uint8_t tui_mp_modidx[4];
static uint8_t tui_force_draw;

uint8_t tui_pollkey(void) {
	uint8_t v = buttons_get();
	mini_mainloop();
	return v;
}

uint8_t tui_waitforkey(void) {
	uint8_t v;
	for(;;) {
		v = tui_pollkey();
		if (v) return v;
	}
}

static void tui_draw_mainpage(void) {
	uint8_t d;
	unsigned char line[17];
	unsigned char buf[16];
	line[16]=0;
	if ((timer_get()&0x1)&&(timer_get_1hzp())) {
		uint8_t i;
		for (i=0;i<4;i++) {
			uint8_t c = tui_mp_modidx[i] + 1;
			if (c >= TUI_MODS_MAXDEPTH) c=0;
			if (tui_mp_mods[i][c] == 255) c=0;
			tui_mp_modidx[i] = c;
		}
	}
	// Line 0
	memset(line,' ',16);
	d = tui_run_mod(tui_mp_mods[0][tui_mp_modidx[0]],buf,8);
	memcpy(line,buf,d);
	d = tui_run_mod(tui_mp_mods[1][tui_mp_modidx[1]],buf,15-d);
	memcpy(&(line[16-d]),buf,d);
	lcd_gotoxy(0,0);
	lcd_puts(line);
	// Line 1
	memset(line,' ',16);
	d = tui_run_mod(tui_mp_mods[2][tui_mp_modidx[2]],buf,8);
	memcpy(line,buf,d);
	d = tui_run_mod(tui_mp_mods[3][tui_mp_modidx[3]],buf,15-d);
	memcpy(&(line[16-d]),buf,d);
	lcd_gotoxy(0,1);
	lcd_puts(line);
	tui_force_draw = 0;
}

void tui_gen_menuheader(unsigned char* line, unsigned char* buf, PGM_P header) {
	uint8_t y,z;
	strcpy_P((char*)buf, header);
	memset(line,'=',16);
	line[16] = 0;
	y = strlen((char*)buf);
	z = (16 - y) / 2;
	memcpy(&(line[z]),buf,y);
	lcd_gotoxy(0,0);
	lcd_puts(line);
}

uint8_t tui_pgm_menupart(unsigned char* line, unsigned char* buf, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start) {
	uint8_t idx=start, z,y;
	for (;;) {
		uint8_t key;
		strcpy_P((char*)buf, (PGM_P)pgm_read_word(&(menu_table[idx])));
		memset(line,' ',16);
		y = strlen((char*)buf);
		z = (16 - y) / 2;
		memcpy(&(line[z]),buf,y);
		lcd_gotoxy(0,1);
		lcd_puts(line);
		timer_delay_ms(50);
		key = tui_waitforkey();
		switch (key) {
			case BUTTON_S1:
				idx++;
				if (idx==itemcnt) idx = 0;
				break;
			case BUTTON_S2:
				if (idx) idx--;
				else idx = itemcnt-1;
				break;
			case BUTTON_BOTH:
				return idx;
		}
	}
}


uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start) {
	unsigned char line[17];
	unsigned char buf[17];
	tui_gen_menuheader(line,buf,header);
	return tui_pgm_menupart(line,buf,menu_table,itemcnt,start);
}

static uint16_t tui_gen_voltmenu(PGM_P header, uint16_t start) {
	unsigned char line[17];
	unsigned char buf[17];
	uint16_t idx=adc_to_dV(start);
	tui_gen_menuheader(line,buf,header);
	memset(line,' ',16);
	lcd_gotoxy(0,1);
	lcd_puts(line);
	for(;;) {
		uint8_t key;
		adc_print_dV(buf,idx);
		lcd_gotoxy(5,1);
		lcd_puts(buf);
		key = tui_waitforkey();
		switch (key) {
			case BUTTON_S1:
				idx++;
				if (idx>=1600) idx=0;
				break;
			case BUTTON_S2:
				if (idx) idx--;
				else idx = 1599;
				break;
			case BUTTON_BOTH:
				return adc_from_dV(idx);
		}
	}
}

uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start) {
	unsigned char line[17];
	unsigned char buf[17];
	uint16_t idx=start;
	uint8_t z,y;
	tui_gen_menuheader(line,buf,header);
	for (;;) {
		uint8_t key;
		uint2str(buf,idx);
		memset(line,' ',16);
		y = strlen((char*)buf);
		z = (16 - y) / 2;
		memcpy(&(line[z]),buf,y);
		lcd_gotoxy(0,1);
		lcd_puts(line);
		key = tui_waitforkey();
		switch (key) {
			case BUTTON_S1:
				if (idx == max) idx = min;
				else idx++;
				break;
			case BUTTON_S2:
				if (idx == min) idx = max;
				else idx--;
				break;
			case BUTTON_BOTH:
				return idx;
		}
	}
}

void tui_gen_message(PGM_P l1, PGM_P l2) {
	lcd_clear();
	lcd_gotoxy((16 - strlen_P(l1))/2,0);
	lcd_puts_P(l1);
	lcd_gotoxy((16 - strlen_P(l2))/2,1);
	lcd_puts_P(l2);
	timer_delay_ms(100);
	tui_waitforkey();
}


//Generic Exit Menu Item
const unsigned char tui_exit_menu[] PROGMEM = "EXIT MENU";


const unsigned char tui_rm_s1[] PROGMEM = "OFF";
const unsigned char tui_rm_s2[] PROGMEM = "ON";
const unsigned char tui_rm_s3[] PROGMEM = "AUTO";
PGM_P const tui_rm_table[] PROGMEM = {
    (PGM_P)tui_rm_s1,
    (PGM_P)tui_rm_s2,
    (PGM_P)tui_rm_s3,
};

static void tui_relaymenu(void) {
	uint8_t sel;
	sel = tui_gen_listmenu(PSTR("RELAY MODE:"), tui_rm_table, 3, relay_get_mode());
	relay_set(sel);
}


const unsigned char tui_blsm_name[] PROGMEM = "DISPLAY";
const unsigned char tui_blsm_s1[] PROGMEM = "BL BRIGHTNESS";
const unsigned char tui_blsm_s2[] PROGMEM = "BL DRV BRIGHT";
const unsigned char tui_blsm_s3[] PROGMEM = "BL TIMEOUT";
const unsigned char tui_blsm_s4[] PROGMEM = "LCD CONTRAST";
PGM_P const tui_blsm_table[] PROGMEM = { // BL Settings Menu
	(PGM_P)tui_blsm_s1,
	(PGM_P)tui_blsm_s2,
	(PGM_P)tui_blsm_s3,
	(PGM_P)tui_blsm_s4,
	(PGM_P)tui_exit_menu
};

const uint8_t contrast_max = 64;
const uint8_t contrast_min = 0;
static inline uint8_t tui_get_contrast(void) {
	uint8_t c = backlight_get_contrast();
	if (c>contrast_max) return 0;
	return contrast_max - c;
}

static inline void tui_set_contrast(uint8_t c) {
	if (c>contrast_max) c = contrast_max;
	backlight_set_contrast(contrast_max-c);
}

static void tui_contrast_set_util(void) {
	unsigned char line[17];
	unsigned char buf[17];
	uint8_t idx=tui_get_contrast(), z,y;
	
	tui_gen_menuheader(line,buf,(PGM_P)tui_blsm_s4);
	for (;;) {
		uint8_t key;
		uchar2str(buf,idx);
		memset(line,' ',16);
		y = strlen((char*)buf);
		z = (16 - y) / 2;
		memcpy(&(line[z]),buf,y);
		lcd_gotoxy(0,1);
		lcd_puts(line);
		key = tui_waitforkey();
		switch (key) {
			case BUTTON_S1:
				if (idx != contrast_max) idx++;
				break;
			case BUTTON_S2:
				if (idx != contrast_min) idx--;
				break;
			case BUTTON_BOTH:
				return;
		}
		tui_set_contrast(idx);
	}
}

static void tui_blsettingmenu(void) {
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_blsm_name, tui_blsm_table, 5, sel);
		switch (sel) {
			case 0: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s1, 0, 16, backlight_get());
			backlight_set(v);
			}
			break;

			case 1: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s2, 0, backlight_get(), backlight_get_dv());
			backlight_set_dv(v);
			}
			break;

			case 2: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_blsm_s3, 1, 255, backlight_get_to());
			backlight_set_to(v);
			}
			break;

			case 3:
			tui_contrast_set_util();
			break;
			

			case 4:
			return;
		}
	}
}


const unsigned char tui_sc_s1[] PROGMEM = "OFF";
const unsigned char tui_sc_s2[] PROGMEM = "24H";
const unsigned char tui_sc_s3[] PROGMEM = "28H";

PGM_P const tui_sc_table[] PROGMEM = { // Show Clock
    (PGM_P)tui_sc_s1,
    (PGM_P)tui_sc_s2,
    (PGM_P)tui_sc_s3
};

PGM_P const tui_cs_table[] PROGMEM = { // Clock Set
    (PGM_P)tui_sc_s2,
    (PGM_P)tui_sc_s3
};

const unsigned char tui_setclock_name[] PROGMEM = "SET CLOCK";
static void tui_set_clock(void) {
	uint8_t v = tui_gen_listmenu((PGM_P)tui_setclock_name, tui_cs_table, 2, 0);
	if (v) {
		uint8_t hours, mins, secs;
		timer_get_time28(&hours,&mins,&secs);
		hours = tui_gen_nummenu(PSTR("HOURS (28)"), 0, 27, hours);
		timer_set_time28(hours,mins,secs);
	} else {
		uint8_t hours, mins, secs;
		timer_get_time24(&hours,&mins,&secs);
		secs = 0;
		hours = tui_gen_nummenu(PSTR("HOURS (24)"), 0, 23, hours);
		mins = tui_gen_nummenu(PSTR("MINUTES"),0,59, mins);
		timer_set_time24(hours,mins,secs);
	}
}	


const unsigned char tui_sm_name[] PROGMEM = "SETTINGS";
const unsigned char tui_sm_s1[] PROGMEM = "RLY VOLTTHRES";
const unsigned char tui_sm_s2[] PROGMEM = "RLY AUTO KEEPON";
// BL Settings menu (3)
const unsigned char tui_sm_s4[] PROGMEM = "TUI CONFIG";
// Set Clock (5)
const unsigned char tui_sm_s6[] PROGMEM = "SAVE SETTINGS";
const unsigned char tui_sm_s7[] PROGMEM = "LOAD SETTINGS";
// Exit Menu (8)

PGM_P const tui_sm_table[] PROGMEM = { // Settings Menu
    (PGM_P)tui_sm_s1,
    (PGM_P)tui_sm_s2,
    (PGM_P)tui_blsm_name,
    (PGM_P)tui_sm_s4,
    (PGM_P)tui_setclock_name,
    (PGM_P)tui_sm_s6,
    (PGM_P)tui_sm_s7,
    (PGM_P)tui_exit_menu
};

static void tui_settingsmenu(void) {
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_sm_name, tui_sm_table, 8, sel);
		switch (sel) {
			case 0: {
			uint16_t v = tui_gen_voltmenu((PGM_P)tui_sm_s1, relay_get_autovoltage());
			relay_set_autovoltage(v);
			}
			break;

			case 1: {
			uint8_t v = tui_gen_nummenu((PGM_P)tui_sm_s2, 1, 255, relay_get_keepon());
			relay_set_keepon(v);
			}
			break;

			case 2:
			tui_blsettingmenu();
			break;

			case 3:
			tui_config_menu();
			break;

			case 4:
			tui_set_clock();
			break;

			case 5:
			saver_save_settings();
			tui_gen_message((PGM_P)tui_sm_name,PSTR("SAVED"));
			return;

			case 6:
			saver_load_settings();
			tui_gen_message((PGM_P)tui_sm_name,PSTR("LOADED"));
			return;

			case 7:
			return;
		}
	}
}


const unsigned char tui_mm_s1[] PROGMEM = "SET RELAY MODE";
// Settings Menu (2)
const unsigned char tui_mm_s3[] PROGMEM = "OTHERS";
// Exit Menu (4)

PGM_P const tui_mm_table[] PROGMEM = {
    (PGM_P)tui_mm_s1,
    (PGM_P)tui_sm_name,
    (PGM_P)tui_mm_s3,
    (PGM_P)tui_exit_menu
};

static void tui_mainmenu(void) {
	uint8_t sel=0;
	for (;;) {
		sel = tui_gen_listmenu(PSTR("MAIN MENU"), tui_mm_table, 4, sel);
		switch (sel) {
			case 0:
				tui_relaymenu();
				break;
			case 1:
				tui_settingsmenu();
				break;
			case 2:
				tui_othermenu();
				break;
			default:
			case 3:
				return;
		}
	}
}


void tui_init(void) {
	uint8_t i;
	for(i=0;i<4;i++) {
		uint8_t n;
		for(n=1;n<TUI_MODS_MAXDEPTH;n++) {
			tui_mp_mods[i][n] = 255;
		}
	}
	tui_mp_mods[0][0] = 0; // Main Bat
	tui_mp_mods[1][0] = 7; // Temp 0
	tui_mp_mods[2][0] = 1; // Sec Bat
	tui_mp_mods[3][0] = 2; // Relay State
	tui_draw_mainpage();
}


void tui_run(void) {
	uint8_t k = buttons_get();
	if (k & BUTTON_S2) {
		tui_mainmenu();
		tui_draw_mainpage();
		return;
	} else {
		if ((timer_get_1hzp())||(tui_force_draw))
			tui_draw_mainpage();
		return;
	}
}


void tui_activate(void) {
	tui_force_draw=1;
}
