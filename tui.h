#ifndef _TUI_H_
#define _TUI_H_

void tui_init(void);
void tui_run(void);
void tui_activate(void);

// This exported for saver and tui-modules.c
#define TUI_MODS_MAXDEPTH 4
extern uint8_t tui_mp_mods[4][TUI_MODS_MAXDEPTH];

// These are for tui-*.c:
uint8_t tui_pollkey(void);
uint8_t tui_waitforkey(void);
uint8_t tui_pgm_menupart(unsigned char* line, unsigned char* buf, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);

uint8_t tui_gen_listmenu(PGM_P header, PGM_P const menu_table[], uint8_t itemcnt, uint8_t start);
void tui_gen_message(PGM_P l1, PGM_P l2);
void tui_gen_message_m(PGM_P l1, const unsigned char* l2m);
uint16_t tui_gen_nummenu(PGM_P header, uint16_t min, uint16_t max, uint16_t start);
void tui_gen_menuheader(unsigned char* line, unsigned char* buf, PGM_P header);
void tui_set_clock(void);
typedef uint8_t printval_func_t(unsigned char*,int32_t);
int32_t tui_gen_adjmenu(PGM_P header, printval_func_t *printer,int32_t min, int32_t max, int32_t start, int32_t step);

extern const unsigned char tui_exit_menu[];

uint8_t tui_are_you_sure(void);

/* tui-other.c */
void tui_othermenu(void);
void tui_time_print(uint32_t nt);
void tui_adc_calibrate(void);

/* tui-modules.c */
uint8_t tui_run_mod(uint8_t mod, uint8_t *p, uint8_t ml);
void tui_config_menu(void);
extern const unsigned char tui_update_rate_cfg[]; // for tui-temp.c
uint8_t tui_temp_printer(unsigned char* mb, int32_t val);
void tui_num_helper(unsigned char* buf, uint8_t n); // Prints 00-99 to buf. No terminator.

/* tui-temp.c */
#define TUI_DEFAULT_REFRESH_INTERVAL 5
uint8_t tui_update_refresh_interval(void);
void tui_refresh_interval_menu(void);


/* tui-calc.c */
void tui_calc(void);
void tui_calc_fuel_cost(void);
void tui_calc_fc_history(void);

// This is maximum supported by the FC history viewer.
#define TUI_FC_HISTORY_SIZE 100

struct __attribute__ ((__packed__)) tui_fc_history_entry {
	uint16_t time_days; // lindate
	uint16_t kilometers; // *10
	uint16_t fuel_price; // *1000
	uint16_t litres; // *100
};

extern uint8_t tui_fc_history_count;
extern uint16_t tui_fc_last_fuel_price;
extern uint16_t tui_fc_last_fuel_efficiency; // l/100km*100
extern uint16_t tui_fc_last_kilometres; // 300km*10

/* poweroff.c */
void tui_poweroff(void);

/* tui-alarm.c if ALARMCLOCK */
void tui_alarm_run(void);
void tui_alarm_menu(void);
uint8_t tui_alarm_mod_str(uint8_t *buf);

#endif
