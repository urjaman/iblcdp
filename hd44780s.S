#include <avr/io.h>
/* This file has both the HD44780 and LCD layers */
// External "API":
/*
/void	lcd_init(void);
void	lcd_putchar(char c);
void lcd_puts(char * str);
void lcd_puts_P(PGM_P str);
void lcd_clear(void);
void lcd_gotoxy(unsigned char x, unsigned char y);
// External ABI is GCC 4 compatible (for now, to allow testing against C code base)
*/
/* HD44780 LCD port connections */
#define HD44780_PORT C
#define HD44780_RS PORT0
#define HD44780_RW PORT1
#define HD44780_E  PORT2
/* The data bits have to be in ascending order. */
#define HD44780_D4 PORT4
#define HD44780_D5 PORT5
#define HD44780_D6 PORT6
#define HD44780_D7 PORT7

#define GLUE(a, b)     a##b
#define PORT(x)        GLUE(PORT, x)
#define PIN(x)         GLUE(PIN, x)
#define DDR(x)         GLUE(DDR, x)

#define HD44780_PORTOUT    PORT(HD44780_PORT)
#define HD44780_PORTIN     PIN(HD44780_PORT)
#define HD44780_DDR        DDR(HD44780_PORT)

#define HD44780_DATABITS \
(_BV(HD44780_D4)|_BV(HD44780_D5)|_BV(HD44780_D6)|_BV(HD44780_D7))

#define HD44780_BUSYFLAG 0x80

/* Clear LCD display command. */
#define HD44780_CLR \
	0x01

/* Home cursor command. */
#define HD44780_HOME \
	0x02

/*
 * Select the entry mode.  inc determines whether the address counter
 * auto-increments, shift selects an automatic display shift.
 */
#define HD44780_ENTMODE(inc, shift) \
	(0x04 | ((inc)? 0x02: 0) | ((shift)? 1: 0))

/*
 * Selects disp[lay] on/off, cursor on/off, cursor blink[ing]
 * on/off.
 */
#define HD44780_DISPCTL(disp, cursor, blink) \
	(0x08 | ((disp)? 0x04: 0) | ((cursor)? 0x02: 0) | ((blink)? 1: 0))

/*
 * With shift = 1, shift display right or left.
 * With shift = 0, move cursor right or left.
 */
#define HD44780_SHIFT(shift, right) \
	(0x10 | ((shift)? 0x08: 0) | ((right)? 0x04: 0))

/*
 * Function set.  if8bit selects an 8-bit data path, twoline arranges
 * for a two-line display, font5x10 selects the 5x10 dot font (5x8
 * dots if clear).
 */
#define HD44780_FNSET(if8bit, twoline, font5x10) \
	(0x20 | ((if8bit)? 0x10: 0) | ((twoline)? 0x08: 0) | \
		((font5x10)? 0x04: 0))

#define HD44780_MINIFNSET(if8bit) \
	(0x02 | ((if8bit)? 0x01: 0))
/*
 * Set the next character generator address to addr.
 */
#define HD44780_CGADDR(addr) \
	(0x40 | ((addr) & 0x3f))

/*
 * Set the next display address to addr.
 */
#define HD44780_DDADDR(addr) \
	(0x80 | ((addr) & 0x7f))

// rs = T
hd44780_setrs:
	brts 1f
	cbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_RS
	rjmp 2f
1:
	sbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_RS
2:
	ret
	
// n = r24, rs = T bit
hd44780_outnibble:
	push r24
	cbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_RW
	rcall hd44780_setrs
	in r25, _SFR_IO_ADDR(HD44780_PORTOUT)
	andi r25, ~HD44780_DATABITS
	swap r24 // Assuming D4 = bit 4 (!!!)
	andi r24, HD44780_DATABITS
	or r25, r24
	out _SFR_IO_ADDR(HD44780_PORTOUT), r25
	// hd44780_pulse_e, no readback...
	sbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_E
	nop
	nop
	nop
	nop // @8Mhz 4 nops = 500ns pulse width
	cbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_E
	pop r24
	ret
// b = r24, rs=T
hd44780_outbyte:
	swap r24
	rcall hd44780_outnibble
	swap r24
	rcall hd44780_outnibble
	ret
	
