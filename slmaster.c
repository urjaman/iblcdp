#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "slmaster.h"
#include "commands.h"
#include <util/crc16.h>

#define TX_BUFLEN 64
#define RX_BUFLEN 64

/* Preliminary channel ideas:
 * 0 debug uart data channel (M1284 debug output, console commands M64C1->M1284)
 * 1 control message channel (debug uart on/off ... other similar stuff, eg. subchannels.)
 * 2 clock channel - calendar+valid_byte @ 1Hz M64C1 -> M1284. Calendar if user set time M1284 -> M64C1.
 * 3 ADC data channel, subchannels for all the ADC channels. @ 1Hz, both have ADC channels other doesnt have
 */

static volatile uint8_t sl_sync;
static uint8_t sl_rxwcnt;

static uint8_t sl_txbuf[TX_BUFLEN];
static uint8_t sl_txwo;
static volatile uint8_t sl_txro;

static uint8_t sl_rxbuf[RX_BUFLEN];
static volatile uint8_t sl_rxwo;
static uint8_t sl_rxro;


ISR(TIMER1_OVF_vect) {
	PORTD &= ~_BV(3);
	uint8_t tmp = sl_txro;
	if (tmp != sl_txwo) {
		SPDR = sl_txbuf[tmp++];
		tmp &= (TX_BUFLEN-1);
		sl_txro = tmp;
		sl_sync = 1;
		TIMSK1 = 0;
		return;
	}
	if ((sl_sync)&&(!sl_rxwcnt)) { // at a break in data, perform a high pulse on ~CS
		PORTD |= _BV(3);
		sl_sync = 0;
		return;
	}
	SPDR = 0; // filler data, the link is always "active"
	TIMSK1 = 0;
}

ISR(SPI_STC_vect) {
	uint8_t d = SPDR;
	if ((d)||(sl_rxwcnt)) {
		uint8_t tmp = sl_rxwo;
		sl_rxbuf[tmp++] = d;
		tmp &= (RX_BUFLEN-1);
		sl_rxwo = tmp;
		if (d) sl_rxwcnt = 16;
		else sl_rxwcnt--;
		if (sl_rxwcnt==0) sl_sync=1;
		timer_set_waiting();
		// the filler is 0s, and we dont write most of it, but we write the provenly
		// non-zero header and a fixed max amount of data after it
	}
	TIMSK1 = _BV(TOV1);
}

void slmaster_init(void) {
	sl_txwo = 0;
	sl_txro = 0;
	sl_rxwo = 0;
	sl_rxro = 0;
	// the startup sequence is to transfer some bytes, then sync even if there was no comms.
	sl_sync = 1;
	sl_rxwcnt = 50; // nominal 5ms and then sync

	DDRB |= _BV(7); // SCK
	DDRB |= _BV(1); // MOSI
	PORTD |= _BV(3); // Slave Select
	DDRD |= _BV(3);
	SPCR = _BV(SPE) | _BV(SPIE) | _BV(MSTR) | _BV(SPR0); // 16Mhz / 16 = 1Mhz SPI frq
	SPSR = 0;

	TCCR1A = _BV(WGM11) | _BV(WGM10);
	TCCR1B = _BV(WGM13) | _BV(WGM12);
	OCR1A = (F_CPU/1000)-1;
	TCNT1 = 0;
	TCCR1B |=  _BV(CS10);
	TIFR1 = _BV(TOV1);
	TIMSK1 = _BV(TOV1);
}

void slmaster_control(uint8_t enable) {
	if (enable) {
		/* Start from scratch... as did the M1284 just... */
		if (!TCCR1B) slmaster_init();
	} else {
		/* This purposefully jams us so we dont TX. */
		TCCR1B = 0;
		SPCR = 0;
		_delay_us(100);
	}
}

void slmaster_run(void) {
	while (sl_rxwo != sl_rxro) {
		uint8_t tmp = sl_rxro;
		uint8_t d = sl_rxbuf[tmp++];
		sl_rxro = tmp & (RX_BUFLEN-1);
		sl_parse_rx(d);
	}
}

void sl_put_txbyte(uint8_t d) {
	uint8_t tmp = sl_txwo;
	sl_txbuf[tmp++] = d;
	tmp &= TX_BUFLEN-1;
	sl_txwo = tmp;
}

uint8_t sl_tx_space(void) {
	int used = sl_txwo - sl_txro;
	if (used < 0) used += TX_BUFLEN;
	return (TX_BUFLEN-1) - used;
}
