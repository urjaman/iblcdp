#include "main.h"
#include "tui.h"
#include "lib.h"
#include "lcd.h"
#include "time.h"
#include "timer.h"
#include "buttons.h"

static void tui_calc_printno(unsigned char* buf, uint32_t num, uint8_t base, uint8_t dpts) {
	uint8_t x;
	if (base==16) {
		luint2xstr(buf,(uint32_t)num);
		x = strlen((char*)buf);
		buf[x] = 'H';
		buf[x+1] = 0;
	} else {
		//uint8_t neg = (num<0)?1:0;
		uint8_t neg = 0;
		luint2str(&(buf[neg]),abs(num));
		if (neg) buf[0] = '-';
		// buf="123" or "0"
		if (dpts) {
			x = strlen((char*)buf);
			if ((x-(1+neg))<dpts) {
				uint8_t i,y;
				y = dpts - (x-(1+neg));
				i = x;
				do { buf[i+y] = buf[i]; } while ((i--) != neg);
				for(i=neg;i<(y+neg);i++) buf[i] = '0';
				x += y;
			}
			// buf="123" or "000"
			uint8_t i=x,y=x-(dpts+1);
			do { buf[i+1] = buf[i]; } while ((i--) != y);
			buf[x-dpts] = '.';
			// buf="1.23" or "0.00"
		}
	}
}

static void tui_gen_calcheader(unsigned char* line, unsigned char* buf, 
				PGM_P header, uint32_t number, uint8_t base, uint8_t dpts) {
	uint8_t y;
	strcpy_P((char*)line, header);
	line[16] = 0;
	y = strlen((char*)line);
	line[y] = ' '; y++;
	tui_calc_printno(buf,number,base,dpts);
	strcpy((char*)&(line[y]), (char*)buf);
	lcd_clear();
	lcd_puts(line);
}


static uint8_t tui_gen_calcmenu(PGM_P header, uint32_t number, uint8_t base, PGM_P const menu_table[], 
				uint8_t itemcnt, uint8_t start, uint8_t dpts) {
	unsigned char line[17];
	unsigned char buf[17];
	tui_gen_calcheader(line,buf,header,number,base,dpts);
	return tui_pgm_menupart(line,buf,menu_table,itemcnt,start);
}



static const unsigned char tui_cmm_s1[] PROGMEM = "CHANGE BASE";
static const unsigned char tui_cmm_s2[] PROGMEM = "SWAP NUMBERS";
static const unsigned char tui_cmm_s3[] PROGMEM = "BACK...";
static const unsigned char tui_cmm_s4[] PROGMEM = "SET DEC PTS";

PGM_P const tui_cmm_table[] PROGMEM = {
    (PGM_P)tui_cmm_s1,
    (PGM_P)tui_cmm_s2,
    (PGM_P)tui_cmm_s3,
    (PGM_P)tui_cmm_s4 
};

static const unsigned char tui_cm_s1[] PROGMEM = "ADD NUMBER";
static const unsigned char tui_cm_s2[] PROGMEM = "CONTINUE";
static const unsigned char tui_cm_s3[] PROGMEM = "MORE...";
static const unsigned char tui_cm_s4[] PROGMEM = "DEL NUMBER";

PGM_P const tui_cm_table[] PROGMEM = {
    (PGM_P)tui_cm_s1,
    (PGM_P)tui_cm_s2,
    (PGM_P)tui_cm_s3,
    (PGM_P)tui_cm_s4
};


struct tcalcstate {
	uint32_t n1;
	uint32_t n2;
	uint8_t base;
	uint8_t dpts;
};

static void tui_calc_alt_entry(PGM_P header, struct tcalcstate *s, uint32_t min, uint32_t max, uint32_t speed1, uint32_t speed2, uint32_t spd_step1,uint32_t spd_step2) {
	unsigned char buf[17];
	unsigned char line[17];
	tui_gen_menuheader(line,buf,header);
	uint32_t speed=1;
	uint32_t speed_step=spd_step1;
	uint32_t num = s->n1;
	for(;;) {
		uint8_t z,y,key;
		tui_calc_printno(buf,num,10,s->dpts);
		memset(line,' ',16);
		y = strlen((char*)buf);
		z = (16 - y) / 2;
		memcpy(&(line[z]),buf,y);
		lcd_gotoxy(0,1);
		lcd_puts(line);
rekey:
		key = tui_pollkey();
		switch (key) {
			case BUTTON_NONE:
				speed=1;
				speed_step=spd_step1;
				goto rekey;
			
			case BUTTON_S1:
				if ((num+speed)>max) num = max;
				else num += speed;
				break;
			
			case BUTTON_S2:
				if (((num-speed)<min)||((num-speed)>max)) num = min;
				else num -= speed;
				break;
			
			case BUTTON_BOTH:
				s->n1 = num;
				return;
		
		}
		if (key) {
			speed_step--;
			if (speed_step==0) {
				speed_step = spd_step2;
				if (speed>1) {
					speed = speed2;
				} else {
					speed = speed1;
				}
			}
		}
	}
}

