uint16_t adc_read_mb(void);
uint16_t adc_read_fb(void);
uint16_t adc_read_ign(void);
int16_t adc_read_diff(void);
void adc_print_v(unsigned char* buf, uint16_t v);
void adc_init(void);
void adc_run(void);
