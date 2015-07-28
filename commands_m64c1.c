#include "main.h"
#include "uart.h"
#include "console.h"
#include "lib.h"
#include "appdb.h"
#include "timer.h"
#include "avrpgm.h"
#include "commands.h"

void avrp_cmd(void) {
	uint8_t d = avrp_test();
	sendcrlf();
	luint2outdual(d);
}
