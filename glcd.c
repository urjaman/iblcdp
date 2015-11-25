/*
$Id:$

ST7565 LCD library! - modified for the no big ram buffer edition

Copyright (C) 2010 Limor Fried, Adafruit Industries
Copyright (C) 2014 Urja Rannikko <urjaman@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef TEST
#include "main.h"
#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h> 
#include <string.h>
#include "glcd.h"
#define dprintf(...)
#else
#define PROGMEM 
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "glcd.h"
static uint8_t pgm_read_byte(const uint8_t* b) {
    return *b;
}
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#endif

static const uint8_t PROGMEM bvlut[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

static void putpixel_(struct drawdata *d, uint8_t x, uint8_t y) {
    d->d[x+(y/8)*(d->w)] |= pgm_read_byte(&(bvlut[y&7]));
}

static void clrpixel_(struct drawdata *d, uint8_t x, uint8_t y) {
    d->d[x+(y/8)*(d->w)] &= ~pgm_read_byte(&(bvlut[y&7]));
}

void putpixel(struct drawdata *d, uint8_t x, uint8_t y)
{
  if ((x >= d->w) || (y >= d->h))
    return;
  putpixel_(d,x,y);
}

void clrpixel(struct drawdata *d, uint8_t x, uint8_t y)
{
  if ((x >= d->w) || (y >= d->h))
    return;
  clrpixel_(d,x,y);
}

void setpixel(struct drawdata *d, uint8_t x, uint8_t y, uint8_t color)
{
  if ((x >= d->w) || (y >= d->h))
    return;
  if (color) putpixel_(d,x,y);
  else clrpixel_(d,x,y);
}

static const uint8_t PROGMEM evlut[8] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };

// filled rectangle
void fillrect(struct drawdata *d, uint8_t x, uint8_t y, uint8_t w, uint8_t h,
	      uint8_t color) {

  /* Clip to buffer */
  uint8_t ye = y+h;
  uint8_t xe = x+w;
  if (ye > d->h) ye = d->h;
  if (ye <= y) return;
  if (xe > d->w) xe = d->w;
  if (xe <= x) return;

  // start
  uint8_t ym8 = y&7;
  if (color) {
      dprintf("fillrect c1 ym8 %d x %d y %d xe %d ye %d\n",ym8,x,y,xe,ye);
	  if (ym8) {
		uint8_t se = ym8+h;
		if (se>8) se = 8;
		uint8_t ov = 0;
		for (uint8_t j=ym8;j<se;j++) ov |= pgm_read_byte(&(bvlut[j]));
		uint8_t *xyb = &(d->d[x+(y/8)*d->w]);
		for (uint8_t i=x;i<xe;i++) *xyb++ |= ov;
		y += (se-ym8);
		dprintf ("ym8 end, se=%d ov=0x%02X y=%d\n",se,ov,y);
		if (y>=ye) return;
	  }
	  if ((ye-y)>=8) { // middle
	    dprintf("middle start, ye=%d, y=%d\n",ye,y);
		for (;y<ye;y+=8) {
		    dprintf("loop, ye-y=%d y=%d\n",ye-y,y);
			if ((ye-y)<8) break;
			uint8_t *xyb = &(d->d[x+(y/8)*d->w]);
			for (uint8_t i=x;i<xe;i++) *xyb++ = 0xFF;
			dprintf("did y=%d\n",y);
		}
	  }
	  // end
	  dprintf("thinking bout end chunk, ye=%d y=%d\n",ye,y);
	  if (ye>y) {
		uint8_t *xyb = &(d->d[x+(y/8)*d->w]);
		uint8_t ov = pgm_read_byte(&(evlut[ye-y]));
	    dprintf("doing end chunk, ov=%02X\n",ov);
		for (uint8_t i=x;i<xe;i++) *xyb++ |= ov;
	  }
  } else {
	  /* OV == OR value ... are AND values here, sorry. */
	  if (ym8) {
		uint8_t se = ym8+h;
		if (se>8) se = 8;
		uint8_t ov = 0;
		for (uint8_t j=ym8;j<se;j++) ov |= pgm_read_byte(&(bvlut[j]));
		ov = ~ov;
		uint8_t *xyb = &(d->d[x+(y/8)*d->w]);
		for (uint8_t i=x;i<xe;i++) *xyb++ &= ov;
		y += (se-ym8);
		if (y>=ye) return;
	  }
	  if ((ye-y)>=8) { // middle
		for (;y<ye;y+=8) {
			if ((ye-y)<8) break;
			uint8_t *xyb = &(d->d[x+(y/8)*d->w]);
			for (uint8_t i=x;i<xe;i++) *xyb++ = 0;
		}
	  }
	  // end
	  if (ye>y) {
		uint8_t *xyb = &(d->d[x+(y/8)*d->w]);
		uint8_t ov = ~pgm_read_byte(&(evlut[ye-y]));
		for (uint8_t i=x;i<xe;i++) *xyb++ &= ov;
	  }
  }
}


