#ifndef _TUI_H_
#define _TUI_H_

#define TUI_MODS_MAXDEPTH 4

// This exported for saver and tui-modules.c
extern uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];

void tui_init(void);
void tui_run(void);
void tui_activate(void);

// These are for tui-*.c:
uint8_t tui_pollkey(void);
uint8_t tui_waitforkey(void);
uint8_t tui_pgm_menupart(unsigned char* line, unsigned char* buf, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);

uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);
void tui_gen_message(PGM_P l1, PGM_P l2);
uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start);
void tui_gen_menuheader(unsigned char* line, unsigned char* buf, PGM_P header);
void tui_set_clock(void);

extern const unsigned char tui_exit_menu[];


/* tui-other.c */
void tui_othermenu(void);

/* tui-modules.c */
uint8_t tui_run_mod(uint8_t mod, uint8_t *p, uint8_t ml);
void tui_config_menu(void);


/* tui-calc.c */
void tui_calc(void);
void tui_calc_fuel_cost(void);
void tui_calc_fc_history(void);

// This is maximum supported by the FC history viewer. Also note that this is 800 bytes of RAM usage.
#define TUI_FC_HISTORY_SIZE 100

struct tui_fc_history_entry {
	uint16_t time_days; // lindate
	uint16_t kilometers; // *10
	uint16_t fuel_price; // *1000
	uint16_t litres; // *100
};

extern uint8_t tui_fc_history_count;
extern struct tui_fc_history_entry tui_fc_history[TUI_FC_HISTORY_SIZE];
extern uint16_t tui_fc_last_fuel_price;
extern uint16_t tui_fc_last_fuel_efficiency; // l/100km*100
extern uint16_t tui_fc_last_kilometres; // 300km*10
#endif