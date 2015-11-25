#include "main.h"
#include "console.h"
#include "appdb.h"

#ifdef ENABLE_UARTIF

const unsigned char echostr[] PROGMEM = "ECHO";
const unsigned char calcstr[] PROGMEM = "CALC";
#ifdef M64C1
const unsigned char avrpstr[] PROGMEM = "AVRP";
const unsigned char sldbgstr[] PROGMEM = "SLDBG";
#else
const unsigned char lcdistr[] PROGMEM = "LINIT";
const unsigned char lcdcstr[] PROGMEM = "LCHAR";
const unsigned char lcdc2str[] PROGMEM = "LUCHR";
const unsigned char lcdclrstr[] PROGMEM = "LCLR";
const unsigned char lcdbrstr[] PROGMEM = "LBRI";
const unsigned char lcdwstr[] PROGMEM = "LW";
const unsigned char ldwstr[]  PROGMEM = "LDW";
const unsigned char lcdbgstr[] PROGMEM = "LBG";
const unsigned char lbenchstr[] PROGMEM = "LBENCH";
const unsigned char lgfxtstr[] PROGMEM = "LGFXT";
const unsigned char blsetstr[] PROGMEM = "BLSET";
const unsigned char faderstr[] PROGMEM = "FADE";
#endif
const unsigned char helpstr[] PROGMEM = "?";

const struct command_t appdb[] PROGMEM = {
	{(PGM_P)echostr, &(echo_cmd)},
	{(PGM_P)calcstr, &(calc_cmd)},
#ifdef M64C1
	{(PGM_P)avrpstr, &(avrp_cmd)},
	{(PGM_P)sldbgstr, &(sldbg_cmd)},
#else
	{(PGM_P)lcdistr, &(lcdr_cmd)},
	{(PGM_P)lcdclrstr, &(lcdclr_cmd)},
	{(PGM_P)lcdcstr, &(lcdc_cmd)},
	{(PGM_P)lcdc2str, &(lcdc2_cmd)},
	{(PGM_P)lcdbrstr, &(lcdbr_cmd)},
	{(PGM_P)lcdwstr, &(lcdw_cmd)},
	{(PGM_P)ldwstr, &(ldw_cmd)},
	{(PGM_P)lcdbgstr, &(lcdbg_cmd)},
	{(PGM_P)lbenchstr, &(lbench_cmd)},
	{(PGM_P)lgfxtstr, &(lgfxt_cmd)},
	{(PGM_P)blsetstr, &(blset_cmd)},
	{(PGM_P)faderstr, &(fader_cmd)},
#endif
	{(PGM_P)helpstr, &(help_cmd)},
	{NULL,NULL}
};

void invalid_command(void) {
	sendstr(tokenptrs[0]);
	sendstr_P(PSTR(": not found"));
	}

void *find_appdb(unsigned char* cmd) {
	uint8_t i;
	const struct command_t * ctptr;
	PGM_P name;
	void* fp;
	for(i=0;;i++) {
		ctptr = &(appdb[i]);
		name = (PGM_P)pgm_read_word(&(ctptr->name));
		fp = (void*)pgm_read_word(&(ctptr->function));
		if (!name) break;
		if (strcmp_P((char*)cmd,name) == 0) {
			return fp;
			}
	}
	return &(invalid_command);
}

#endif
