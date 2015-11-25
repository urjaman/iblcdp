#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "slmaster.h"
#include "commands.h"

#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

#define TX_BUFLEN 128
#define RX_BUFLEN 16

/* Preliminary channel ideas:
 * 0 debug uart data channel (M1284 debug output, console commands M64C1->M1284)
 * 1 control message channel (debug uart on/off ... other similar stuff, eg. subchannels.)
 * 2 clock channel - calendar+valid_byte @ 1Hz M64C1 -> M1284. Calendar if user set time M1284 -> M64C1.
 * 3 ADC data channel, subchannels for all the ADC channels. @ 1Hz, both have ADC channels other doesnt have
 */

static uint8_t sl_active = 0;
static uint8_t sl_halt = 0;

static uint8_t sl_txbuf[TX_BUFLEN];
static uint8_t sl_txwo;
static uint8_t sl_txro;

static uint8_t sl_rxbuf[RX_BUFLEN];
static uint8_t sl_rxwo;
static uint8_t sl_rxro;

static uint8_t sl_rxcnt;
static uint8_t sl_rxpct;

void slmaster_init(void) {
	DDRB |= _BV(7); // SCK
	DDRB |= _BV(1); // MOSI
	PORTD |= _BV(3); // Slave Select
	DDRD |= _BV(3);
	SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);
	SPSR = _BV(SPI2X);
	SPSR = 0;
	sl_txwo = 0;
	sl_txro = 0;
	sl_rxwo = 0;
	sl_rxro = 0;
	sl_rxcnt = 0;
	sl_rxpct = 0;
	sl_active = 0;
	sl_halt = 0;
}

void slmaster_control(uint8_t enable) {
	if (enable) {
		/* Start from scratch... as did the M1284 just... */
		slmaster_init();
	} else {
		/* This purposefully jams us so we dont TX. */
		sl_halt = 1;
	}
}

static void sl_parse_rx(void);

static void slmaster_tx_one(void)
{
	uint8_t tmp = sl_txro;
	if (tmp != sl_txwo) {
		SPDR = sl_txbuf[tmp++];
		if (tmp>=TX_BUFLEN) tmp = 0;
		sl_txro = tmp;
	} else {
		if (sl_rxcnt) {
			SPDR = 0;
		} else {
			sl_active--;
			if (sl_active) {
				SPDR = 0;
			}
		}
	}
	timer_set_waiting();
}

void slmaster_run(void) {
	uint8_t trigger = 0;
	if (sl_rxwo != sl_rxro) {
		/* Read out rx_bufrp */
		sl_parse_rx();
		trigger = 1; /* Ask once for more data after we've parsed what we got. */
	}
	if (sl_halt) return;
	if (sl_active) {
		if (SPSR & _BV(SPIF)) {
			uint8_t d = SPDR;
			if ((sl_rxcnt)||(d)) {
			if (!sl_rxcnt) {
				sl_rxcnt = LEN(d);
			} else {
				sl_rxcnt--;
			}
			uint8_t tmp = sl_rxwo;
			sl_rxbuf[tmp++] = d;
			if (tmp>=RX_BUFLEN) tmp = 0;
			sl_rxwo = tmp;
			}
			_delay_us(10); /* Because our SPIs are so unbuffered, need to give slave time to run ISR. */
			slmaster_tx_one();
		}
		return;
	}
	if ((sl_txwo != sl_txro)||(timer_get_5hzp())||(trigger) ) {
		/* Initiate a transaction. */
		PORTD |= _BV(3);
		_delay_us(5);
		PORTD &= ~_BV(3);
		_delay_us(1);
		sl_active = 32;
		slmaster_tx_one();
	}
}

static void sl_parse_rx(void) {
	do {
		uint8_t d = sl_rxbuf[sl_rxro++];
		if (sl_rxro>=RX_BUFLEN) sl_rxro = 0;

		if (sl_rxpct) {
			switch (CHAN(sl_rxpct)) {
				case 0: /* Debug Data Channel. */
					sldbg_putchar(d);
					break;
				default: /* Unknown... */
					SEND('#');
					sldbg_putchar(d);
					break;
			}
			uint8_t l = LEN(sl_rxpct)-1;
			if (l) sl_rxpct = CHANLEN(CHAN(sl_rxpct), l);
			else sl_rxpct = 0;
		} else {
			if (CHAN(d)) {
				luint2outdual(d);
			}
			if (LEN(d)) {
				sl_rxpct = d;
			} else {
				/* Parse NUL messages (for non-0 channels) here if wanted. */
			}
		}
	} while (sl_rxwo != sl_rxro);
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
