#include "main.h"
#include "i2c.h"


#define I2CUART_RHR 0x00
#define I2CUART_THR 0x00
#define I2CUART_IER 0x01
#define I2CUART_IER 0x02


/* Generic functions to handle an I2C UART. Address provided by module user. */

void i2c-uart_init(uint8_t addr, 

uint8_t i2c-uart_poll_rx(uint8_t addr) {
	}
