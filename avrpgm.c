#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "avrpgm.h"
#include "slmaster.h"

#define AVR_TARGET_SIGNATURE 0x1E9706UL

#define sck(x) do { if (x) { PORTB |= _BV(7); } else { PORTB &= ~_BV(7); } } while(0);
#define mosi(x) do { if (x) { PORTB |= _BV(1); } else { PORTB &= ~_BV(1); } } while(0);
#define miso() PINB&_BV(0)


void avrp_init(void) {
	slmaster_control(0);
	DDRB |= _BV(7); // SCK
	DDRB |= _BV(1); // MOSI
	PORTD |= _BV(3); // Slave Select
	DDRD |= _BV(3);
#if 1
	SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR1);
	SPSR = 0;
#else
	sck(0);
#endif
}

static void reset(uint8_t v) {
	if (v) {
		DDRC &= ~_BV(7); // RST=OFF
		PORTC |= _BV(7);
	} else {
		PORTC &= ~_BV(7);
		DDRC |= _BV(7);
	}
}



static uint8_t avrp_spibyte(uint8_t d) {
#if 1
	uint8_t r;
	SPDR = d;
	loop_until_bit_is_set(SPSR,SPIF);
	r = SPDR;
#else
	// Assume: SCK=0
	uint8_t r=0;
	for (int i=0;i<8;i++) {
		mosi(d&0x80);
		d = d<<1;
		_delay_us(10);
		sck(1);
		r = r<<1;
		r |= miso();
		_delay_us(10);
 		sck(0);
	}
#endif
	return r;
}


static uint8_t avrp_read_busy(void) {
	avrp_spibyte(0xF0);
	avrp_spibyte(0x00);
	avrp_spibyte(0x00);
	return avrp_spibyte(0x00)&1;
}

void avrp_wait_busy(void) {
	uint16_t loops = 4000;
	uint8_t busy;
	do {
		busy = avrp_read_busy();
	} while ((--loops) && busy);
}


uint8_t avrp_lvp_entry(void) {
	uint8_t retries = 3;
	do {
		reset(1);
		_delay_us(100);
		reset(0);
		_delay_ms(25);
		// Serial Programming Enable
		avrp_spibyte(0xAC);
		avrp_spibyte(0x53);
		uint8_t echo = avrp_spibyte(0x00);
		avrp_spibyte(0x00);
		if (echo == 0x53)
			return 0;
	} while (retries--);
	return 1;
}

uint8_t avrp_read_signature(uint8_t lsb) {
	avrp_wait_busy();
	avrp_spibyte(0x30);
	avrp_spibyte(0x00);
	avrp_spibyte(lsb);
	return avrp_spibyte(0x00);
}

uint8_t avrp_vfy_signature(void) {
	uint32_t signature=0;
	signature |= ((uint32_t)avrp_read_signature(0))<<16;
	signature |= ((uint32_t)avrp_read_signature(1))<<8;
	signature |=       avrp_read_signature(2);
	dprint("SIGNATURE ");
//	dprint_hl(signature);
	if (signature==AVR_TARGET_SIGNATURE) {
		dprint(" OK\r\n");
		return 0;
	} else {
		dprint(" FAIL\r\n");
		return 1;
	}
}

uint16_t avrp_read_progmem(uint16_t addr) {
	uint16_t r;
	avrp_spibyte(0x20);
	avrp_spibyte(addr>>8);
	avrp_spibyte(addr&0xFF);
	r = avrp_spibyte(0x00);
	avrp_spibyte(0x28);
	avrp_spibyte(addr>>8);
	avrp_spibyte(addr&0xFF);
	r |= ((uint16_t)avrp_spibyte(0x00))<<8;
	return r;
}

void avrp_chip_erase(void) {
	avrp_wait_busy();
	avrp_spibyte(0xAC);
	avrp_spibyte(0x80);
	avrp_spibyte(0x00);
	avrp_spibyte(0x00);
}


