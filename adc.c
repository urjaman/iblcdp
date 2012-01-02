#include "main.h"
#include "adc.h"
#include "timer.h"

// 65536 == no scaling applied
#define ADC_MB_SCALE 65098
// measures 11.99 real 11.91
#define ADC_FB_SCALE 65317
//  measures 11.95 real 11.91


static uint16_t adc_values[ADC_MUX_CNT];
static int16_t adc_bat_diff;
uint16_t adc_avg_cnt=0;

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

uint16_t adc_read_sb(void) {
	return adc_values[1];
}

int16_t adc_read_diff(void) {
	return adc_bat_diff;
}

uint16_t adc_to_dV(uint16_t v) {
	v = ((((uint32_t)v)*390625UL)+500000UL) / 1000000UL;
	return v;
}

uint16_t adc_from_dV(uint16_t v) {
	v = ((((uint32_t)v)*256UL)+50UL) / 100UL;
	return v;
}

void adc_print_v(unsigned char* buf, uint16_t v) {
	v = adc_to_dV(v);
	adc_print_dV(buf,v);
}

void adc_print_dV(unsigned char* buf, uint16_t v) {
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
	DIDR0 = _BV(ADC0D) | _BV(ADC1D);
	for (i=0;i<ADC_MUX_CNT;i++) {
		adc_values[i] = adc_single_read(i)<<2;
	}
	adc_bat_diff = adc_values[0] - adc_values[1];
	adc_init_ll();
	adc_set_mux(0);
	ADCSRA |= _BV(ADSC) | _BV(ADIE);
}

void adc_run(void) {
	if (timer_get_1hzp()) {
		adc_avg_cnt = adc_run_ll(adc_values);
		if (adc_avg_cnt) {
			adc_values[0] = (((uint32_t)adc_values[0])*ADC_MB_SCALE)/65536;	
			adc_values[1] = (((uint32_t)adc_values[1])*ADC_FB_SCALE)/65536;	
			adc_bat_diff = adc_values[0] - adc_values[1];
		}
	}
}
