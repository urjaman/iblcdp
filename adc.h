uint16_t adc_read_mb(void);
uint16_t adc_read_sb(void);
int16_t adc_read_diff(void);
void adc_print_v(unsigned char* buf, uint16_t v);
void adc_init(void);
void adc_run(void);
uint16_t adc_to_dV(uint16_t v);
uint16_t adc_from_dV(uint16_t v);
void adc_print_dV(unsigned char* buf, uint16_t v);
// LOTS of places need to be changed if ADC_MUX_CNT!=2.
#define ADC_MUX_CNT 2
extern uint16_t adc_raw_values[ADC_MUX_CNT];
extern int16_t adc_calibration_diff[ADC_MUX_CNT];

void adc_init_ll(void);
uint16_t adc_run_ll(uint16_t adc_values[ADC_MUX_CNT]);