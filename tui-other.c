#include "main.h"
#include "backlight.h"
#include "timer.h"
#include "tui.h"
#include "lcd.h"
#include "lib.h"
#include "buttons.h"
#include "dallas.h"
#include "rtc.h"
#include "adc.h"
#include "i2c.h"
#include "i2c-uart.h"
#include "cron.h"

// I2C UART test menu start

const unsigned char tui_um_s2[] PROGMEM = "INIT UART";
const unsigned char tui_um_s3[] PROGMEM = "VIEW RX";
const unsigned char tui_um_s4[] PROGMEM = "TX HELLO";
const unsigned char tui_um_s5[] PROGMEM = "CHECK PRESENSE";

PGM_P const tui_um_table[] PROGMEM = {
    (PGM_P)tui_um_s2, // init
    (PGM_P)tui_um_s3, // rx view
    (PGM_P)tui_um_s4, // tx hello
    (PGM_P)tui_um_s5, // tx hello
    (PGM_P)tui_exit_menu, // exit
};

static uint8_t tui_hexbyte_printer(unsigned char*buf, int32_t v) {
	buf[0] = '0';
	buf[1] = 'x';
	uchar2xstr(buf+2,v);
	return 4;
}

const uint24_t baud_table[] PROGMEM = { 9600, 19200, 38400, 57600, 115200 };

static uint8_t tui_baud_printer(unsigned char*buf, int32_t v) {
	uint24_t v2 = (uint24_t)(pgm_read_dword(&(baud_table[v])));
	return luint2str(buf,v2);
}

const unsigned char tui_dm_s7[] PROGMEM = "I2C UART TESTS";
static void tui_i2cuart_menu(void) {
	static uint8_t i2cuart_addr = 0x98;
	uint8_t sel=0;
	for (;;) {
		sel = tui_gen_listmenu((PGM_P)tui_dm_s7, tui_um_table, 5, sel);
		switch (sel) {
			default: 
				return;
			case 0:{
				// Init UART
				unsigned char buf[12];
				i2cuart_addr = tui_gen_adjmenu(PSTR("I2C ADDR"),tui_hexbyte_printer,0x00,0xFF,i2cuart_addr,2);
				uint8_t baudsel = tui_gen_adjmenu(PSTR("BAUD RATE"),tui_baud_printer,0,4,0,1);
				uint24_t baudrate = (uint24_t)(pgm_read_dword(&baud_table[baudsel]));
				uint16_t rv = i2cuart_init(i2cuart_addr,baudrate);
				lcd_clear();
				luint2str(buf,rv);
				lcd_gotoxy(2,0);
				lcd_puts_P(PSTR("POLL PERIOD:"));
				lcd_gotoxy((LCD_MAXX-strlen((char*)buf))>>1,1);
				lcd_puts(buf);
				tui_waitforkey();
				}
				break;
			case 1:{
				// RX Viewer
				uint8_t x = 0;
				lcd_clear();
				uint8_t lcdx = 0;
				uint8_t lcdy = 0;
				while (!x) {
					uint8_t r = i2cuart_poll_rx(i2cuart_addr,NULL);
					if (r) {
						uint8_t c=' ';
						i2cuart_readfifo(i2cuart_addr,&c,1);
						if (c==0xd) {
							lcdy ^= 1;
							lcdx = 0;
							lcd_gotoxy(lcdx,lcdy);
						} else {
							lcd_putchar(c);
							lcdx++;
							if (lcdx>=LCD_MAXX) {
								lcdy ^= 1;
								lcdx = 0;
								lcd_gotoxy(lcdx,lcdy);
							}
						}
					}
					timer_set_waiting();
					x = buttons_get();
					mini_mainloop();
				}
				}
				break;
			case 2:{ // TX Hello
				unsigned char buf[16];
				strcpy_P((char*)buf,PSTR("Hello World!\r\n"));
				i2cuart_writefifo(i2cuart_addr,buf,14);
				}
				break;
			case 3:{ // Check Presence
				PGM_P l1 = PSTR("I2C-UART IS");
				if (i2cuart_exists(i2cuart_addr)) {
					tui_gen_message(l1,PSTR("PRESENT"));
				} else {
					tui_gen_message(l1,PSTR("NOT PRESENT"));
				}
				}
				break;
		}
	}
}

const unsigned char tui_dm_s6[] PROGMEM = "I2C SCAN";

void tui_i2c_scan(void) {
	unsigned char buf[5];
	buf[4] = 0;
	for (uint16_t a=0;a<0x100;a+=2) {
		uint8_t v = i2c_start(a);
		if (v==0) {
			i2c_stop();
			tui_hexbyte_printer(buf,a);
			tui_gen_message_m(PSTR("FOUND DEVICE:"),buf);
		}
	}
	tui_gen_message((PGM_P)tui_dm_s6,PSTR("END OF LIST"));
}

