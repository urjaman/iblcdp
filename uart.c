#include "main.h"
#include "lib.h"
#include "uart.h"
#include "console.h"
#include "timer.h"
#include "i2c-uart.h"
#include "cron.h"
#ifdef ENABLE_UARTMODULE

// UART MODULE START
typedef unsigned char urxbufoff_t;
#define UART_BUFLEN 16
unsigned char volatile uart_rcvbuf[UART_BUFLEN];
urxbufoff_t volatile uart_rcvwptr;
urxbufoff_t volatile uart_rcvrptr;


ISR(USART_RX_vect) {
	urxbufoff_t reg = uart_rcvwptr;
	uart_rcvbuf[reg++] = UDR0;
	if(reg==UART_BUFLEN) reg = 0;
	uart_rcvwptr = reg;
}


ISR(USART_UDRE_vect) {
	utxbufoff_t reg = uart_sndrptr;
	if (uart_sndwptr != reg) {
		UDR0 = uart_sndbuf[reg++];
		if (reg==UARTTX_BUFLEN) reg = 0;
		uart_sndrptr = reg;
		return;
	} else {
		UCSR0B &= ~_BV(5);
		return;
	}
}


uint8_t uart_isdata(void) {
	if (uart_rcvwptr != uart_rcvrptr) { return 1; }
	else { return 0; }
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

void uart_send(uint8_t val) {
	utxbufoff_t reg;
	while (uart_sndwptr+1==uart_sndrptr || (uart_sndwptr+1==UARTTX_BUFLEN && !uart_sndrptr)); // wait for space in buf
	cli();
	reg = uart_sndwptr;
	UCSR0B |= _BV(5); // make sure the transmit int is on
	uart_sndbuf[reg++] = val; // add byte to the transmit queue
	if(reg==UARTTX_BUFLEN) reg = 0;
	uart_sndwptr = reg;
	sei();
	}

void uart_init(void) {
	cli();

#include <util/setbaud.h>
// Assuming uart.h defines BAUD
	uart_rcvwptr = 0;
	uart_rcvrptr = 0;
	uart_sndwptr = 0;
	uart_sndrptr = 0;
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0C = 0x06; // Async USART,No Parity,1 stop bit, 8 bit chars
	UCSR0A &= 0xFC;
#if USE_2X
	UCSR0A |= (1 << U2X0);
#endif
	UCSR0B = 0xB8; // RX complete interrupt enable, UDRE int en, Receiver & Transmitter enable
	sei();
	}

void uart_wait_txdone(void) {
	while (uart_sndwptr != uart_sndrptr);
}

#else
#ifdef ENABLE_I2CUARTCON
#define I2CUART_MAX_RX 64
#define I2CUART_POLL_PERIOD 10
static int i2cuarthw_exists = 0;
static uint8_t i2cuart_lastpoll = 0;
static uint8_t i2cuart_buf[I2CUART_MAX_RX];
static uint8_t i2cuart_buf_sz = 0;
static uint8_t i2cuart_buf_of = 0;

static void i2cuart_poll_task(void);
static struct cron_task i2cuart_poll = { NULL, i2cuart_poll_task, SSTC, 0 };


void uart_wait_txdone(void) { }

void uart_init(void) {
	i2cuart_buf_sz = 0;
	i2cuart_buf_of = 0;
	if (i2cuart_exists(ENABLE_I2CUARTCON)) {
		i2cuart_poll.ss_freq = i2cuart_init(ENABLE_I2CUARTCON, BAUD);
		i2cuarthw_exists = 1;
		cron_add_task(&i2cuart_poll);
	} else {
		i2cuarthw_exists = 0;
	}
	i2cuart_lastpoll =  timer_get_5hz_cnt();
}

static uint8_t i2cuart_exist_poll(void) {
	if (!i2cuarthw_exists) {
		uint8_t diff = timer_get_5hz_cnt() - i2cuart_lastpoll;
		if (diff>=I2CUART_POLL_PERIOD) {
			uart_init();
		}
	} else {
		if (!i2cuart_exists(ENABLE_I2CUARTCON)) {
			i2cuarthw_exists = 0;
		}
		i2cuart_lastpoll =  timer_get_5hz_cnt();
	}
	return i2cuarthw_exists;
}

void uart_send(uint8_t val) {
	if (!i2cuarthw_exists) return;
	i2cuart_writefifo(ENABLE_I2CUARTCON, &val, 1);
}

static uint8_t i2cuart_rx_data(void) {
	if (!i2cuart_exist_poll()) return 0;
	uint8_t d = i2cuart_poll_rx(ENABLE_I2CUARTCON, NULL);
	if (!d) return 0;
	if (d>I2CUART_MAX_RX) d = I2CUART_MAX_RX;
	if ((d=i2cuart_readfifo(ENABLE_I2CUARTCON, i2cuart_buf, d))) {
		i2cuart_buf_of = 0;
		i2cuart_buf_sz = d;
	}
	return d;
}

static void i2cuart_poll_task(void) {
	if (uart_isdata()) {
		timer_set_waiting();
	}
	if (!i2cuarthw_exists) {
		cron_rm_task(&i2cuart_poll);
	}
}

uint8_t uart_recv(void) {
ret_buf:
	if (i2cuart_buf_sz) {
		uint8_t c = i2cuart_buf[i2cuart_buf_of++];
		if (i2cuart_buf_of>=i2cuart_buf_sz) {
			i2cuart_buf_of = 0;
			i2cuart_buf_sz = 0;
		}
		return c;
	}
wait_data:
	if (i2cuart_rx_data()) {
		goto ret_buf;
	} else {
		if (i2cuart_exist_poll()) {
			goto wait_data;
		}
	}
	return 0;
}

uint8_t uart_isdata(void) {
	if (i2cuart_buf_sz) {
		return 1;
	}
	if (i2cuart_rx_data()) {
		return 1;
	}
	return 0;
}

#else
void uart_wait_txdone(void) { }
void uart_init(void) { }
void uart_send(uint8_t val) { val = val; }
uint8_t uart_recv(void) { return 0; }
uint8_t uart_isdata(void) { return 0; }
#endif
#endif
