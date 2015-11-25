#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "slslave.h"
#include "sluart.h"

#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

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

static uint8_t sl_rxpct; /* Main only */


void slslave_init(void) {
	sl_txwo = 0;
	sl_txro = 0;
	sl_rxwo = 0;
	sl_rxro = 0;
	sl_rxpct = 0;
	DDRB |= _BV(6); // MISO
	SPDR = 0;
	SPCR = _BV(SPIE) | _BV(SPE);
	SPSR = 0;
}

ISR(SPI_STC_vect) {
	uint8_t d = SPDR;
	uint8_t tmp = sl_rxwo;
	sl_rxbuf[tmp++] = d;
	if (tmp>=RX_BUFLEN) tmp = 0;
	sl_rxwo = tmp;

	tmp = sl_txro;
	if (tmp != sl_txwo) {
		SPDR = sl_txbuf[tmp++];
		if (tmp>=TX_BUFLEN) tmp = 0;
		sl_txro = tmp;
	} else {
		SPDR = 0;
		timer_set_waiting();
	}
}

static void sl_parse_rx(void) {
	do {
		uint8_t d = sl_rxbuf[sl_rxro++];
		if (sl_rxro>=RX_BUFLEN) sl_rxro = 0;

		if (sl_rxpct) {
			switch (CHAN(sl_rxpct)) {
				case 0: /* Debug Data Channel. */
					sluart_putbyte(d);
					break;
				default: /* Unknown... */
					break;
			}
			uint8_t l = LEN(sl_rxpct)-1;
			if (l) sl_rxpct = CHANLEN(CHAN(sl_rxpct), l);
			else sl_rxpct = 0;
		} else {
			if (LEN(d)) {
				sl_rxpct = d;
			} else {
				/* Parse NUL messages (for non-0 channels) here if wanted. */
			}
		}
	} while (sl_rxwo != sl_rxro);
}

void slslave_run(void) {
	if (sl_rxwo != sl_rxro) {
		sl_parse_rx();
	}
}

static void sl_put_txbyte(uint8_t d) {
	uint8_t tmp = sl_txwo;
	sl_txbuf[tmp++] = d;
	if (tmp>=TX_BUFLEN) tmp = 0;
	sl_txwo = tmp;
}

uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf) {
	int used = sl_txwo - sl_txro;
	if (used < 0) used += TX_BUFLEN;
	uint8_t space = (TX_BUFLEN-1) - used;
	uint8_t need = l;
	uint8_t frames = 1;
	/* Encoding so we have atleast 1 frame (with 0 len) */
	/* And then no 0 len frames if 15 bytes... is a bit tricky. */
	if (l) frames += (l-1)/15;
	need += frames;
	if (space < need) {
		/* Let the caller figure out what to do if we cant
		 * send all . */
		 return 0;
	}
	uint8_t dato = 0;
	for (uint8_t f=0;f<frames;f++) {
		uint8_t ll = l - dato;
		if (ll > 15) ll = 15;
		sl_put_txbyte(CHANLEN(ch, ll));
		for (uint8_t n=0;n<ll;n++) {
			sl_put_txbyte(buf[dato+n]);
		}
		dato += ll;
	}
	return l;
}
