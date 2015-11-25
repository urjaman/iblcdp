#include "main.h"
#include "lib.h"
#include "uart.h"
#include "console.h"
#include "timer.h"
#include "cron.h"
#ifdef ENABLE_UARTMODULE

/* This is an UART module using the ATmega64C1 LIN
 * UART for RX and soft serial for TX. Funky. */

typedef unsigned int urxbufoff_t;
#define UART_BUFLEN 512
static unsigned char volatile uart_rcvbuf[UART_BUFLEN];
urxbufoff_t volatile uart_rcvwptr;
static urxbufoff_t volatile uart_rcvrptr;


ISR(LIN_TC_vect) {
	uint8_t d = LINDAT;
	urxbufoff_t reg = uart_rcvwptr;
	uart_rcvbuf[reg] = d;
	reg++;
	if(reg==UART_BUFLEN) reg = 0;
	uart_rcvwptr = reg;
}


/* This is 2s of our 16-bit timer. */
#define UART_TIMEOUT (F_CPU/512)

uint8_t uart_isdata(void) {
//	cli();
	urxbufoff_t rcvw = uart_rcvwptr;
//	sei();
	if (rcvw != uart_rcvrptr) return 1;
	return 0;
}

static void uart_waiting(void) {
	cli();
	if (uart_rcvwptr == uart_rcvrptr) { /* Race condition check */
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
	}
	sei();
}

int uart_recv_to(uint8_t to) {
	urxbufoff_t reg;
	unsigned char val;
	TCNT1 = 0;
	while (!uart_isdata()) {
		uart_waiting();
		if (to) if (TCNT1 > UART_TIMEOUT) return -1;
	}
	reg = uart_rcvrptr;
	val = uart_rcvbuf[reg++];
	if(reg==UART_BUFLEN) reg = 0;
	uart_rcvrptr = reg;
	return val;
}

uint8_t uart_recv(void) {
	return uart_recv_to(0);
}

/* 2Mbaud or 115200... */
#if 0
#define BTR 14
#define BRR 9
#else
#define BTR 8
#define BRR 0
#endif

void uart_init(void) {
	cli();
	TCCR1A = 0;
	TCCR1B = 5; // clkIO/1024

	PORTD |= _BV(2);
	PORTD |= _BV(4);
	DDRD |= _BV(2);
	uart_rcvwptr = 0;
	uart_rcvrptr = 0;
	LINBTR = _BV(LDISR) | BTR;
	LINCR = _BV(LENA) | _BV(LCMD2) | _BV(LCMD1);
	LINSIR = _BV(LERR) | _BV(LIDOK) | _BV(LTXOK) | _BV(LRXOK);
	LINENIR = _BV(LENRXOK);
	/* LINBTR to 8 for USE_2X and 16 for not would be compatible, but
	 * the LIN can do better. */
	/* We have pretty good 115200 @ 16Mhz here. */
	/* 16Mhz / ( 14 * (9+1)) */
	LINBTR = _BV(LDISR) | BTR;
	LINBRR = BRR;
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
