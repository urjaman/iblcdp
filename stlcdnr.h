/* This is the external API */
// These are in pixels
#define LCDWIDTH 128
#define LCDHEIGHT 64
// These are in char blocks
#define LCD_MAXX 16
#define LCD_MAXY 8

//This is defined by the font and must match with the defines above... so kinda hardcoded.
#define LCD_CHARW 8
// This is hardcoded by the hardware (_could_ be 16 too, but...)
#define LCD_CHARH 8

void lcd_init(void);
void lcd_putchar(unsigned char c);
void lcd_puts(const unsigned char* str);
void lcd_puts_P(PGM_P str);
void lcd_clear(void);
// This is compatibility, in char blocks
void lcd_gotoxy(uint8_t x, uint8_t y);

// This is in the native format, x= pixels, y=blocks
void lcd_gotoxy_nt(uint8_t x, uint8_t y);


uint8_t lcd_puts_dyn(const unsigned char *str);
uint8_t lcd_puts_dyn_P(PGM_P str);

// These are for the dynamic extension of the font, obviously
uint8_t lcd_strwidth(const unsigned char *str);
uint8_t lcd_strwidth_P(PGM_P str);


/* Buffer should be w*h bytes big. */
void lcd_write_block_P(const PGM_P buffer, uint8_t w, uint8_t h);
void lcd_write_block(const uint8_t* buffer, uint8_t w, uint8_t h);

void st7565_set_contrast(uint8_t val);

/* End header */
