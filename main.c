#include "main.h"
#include <avr/power.h>
#include "uart.h"
#include "console.h"
#include "appdb.h"
#include "lib.h"
#include "timer.h"
#include "slmaster.h"

#ifdef ENABLE_UARTIF
#define RECVBUFLEN 64
const unsigned char prompt[] PROGMEM = "\x0D\x0AM64C1>";
unsigned char recvbuf[RECVBUFLEN];
unsigned char token_count;
unsigned char* tokenptrs[MAXTOKENS];

static void uartif_run(void) {
	void(*func)(void);
	if (getline_mc(recvbuf,RECVBUFLEN)) {
		tokenize(recvbuf,tokenptrs, &token_count);
		if (token_count) {
			func = find_appdb(tokenptrs[0]);
			func();
		}
		sendstr_P((PGM_P)prompt);
	}
}
#else
static void uartif_run(void) { }
#endif


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


