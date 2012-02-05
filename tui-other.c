#include "main.h"
#include "backlight.h"
#include "timer.h"
#include "tui.h"
#include "lcd.h"
#include "lib.h"
#include "buttons.h"
#include "dallas.h"
#include "rtc.h"

// Debug Info Menu start	


const unsigned char tui_dm_s1[] PROGMEM = "UPTIME";
const unsigned char tui_dm_s2[] PROGMEM = "RTC INFO";
const unsigned char tui_dm_s3[] PROGMEM = "ADC SAMPLES/S";
const unsigned char tui_dm_s4[] PROGMEM = "5HZ COUNTER";
PGM_P const tui_dm_table[] PROGMEM = {
    (PGM_P)tui_dm_s1, // uptime
    (PGM_P)tui_dm_s2, // rtc info
    (PGM_P)tui_dm_s3, // adc samples
    (PGM_P)tui_dm_s4, // 5hz counter
    (PGM_P)tui_exit_menu, // exit
};


static void tui_uptime(void) {
	unsigned char uptime[16];
	// Uptime Format: "DDDDDd HH:MM:SS"
	uint16_t days;
	uint8_t hours, mins, secs, x,z;
	lcd_clear();
	lcd_gotoxy(4,0);
	lcd_puts_P((PGM_P)tui_dm_s1);
	for (;;) {
		uint32_t nt = timer_get();
		days = nt / 86400L; nt = nt % 86400L;
		hours = nt / 3600L; nt = nt % 3600L;
		mins = nt / 60; 
		secs = nt % 60;

		uptime[0] = (days/10000)|0x30; days = days % 10000;
		uptime[1] = (days /1000)|0x30; days = days % 1000;
		uptime[2] = (days / 100)|0x30; days = days % 100;
		uptime[3] = (days / 10 )|0x30;
		uptime[4] = (days % 10 )|0x30;
		uptime[5] = 'd';
		uptime[6] = ' ';
		uptime[7] = (hours/10) | 0x30;
		uptime[8] = (hours%10) | 0x30;
		uptime[9] = ':';
		uptime[10]= (mins /10) | 0x30;
		uptime[11]= (mins %10) | 0x30;
		uptime[12]= ':';
		uptime[13]= (secs /10) | 0x30;
		uptime[14]= (secs %10) | 0x30;
		uptime[15] = 0;
		z=0;
		if (uptime[0] == '0') {
			z=1;
			if (uptime[1] == '0') {
				z=2;
				if (uptime[2] == '0') {
					z=3;
					if (uptime[3]  == '0') {
						z=4;
						if (uptime[4] == '0') {
							z = 7;
							if (hours == 0) {
								z = 10;
							}
						}
					}
				}
			}
		}
		x = (16 - (15-z)) / 2;
		
		lcd_gotoxy(x,1);
		lcd_puts(&(uptime[z]));
		for (;;) {
			x = buttons_get();
			mini_mainloop();
			if (x) break;
			if (timer_get_1hzp()) 
				break;
		}
		if (x) break;
	}
}

static void tui_adc_ss(void) {
	unsigned char buf[10];
	extern uint16_t adc_avg_cnt;
	for (;;) {
		uint8_t x;
		lcd_clear();
		uint2str(buf,adc_avg_cnt);
		lcd_puts(buf);
		for(;;) {
			x = buttons_get();
			mini_mainloop();
			if (x) break;
			if (timer_get_1hzp()) break;
		}
		if (x) break;
	}
}

static void tui_timer_5hzcnt(void) {
	unsigned char buf[10];
	for (;;) {
		uint8_t x;
		uint8_t timer=timer_get_5hz_cnt();
		lcd_clear();
		uchar2str(buf,timer);
		lcd_puts(buf);
		while (timer==timer_get_5hz_cnt()) {
			x = buttons_get();
			mini_mainloop();
			if (x) return;
		}
	}
}	

const unsigned char tui_om_s5[] PROGMEM = "DEBUG INFO";
static void tui_debuginfomenu(void) {
	uint8_t sel=0;	
	for (;;) {
		sel = tui_gen_listmenu((PGM_P)tui_om_s5, tui_dm_table, 5, sel);
		switch (sel) {
			case 0:
				tui_uptime();
				break;
			case 1: {
				PGM_P l1 = PSTR("RTC IS");
				if (rtc_valid()) {
					tui_gen_message(l1,PSTR("VALID"));
				} else {
					tui_gen_message(l1,PSTR("INVALID"));
				}
				}
				break;
			case 2: 
				tui_adc_ss();
				break;
			case 3:
				tui_timer_5hzcnt();
				break;
			default:
				return;
		}
	}
}
// Useful tools start

const unsigned char tui_om_s2[] PROGMEM = "STOPWATCH";
static void tui_stopwatch(void) {
	uint16_t timer=0;
	unsigned char time[8];
	// Format: mm:ss.s
	lcd_clear();
	lcd_gotoxy(3,0);
	lcd_puts_P((PGM_P)tui_om_s2);
	for(;;) {
		mini_mainloop();
		if (!buttons_get_v()) break;
	}
	timer_delay_ms(100);
	for(;;) {
		mini_mainloop();
		if (buttons_get_v()) break;
	}
	uint8_t last_5hztimer = timer_get_5hz_cnt();
	time[6] = time[4] = time[3] = time[1] = time[0] = '0';
	time[2] = ':';
	time[5] = '.';
	time[7] = 0;
	lcd_gotoxy(4,1);
	lcd_puts(time);
	timer_delay_ms(150);
	for(;;) {
		mini_mainloop();
		if (timer_get_1hzp()) backlight_activate(); // Keep backlight on
		uint16_t tt,t2;
		uint8_t passed = timer_get_5hz_cnt() - last_5hztimer;
		timer += passed*2;
		last_5hztimer += passed;
		if (timer>=60000) timer=0;
		time[6] = 0x30 | (timer%10);
		tt = timer/10;
		t2 = tt%60;
		tt = tt/60;
		time[4] = 0x30 | (t2%10);
		time[3] = 0x30 | (t2/10);
		time[1] = 0x30 | (tt%10);
		time[0] = 0x30 | (tt/10);
		lcd_gotoxy(4,1);
		lcd_puts(time);
		if (buttons_get_v()) break;
	}
	tui_waitforkey();
}

const unsigned char tui_om_s1[] PROGMEM = "CALC";
// StopWatch
const unsigned char tui_om_s3[] PROGMEM = "FUEL COST";
const unsigned char tui_om_s4[] PROGMEM = "FC HISTORY";
// Debug Info

PGM_P const tui_om_table[] PROGMEM = {
    (PGM_P)tui_om_s1, // calc
    (PGM_P)tui_om_s2, // stopwatch
    (PGM_P)tui_om_s3, // fc
    (PGM_P)tui_om_s4, // fc history
    (PGM_P)tui_om_s5, // debug info	
    (PGM_P)tui_exit_menu, // exit
};

void tui_othermenu(void) {
	uint8_t sel=0;	
	for (;;) {
		sel = tui_gen_listmenu(PSTR("OTHERS"), tui_om_table, 6, sel);
		switch (sel) {
			default:
			case 0:
				tui_calc();
				break;
			case 1:
				tui_stopwatch();
				break;
			case 2: 
				tui_calc_fuel_cost();
				break;
			case 3:
				tui_calc_fc_history();
				break;
			case 4:
				tui_debuginfomenu();
				break;
			case 5:
				return;
		}
	}
}