#pragma once

#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

#define SL_MAX_CH 4

// sl-link "public" API
uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf);
int sl_reg_ch_handler(uint8_t ch, void(*cb)(uint8_t, uint8_t, uint8_t*) );
int sl_unreg_ch_handler(uint8_t ch);

// to be used by sl{slave,master}.c
void sl_parse_rx(uint8_t din);

// Implemented by sl{slave/master}.c for sl-link.c
void sl_put_txbyte(uint8_t d);
uint8_t sl_tx_space(void);

