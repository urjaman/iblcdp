#ifndef _TUI_LIB_H_
#define _TUI_LIB_H_

uint8_t tui_pollkey(void);
uint8_t tui_waitforkey(void);
uint8_t tui_pgm_menupart(unsigned char* buf, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);

uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);
void tui_gen_message(PGM_P l1, PGM_P l2);
void tui_gen_message_m(PGM_P l1, const unsigned char* l2m);
uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start);
void tui_gen_menuheader(PGM_P header);
void tui_set_clock(void);

typedef uint8_t printval_func_t(unsigned char*,int32_t);

int32_t tui_gen_adjmenu(PGM_P header, printval_func_t *printer,int32_t min, int32_t max, int32_t start, int32_t step);

extern const unsigned char tui_exit_menu[];

uint8_t tui_are_you_sure(void);

uint16_t tui_gen_voltmenu(PGM_P header, uint16_t start);


#endif
