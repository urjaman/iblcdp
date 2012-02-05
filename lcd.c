/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, upper layer of LCD driver.
 *
 * $Id: lcd.c,v 1.1.2.1 2005/12/28 22:35:08 joerg_wunsch Exp $
 */

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>

#include <util/delay.h>
#include "hd44780.h"
#include "lcd.h"
#if LCD_MAXY > 2
#error "LCD_MAXY > 2 not supported, yet."
#endif


static const uint8_t lcd_eur_sign[8] PROGMEM = {
	0b00111,
	0b01000,
	0b11110,
	0b01000,
	0b11110,
	0b01000,
	0b00111,
	0b00000
};

static const uint8_t lcd_ul_sign[8] PROGMEM = {
	0b00000,
	0b11110,
	0b11000,
	0b10100,
	0b10010,
	0b00001,
	0b00000,
	0b00000
};

static const uint8_t lcd_ur_sign[8] PROGMEM = {
	0b00000,
	0b01111,
	0b00011,
	0b00101,
	0b01001,
	0b10000,
	0b00000,
	0b00000
};

static const uint8_t lcd_dl_sign[8] PROGMEM = {
	0b00000,
	0b00000,
	0b00001,
	0b10010,
	0b10100,
	0b11000,
	0b11110,
	0b00000
};

static const uint8_t lcd_dr_sign[8] PROGMEM = {
	0b00000,
	0b00000,
	0b10000,
	0b01001,
	0b00101,
	0b00011,
	0b01111,
	0b00000
};


/*
 * Setup the LCD controller.  First, call the hardware initialization
 * function, then adjust the display attributes we want.
 */
void
lcd_init(void)
{

  hd44780_init();

  /*
   * Clear the display.
   */
  hd44780_outcmd(HD44780_CLR);
  hd44780_wait_ready();

  /*
   * Entry mode: auto-increment address counter, no display shift in
   * effect.
   */
  hd44780_outcmd(HD44780_ENTMODE(1, 0));
  hd44780_wait_ready();

  /*
   * Enable display
   */
  hd44780_outcmd(HD44780_DISPCTL(1, 0, 0));
  hd44780_wait_ready();
  
  /* Program CGRAM characters. */
  lcd_program_char((PGM_P)lcd_eur_sign, 0); /* EUR sign at index 0 (or 8) */
  lcd_program_char((PGM_P)lcd_ul_sign,1);   /* UL arrow at index 1 (or 9) */
  lcd_program_char((PGM_P)lcd_ur_sign,2);   /* UR arrow at index 2 (or 10) */
  lcd_program_char((PGM_P)lcd_dl_sign,3);   /* DL arrow at index 3 (or 11) */
  lcd_program_char((PGM_P)lcd_dr_sign,4);   /* DR arrow at index 4 (or 12) */
  
  /* Go to the corner... */
  lcd_gotoxy(0,0);
  
}

void lcd_gotoxy(uint8_t x, uint8_t y) {
	uint8_t addr;
	addr = 0;
	if (y) addr = 0x40;
	addr += x;
	hd44780_wait_ready();
	hd44780_outcmd(HD44780_DDADDR(addr));
	}


void lcd_clear(void) {
	hd44780_wait_ready();
	hd44780_outcmd(HD44780_CLR);
	lcd_gotoxy(0,0);
	}

void lcd_putchar(unsigned char c) {
      hd44780_wait_ready();
      hd44780_outdata(c);
}

void lcd_puts(unsigned char * str) {
start:
	if (*str) lcd_putchar(*str);
	else return;
	str++;
	goto start;
	}

void lcd_puts_P(PGM_P str) {
	unsigned char c;
start:
	c = pgm_read_byte(str);
	if (c) lcd_putchar(c);
	else return;
	str++;
	goto start;
	}
	
void lcd_program_char(PGM_P data, uint8_t index) {
	hd44780_wait_ready();
	hd44780_outcmd(HD44780_CGADDR(index<<3));
	for(uint8_t i=0;i<8;i++) {
		hd44780_wait_ready();
		hd44780_outdata(pgm_read_byte(data));
		data++;
	}
}
		
