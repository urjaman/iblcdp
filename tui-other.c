#include "main.h"
#include "timer.h"
#include "tui.h"
#include "lcd.h"
#include "lib.h"
#include "buttons.h"
#include "dallas.h"


static void tui_uptime(void) {
	unsigned char uptime[16];
	// Uptime Format: "DDDDDd HH:MM:SS"
	uint16_t days;
	uint8_t hours, mins, secs, x,z;
	lcd_clear();
	lcd_gotoxy(4,0);
	lcd_puts_P(PSTR("UPTIME:"));
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
const unsigned char tui_om_s3[] PROGMEM = "STOPWATCH";
static void tui_stopwatch(void) {
	uint16_t timer=0;
	unsigned char time[8];
	// Format: mm:ss.s
	lcd_clear();
	lcd_gotoxy(3,0);
	lcd_puts_P((PGM_P)tui_om_s3);
	for(;;) {
		mini_mainloop();
		if (buttons_get_v()) break;
	}
	time[6] = time[4] = time[3] = time[1] = time[0] = '0';
	time[2] = ':';
	time[5] = '.';
	time[7] = 0;
	lcd_gotoxy(4,1);
	lcd_puts(time);
	_delay_ms(150);
	for(;;) {
		mini_mainloop();
		if (timer_get_5hzp()) {
			uint16_t tt,t2;
			timer += 2;
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
		}
		if (buttons_get_v()) break;
	}
	tui_waitforkey();
}

const unsigned char tui_om_s1[] PROGMEM = "CALC";
const unsigned char tui_om_s2[] PROGMEM = "UPTIME";

const unsigned char tui_om_s4[] PROGMEM = "1WR";
const unsigned char tui_om_s5[] PROGMEM = "EXIT MENU";
PGM_P const tui_om_table[] PROGMEM = {
    (PGM_P)tui_om_s1,
    (PGM_P)tui_om_s2,
    (PGM_P)tui_om_s3,
    (PGM_P)tui_om_s4,
    (PGM_P)tui_om_s5,
};

void tui_othermenu(void) {
	uint8_t sel=0;	
	for (;;) {
		sel = tui_gen_listmenu(PSTR("OTHERS"), tui_om_table, 5, sel);
		switch (sel) {
			default:
			case 0:
				tui_calc();
				break;
			case 1:
				tui_uptime();
				break;
			case 2:
				tui_stopwatch();
				break;
			case 3:
			//	dallas_reset();
				break;
			case 4: 
				return;
		}
	}
}