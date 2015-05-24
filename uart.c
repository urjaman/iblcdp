#include "main.h"
#include "lib.h"
#include "uart.h"
#include "console.h"
#include "timer.h"
#include "cron.h"
#ifdef ENABLE_UARTMODULE

/* This is an UART module using the ATmega64C1 LIN
 * UART for RX and soft serial for TX. Funky. */

typedef unsigned char urxbufoff_t;
#define UART_BUFLEN 16
unsigned char volatile uart_rcvbuf[UART_BUFLEN];
urxbufoff_t volatile uart_rcvwptr;
urxbufoff_t volatile uart_rcvrptr;


ISR(LIN_TC_vect) {
	uint8_t d = LINDAT;
	urxbufoff_t reg = uart_rcvwptr;
	uart_rcvbuf[reg] = d;
	reg++;
	if(reg==UART_BUFLEN) reg = 0;
	uart_rcvwptr = reg;
}

uint8_t uart_isdata(void) {
	if (uart_rcvwptr != uart_rcvrptr) return 1;
	return 0;
}

uint8_t uart_recv(void) {
	urxbufoff_t reg;
	unsigned char val;
	while (!uart_isdata()); // when there's nothing to do, one might idle...
	reg = uart_rcvrptr;
	val = uart_rcvbuf[reg++];
	if(reg==UART_BUFLEN) reg = 0;
	uart_rcvrptr = reg;
	return val;
}


void uart_init(void) {
	cli();
	PORTD |= _BV(2);
	PORTD |= _BV(4);
	DDRD |= _BV(2);
	uart_rcvwptr = 0;
	uart_rcvrptr = 0;
	LINBTR = _BV(LDISR) | 14;
	LINCR = _BV(LENA) | _BV(LCMD2) | _BV(LCMD1);
	LINSIR = _BV(LERR) | _BV(LIDOK) | _BV(LTXOK) | _BV(LRXOK);
	LINENIR = _BV(LENRXOK);
	/* LINBTR to 8 for USE_2X and 16 for not would be compatible, but
	 * the LIN can do better. */
	/* We have pretty good 115200 @ 16Mhz here. */
	/* 16Mhz / ( 14 * (9+1)) */
	LINBTR = _BV(LDISR) | 14;
	LINBRR = 9;
	sei();
}

void uart_wait_txdone(void) {
}

#else

void uart_wait_txdone(void) { }
void uart_init(void) { }
void uart_send(uint8_t val) { val = val; }
uint8_t uart_recv(void) { return 0; }
uint8_t uart_isdata(void) { return 0; }

#endif
