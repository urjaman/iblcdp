#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "appdb.h"
#include "timer.h"
#include "avrpgm.h"
#include "slmaster.h"

CIFACE_APP(avrp_cmd, "AVRP")
{
	uint8_t d = avrp_test();
	sendcrlf();
	luint2outdual(d);
}


static void sldbg_handler(uint8_t ch, uint8_t l, uint8_t *buf) {
	if (ch!=0) return;
	for (uint8_t n=0;n<l;n++) SEND(buf[n]);
}

CIFACE_APP(sldbg_cmd, "SLDBG")
{
	uint8_t c;
	sl_reg_ch_handler(0, sldbg_handler);
	while (1) {
		if (uart_isdata()) {
			c = RECEIVE();
			if (c == 'Q') break;
			sl_add_tx(0,1,&c);
		}
		mini_mainloop();
	}
	sl_unreg_ch_handler(0);
}
