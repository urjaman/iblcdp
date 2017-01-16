#include "main.h"
#include <avr/power.h>
#include "uart.h"
#include "console.h"
#include "appdb.h"
#include "lib.h"
#include "timer.h"
#include "slmaster.h"
#include "uartif.h"

/* Touch no UI from here. */
/* Make additional mainloop hooks for running the not-caller UI if needed. */
/* But for M64C1 the only UI is cli so we left that out from here. */
void mini_mainloop(void) {
	timer_run();
	slmaster_run();
}

void main (void) __attribute__ ((noreturn));

void main(void) {
	cli();
	PRR = _BV(PRCAN);
	uart_init();
	timer_init(); // must be after backlight init
	slmaster_init();
	sei();
	for(;;) {
		mini_mainloop();
		uartif_run();
	}
}


