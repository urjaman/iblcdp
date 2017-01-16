#include "main.h"
#include <avr/power.h>
#include "uart.h"
#include "console.h"
#include "appdb.h"
#include "lib.h"
#include "slslave.h"
#include "sluart.h"
#include "timer.h"
#include "rgbbl.h"
#include "lcd.h"
#include "adc.h"
#include "relay.h"
#include "backlight.h"
#include "buttons.h"
#include "tui.h"
#include "uartif.h"

void mini_mainloop_cli(void) {
	timer_run();
	adc_run();
	slslave_run();
	sluart_run();
	backlight_run();
	relay_run();
	buttons_get();
}

void mini_mainloop(void) {
	mini_mainloop_cli();
	uartif_run();
}

void main (void) __attribute__ ((noreturn));

void main(void) {
	cli();
	clock_prescale_set(clock_div_1);
	uart_init();
	timer_init();
	slslave_init();
	rgbbl_init();
	backlight_init();
	adc_init();
	lcd_init();
	buttons_init();
	relay_init();
	tui_init();
	sei();
	for(;;) {
		mini_mainloop();
		tui_run();
	}
}

