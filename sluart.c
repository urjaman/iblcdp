#include "main.h"
#include "uart.h"
#include "sluart.h"
#include "slslave.h"

#define UART_BUFLEN 256
typedef unsigned char urxbufoff_t;
static unsigned char uart_rcvbuf[UART_BUFLEN];
static urxbufoff_t uart_rcvwptr;
static urxbufoff_t uart_rcvrptr;

static uint8_t uart_txbuf[15];
static uint8_t uart_txc = 0;

void sluart_putbyte(uint8_t d) {
	urxbufoff_t reg = uart_rcvwptr;
	uart_rcvbuf[reg] = d;
	reg++;
	uart_rcvwptr = reg;
}

uint8_t uart_isdata(void) {
	if (uart_rcvwptr != uart_rcvrptr) return 1;
	return 0;
}

uint8_t uart_recv(void) {
	urxbufoff_t reg;
	unsigned char val;
	while (!uart_isdata()) mini_mainloop_cli();
	reg = uart_rcvrptr;
	val = uart_rcvbuf[reg++];
	uart_rcvrptr = reg;
	return val;
}

void uart_send(uint8_t c) {
	uart_txbuf[uart_txc++] = c;
	if (uart_txc>=8) {
		sl_add_tx(0,uart_txc,uart_txbuf);
		uart_txc = 0;
	}
}

void sluart_run(void) {
	if (uart_txc) {
		sl_add_tx(0,uart_txc,uart_txbuf);
		uart_txc = 0;
	}
}

void uart_init(void) {
	uart_rcvwptr = 0;
	uart_rcvrptr = 0;
	uart_txc = 0;
	sei();
}

void uart_wait_txdone(void) {
	sluart_run();
}