static void tui_calc_entry(struct tcalcstate *s, uint8_t no) {
	uint8_t ss=0;
	uint8_t sel=0;
	PGM_P nop;
	uint32_t *np;
	if (no==2) {
		np = &(s->n2);
		nop = PSTR("N2:");
	} else {
		np = &(s->n1);
		nop = PSTR("N1:");
	}
	for(;;) {
		sel = tui_gen_calcmenu(nop, *np, s->base, tui_cm_table, 4, sel, s->dpts);
		switch (sel) {
			default:
			case 0:
				{
					uint32_t tmp = *np;
					uint8_t v = tui_gen_nummenu((PGM_P)tui_cm_s1, 0, s->base-1, 0);
					tmp = (tmp*s->base)+v;
					*np = tmp;
				}
				break;
			case 1:
				return;
			case 3:
				*np = *np/s->base;
				break;
			case 2: 
				ss = tui_gen_calcmenu(nop,*np,s->base,tui_cmm_table,(s->base==10)?4:3,ss,s->dpts);
				switch (ss) {
					case 0:
						if (s->base==10) {
							s->base = 16;
						} else {
							s->base = 10;
						}
						break;
					case 1: {
							uint32_t tmp=s->n1;
							s->n1 = s->n2;
							s->n2 = tmp;
						}
						break;
					case 2: 
						ss=0;sel=0;
						break;
					case 3: 
						s->dpts = tui_gen_nummenu((PGM_P)tui_cmm_s4, 0, 4, s->dpts);
						break;
				}
				break;
		}
	}
}

static const unsigned char tui_com_s1[] PROGMEM = "+ (PLUS)";
static const unsigned char tui_com_s2[] PROGMEM = "- (MINUS)";
static const unsigned char tui_com_s3[] PROGMEM = "* (MULTIPLY)";
static const unsigned char tui_com_s4[] PROGMEM = "/ (DIVIDE)";
static const unsigned char tui_com_s5[] PROGMEM = "% (REMAINDER)";
static const unsigned char tui_com_s6[] PROGMEM = "& (AND)";
static const unsigned char tui_com_s7[] PROGMEM = "| (OR)";
static const unsigned char tui_com_s8[] PROGMEM = "^ (XOR)";

PGM_P const tui_com_table[] PROGMEM = {
    (PGM_P)tui_com_s1,
    (PGM_P)tui_com_s2,
    (PGM_P)tui_com_s3,
    (PGM_P)tui_com_s4,
    (PGM_P)tui_com_s5,
    (PGM_P)tui_com_s6,
    (PGM_P)tui_com_s7,
    (PGM_P)tui_com_s8
};

static void tui_do_calc_op(struct tcalcstate *s, uint8_t op) {
	uint8_t dpts;
	uint32_t e=1;
	// auto-dpts support:
	if ((op==3)&&(s->dpts==0)&&(s->n1%s->n2)) {
		if ((labs(s->n1)<4294)&&(labs(s->n2)<4294967)) {
		s->dpts = 3;
		s->n1 *= 1000;
		s->n2 *= 1000;
		}
	}
	dpts = s->dpts;
	if (s->base==10) while (dpts--) e*=10;
	switch (op) {
		case 0: s->n1 = s->n1 + s->n2; break;
		case 1: s->n1 = s->n1 - s->n2; break;
		case 2: s->n1 = (s->n1 * s->n2) / e; break;
		case 3: s->n1 = (s->n1 * e) / s->n2; break;
		case 4: s->n1 = s->n1 % s->n2; break;
		case 5: s->n1 = s->n1 & s->n2; break;
		case 6: s->n1 = s->n1 | s->n2; break;
		case 7: s->n1 = s->n1 ^ s->n2; break;
	}
}


static void tui_show_calc_result(struct tcalcstate*s, PGM_P header, PGM_P unit) {
	unsigned char buf[16];
	uint8_t x = strlen_P(header);
	lcd_clear();
	lcd_gotoxy((16-x)>>1,0);
	lcd_puts_P(header);
	tui_calc_printno(buf,s->n1,s->base,s->dpts);
	strcat_P((char*)buf,unit);
	lcd_gotoxy((16-strlen((char*)buf))/2,1);
	lcd_puts(buf);
	tui_waitforkey();
}

