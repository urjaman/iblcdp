uint8_t batlvl_get_mb(void);
uint8_t batlvl_get_sb(void);
void batlvl_init(void);
void batlvl_run(void);

typedef struct {
	uint8_t charge_hours_fast; // small number
	uint8_t charge_hours_slow; // big number
	uint16_t charge_voltage_slow; // minimum charge voltage
	uint16_t charge_voltage_fast; // voltage for the fast hours
} batlvl_setting_t;

extern batlvl_setting_t batlvl_settings;