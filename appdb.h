struct command_t {
	PGM_P name;
	void(*function)(void);
};

extern const struct command_t appdb[] PROGMEM;

void *find_appdb(unsigned char* cmd);
void echo_cmd(void);
void help_cmd(void);
void calc_cmd(void);

void avrp_cmd(void);
void sldbg_cmd(void);

void lcdr_cmd(void);
void lcdbr_cmd(void);
void lcdw_cmd(void);
void lcdc_cmd(void);
void lcdbg_cmd(void);
void lbench_cmd(void);
void lgfxt_cmd(void);
void blset_cmd(void);
void ldw_cmd(void);
void fader_cmd(void);
void lcdclr_cmd(void);
void lcdc2_cmd(void);
