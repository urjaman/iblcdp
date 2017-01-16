#include "main.h"
#include "uart.h"
#include "ciface.h"
#include "console.h"
#include "appdb.h"

#ifdef ENABLE_UARTIF
#define RECVBUFLEN 64
#ifdef M64C1
const unsigned char prompt[] PROGMEM = "\x0D\x0AM64C1>";
#endif
#ifdef M1284
const unsigned char prompt[] PROGMEM = "\x0D\x0AM1284>";
#endif
struct ciface_info ciface_mi;

void uartif_run(void) {
	void(*func)(void);
	if (getline_mc(ciface_recvbuf,RECVBUFLEN)) {
		tokenize(ciface_recvbuf,tokenptrs, &token_count);
		if (token_count) {
			func = find_appdb(tokenptrs[0]);
			func();
		}
		sendstr_P((PGM_P)prompt);
	}
}
#else
void uartif_run(void) { }
#endif
