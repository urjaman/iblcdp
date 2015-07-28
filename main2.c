#include "main.h"
#include <avr/power.h>
#include "uart.h"
#include "console.h"
#include "appdb.h"
#include "lib.h"
#include "slslave.h"
#include "sluart.h"
#include "timer.h"

#ifdef ENABLE_UARTIF
#define RECVBUFLEN 64
const unsigned char prompt[] PROGMEM = "\x0D\x0AM1284>";
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

void mini_mainloop_cli(void) {
	timer_run();
	slslave_run();
}

void mini_mainloop(void) {
	mini_mainloop_cli();
	uartif_run();
}

void main (void) __attribute__ ((noreturn));

void main(void) {
	cli();
	uart_init();
	timer_init(); // must be after backlight init
	slslave_init();
	sei();
	for(;;) {
		mini_mainloop();
	}
}

