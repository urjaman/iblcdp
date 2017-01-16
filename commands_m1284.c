/*
 * Copyright (C) 2013,2014,2016 Urja Rannikko <urjaman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "appdb.h"
#include "glcd.h"
#include "rgbbl.h"
#include "stlcdnr.h"
#include "ciface.h"

CIFACE_APP(lcdw_cmd, "LCDW")
{
	if (token_count >= 3) {
		uint8_t y = astr2luint(tokenptrs[1]);
		uint8_t x = astr2luint(tokenptrs[2]);
		lcd_gotoxy(x,y);
		for (uint8_t i=3;i<token_count;i++) {
			lcd_puts(tokenptrs[i]);
			lcd_putchar(' ');
		}
	}
}

CIFACE_APP(ldw_cmd, "LDW")
{
	if (token_count >= 3) {
		uint8_t y = astr2luint(tokenptrs[1]);
		uint8_t x = astr2luint(tokenptrs[2]);
		lcd_gotoxy_dw(x,y);
		for (uint8_t i=3;i<token_count;i++) {
		    lcd_puts_dw(tokenptrs[i]);
		    lcd_puts_dw_P(PSTR(" "));
		}
	}
}

CIFACE_APP(blset_cmd, "BLSET")
{
    if (token_count >= 3) {
        uint8_t r = astr2luint(tokenptrs[1]);
        uint8_t g = astr2luint(tokenptrs[2]);
        uint8_t b = astr2luint(tokenptrs[3]);
        rgbbl_set(r,g,b);
    }
}


static unsigned char tgt_red;
static unsigned char tgt_green;
static unsigned char tgt_blue;

static unsigned char fadechan(unsigned char current, unsigned char target)
{
	if (current != target) {
		unsigned char change = current / 16;
		if (!change) change = 1;
		unsigned char diff;
		if (current < target) {
			diff = target - current;
			if (diff < change) change = diff;
			current += change;
		} else {
			diff = current - target;
			if (diff < change) change = diff;
			current -= change;
		}
	}
	return current;
}

static unsigned char cur_red = 0;
static unsigned char cur_green = 0;
static unsigned char cur_blue = 0;

void fader(void) {
	cur_red = fadechan(cur_red, tgt_red);
	cur_green = fadechan(cur_green, tgt_green);
	cur_blue = fadechan(cur_blue, tgt_blue);
	rgbbl_set(cur_red, cur_green, cur_blue);
	_delay_ms(4);
}

CIFACE_APP(fader_cmd, "FADER")
{
	if (token_count >= 7) {
	        cur_red = astr2luint(tokenptrs[4]);
	        cur_green = astr2luint(tokenptrs[5]);
	        cur_blue = astr2luint(tokenptrs[6]);
	}
	if (token_count >= 4) {
	        tgt_red = astr2luint(tokenptrs[1]);
	        tgt_green = astr2luint(tokenptrs[2]);
	        tgt_blue = astr2luint(tokenptrs[3]);
	}
	unsigned int loops = 0;
	while ((cur_red != tgt_red) || (cur_green != tgt_green) || (cur_blue != tgt_blue)) {
		fader();
		loops++;
	}
	luint2outdual(loops);
}


CIFACE_APP(lcdr_cmd, "LCDR")
{
	lcd_init();
}

CIFACE_APP(lcdbr_cmd, "LCDBR")
{
	if (token_count >= 2) {
		uint32_t val = astr2luint(tokenptrs[1]);
		if (val>63) return;
		st7565_set_contrast(val);
	}
}

static void bargraph(uint8_t x, uint8_t y, uint8_t h, uint8_t w, uint8_t f) {
	struct drawdata *dd;
	make_drawdata(dd,w,h);
	drawrect(dd,0,0,w*LCD_CHARW,h*LCD_CHARH,1);
	fillrect(dd,0,0,f,h*LCD_CHARH,1);
	lcd_gotoxy(x,y);
	lcd_write_block(dd->d,w,h);
}


CIFACE_APP(lcdbg_cmd, "LBG")
{
	if (token_count >= 6) {
		uint8_t y = astr2luint(tokenptrs[1]);
		uint8_t x = astr2luint(tokenptrs[2]);
		uint8_t h = astr2luint(tokenptrs[3]);
		uint8_t w = astr2luint(tokenptrs[4]);
		uint8_t f = astr2luint(tokenptrs[5]);
		bargraph(x,y,h,w,f);
	}
}

CIFACE_APP(lcdclr_cmd, "LCLR")
{
    lcd_clear();
}

