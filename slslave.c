#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "slslave.h"
#include "sluart.h"

/* SPI transactions: */
/*          [ <---------------- bytecnt --------------> ] */
/* bytecnt  [ chan+len, [ len data ] ], [chan+len  ...  ] */
/* before each start of sequence, SS must go up and down. */

#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

#define BUFFERS 4
#define TX_BUFLEN 255
#define RX_BUFLEN 128

/* Preliminary channel ideas:
 * 0 debug uart data channel (M1284 debug output, console commands M64C1->M1284)
 * 1 control message channel (debug uart on/off ... other similar stuff, eg. subchannels.)
 * 2 clock channel - calendar+valid_byte @ 1Hz M64C1 -> M1284. Calendar if user set time M1284 -> M64C1.
 * 3 ADC data channel, subchannels for all the ADC channels. @ 1Hz, both have ADC channels other doesnt have
 */

static uint8_t tx_bufwp = 0;
static uint8_t volatile tx_bufrp = 0; /* Read out active from this buffer if != tx_bufwp. */

static struct sl_txbuffer {
	uint8_t off;
	uint8_t amt;
	uint8_t data[TX_BUFLEN];
} tx_buffers[BUFFERS];

static uint8_t volatile rx_bufwp = 0; /* Write to this buffer possibly active. */
static uint8_t rx_bufrp = 0;

static struct sl_rxbuffer {
	uint8_t off;
	uint8_t amt;
	uint8_t data[RX_BUFLEN];
} rx_buffers[BUFFERS];

void slslave_init(void) {
	PCMSK1 |= _BV(PCINT12);
	PCICR |= _BV(PCIE1);
	PCIFR = _BV(PCIF1);
	DDRB |= _BV(6); // MISO
	SPDR = 0;
	SPCR = _BV(SPIE) | _BV(SPE);
	SPSR = 0;
	for (uint8_t i=0;i<BUFFERS;i++) {
		tx_buffers[i].off = 0;
		tx_buffers[i].amt = 0;
		rx_buffers[i].off = 0;
		rx_buffers[i].amt = 0;
	}
	tx_bufwp = 0;
	tx_bufrp = 0;
	rx_bufwp = 0;
	rx_bufrp = 0;
}

ISR(PCINT1_vect) {
	/* On De-SS . */
	if (PINB & _BV(4)) {
		rx_buffers[rx_bufwp].amt = 0;
	}
}

ISR(SPI_STC_vect) {
	uint8_t wc = SPSR & _BV(WCOL);
	uint8_t d = SPDR;
	uint8_t tmp = tx_bufrp;
	if (tmp != tx_bufwp) {
		if (wc) {
			/* Assume: They got a 0, too bad, send proper value next time... */
			SPDR = tx_buffers[tmp].amt;
		} else {
			uint8_t off = tx_buffers[tmp].off;
			SPDR = tx_buffers[tmp].data[off];
			off++;
			tx_buffers[tmp].off = off;
			if (off >= tx_buffers[tmp].amt) {
				tx_buffers[tmp].off = 0;
				tx_buffers[tmp].amt = 0;
				tmp++;
				if (tmp >= BUFFERS) tmp = 0;
				tx_bufrp = tmp;
				timer_set_waiting();
			}
		}
	} else {
		SPDR = 0;
	}
	tmp = rx_bufwp;
	if (rx_buffers[tmp].amt) {
		uint8_t off = rx_buffers[tmp].off;
		rx_buffers[tmp].data[off] = d;
		off++;
		rx_buffers[tmp].off = off;
		if (off >= rx_buffers[tmp].amt) {
			rx_buffers[tmp].amt = 0;
			tmp++;
			if (tmp >= BUFFERS) tmp = 0;
			rx_bufwp = tmp;
			timer_set_waiting(); /* Main loop Go Go Go ! */
		}
	} else if (d) {
		/* This does mean that during a so-far-TX-only
		 * transaction the master can at any point send
		 * non-zero to start a RX packet.
		 * I think this is fine. (Master doesnt do that though.) */
		/* Also, they could chain more than one RX packet per our TX packet session.. */
		rx_buffers[tmp].off = 0;
		rx_buffers[tmp].amt = d;
	}
}

static void sl_parse_rx(uint8_t *buf, uint8_t amt);

void slslave_run(void) {
	if (rx_bufrp != rx_bufwp) {
		/* Read out rx_bufrp */
		sl_parse_rx(rx_buffers[rx_bufrp].data, rx_buffers[rx_bufrp].off);
		rx_bufrp++;
		if (rx_bufrp >= BUFFERS) rx_bufrp = 0;
	}
	if ((tx_bufrp == tx_bufwp)&&(tx_buffers[tx_bufwp].amt)) { /* Schedule for TX. */
		uint8_t amt = tx_buffers[tx_bufwp].amt;
		tx_bufwp++;
		if (tx_bufwp>=BUFFERS) tx_bufwp = 0;
		SPDR = amt;
	}
}

static void sl_parse_rx(uint8_t *buf, uint8_t amt) {
	uint8_t i=0;
	do {
		uint8_t chanlen = buf[i++];
		switch (CHAN(chanlen)) {
			case 0: /* Debug Data Channel. */
				for (uint8_t n=0;n<LEN(chanlen);n++) sluart_putbyte(buf[i+n]);
				break;
			default: /* Dunno what this is... */
				break;
		}
		i += LEN(chanlen);
	} while (i<amt);
}

uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf) {
	uint8_t space = TX_BUFLEN - tx_buffers[tx_bufwp].amt;
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
	uint8_t *txb = tx_buffers[tx_bufwp].data + tx_buffers[tx_bufwp].amt;
	uint8_t dato = 0;
	for (uint8_t f=0;f<frames;f++) {
		uint8_t ll = l - dato;
		if (ll > 15) ll = 15;
		txb[0] = CHANLEN(ch, ll);
		memcpy(txb+1, buf+dato, ll);
		txb += (ll+1);
		dato += ll;
	}
	tx_buffers[tx_bufwp].amt += need;
	return l;
}
