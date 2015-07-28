#include "main.h"
#include "uart.h"
#include "sluart.h"
#include "slslave.h"

#define UART_BUFLEN 256
typedef unsigned char urxbufoff_t;
static unsigned char uart_rcvbuf[UART_BUFLEN];
static urxbufoff_t uart_rcvwptr;
static urxbufoff_t uart_rcvrptr;

static unsigned char uart_sndbuf[UART_BUFLEN];
static urxbufoff_t uart_sndwptr;
static urxbufoff_t uart_sndrptr;
static uint8_t txbcnt = 0;

void sluart_putbyte(uint8_t d) {
	urxbufoff_t reg = uart_rcvwptr;
	uart_rcvbuf[reg] = d;
	reg++;
	uart_rcvwptr = reg;
}

static uint8_t sluart_getbyte(void) {
	urxbufoff_t reg;
	unsigned char val;
	reg = uart_sndrptr;
	val = uart_sndbuf[reg++];
	uart_sndrptr = reg;
	return val;
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
	urxbufoff_t reg = uart_sndwptr;
	uart_sndbuf[reg] = c;
	reg++;
	uart_sndwptr = reg;
	txbcnt++;
	if (txbcnt >= 15) {
		sluart_run();
	}
}

void sluart_run(void) {
	uint8_t txb[15];
	uint8_t txc = 0;
	while (txbcnt) {
		txb[txc++] = sluart_getbyte();
		txbcnt--;
		if (txc >= 15) break;
	}
	sl_add_tx(0,txc,txb);
}

void uart_init(void) {
	uart_rcvwptr = 0;
	uart_rcvrptr = 0;
	uart_sndwptr = 0;
	uart_sndrptr = 0;
	sei();
}

void uart_wait_txdone(void) {
	while (txbcnt) sluart_run();
}