void tui_calc(void) {
	static uint32_t n1r=0;
	static uint8_t dptsr=0;
	static uint8_t baser=10;
	struct tcalcstate s = { n1r, 0, baser, dptsr };
	uint8_t op;

	tui_calc_entry(&s,1);
	tui_calc_entry(&s,2);
	op = tui_gen_listmenu(PSTR("PICK OPERATION"), tui_com_table, ((s.base==10)&&(s.dpts))?4:8, 0);
	
	tui_do_calc_op(&s,op);
	dptsr = s.dpts;
	n1r = s.n1;
	baser = s.base;
	tui_show_calc_result(&s, PSTR("RESULT:"), PSTR(""));
}

// This is maximum supported by the FC history viewer. Also note that this is 800 bytes of RAM usage.
#define TUI_FC_HISTORY_SIZE 100

struct tui_fc_history_entry {
	uint16_t time_days; // Days in lintime (epoch 2000)
	uint16_t kilometers; // *10
	uint16_t fuel_price; // *1000
	uint16_t litres; // *100
};
// This is only kept during system runtime.
static uint8_t tui_fc_history_count=0;
static struct tui_fc_history_entry tui_fc_history[TUI_FC_HISTORY_SIZE];
static int16_t tui_fc_day_correction = 0;
static uint32_t tui_fc_time_baseline;
// This is saved by saver.c:
uint16_t tui_fc_last_fuel_price = 1500;
uint16_t tui_fc_last_fuel_efficiency = 800; // l/100km*100
uint16_t tui_fc_last_kilometres = 3000; // 300km*10

static void tui_calc_fuel_add_history(uint16_t time_days, uint16_t kilometers, uint16_t fuel_price, uint16_t litres) {
	if (tui_fc_history_count==TUI_FC_HISTORY_SIZE) { // Delete oldest
		uint8_t i;
		for(i=1;i<TUI_FC_HISTORY_SIZE;i++) {
			tui_fc_history[i-1] = tui_fc_history[i];
		}
		tui_fc_history_count = TUI_FC_HISTORY_SIZE-1;
	}
	tui_fc_history[tui_fc_history_count].time_days = time_days;
	tui_fc_history[tui_fc_history_count].kilometers = kilometers;
	tui_fc_history[tui_fc_history_count].fuel_price = fuel_price;
	tui_fc_history[tui_fc_history_count].litres = litres;
	tui_fc_history_count++;
}

static const uint8_t tui_eur_sign[8] PROGMEM = {
	0b00111,
	0b01000,
	0b11110,
	0b01000,
	0b11110,
	0b01000,
	0b00111,
	0b00000
};


uint16_t tui_confirm_lindate(PGM_P header, uint16_t lindate) {
	unsigned char line[17];
	unsigned char buf[17];
	uint16_t idx=lindate;
	uint8_t z,y;
	tui_gen_menuheader(line,buf,header);
	for (;;) {
		uint8_t key;
		linear_date_string(buf, ((uint32_t)idx)*86400);
		memset(line,' ',16);
		y = strlen((char*)buf);
		z = (16 - y) / 2;
		memcpy(&(line[z]),buf,y);
		lcd_gotoxy(0,1);
		lcd_puts(line);
		key = tui_waitforkey();
		switch (key) {
			case BUTTON_S1:
				idx++;
				break;
			case BUTTON_S2:
				idx--;
				break;
			case BUTTON_BOTH:
				return idx;
		}
	}	
}



