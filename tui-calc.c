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
		x = luint2xstr(buf,(uint32_t)num);
		buf[x] = 'H';
		buf[x+1] = 0;
	} else {
		//uint8_t neg = (num<0)?1:0;
		uint8_t neg = 0;
		luint2str(&(buf[neg]),abs(num));
		if (neg) buf[0] = '-';
		// buf="123" or "0"
		if (dpts) {
			x = strlen((char*)buf); // might not match luint2str expectations if neg.
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


// These are RAM copies.
uint8_t tui_fc_history_count=0;
uint8_t tui_fc_history_offset=0; // For some eeprom cycle reducing
uint16_t tui_fc_last_fuel_price = 1500;
uint16_t tui_fc_last_fuel_efficiency = 800; // l/100km*100
uint16_t tui_fc_last_kilometres = 3000; // 300km*10

struct tui_fc_history_entry tui_fc_history[TUI_FC_HISTORY_SIZE];

#define TUI_FC_HISTORY_EE_START 216
// 808 bytes before eeprom end, 8 bytes for status data & above stuff, 800 bytes for 100 history entries

struct __attribute__ ((__packed__)) tui_fc_ee_intro {
	uint8_t count;
	uint8_t offset;
	uint16_t fuel_price;
	uint16_t fuel_eff;
	uint16_t km;
};


static void tui_fc_history_ee_init(void) {
	struct tui_fc_ee_intro st;
	eeprom_read_block(&st,(void*)TUI_FC_HISTORY_EE_START, sizeof(struct tui_fc_ee_intro));
	if (st.count>TUI_FC_HISTORY_SIZE) return; // Invalid data
	if (st.offset>=TUI_FC_HISTORY_SIZE) return; // --''--
	tui_fc_history_count = st.count;
	tui_fc_history_offset = st.offset;
	tui_fc_last_fuel_price = st.fuel_price;
	tui_fc_last_fuel_efficiency = st.fuel_eff;
	tui_fc_last_kilometres = st.km;
}

static void tui_fc_history_ee_exit(void) {
	struct tui_fc_ee_intro st;
	st.count = tui_fc_history_count;
	st.offset = tui_fc_history_offset;
	st.fuel_price = tui_fc_last_fuel_price;
	st.fuel_eff = tui_fc_last_fuel_efficiency;
	st.km = tui_fc_last_kilometres;
	eeprom_update_block(&st,(void*)TUI_FC_HISTORY_EE_START, sizeof(struct tui_fc_ee_intro));
}

#define TUI_FC_HISTORY_EE_DATA_ADDR (TUI_FC_HISTORY_EE_START+sizeof(struct tui_fc_ee_intro))
static void tui_fc_history_read_idx(uint8_t idx, struct tui_fc_history_entry *e) {
	idx += tui_fc_history_offset;
	if (idx>=TUI_FC_HISTORY_SIZE) idx = idx - TUI_FC_HISTORY_SIZE;
	eeprom_read_block(e,(void*)TUI_FC_HISTORY_EE_DATA_ADDR+(idx*sizeof(struct tui_fc_history_entry)),sizeof(struct tui_fc_history_entry));
}

static void tui_fc_history_write_idx(uint8_t idx, struct tui_fc_history_entry *e) {
	idx += tui_fc_history_offset;
	if (idx>=TUI_FC_HISTORY_SIZE) idx = idx - TUI_FC_HISTORY_SIZE;
	eeprom_update_block(e,(void*)TUI_FC_HISTORY_EE_DATA_ADDR+(idx*sizeof(struct tui_fc_history_entry)),sizeof(struct tui_fc_history_entry));
}	

static void tui_fc_history_del_idx(uint8_t idx) {
	uint8_t i;
	if (idx>=tui_fc_history_count) return;
	if (idx==0) {
		tui_fc_history_offset++;
		if (tui_fc_history_offset>=TUI_FC_HISTORY_SIZE) tui_fc_history_offset = 0;
	} else {
		for (i=idx+1;i<tui_fc_history_count;i++) {
			struct tui_fc_history_entry e;
			tui_fc_history_read_idx(i,&e);
			tui_fc_history_write_idx(i-1,&e);
		}
	}
	tui_fc_history_count--;
}

void tui_calc_fuel_add_history(uint16_t time_days, uint16_t kilometers, uint16_t fuel_price, uint16_t litres) {
	if (tui_fc_history_count==TUI_FC_HISTORY_SIZE) { // Delete oldest
		tui_fc_history_del_idx(0);
	}
	struct tui_fc_history_entry e;
	e.time_days = time_days;
	e.kilometers = kilometers;
	e.fuel_price = fuel_price;
	e.litres = litres;
	tui_fc_history_write_idx(tui_fc_history_count,&e);
	tui_fc_history_count++;
}


void tui_calc_fuel_cost(void) {
	struct tcalcstate s = { 0,0,10,1 };
	tui_fc_history_ee_init();
	uint32_t km = tui_fc_last_kilometres;
	uint32_t price = tui_fc_last_fuel_price;
	uint32_t efficiency = tui_fc_last_fuel_efficiency;
	uint32_t litres;
	// 1. Ask(/Verify) KM
	s.n1 = km;
	tui_calc_alt_entry(PSTR("TRIP KM:"),&s,200,65535,10,100,10,10);
	km = s.n1;
	// 2. Verify(/Ask) E/l
	s.dpts = 3;
	s.n1 = price;
	tui_calc_alt_entry(PSTR("\x08/L:"),&s,100,65535,10,10,10,10);
	price = s.n1;
	// 3. Verify(/Ask) l/100km
	s.dpts = 2;
	s.n1 = efficiency;
	tui_calc_alt_entry(PSTR("L/100KM:"),&s,100,65535,10,10,10,10);
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
	tui_calc_alt_entry(PSTR("LITRES:"),&s,20,65535,10,100,10,10);
	litres = s.n1;
	// 6. Calculate and show fuel efficiency
	s.n1 = (litres*1000)/km;
	tui_show_calc_result(&s,PSTR("EFFICIENCY:"),PSTR(" L/100KM"));
	// 7. Verify/Ask date
	uint16_t lindate;
	if (!timer_time_isvalid()) { // Use the normal time setting menu.
		tui_set_clock();
	}
	struct mtm tm;
	timer_get_time(&tm);
	lindate = mtm2lindate(&tm);
	tui_calc_fuel_add_history(lindate,km,price,litres);
	tui_fc_history_ee_exit();
}

const unsigned char tui_fchm_name[] PROGMEM = "FC HISTORY";
const unsigned char tui_fchm_s1[] PROGMEM = "EXIT"; // TUI FUEL COST HISTORY MENU
const unsigned char tui_fchm_s2[] PROGMEM = "DEL ENTRY";
const unsigned char tui_fchm_s3[] PROGMEM = "SHOW L/100KM";
const unsigned char tui_fchm_s4[] PROGMEM = "SHOW \x08/L";


PGM_P const tui_fchm_table_m0[] PROGMEM = {
    (PGM_P)tui_fchm_s1,
    (PGM_P)tui_fchm_s2,
    (PGM_P)tui_fchm_s3
};

PGM_P const tui_fchm_table_m1[] PROGMEM = {
    (PGM_P)tui_fchm_s1,
    (PGM_P)tui_fchm_s2,
    (PGM_P)tui_fchm_s4
};


// History Screen:
// 0123456789012345
// NN:YYMMDD  1.504
// 1600.0KM 148.00L
// static void tui_calc_printno(unsigned char* buf, uint32_t num, uint8_t base, uint8_t dpts) {

void tui_calc_fc_history(void) {
	uint8_t mode=0;
	uint8_t idx=0;
	tui_fc_history_ee_init();
reload:
	if (tui_fc_history_count==0) {
		tui_gen_message(PSTR("NO FUEL COST"),PSTR("HISTORY"));
		return;
	}
	for(;;) {
		struct tui_fc_history_entry e;
		tui_fc_history_read_idx(idx,&e);
		struct mtm tm;
		unsigned char buf[17];
		unsigned char line[17];
		line[16] = 0;
		memset(line,' ',16);
		line[0] = (idx/10)|0x30;
		line[1] = (idx%10)|0x30;
		line[2] = ':';
		lindate2mtm(&tm, e.time_days);
		tm.year = tm.year % 100;
		line[3] = (tm.year/10)|0x30;
		line[4] = (tm.year%10)|0x30;
		line[5] = (tm.month/10)|0x30;
		line[6] = (tm.month%10)|0x30;
		line[7] = (tm.day/10)|0x30;
		line[8] = (tm.day%10)|0x30;
		if (mode==0) {
			tui_calc_printno(buf,e.fuel_price,10,3);
		} else {
			uint32_t litres = e.litres;
			uint32_t km = e.kilometers;
			uint32_t eff = (litres*1000)/km;
			tui_calc_printno(buf,eff,10,2);
		}
		uint8_t s = strlen((char*)buf);
		memcpy(&(line[16-s]),buf,s);
		lcd_gotoxy(0,0);
		lcd_puts(line);
		memset(line,' ',16);
		tui_calc_printno(buf,e.kilometers,10,1);
		s = strlen((char*)buf);
		memcpy(line,buf,s);
		line[s] = 'K';
		line[s+1] = 'M';
		tui_calc_printno(buf,e.litres,10,2);
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
			case BUTTON_BOTH: {
					uint8_t v;
					if (mode) v = tui_gen_listmenu((PGM_P)tui_fchm_name, tui_fchm_table_m1, 3, 0);
					else v = tui_gen_listmenu((PGM_P)tui_fchm_name, tui_fchm_table_m0, 3, 0);
					switch (v) {
						default:
						case 0: // EXIT
							return;
						case 1: // DEL IDX
							if (tui_are_you_sure()) {
								tui_fc_history_del_idx(idx);
								tui_fc_history_ee_exit();
								if (idx) idx--;
								goto reload;
							}
							break;
						case 2: // FLIP MODE
							mode ^= 1;
							break;
					}
				}
				break;
		}
	}
}
