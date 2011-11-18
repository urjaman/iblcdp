#include "main.h"
#include "adc.h"
#include "timer.h"

#define ADC_MUX_CNT 2
// 65536 == no scaling applied
#define ADC_MB_SCALE 65113
#define ADC_FB_SCALE 64957

static uint16_t adc_t_values[ADC_MUX_CNT];
static uint16_t adc_values[ADC_MUX_CNT];
static int16_t adc_bat_diff;
static uint8_t adc_diff_rsamples;

static void adc_set_mux(uint8_t c) {
	ADMUX = (ADMUX&0xF0) |  (c&0xF);
}

static uint16_t adc_single_read(uint8_t c) {
	uint16_t rv;
	adc_set_mux(c);
	ADCSRA |= _BV(ADSC);
	while (ADCSRA & _BV(ADSC));
	rv = ADCL;
	rv |= (ADCH)<<8;
	return rv;
}

uint16_t adc_read_mb(void) {
	return adc_values[0];
}

uint16_t adc_read_fb(void) {
	return adc_values[1];
}

int16_t adc_read_diff(void) {
	return adc_bat_diff;
}

void adc_print_v(unsigned char* buf, uint16_t v) {
	v = ((((uint32_t)v)*15625UL)+5000UL) / 10000UL;
	buf[0] = (v/1000)|0x30; v = v%1000;
	buf[1] = (v/100 )|0x30; v = v%100;
	buf[2] = '.';
	buf[3] = (v/10  )|0x30; v = v%10;
	buf[4] =  v      |0x30;
	buf[5] = 'V';
	buf[6] = 0;
	if (buf[0] == '0') buf[0] = ' ';
}

void adc_init(void) {
	uint8_t i;
	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1);
	DIDR0 = _BV(ADC0D) | _BV(ADC1D) | _BV(ADC2D);
	for (i=0;i<ADC_MUX_CNT;i++) adc_values[i] = adc_single_read(i);
}

static void adc_scale_diffs(void) {
	uint8_t i;
	for (i=0;i<ADC_MUX_CNT;i++) adc_t_values[i] = adc_t_values[i]>>2;
	adc_t_values[0] = (((uint32_t)adc_t_values[0])*ADC_MB_SCALE)/65536;
	adc_t_values[1] = (((uint32_t)adc_t_values[1])*ADC_FB_SCALE)/65536;
	adc_bat_diff = adc_t_values[0] - adc_t_values[1];
	for (i=0;i<ADC_MUX_CNT;i++) adc_values[i] = adc_t_values[i]>>2;	
	for (i=0;i<ADC_MUX_CNT;i++) adc_t_values[i] = 0;
}

void adc_run(void) {
	uint8_t i;
	if (timer_get_1hzp()) {
		adc_scale_diffs();
	} else if (timer_get_5hzp()) {
		adc_diff_rsamples = 4;
		timer_set_waiting();
	} else if (adc_diff_rsamples) {
		uint16_t m,f;
		adc_diff_rsamples--;
		m  = adc_single_read(0);
		f  = adc_single_read(1);
		m += adc_single_read(0);
		f += adc_single_read(1);
		m += adc_single_read(0);
		adc_t_values[0] += (m/3);
		adc_t_values[1] += (f/2);
//		for(i=2;i<ADC_MUX_CNT;i++) adc_t_values[i] += adc_single_read(i);
		if (adc_diff_rsamples) {
			timer_set_waiting();
		}
	}
}
