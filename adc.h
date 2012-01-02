uint16_t adc_read_mb(void);
uint16_t adc_read_sb(void);
int16_t adc_read_diff(void);
void adc_print_v(unsigned char* buf, uint16_t v);
void adc_init(void);
void adc_run(void);
uint16_t adc_to_dV(uint16_t v);
uint16_t adc_from_dV(uint16_t v);
void adc_print_dV(unsigned char* buf, uint16_t v);

#define ADC_MUX_CNT 2

void adc_init_ll(void);
uint16_t adc_run_ll(uint16_t adc_values[ADC_MUX_CNT]);