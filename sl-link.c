#include "main.h"
#include "sl-link.h"
#include <util/crc16.h>


static uint16_t sl_stat_bytes = 0;
static uint16_t sl_stat_crc_errs = 0;
static uint16_t sl_stat_frames = 0;
static uint16_t sl_stat_ignframes = 0;


static void (*sl_handlers[SL_MAX_CH])(uint8_t, uint8_t, uint8_t*) = { };

int sl_reg_ch_handler(uint8_t ch, void(*cb)(uint8_t, uint8_t, uint8_t*) ) {
	if (ch >= SL_MAX_CH) {
		return -2; // out of space, in other way...
	}
	if (sl_handlers[ch]) {
		return -1; // we're stuffed, sorry
	}
	sl_handlers[ch] = cb;
	return 0; // a-ok ;)
}

int sl_unreg_ch_handler(uint8_t ch) {
	if (ch >= SL_MAX_CH) {
		return -1; // this not a channel
	}
	sl_handlers[ch] = 0;
	return 0; // done anyways
}

static void sl_dispatch_rx(uint8_t ch, uint8_t l, uint8_t* buf)
{
	sl_stat_frames++;
	if (ch >= SL_MAX_CH) {
		sl_stat_ignframes++;
		return;
	}
	if (!sl_handlers[ch]) {
		sl_stat_ignframes++;
		return;
	}
	sl_handlers[ch](ch, l, buf);
}

void sl_parse_rx(uint8_t din) {
	static enum rxs {
		S_FB1 = 0,
		S_FB2,
		S_DAT,
		S_CRC
	} s = S_FB1;
	static uint8_t s_crc;
	static uint8_t s_chid;
	static uint8_t s_frame[15];
	static uint8_t s_doff;
	sl_stat_bytes++;
	switch (s) {
		default:
		case S_FB1:
			if ((!din)||(din == 0xFF)) return; // ignore fillers or invalid
			s_chid = din;
			s = S_FB2;
			return;
		case S_FB2:
			if ((din == 0)||(din == 0xFF)) {
				s = S_FB1;
				return;
			}
			{
				uint8_t ndin = ~din;
				if (s_chid != ndin) {
					s_chid = din;
					return;
				}
			}
			s_crc = _crc_ibutton_update(0, s_chid);
			s_crc = _crc_ibutton_update(s_crc, din);
			s_doff = 0;
			if (LEN(s_chid)) s = S_DAT;
			else s = S_CRC;
			return;
		case S_DAT:
			s_frame[s_doff++] = din;
			s_crc = _crc_ibutton_update(s_crc, din);
			if (s_doff >= LEN(s_chid)) s = S_CRC;
			return;
		case S_CRC:
			s = S_FB1;
			if (din != s_crc) {
				sl_stat_crc_errs++;
				return;
			}
			sl_dispatch_rx(CHAN(s_chid), LEN(s_chid), s_frame);
			return;
	}
}


uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf) {
	uint8_t space = sl_tx_space();
	uint8_t frm_sz = (ch==15) ? 14 : 15;
	uint8_t need = l;
	uint8_t frames = 1;
	/* Encoding so we have atleast 1 frame (with 0 len) */
	/* And then no 0 len frames if 15 bytes... is a bit tricky. */
	if (l) frames += (l-1)/ frm_sz;
	need += frames*3;
	if (space < need) {
		/* Let the caller figure out what to do if we cant
		 * send all . */
		 return 0;
	}
	uint8_t dato = 0;
	for (uint8_t f=0;f<frames;f++) {
		uint8_t ll = l - dato;
		if (ll > frm_sz) ll = frm_sz;
		uint8_t chid = CHANLEN(ch, ll);
		uint8_t crc = _crc_ibutton_update(0, chid);
		crc =_crc_ibutton_update(crc, ~chid);
		sl_put_txbyte(chid);
		sl_put_txbyte(~chid);
		for (uint8_t n=0;n<ll;n++) {
			sl_put_txbyte(buf[dato+n]);
			crc = _crc_ibutton_update(crc, buf[dato+n]);
		}
		sl_put_txbyte(crc);
		dato += ll;
	}
	return l;
}