// Debug Info Menu start
const unsigned char tui_dm_s1[] PROGMEM = "UPTIME";
const unsigned char tui_dm_s2[] PROGMEM = "RTC INFO";
const unsigned char tui_dm_s3[] PROGMEM = "ADC SAMPLES/S";
const unsigned char tui_dm_s4[] PROGMEM = "5HZ COUNTER";
const unsigned char tui_dm_s5[] PROGMEM = "RAW ADC VIEW";
PGM_P const tui_dm_table[] PROGMEM = {
    (PGM_P)tui_dm_s1, // uptime
    (PGM_P)tui_dm_s2, // rtc info
    (PGM_P)tui_dm_s3, // adc samples
    (PGM_P)tui_dm_s4, // 5hz counter
    (PGM_P)tui_dm_s5, // Raw ADC view
    (PGM_P)tui_dm_s6, // I2C SCAN
    (PGM_P)tui_dm_s7, // I2C UART
    (PGM_P)tui_exit_menu, // exit
};

void tui_time_print(uint32_t nt) {
	unsigned char time[16];
	// Time Format: "DDDDDd HH:MM:SS"
	uint16_t days;
	uint8_t hours, mins, secs, x,z;
	days = nt / 86400L; nt = nt % 86400L;
	hours = nt / 3600L; nt = nt % 3600L;
	mins = nt / 60; 
	secs = nt % 60;

	time[0] = (days/10000)|0x30; days = days % 10000;
	time[1] = (days /1000)|0x30; days = days % 1000;
	time[2] = (days / 100)|0x30; days = days % 100;
	time[3] = (days / 10 )|0x30;
	time[4] = (days % 10 )|0x30;
	time[5] = 'd';
	time[6] = ' ';
	time[7] = (hours/10) | 0x30;
	time[8] = (hours%10) | 0x30;
	time[9] = ':';
	time[10]= (mins /10) | 0x30;
	time[11]= (mins %10) | 0x30;
	time[12]= ':';
	time[13]= (secs /10) | 0x30;
	time[14]= (secs %10) | 0x30;
	time[15] = 0;
	z=0;
	if (time[0] == '0') {
		z=1;
		if (time[1] == '0') {
			z=2;
			if (time[2] == '0') {
				z=3;
				if (time[3]  == '0') {
					z=4;
					if (time[4] == '0') {
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
	lcd_puts(&(time[z]));
}

static void tui_uptime(void) {
	uint8_t x;
	lcd_clear();
	lcd_gotoxy(5,0);
	lcd_puts_P((PGM_P)tui_dm_s1);
	for (;;) {
		tui_time_print(timer_get());
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

static void tui_raw_adc_view(void) {
	unsigned char buf[10];
	for (;;) {
		uint8_t x;
		lcd_clear();
		uint2str(buf,adc_raw_values[0]);
		lcd_puts(buf);
		adc_print_v(buf,adc_raw_values[0]);
		lcd_gotoxy(10,0);
		lcd_puts(buf);
		lcd_gotoxy(0,1);
		uint2str(buf,adc_raw_values[1]);
		lcd_puts(buf);
		adc_print_v(buf,adc_raw_values[1]);
		lcd_gotoxy(10,1);
		lcd_puts(buf);
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

// Voltage: 13.63V
// A: 3515
// B: 3507
// Raw 13.63V is 3489,28
// Thus MB_SCALE = 3489,28 / 3515 * 65536 => 65056,45919
#define ADC_MB_SCALE 65056
// SB_SCALE = 3489,28 / 3507 * 65536 => 65204,86284
#define ADC_SB_SCALE 65205
// Calib is 65536+diff, thus saved is diff = calib - 65536.
//int16_t adc_calibration_diff[ADC_MUX_CNT] = { ADC_MB_SCALE-65536, ADC_SB_SCALE-65536 };

/* Used from tui.c / settings menu */
void tui_adc_calibrate(void) {
	const uint16_t min_calib_v = 6*256; // Min 6V on a channel to calibrate it.
	uint16_t target;
	uint16_t mcv = adc_read_mb();
	uint16_t scv = adc_read_sb();
	if ((mcv>min_calib_v)&&(scv>min_calib_v)) {
		target = (mcv+scv)/2;
	} else if (mcv>min_calib_v) {
		target = mcv;
	} else if (scv>min_calib_v) {
		target = scv;
	} else {
		tui_gen_message(PSTR("INVALID VOLTAGE"),PSTR("VALUES; <6V"));
		return;
	}
	uint16_t dV_target = adc_to_dV(target);
	uint32_t mbc=0;
	uint32_t sbc=0;
	for(;;) {
		uint8_t buf[10];
		uint8_t x;
		if (adc_raw_values[0]>min_calib_v) { // Generate MB calib value
			uint16_t v = adc_raw_values[0];
			mbc = ((((uint32_t)target)*65536UL)+(v/2))/v;
			if ((mbc<33000)||(mbc>98000)) mbc=0;
		}
		if (adc_raw_values[1]>min_calib_v) { // Generate SB calib value
			uint16_t v = adc_raw_values[1];
			sbc = ((((uint32_t)target)*65536UL)+(v/2))/v;
			if ((sbc<33000)||(sbc>98000)) sbc=0;
		}
		lcd_clear();
		buf[0] = 'M';
		buf[1] = ':';
		luint2str(buf+2,mbc);
		lcd_puts(buf);
		lcd_gotoxy(0,1);
		buf[0] = 'S';
		luint2str(buf+2,sbc);
		lcd_puts(buf);
		adc_print_dV(buf,dV_target);
		lcd_gotoxy(10,0);
		lcd_puts(buf);
		for (;;) {
			x = buttons_get();
			mini_mainloop();
			if (x) break;
			if (timer_get_1hzp()) 
				break;
		}
		switch (x) {
			default: break;
			case BUTTON_S1:
				dV_target++;
				if (dV_target>1600) dV_target = 800;
				target = adc_from_dV(dV_target);
				x = 0;
				break;
			case BUTTON_S2:
				dV_target--;
				if (dV_target<800) dV_target = 1600;
				target = adc_from_dV(dV_target);
				x = 0;
				break;
		}
		if (x) break;
	}
	PGM_P gts = PSTR("GOING TO SAVE");
	if (mbc) {
		tui_gen_message(gts,PSTR("MB ADC CALIB"));
		if (tui_are_you_sure()) {
			int32_t v = mbc;
			v = v - 65536;
			adc_calibration_diff[0] = v;
		}
	}
	if (sbc) {
		tui_gen_message(gts,PSTR("SB ADC CALIB"));
		if (tui_are_you_sure()) {
			int32_t v = sbc;
			v = v - 65536;
			adc_calibration_diff[1] = v;
		}
	}
}

const unsigned char tui_om_s5[] PROGMEM = "DEBUG INFO";
static void tui_debuginfomenu(void) {
	uint8_t sel=0;
	for (;;) {
		sel = tui_gen_listmenu((PGM_P)tui_om_s5, tui_dm_table, 8, sel);
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

			case 4:
				tui_raw_adc_view();
				break;

			case 5:
				tui_i2c_scan();
				break;

			case 6:
				tui_i2cuart_menu();
				break;
			default:
				return;
		}
	}
}
// Useful tools start

static volatile uint16_t stopwatch_timer=0;
void stopwatch_taskf(void) {
	uint16_t timer = stopwatch_timer;
	timer += 1;
	if (timer>=60000) timer=0;
	stopwatch_timer = timer;
	timer_set_waiting();
}

const unsigned char tui_om_s2[] PROGMEM = "STOPWATCH";
static void tui_stopwatch(void) {
	unsigned char time[8];
	stopwatch_timer = 0;
	struct cron_task stopwatch_task = { NULL, stopwatch_taskf, SSTC/10, 0 };
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
	cron_add_task(&stopwatch_task);
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
		uint16_t timer = stopwatch_timer;
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
	cron_rm_task(&stopwatch_task);
	tui_waitforkey();
}

const unsigned char tui_om_s1[] PROGMEM = "CALC";
// StopWatch
const unsigned char tui_om_s3[] PROGMEM = "FUEL COST";
const unsigned char tui_om_s4[] PROGMEM = "FC HISTORY";
// Debug Info
const unsigned char tui_om_s6[] PROGMEM = "POWER OFF";

PGM_P const tui_om_table[] PROGMEM = {
    (PGM_P)tui_om_s1, // calc
    (PGM_P)tui_om_s2, // stopwatch
    (PGM_P)tui_om_s3, // fc
    (PGM_P)tui_om_s4, // fc history
    (PGM_P)tui_om_s5, // debug info
    (PGM_P)tui_om_s6, // power off
    (PGM_P)tui_exit_menu, // exit
};

void tui_othermenu(void) {
	uint8_t sel=0;	
	for (;;) {
		sel = tui_gen_listmenu(PSTR("OTHERS"), tui_om_table, 7, sel);
		switch (sel) {
			default:
				return;
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
				tui_poweroff();
				break;
		}
	}
}
