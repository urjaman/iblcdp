#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "slslave.h"
#include "sluart.h"


#define TX_BUFLEN 128
#define RX_BUFLEN 128

/* Preliminary channel ideas:
 * 0 debug uart data channel (M1284 debug output, console commands M64C1->M1284)
 * 1 control message channel (debug uart on/off ... other similar stuff, eg. subchannels.)
 * 2 clock channel - calendar+valid_byte @ 1Hz M64C1 -> M1284. Calendar if user set time M1284 -> M64C1.
 * 3 ADC data channel, subchannels for all the ADC channels. @ 1Hz, both have ADC channels other doesnt have
 */

static uint8_t sl_txbuf[TX_BUFLEN];
static uint8_t volatile sl_txwo;
static uint8_t volatile sl_txro;

static uint8_t sl_rxbuf[RX_BUFLEN];
static uint8_t volatile sl_rxwo;
static uint8_t sl_rxro;

static uint8_t sl_rxwcnt;


void slslave_init(void) {
	sl_txwo = 0;
	sl_txro = 0;
	sl_rxwo = 0;
	sl_rxro = 0;
	sl_rxwcnt = 0;
	DDRB |= _BV(6); // MISO
	SPSR = 0;
	SPDR = 0;
	SPCR = _BV(SPIE) | _BV(SPE);
}

ISR(SPI_STC_vect) {
	uint8_t tmp = sl_rxwo;
	volatile uint8_t *p = sl_rxbuf + tmp++;
	uint8_t d = SPDR;
	if ((d)||(sl_rxwcnt)) {
		sl_rxwo = tmp & (RX_BUFLEN-1);
		*p = d;
		sl_rxwcnt--;
		if (d) sl_rxwcnt = 16;
		timer_set_waiting();
	}
	uint8_t w = 0;
	tmp = sl_txro;
	if (tmp != sl_txwo) {
		p = sl_txbuf + tmp++;
		sl_txro = tmp & (RX_BUFLEN-1);
		w = *p;
	}
	SPDR = w;
}

void slslave_run(void) {
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
	sl_txwo = tmp & (TX_BUFLEN-1);
}

uint8_t sl_tx_space(void) {
	int used = sl_txwo - sl_txro;
	if (used < 0) used += TX_BUFLEN;
	return (TX_BUFLEN-1) - used;
}
