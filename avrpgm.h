void avrp_init(void);
uint8_t avrp_lvp_entry(void);
uint8_t avrp_read_signature(uint8_t lsb);
uint8_t avrp_vfy_signature(void);
uint16_t avrp_read_progmem(uint16_t addr);
void avrp_chip_erase(void);
void avrp_program_fw(const uint16_t * data, uint16_t baseaddr, uint16_t words);
uint8_t avrp_read_eeprom(uint16_t addr);
void avrp_write_eeprom(uint16_t addr, const uint8_t *d, uint16_t len);
uint8_t avrp_program_fuses(void);
void avrp_run_avr(void);
void avrp_halt_avr(void);
uint8_t avrp_test(void);
void avrp_wait_busy(void);

#define AVR_PAGE_SIZE 128 // WORDS
#define AVR_PAGE_COUNT 512
