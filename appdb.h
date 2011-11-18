struct command_t {
	PGM_P name;
	void(*function)(void);
};

extern const struct command_t appdb[] PROGMEM;

void *find_appdb(unsigned char* cmd);
void echo_cmd(void);
void help_cmd(void);
void calc_cmd(void);
void lcdsay_cmd(void);
void lcdinit_cmd(void);
void blset_cmd(void);
void timer_cmd(void);
void btns_cmd(void);
void adc_cmd(void);
void relay_cmd(void);