// bresenham with fillrects
void drawline(struct drawdata *d,
	      uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
	      uint8_t color) {
  if (x0==x1) { // Vertical line
    dprintf("Vertical line x %d y0 %d y1 %d\n",x0,y0,y1);
    if (y1>y0) {
		fillrect(d,x0,y0,1,(y1-y0)+1,color);
	} else {
		fillrect(d,x0,y1,1,(y0-y1)+1,color);
	}
	return;
  }
  if (y0==y1) { // Horizontal line
    dprintf("Horizontal line y %d x0 %d x1 %d\n",y0,x0,x1);
	if (x1>x0) {
		fillrect(d,x0,y0,(x1-x0)+1,1,color);
	} else {
		fillrect(d,x1,y0,(x0-x1)+1,1,color);
	}
	return;
  }
  dprintf("drawline (%d,%d) -> (%d,%d)\n",x0,y0,x1,y1);
  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
    dprintf ("steep; virtually (%d,%d) -> (%d,%d)\n",x0,y0,x1,y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
    dprintf ("x0>x1; swapped; now (%d,%d) -> (%d,%d)\n",x0,y0,x1,y1);
  }

  uint8_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int8_t err = dx / 2;
  int8_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }
  uint8_t xg=x0;
  dprintf("loop go, xg %d, ystep %d, err %d dx %d dy %d\n",xg,ystep,err,dx,dy);
  for (; x0<x1; x0++) {
    err -= dy;
    if (err < 0) {
      if (steep) {
        dprintf("steep; fillrect %d,%d,1,%d\n",y0,xg,(x0-xg)+1);
        fillrect(d,y0,xg,1,(x0-xg)+1,color);
      } else {
        dprintf("nostp; fillrect %d,%d,%d,1\n",xg,y0,(x0-xg)+1);
        fillrect(d,xg,y0,(x0-xg)+1,1,color);
      }
      xg = x0+1;
      y0 += ystep;
      err += dx;
    }
  }
  if (x0>=xg) {
    dprintf("end; last fillrect\n");
    if (steep) {
      dprintf("steep; fillrect %d,%d,1,%d\n",y0,xg,(x0-xg)+1);
      fillrect(d,y0,xg,1,(x0-xg)+1,color);
    } else {
      dprintf("nostp; fillrect %d,%d,%d,1\n",xg,y0,(x0-xg)+1);
      fillrect(d,xg,y0,(x0-xg)+1,1,color);
    }
  }
}


// draw a rectangle
void drawrect(struct drawdata *d,
	      uint8_t x, uint8_t y, uint8_t w, uint8_t h,
	      uint8_t c) {
  fillrect(d,x,    y,    w,1,c);
  fillrect(d,x,    y,    1,h,c);
  fillrect(d,x+w-1,y,    1,h,c);
  fillrect(d,x,    y+h-1,w,1,c);
}

static void dc_rects(struct drawdata *d, uint8_t x0, uint8_t xg, uint8_t x, uint8_t y0, uint8_t y, uint8_t cl, uint8_t c)
{
    if (cl==1) {
        setpixel(d,x0+xg,y0+y,c);
        setpixel(d,x0-x,y0+y,c);
        setpixel(d,x0+xg,y0-y,c);
        setpixel(d,x0-x,y0-y,c);
        
        setpixel(d,x0+y,y0+xg,c);
        setpixel(d,x0-y,y0+xg,c);
        setpixel(d,x0+y,y0-x,c);
        setpixel(d,x0-y,y0-x,c);
    } else {
        fillrect(d, x0 + xg, y0 + y,cl,1,c);
        fillrect(d, x0 - x,  y0 + y,cl,1,c);
        fillrect(d, x0 + xg, y0 - y,cl,1,c);
        fillrect(d, x0 - x,  y0 - y,cl,1,c);
    
        fillrect(d, x0 + y, y0 + xg,1,cl,c);
        fillrect(d, x0 - y, y0 + xg,1,cl,c);
        fillrect(d, x0 + y, y0 - x, 1,cl,c);
        fillrect(d, x0 - y, y0 - x, 1,cl,c);
    }
}

// draw a circle
void drawcircle(struct drawdata *d,
	      uint8_t x0, uint8_t y0, uint8_t r, 
	      uint8_t color) {
  int8_t f = 1 - r;
  int8_t ddF_x = 1;
  int8_t ddF_y = -2 * r;
  int8_t x = 0;
  int8_t y = r;

  uint8_t xg = x;  
  while (x<y) {
    if (f >= 0) {
      dc_rects(d,x0,xg,x,y0,y,(x-xg+1),color);
      xg = x+1;
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

  }
  dc_rects(d,x0,xg,x,y0,y,(x-xg+1),color);
  
}


// draw a circle
void fillcircle(struct drawdata *d,
	      uint8_t x0, uint8_t y0, uint8_t r,
	      uint8_t color) {
  int8_t f = 1 - r;
  int8_t ddF_x = 1;
  int8_t ddF_y = -2 * r;
  int8_t x = 0;
  int8_t y = r;


  for (uint8_t i=y0-r; i<=y0+r; i++) {
    setpixel(d, x0, i, color);
  }
//  uint8_t xg = x;
  while (x<y) {
    if (f >= 0) {
#if 0
      fillrect(d, x0+xg, y0-y,(x-xg)+1,y*2+1, color);
      fillrect(d, x0-x,  y0-y,(x-xg)+1,y*2+1, color);
      fillrect(d, x0+y,  y0-x,
#endif
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    for (uint8_t i=y0-y; i<=y0+y; i++) {
      setpixel(d, x0+x, i, color);
      setpixel(d, x0-x, i, color);
    }
    for (uint8_t i=y0-x; i<=y0+x; i++) {
      setpixel(d, x0+y, i, color);
      setpixel(d, x0-y, i, color);
    }
  }
}

