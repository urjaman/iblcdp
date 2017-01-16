#pragma once

#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

/* channel ideas:
 * 0 debug uart data channel (M1284 debug output, console commands M64C1->M1284)
 * 1 clock channel - calendar+valid_byte @ 1Hz M64C1 -> M1284. Calendar if user set time M1284 -> M64C1.
 * 2 ADC data channel, subchannels for all the ADC channels. @ 1Hz, both have ADC channels other doesnt have
 */

enum sl_ch {
	SL_CH_DBG = 0,
	SL_CH_CLK, // M64C1 -> M1284 calendar time sync; M1284 -> M64C1 set if by user / if M64C1 reset
	SL_CH_ADC,
	SL_MAX_CH
};

// sl-link "public" API
uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf);
int sl_reg_ch_handler(uint8_t ch, void(*cb)(uint8_t, uint8_t, uint8_t*) );
int sl_unreg_ch_handler(uint8_t ch);

// Used by sl{slave,master}.c
void sl_parse_rx(uint8_t din);

// Implemented by sl{slave/master}.c for sl-link.c
void sl_put_txbyte(uint8_t d);
uint8_t sl_tx_space(void);

