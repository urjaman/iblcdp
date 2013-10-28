
/* Return value: recommended poll period in subsectimer units. */
uint16_t i2cuart_init(uint8_t addr, uint24_t baud);
uint8_t i2cuart_poll_rx(uint8_t addr, uint8_t *slsr);
uint8_t i2cuart_poll_tx(uint8_t addr);
uint8_t i2cuart_readfifo(uint8_t addr, uint8_t *buf, uint8_t cnt);
void i2cuart_writefifo(uint8_t addr, uint8_t *buf, uint8_t cnt);
uint8_t i2cuart_exists(uint8_t addr);
