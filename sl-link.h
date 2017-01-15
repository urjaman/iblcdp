#pragma once

#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

// sl-link.c
uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf);
void sl_parse_rx(uint8_t din);

// Implemented by slave/master
void sl_dispatch_rx(uint8_t ch, uint8_t l, uint8_t *buf);
void sl_put_txbyte(uint8_t d);
uint8_t sl_tx_space(void);