void tui_calc_fuel_cost(void) {
	struct tcalcstate s = { 0,0,10,1 };
	uint32_t km = tui_fc_last_kilometres;
	uint32_t price = tui_fc_last_fuel_price;
	uint32_t efficiency = tui_fc_last_fuel_efficiency;
	uint32_t litres;
	// 0. Send euro sign to LCD ...
	lcd_program_char((PGM_P)tui_eur_sign, 0);
	// 1. Ask(/Verify) KM
	s.n1 = km;
	tui_calc_alt_entry(PSTR("Trip km:"),&s,200,65535,10,100,10,10);
	km = s.n1;
	// 2. Verify(/Ask) E/l
	s.dpts = 3;
	s.n1 = price;
	tui_calc_alt_entry(PSTR("\x08/l:"),&s,100,65535,10,10,10,10);
	price = s.n1;
	// 3. Verify(/Ask) l/100km
	s.dpts = 2;
	s.n1 = efficiency;
	tui_calc_alt_entry(PSTR("l/100km:"),&s,100,65535,10,10,10,10);
	efficiency = s.n1;
	tui_fc_last_kilometres = km;
	tui_fc_last_fuel_price = price;
	tui_fc_last_fuel_efficiency = efficiency;
	// 3.5. Calculate FC estimate
	litres = (km*efficiency)/1000;
	s.n1 = (litres*price)/1000;
	// 4. Show FC estimate
	tui_show_calc_result(&s,PSTR("FUEL COST:"),PSTR(" \x08"));
	// 5. Verify/Ask Litres
	s.n1 = litres;
	tui_calc_alt_entry(PSTR("Litres:"),&s,20,65535,10,100,10,10);
	litres = s.n1;
	// 6. Calculate and show fuel efficiency
	s.n1 = (litres*1000)/km;
	tui_show_calc_result(&s,PSTR("EFFICIENCY:"),PSTR(" l/100km"));
	// 7. Verify/Ask date
	uint16_t lindate;
	if (tui_fc_history_count==0) { // Initial date request
		struct mtm tm;
		uint16_t year = tui_gen_nummenu(PSTR("Year:"),TIME_EPOCH_YEAR,TIME_EPOCH_YEAR+254,TIME_EPOCH_YEAR);
		uint8_t month = tui_gen_nummenu(PSTR("Month:"),1,12,1);
		year = year - TIME_EPOCH_YEAR;
		tm.year = year;
		tm.month = month;
		uint8_t day = tui_gen_nummenu(PSTR("Day:"),1,month_days(year,month-1),1);
		tm.day = day;
		uint8_t h,m,s;
		timer_get_time24(&h,&m,&s);
		tm.hour = h;
		tm.min = m;
		tm.sec = s;
		uint32_t linear_now = mtm2linear(&tm);
		tui_fc_time_baseline = linear_now - timer_get();
		lindate = linear_now/86400;
	} else {
		lindate = (timer_get()+tui_fc_time_baseline)/86400;
		uint16_t user_lindate = tui_confirm_lindate(PSTR("Date:"),lindate+tui_fc_day_correction);
		tui_fc_day_correction = user_lindate - lindate;
		lindate = user_lindate;
	}
	tui_calc_fuel_add_history(lindate,km,price,litres);
}

// History Screen:
// 0123456789012345
// NN:YYMMDD  1.504
// 1600.0KM 148.00L
// static void tui_calc_printno(unsigned char* buf, uint32_t num, uint8_t base, uint8_t dpts) {

void tui_calc_fc_history(void) {
	if (tui_fc_history_count==0) {
		tui_gen_message(PSTR("NO FUEL COST"),PSTR("HISTORY"));
		return;
	}
	uint8_t idx=0;
	for(;;) {
		struct mtm tm;
		unsigned char buf[17];
		unsigned char line[17];
		line[16] = 0;
		memset(line,' ',16);
		line[0] = (idx/10)|0x30;
		line[1] = (idx%10)|0x30;
		line[2] = ':';
		linear2mtm(&tm, ((uint32_t)tui_fc_history[idx].time_days)*86400);
		tm.year = tm.year % 100;
		line[3] = (tm.year/10)|0x30;
		line[4] = (tm.year%10)|0x30;
		line[5] = (tm.month/10)|0x30;
		line[6] = (tm.month%10)|0x30;
		line[7] = (tm.day/10)|0x30;
		line[8] = (tm.day%10)|0x30;
		tui_calc_printno(buf,tui_fc_history[idx].fuel_price,10,3);
		uint8_t s = strlen((char*)buf);
		memcpy(&(line[16-s]),buf,s);
		lcd_gotoxy(0,0);
		lcd_puts(line);
		memset(line,' ',16);
		tui_calc_printno(buf,tui_fc_history[idx].kilometers,10,1);
		s = strlen((char*)buf);
		memcpy(line,buf,s);
		line[s] = 'K';
		line[s+1] = 'M';
		tui_calc_printno(buf,tui_fc_history[idx].litres,10,2);
		s = strlen((char*)buf);
		memcpy(&(line[15-s]),buf,s);
		line[15] = 'L';
		lcd_gotoxy(0,1);
		lcd_puts(line);
		uint8_t key = tui_waitforkey();
		switch (key) {
			case BUTTON_S1:
				idx++;
				if (idx>=tui_fc_history_count) idx=0;
				break;
			case BUTTON_S2:
				if (idx) idx--;
				else idx = tui_fc_history_count-1;
				break;
			case BUTTON_BOTH:
				return;
		}
	}
}
