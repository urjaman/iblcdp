#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "appdb.h"
#include "timer.h"
#include "avrpgm.h"
#include "commands.h"
#include "slmaster.h"

void avrp_cmd(void) {
	uint8_t d = avrp_test();
	sendcrlf();
	luint2outdual(d);
}


static uint8_t sldbg_active = 0;

void sldbg_putchar(uint8_t c) {
	if (sldbg_active) SEND(c);
}

void sldbg_cmd(void) {
	uint8_t c;
	sldbg_active = 1;
	while (1) {
		if (uart_isdata()) {
			c = RECEIVE();
			if (c == 'Q') break;
			sl_add_tx(0,1,&c);
		}
		mini_mainloop();
//		SEND('.');
	}
	sldbg_active = 0;
}
