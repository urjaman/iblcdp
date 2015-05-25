#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "avrpgm.h"
#include "avrpgmif.h"

static uint8_t ser_state = 0;
static uint16_t address = 0;
static uint8_t connected = 0;

static uint8_t BlockLoad(uint16_t size, uint8_t mem) {
	uint8_t buffer[AVR_PAGE_SIZE*2];
	uint16_t i;
	if (!connected) return '?';
	if ((mem != 'E') && (mem != 'F')) return '?';
	for (i=0;i<size;i++) buffer[i] = RECEIVE();
	if (mem == 'E') {
		avrp_write_eeprom(address, buffer, size);
		address += size;
		return '\r';
	}
	if (mem == 'F') {
		avrp_program_fw((void*)buffer, address, size/2);
		address += size/2;
		return '\r';
	}
	return '?';
}

static void BlockRead(uint16_t size, uint8_t mem) {
	avrp_wait_busy();
	if (mem == 'E') {
		do {
			SEND(avrp_read_eeprom(address));
			address++;
			size--;
		} while (size);
	} else if (mem == 'F') {
		do {
			uint16_t d = avrp_read_progmem(address++);
			SEND(d & 0xFF);
			SEND(d >> 8);
			size -= 2;
		} while (size);
	}
}

uint8_t avrpgm_sercheck(uint8_t byte) {
	uint16_t tmpw;
	uint8_t tmpb;
	switch (ser_state) {
		case 0:
			if (byte == 0x1B) {
				ser_state++;
				return 1;
			}
			break;
		case 1:
			if (byte == 'S') {
				ser_state++;
			}
			break;
	}
	if (ser_state < 2) {
		ser_state = 0;
		return 0;
	}
	switch (byte) {
		case 0x1B:
			break;
		default:
			SEND('?');
			break;

		case 'S':
			sendstr_P(PSTR("AVRBOOT"));
			break;
		case 'a':
			SEND('Y');
			break;
		case 'A':
			address = RECEIVE() << 8;
			address |= RECEIVE();
			SEND('\r');
			break;
		case 'e':
			if (!connected) {
				SEND('?');
				break;
			}
			avrp_chip_erase();
			SEND('\r');
			break;
		case 'b':
			SEND('Y');
			SEND((AVR_PAGE_SIZE*2) >> 8);
			SEND((AVR_PAGE_SIZE*2) & 0xFF);
			break;
		case 'B':
			tmpw = RECEIVE() << 8;
			tmpw |= RECEIVE();
			tmpb = RECEIVE();
			SEND( BlockLoad(tmpw, tmpb) );
			break;
		case 'g':
			tmpw = RECEIVE() << 8;
			tmpw |= RECEIVE();
			tmpb = RECEIVE();
			BlockRead(tmpw, tmpb);
			break;
		case 'R':
		case 'c':
		case 'C':
		case 'm':
		case 'D':
		case 'd':
		case 'l':
			SEND('?');
			break;
		case 'r':
			SEND(0xFF);
			break;
		case 'F':
		case 'N':
		case 'Q':
			SEND(0);
			break;
		case 'P':
			avrp_init();
			if (avrp_lvp_entry()) {
				SEND('?');
				break;
			}
			connected = 1;
			SEND('\r');
			break;
		case 'L':
			connected = 0;
			avrp_run_avr();
			SEND('\r');
			break;
		case 'E':
			SEND('\r');
			if (connected) {
				avrp_run_avr();
				connected = 0;
			}
			ser_state = 0;
			break;
		case 'p':
			SEND('S');
			break;
		case 't':
			SEND(1);
			SEND(0);
			break;
		case 'x':
		case 'y':
		case 'T':
			RECEIVE();
			SEND('\r');
			break;
		case 'V':
		case 'v':
			sendstr_P(PSTR("99"));
			break;
		case 's':
			if (!connected) {
				SEND(0xFF);
				SEND(0xFF);
				SEND(0xFF);
				break;
			}
			SEND(avrp_read_signature(2));
			SEND(avrp_read_signature(1));
			SEND(avrp_read_signature(0));
			break;
	}
	return 1;
}