// rv = r24, rs = T
hd44780_innibble:
	push r25
	sbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_RW
	in r25, _SFR_IO_ADDR(HD44780_DDR)
	andi r25, ~HD44780_DATABITS
	out _SFR_IO_ADDR(HD44780_DDR), r25
	rcall hd44780_setrs
	// hd44780_pulse_e, readback
	sbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_E
	nop
	nop
	nop
	nop // @8Mhz 4 nops = 500ns pulse width
	// another 4 nops...
	nop
	nop
	nop
	nop
	in r24, _SFR_IO_ADDR(HD44780_PORTIN)
	cbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_E
	andi r24, HD44780_DATABITS
	swap r24
	ori r25, HD44780_DATABITS
	out _SFR_IO_ADDR(HD44780_DDR), r25
	cbi _SFR_IO_ADDR(HD44780_PORTOUT), HD44780_RW
	pop r25
	ret
	
// rv = r24, rs = T
hd44780_inbyte:
	rcall hd44780_innibble
	mov r25, r24
	rcall hd44780_innibble
	swap r25
	or r24, r25
	ret
	
hd44780_wait_ready:
	clt // rs = 0 (incmd)
	push r24
	1:
	rcall hd44780_inbyte
	andi r24, HD44780_BUSYFLAG
	breq 1b
	pop r24
	ret


	
hd44780_init:
	ldi r24, _BV(HD44780_RS) | _BV(HD44780_RW) | _BV(HD44780_E) | _BV(3) | HD44780_DATABITS
	out _SFR_IO_ADDR(HD44780_DDR), r24
//	ldi r24, 40
//	rcall mdelay_ms

ldi     r18, 0x90
ldi     r19, 0x01
ldi     r24, 0xC8
ldi     r25, 0x00
sbiw    r24, 0x01
brne    .-4
subi    r18, 0x01
sbci    r19, 0x00
brne    .-14


	// clt (already in lcd_init)
	ldi r24, 0x03
	rcall hd44780_outnibble
//	ldi r24, 5
//	rcall mdelay_ms

ldi     r24, 0x08
ldi     r25, 0x20
sbiw    r24, 0x01
brne    .-4

	ldi r24, 0x03
	rcall hd44780_outnibble
//	ldi r24, 1
//	rcall mdelay_ms

ldi     r24, 0xC8
ldi     r25, 0x00
sbiw    r24, 0x01
brne    .-4

	ldi r24, 0x03
	rcall hd44780_outnibble
	ldi r24, 0x03
	rcall hd44780_outnibble
	rcall hd44780_wait_ready
	ldi r24, 0x28
	rcall hd44780_outbyte
	rcall hd44780_wait_ready
	ldi r24, 0x08
	rcall hd44780_outbyte
	rcall hd44780_wait_ready
	ret
	
	
.global lcd_init
lcd_init:
	clt
	rcall hd44780_init
	ldi r24, HD44780_CLR
	rcall hd44780_outbyte
	rcall hd44780_wait_ready
	ldi r24, 0x06
	rcall hd44780_outbyte
	rcall hd44780_wait_ready
	ldi r24, 0x0C
	rcall hd44780_outbyte
	rcall hd44780_wait_ready
	ret

// x = r24, y = r22
.global lcd_gotoxy
lcd_gotoxy:
	and r22,r22
	breq 1f
	subi r24, -0x40
1:
	ori r24, 0x80
	rcall hd44780_wait_ready
	// clt already in wait_ready
	rcall hd44780_outbyte
	ret
	
.global lcd_clear
lcd_clear:
	rcall hd44780_wait_ready
	// clt already in wait_ready
	ldi r24, HD44780_CLR
	rcall hd44780_outbyte
	clr r24
	clr r22
	rcall lcd_gotoxy
	ret
	
// r24 = c
.global lcd_putchar
lcd_putchar:
	rcall hd44780_wait_ready
	set
	rcall hd44780_outbyte
	ret
	
// str = r25:24
.global lcd_puts
lcd_puts:
//	movw r26, r24
	mov r26, r24
	mov r27, r25
0:
	ld r24, X+
	and r24, r24
	breq 1f
	rcall lcd_putchar
	rjmp lcd_puts
1: ret

// str = r25:r24
.global lcd_puts_P
lcd_puts_P:
//	movw r30, r24
	mov r30, r24
	mov r31, r25
0:
	lpm
	adiw r30, 1
	mov r24, r0
	and r24, r24
	breq 1f
	rcall lcd_putchar
	rjmp 0b
1:
	ret
