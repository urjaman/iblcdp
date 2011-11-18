#define TUI_MODS_MAXDEPTH 4

// This exported for saver and tui-modules.c
extern uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];

void tui_init(void);
void tui_run(void);
void tui_activate(void);

// These are for tui-*.c:
uint8_t tui_waitforkey(void);
uint8_t tui_pgm_menupart(unsigned char* line, unsigned char* buf, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);

uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);
void tui_gen_message(PGM_P l1, PGM_P l2);
uint8_t tui_gen_nummenu(PGM_P header, uint8_t min, uint8_t max, uint8_t start);

/* tui-other.c */
void tui_othermenu(void);

/* tui-modules.c */
uint8_t tui_run_mod(uint8_t mod, uint8_t *p, uint8_t ml);
void tui_config_menu(void);


/* tui-calc.c */
void tui_calc(void);