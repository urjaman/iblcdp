
#ifdef TEST
#define PGM_P char*
#endif

#include <alloca.h>
#include <string.h>
#include "stlcdnr.h"




#define swap(a, b) { uint8_t t = a; a = b; b = t; }

struct drawdata {
	uint8_t w;
	uint8_t h;
	uint8_t d[];
};

#define make_drawdata_bg(x,e,j,b) do { \
                const int sz = (e)*(j)*LCD_CHARW; \
				x = alloca(sizeof(struct drawdata)+sz); \
				memset(x->d,b,sz); \
				x->w = e*LCD_CHARW; \
				x->h = j*LCD_CHARH; \
				} while(0)

#define make_drawdata(d,e,j) make_drawdata_bg(d,e,j,0)
#define make_drawdata_inv(d,e,j) make_drawdata_bg(d,e,j,0xFF)

void putpixel(struct drawdata *d, uint8_t x, uint8_t y);
void clrpixel(struct drawdata *d, uint8_t x, uint8_t y);
void setpixel(struct drawdata *d, uint8_t x, uint8_t y, uint8_t color);

void fillrect(struct drawdata *d,uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

void drawrect(struct drawdata *d,
	      uint8_t x, uint8_t y, uint8_t w, uint8_t h,
	      uint8_t color);

void drawline(struct drawdata *d,
	      uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
	      uint8_t color);

void drawcircle(struct drawdata *d, uint8_t x0, uint8_t y0, uint8_t r,uint8_t color);
void fillcircle(struct drawdata *d, uint8_t x0, uint8_t y0, uint8_t r,uint8_t color);