static void avrp_program_page(const uint16_t * data, uint16_t pageaddr, uint16_t words) {
	avrp_wait_busy();
	for (uint16_t i=0;i<words;i++) {
		uint16_t addr = pageaddr+i;
		avrp_spibyte(0x40);
		avrp_spibyte(addr>>8);
		avrp_spibyte(addr&0xFF);
		avrp_spibyte(data[i]&0xFF);
		avrp_spibyte(0x48);
		avrp_spibyte(addr>>8);
		avrp_spibyte(addr&0xFF);
		avrp_spibyte((data[i]>>8)&0xFF);
	}
	avrp_spibyte(0x4C);
	avrp_spibyte(pageaddr>>8);
	avrp_spibyte(pageaddr&0xFF);
	avrp_spibyte(0x00);
	return;
}

void avrp_program_fw(const uint16_t * data, uint16_t baseaddr, uint16_t words) {
	uint16_t end = baseaddr+words;
	for (uint16_t i=baseaddr;i<end;i+=AVR_PAGE_SIZE) {
		int wleft = end-i;
		if (wleft>AVR_PAGE_SIZE) wleft = AVR_PAGE_SIZE;
		avrp_program_page(&(data[i-baseaddr]),i,wleft);
	}
}

uint8_t avrp_read_eeprom(uint16_t addr) {
	avrp_spibyte(0xA0);
	avrp_spibyte(addr >> 8);
	avrp_spibyte(addr&0xFF);
	return avrp_spibyte(0x00);
}

void avrp_write_eeprom(uint16_t addr, const uint8_t *d, uint16_t len) {
	for (uint16_t i=0;i<len;i++) {
		avrp_wait_busy();
		avrp_spibyte(0xC0);
		avrp_spibyte((addr+i) >> 8);
		avrp_spibyte((addr+i) & 0xFF);
		avrp_spibyte(d[i]);
	}
}

static void avrp_write_special(uint8_t thing, uint8_t value) {
	avrp_spibyte(0xAC);
	avrp_spibyte(thing);
	avrp_spibyte(0x00);
	avrp_spibyte(value);
	avrp_wait_busy();
}

static uint8_t avrp_read_special(uint8_t id) {
	switch (id) {
		case 0: // low fuse
			avrp_spibyte(0x50);
			avrp_spibyte(0x00);
			break;
		case 1: // hi fuse
			avrp_spibyte(0x58);
			avrp_spibyte(0x08);
			break;
		case 2: // ext fuse
			avrp_spibyte(0x50);
			avrp_spibyte(0x08);
			break;
	}
	avrp_spibyte(0x00);
	return avrp_spibyte(0x00);
}

uint8_t avrp_program_fuses(void) {
	// Extended Fuse = Not Programmed (We dont need to enable SELFPRGEN).
	const uint8_t fuse_hi =  0b11010001; // OCD=off, JTAG=off, SPI=on, WDT=off, EESAVE, default bl values
	const uint8_t fuse_lo =  0b11010111; // CKDIV8=no, CLKOUT=no, SUT=01, CKSEL=0110, Full-swing XO BOD enabled
	const uint8_t fuse_ext = 0b11111101; // BOD = 2.7V

	avrp_write_special(0xA0, fuse_lo);
	avrp_write_special(0xA8, fuse_hi);
	avrp_write_special(0xA4, fuse_ext);
	// Verify
	uint8_t fuselo_vfy = avrp_read_special(0);
	uint8_t fusehi_vfy = avrp_read_special(1);
	uint8_t fuseex_vfy = avrp_read_special(2);

	if (fuselo_vfy != fuse_lo) {
		dprint("FUSE_LO FAIL\r\n");
		return 1;
	}
	if (fusehi_vfy != fuse_hi) {
		dprint("FUSE_HI FAIL\r\n");
		return 2;
	}
	if (fuseex_vfy != fuse_ext) {
		dprint("FUSE_EXT FAIL\r\n");
		return 3;
	}
	return 0;
}

void avrp_run_avr(void) {
	_delay_us(10);
	reset(1);
	_delay_us(100);
	slmaster_control(1);
}

void avrp_halt_avr(void) {
	avrp_init();
	reset(0);
}

uint8_t avrp_test(void) {
	dprint("AVRPGM M1284\r\n");
	avrp_init();
	dprint("INIT\r\n");
	if (avrp_lvp_entry()) {
	 return 1;
	}
	dprint("ENTRY-OK\r\n");
	if (avrp_vfy_signature()) return 2;
	if (avrp_program_fuses()) return 5;
	dprint("FUSES-OK\r\n");
	avrp_run_avr();
	return 0;
}

