#include "main.h"
#include "console.h"
#include "uart.h"
#include "timer.h"
#include "avrpgm.h"
#include "avrpgmif.h"

uint8_t avrpgm_sercheck(uint8_t byte) {
	uint8_t buffer[AVR_PAGE_SIZE*2];
	uint16_t cmd;
	if (byte!=0xAA) return 0;
	avrp_init();
	if (avrp_lvp_entry()) {
		SEND('E');
		return 1;
	}
	SEND(0x55);
	SEND(AVR_PAGE_SIZE*2>255?0:AVR_PAGE_SIZE*2);
	do {
		cmd = uart_recv_to(200);
		cmd |= uart_recv_to(200) << 8;
		if (cmd != 0xFFFF) {
			uint8_t w = 1;
			for (int n=0;n<AVR_PAGE_SIZE*2;n++) {
				int b = uart_recv_to(200);
				if (b<0) {
					w = 0;
					break;
				}
				buffer[n] = b;
			}
			if (!w) {
				SEND('E');
				continue;
			}
			if (cmd==0) {
				avrp_chip_erase();
			}
			avrp_program_fw((void*)buffer, cmd * AVR_PAGE_SIZE, AVR_PAGE_SIZE);
			SEND(0x55);
		}
	} while (cmd != 0xFFFF);
	avrp_run_avr();
	return 1;
}
