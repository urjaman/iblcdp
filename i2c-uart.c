#include "main.h"
#include "i2c.h"
#include "timer.h"

#define UART_XTAL 14745600UL

#define RG(x) ((x)<<3)

#define RHR RG(0)
#define THR RG(0)
#define IER RG(1)
#define FCR RG(2)
#define IIR RG(2)
#define LCR RG(3)
#define MCR RG(4)
#define LSR RG(5)
#define MSR RG(6)
#define SPR RG(7)
#define TXLVL RG(8)
#define RXLVL RG(9)
#define IODIR RG(0xA)
#define IOSTAT RG(0xB)
#define IOIENA RG(0xC)
#define IOCTRL RG(0xE)
#define EFCR RG(0xF)

/* MCR[2] = 1 and EFR[4] = 1 */
#define TCR RG(0x6)
#define TLR RG(0x7)

/* LCR[7] = 1, and != 0xBF */
#define DLL RG(0x0)
#define DLH RG(0x1)

/* LCR == 0xBF */
#define EFR RG(0x2)
#define XON1 RG(0x4)
#define XON2 RG(0x5)
#define XOFF1 RG(0x6)
#define XOFF2 RG(0x7)


/* Generic functions to handle an I2C UART. Address provided by module user. */

/* Return value: recommended poll period in subsectimer units. */
uint16_t i2cuart_init(uint8_t addr, uint24_t baud)
{
	/* We can generate baud rates between 15 baud and 921600 without div-4. */
	/* => Using the extra divisor not worth the code. */
	/* TODO: Implement rounding if needed here. */
	/* Note: cast baud to uint32_t if we need >1Mbaud. */
	uint16_t divisor = UART_XTAL / (baud*16);
	if (i2c_write_reg(addr,LCR,0x83)) goto err;
	if (i2c_write_reg(addr,DLH,divisor>>8)) goto err;
	if (i2c_write_reg(addr,DLL,divisor&0xFF)) goto err;
	if (i2c_write_reg(addr,LCR,0xBF)) goto err;
	if (i2c_write_reg(addr,EFR,0x10)) goto err;
	if (i2c_write_reg(addr,LCR,0x03)) goto err;
	if (i2c_write_reg(addr,MCR,0x04)) goto err;
	if (i2c_write_reg(addr,TCR,0x2C)) goto err;
	if (i2c_write_reg(addr,TLR,0x00)) goto err;
	if (i2c_write_reg(addr,MCR,0x00)) goto err;
	if (i2c_write_reg(addr,LCR,0xBF)) goto err;
	if (i2c_write_reg(addr,EFR,0x50)) goto err;
	if (i2c_write_reg(addr,LCR,0x03)) goto err;
	if (i2c_write_reg(addr,IOCTRL,0x02)) goto err;
	if (i2c_write_reg(addr,FCR,0x07)) goto err;
	uint16_t pollhz = (baud/10) / 48;
	return (SSTC/pollhz)+1;

err:
	return SSTC; /* No poll, no UART. */
}

uint8_t i2cuart_poll_rx(uint8_t addr, uint8_t *slsr) {
	uint8_t lsr;
	uint8_t cnt=0;
	if (i2c_read_reg(addr,LSR,&lsr)) goto err;
	if (slsr) *slsr = lsr;
	if (lsr&1) {
		if (i2c_read_reg(addr,RXLVL,&cnt)) goto err;
	}
	return cnt;
err:
	return 0; // No data, NO UART.
}

uint8_t i2cuart_readfifo(uint8_t addr, uint8_t *buf, uint8_t cnt) {
	if (i2c_read_regs(addr,RHR,cnt,buf)) return 0; /* Nothing was read. */
	return cnt; /* OK */
}

void i2cuart_writefifo(uint8_t addr, uint8_t *buf, uint8_t cnt) {
	while (cnt) {
		uint8_t txamt;
		if (i2c_read_reg(addr,TXLVL,&txamt)) goto err;
		if (cnt<txamt) txamt = cnt;
		if (txamt) {
			if (i2c_write_regs(addr,THR,txamt,buf)) goto err;
			cnt -= txamt;
			buf += txamt;
		}
	}
err:
	// Discard data...
	return;
}

uint8_t i2cuart_exists(uint8_t addr) {
	uint8_t lsr;
	if (i2c_read_reg(addr,LSR,&lsr)) return 0;
	return 1;
}

