#include "main.h"
#include "tui.h"
#include "lcd.h"
#include "timer.h"
#include "dallas.h"
#include "lib.h"

/* DEFAULT: At -30C 4s, at 5C 1s, linear change between. */
int16_t tui_temprefresh_lp_temp=-300;
int16_t tui_temprefresh_hp_temp=50;
uint8_t tui_temprefresh_lp_interval=20;
uint8_t tui_temprefresh_hp_interval=5;

uint8_t tui_update_refresh_interval(void) {
	int16_t temp = dallas_temp_get(0);
	if ((temp < -550)||(temp >= 850)) return TUI_DEFAULT_REFRESH_INTERVAL;
	if (temp<tui_temprefresh_lp_temp) return tui_temprefresh_lp_interval;
	if (temp>tui_temprefresh_hp_temp) return tui_temprefresh_hp_interval;
	uint8_t int_base,int_count;
	int_base = tui_temprefresh_hp_interval;
	int_count = tui_temprefresh_lp_interval-int_base;
	int16_t temp_base,temp_count;
	temp_base = tui_temprefresh_lp_temp;
	temp_count = tui_temprefresh_hp_temp-temp_base;
	temp = temp - temp_base; // Now temp should be >=0.
	uint16_t int_val = (((uint16_t)temp*(uint16_t)int_count*2)+1) / (((uint16_t)temp_count)*2);
	if (int_val>int_count) int_val = int_count;
	int_val = int_count - int_val; // invert
	return (uint8_t)(int_val+int_base);
}

static uint8_t tui_print_refresh_interval(unsigned char* buf, int32_t interval) {
	uint8_t off = uchar2str(buf,interval/5);
	uint8_t leftover = (interval%5)*2;
	buf[off] = '.';
	buf[off+1] = '0' + leftover;
	buf[off+2] = 's';
	buf[off+3] = 0;
	return off+3;
}

static void tui_show_current_refresh_interval(void) { // This should be genericised when an another place needs to show a constant header + ram string buffer.
	unsigned char buf[16];
	lcd_clear();
	lcd_gotoxy(2,0);
	lcd_puts_P(PSTR("INTERVAL NOW:"));
	tui_print_refresh_interval(buf,tui_update_refresh_interval());
	lcd_gotoxy((16 - strlen((char*)buf))/2,1);
	lcd_puts(buf);
	timer_delay_ms(100);
	tui_waitforkey();
}

static const unsigned char tui_rim_s1[] PROGMEM = "SHOW CURRENT";
static const unsigned char tui_rim_s2[] PROGMEM = "SET HITEMP RATE";
static const unsigned char tui_rim_s3[] PROGMEM = "SET LOTEMP RATE";
static const unsigned char tui_rim_s4[] PROGMEM = "SET HIGH TEMP";
static const unsigned char tui_rim_s5[] PROGMEM = "SET LOW TEMP";


PGM_P const tui_rim_table[] PROGMEM = {
    (PGM_P)tui_rim_s1,
    (PGM_P)tui_rim_s2,
    (PGM_P)tui_rim_s3,
    (PGM_P)tui_rim_s4,
    (PGM_P)tui_rim_s5,
    (PGM_P)tui_exit_menu
};

int16_t tui_gen_tempmenu(PGM_P header, int16_t start, int16_t min, int16_t max) {
	return (int16_t)tui_gen_adjmenu(header,tui_temp_printer,min<-550?-550:min,max>845?845:max,start,5);
}

uint8_t tui_gen_intvmenu(PGM_P header, uint8_t start, uint8_t min, uint8_t max) {
	return (uint8_t)tui_gen_adjmenu(header,tui_print_refresh_interval,min,max,start,1);
}

void tui_refresh_interval_menu(void) {
	uint8_t sel = 0;
	for(;;) {
		sel = tui_gen_listmenu((PGM_P)tui_update_rate_cfg, tui_rim_table, 6, sel);
		switch (sel) {
			case 0:
				tui_show_current_refresh_interval();
				break;
			case 1: 
				tui_temprefresh_hp_interval = 
					tui_gen_intvmenu((PGM_P)tui_rim_s2, 
					tui_temprefresh_hp_interval, 1, tui_temprefresh_lp_interval-1);
				break;
			case 2:
				tui_temprefresh_lp_interval = 
					tui_gen_intvmenu((PGM_P)tui_rim_s3, 
					tui_temprefresh_lp_interval, tui_temprefresh_hp_interval+1, 60);
				break;
			case 3:
				tui_temprefresh_hp_temp =
					tui_gen_tempmenu((PGM_P)tui_rim_s4,
					tui_temprefresh_hp_temp, tui_temprefresh_lp_temp+5, 845);
				break;
			case 4:
				tui_temprefresh_lp_temp = 
					tui_gen_tempmenu((PGM_P)tui_rim_s5,
					tui_temprefresh_lp_temp, -550, tui_temprefresh_hp_temp-5);
				break;
				
			default:
				return;
		}
	}	
}
