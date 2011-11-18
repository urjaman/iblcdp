#include "main.h"
#include "tui.h"
#include "lib.h"
#include "lcd.h"


static void tui_calc_printno(unsigned char* buf, int32_t num, uint8_t base, uint8_t dpts) {
	uint8_t x;
	if (base==16) {
		luint2xstr(buf,(uint32_t)num);
		x = strlen((char*)buf);
		buf[x] = 'H';
		buf[x+1] = 0;
	} else {
		uint8_t neg = (num<0)?1:0;
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
				PGM_P header, int32_t number, uint8_t base, uint8_t dpts) {
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


static uint8_t tui_gen_calcmenu(PGM_P header, int32_t number, uint8_t base, PGM_P const menu_table[], 
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
	int32_t n1;
	int32_t n2;
	uint8_t base;
	uint8_t dpts;
};

static void tui_calc_entry(struct tcalcstate *s, uint8_t no) {
	uint8_t ss=0;
	uint8_t sel=0;
	PGM_P nop;
	int32_t *np;
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

static const unsigned char tui_com_s1[] PROGMEM = "+";
static const unsigned char tui_com_s2[] PROGMEM = "-";
static const unsigned char tui_com_s3[] PROGMEM = "*";
static const unsigned char tui_com_s4[] PROGMEM = "/";
static const unsigned char tui_com_s5[] PROGMEM = "%";
static const unsigned char tui_com_s6[] PROGMEM = "&";
static const unsigned char tui_com_s7[] PROGMEM = "|";
static const unsigned char tui_com_s8[] PROGMEM = "^";

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


void tui_calc(void) {
	static int32_t n1r=0;
	static uint8_t dptsr=0;
	static uint8_t baser=10;
	unsigned char buf[16];
	struct tcalcstate s = { n1r, 0, baser, dptsr };
	int32_t e=1;
	uint8_t op;

	tui_calc_entry(&s,1);
	tui_calc_entry(&s,2);
	op = tui_gen_listmenu(PSTR("PICK OPERATION"), tui_com_table, ((s.base==10)&&(s.dpts))?4:8, 0);
	
	// auto-dpts support:
	if ((op==3)&&(s.dpts==0)&&(s.n1%s.n2)) {
		if ((abs(s.n1)<2000)&&(abs(s.n2)<2000)) {
		s.dpts = 3;
		s.n1 *= 1000;
		s.n2 *= 1000;
		}
	}
	dptsr = s.dpts;
	
	if (s.base==10) while (s.dpts--) e*=10;
	switch (op) {
		case 0: s.n1 = s.n1 + s.n2; break;
		case 1: s.n1 = s.n1 - s.n2; break;
		case 2: s.n1 = (s.n1 * s.n2) / e; break;
		case 3: s.n1 = (s.n1 * e) / s.n2; break;
		case 4: s.n1 = s.n1 % s.n2; break;
		case 5: s.n1 = s.n1 & s.n2; break;
		case 6: s.n1 = s.n1 | s.n2; break;
		case 7: s.n1 = s.n1 ^ s.n2; break;
	}
	n1r = s.n1;
	baser = s.base;

	lcd_clear();
	lcd_gotoxy(4,0);
	lcd_puts_P(PSTR("RESULT:"));
	tui_calc_printno(buf,s.n1,s.base,dptsr);
	lcd_gotoxy((16-strlen((char*)buf))/2,1);
	lcd_puts(buf);
	tui_waitforkey();
}
