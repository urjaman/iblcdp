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
