#define _SFR_ASM_COMPAT 1
#include "main.h"
#include "adc.h"
#define S(s) str(s)
#define str(s) #s

uint8_t adc_isr_ch;
uint8_t adc_isr_sl;
uint16_t adc_isr_supersample[ADC_MUX_CNT];
volatile uint32_t adc_isr_out[ADC_MUX_CNT];
volatile uint16_t adc_isr_out_cnt;


ISR(ADC_vect, ISR_NAKED) {
	asm volatile (
	"push r1\n\t"
	"in r1, __SREG__\n\t"
	"push r24\n\t"
	"push r25\n\t"
	"lds r24, adc_isr_sl\n\t"
	"and r24, r24\n\t"
	"brne 1f\n\t"
	"lds r24, adc_isr_ch\n\t"
	"ldi r25, 0x01\n\t"
	"eor r24, r25\n\t"
	"lds r25, " S(ADMUX) "\n\t"
	"andi r25, 0xF0\n\t"
	"or r25, r24\n\t"
	"sts " S(ADMUX) ", r25\n\t"
	"1:\n\t"
	"lds r24, " S(ADCSRA) "\n\t"
	"ori r24, 0x40\n\t"
	"sts " S(ADCSRA) ", r24\n\t"
	"lds r24, " S(ADCL) "\n\t"
	"lds r25, " S(ADCH) "\n\t"
	"push r16\n\t"
	"ldi r16, 0\n\t"
	"push r28\n\t"
	"push r29\n\t"
	"push r30\n\t"
	"push r31\n\t"
	"lds r28, adc_isr_ch\n\t"
	"lsl r28\n\t"
	"ldi r30, lo8(adc_isr_supersample)\n\t"
	"ldi r31, hi8(adc_isr_supersample)\n\t"
	"add r30, r28\n\t"
	"adc r31, r16\n\t"
	"ld r28, Z\n\t"
	"ldd r29, Z+1\n\t"
	"add r28, r24\n\t"
	"adc r29, r25\n\t"
	"st Z, r28\n\t"
	"std Z+1, r29\n\t"
	"lds r24, adc_isr_sl\n\t"
	"and r24, r24\n\t"
	"breq 4f\n\t"
	"rjmp 2f\n\t"
	"4:\n\t"
	"lds r25, adc_isr_ch\n\t"
	"ldi r28, 0x01\n\t"
	"eor r25, r28\n\t"
	"sts adc_isr_ch, r25\n\t"
	"brne 3f\n\t"
	"ldi r28, lo8(adc_isr_out)\n\t"
	"ldi r29, hi8(adc_isr_out)\n\t"
	"ldi r30, lo8(adc_isr_supersample)\n\t"
	"ldi r31, hi8(adc_isr_supersample)\n\t"
//	"rcall adc_isr_forloop\n\t"  Inlined here:
	"ld r24, Z\n\t"
	"ldd r25, Z+1\n\t"
	"st Z, r16\n\t"
	"std Z+1, r16\n\t"
	"ld r30, Y\n\t"
	"ldd r31, Y+1\n\t"
	"add r30, r24\n\t"
	"adc r31, r25\n\t"
	"st Y, r30\n\t"
	"std Y+1, r31\n\t"
	"ldd r30, Y+2\n\t"
	"ldd r31, Y+3\n\t"
	"adc r30, r16\n\t"
	"adc r31, r16\n\t"
	"std Y+2, r30\n\t"
	"std Y+3, r31\n\t"
// Inline end
	"ldi r28, lo8(adc_isr_out+4)\n\t"
	"ldi r29, hi8(adc_isr_out+4)\n\t"
	"ldi r30, lo8(adc_isr_supersample+2)\n\t"
	"ldi r31, hi8(adc_isr_supersample+2)\n\t"
//	"rcall adc_isr_forloop\n\t" Inlined here:
	"ld r24, Z\n\t"
	"ldd r25, Z+1\n\t"
	"st Z, r16\n\t"
	"std Z+1, r16\n\t"
	"ld r30, Y\n\t"
	"ldd r31, Y+1\n\t"
	"add r30, r24\n\t"
	"adc r31, r25\n\t"
	"st Y, r30\n\t"
	"std Y+1, r31\n\t"
	"ldd r30, Y+2\n\t"
	"ldd r31, Y+3\n\t"
	"adc r30, r16\n\t"
	"adc r31, r16\n\t"
	"std Y+2, r30\n\t"
	"std Y+3, r31\n\t"
// Inline end
	"lds r24, adc_isr_out_cnt\n\t"
	"lds r25, adc_isr_out_cnt+1\n\t"
	"adiw r24, 1\n\t"
	"sts adc_isr_out_cnt, r24\n\t"
	"sts adc_isr_out_cnt+1, r25\n\t"
	"3:\n\t"
	"ldi r24, 0x10\n\t"
	"sts adc_isr_sl, r24\n\t"
	"2:\n\t"
	"lds r24, adc_isr_sl\n\t"
	"subi r24, 0x01\n\t"
	"sts adc_isr_sl, r24\n\t"
	"pop r31\n\t"
	"pop r30\n\t"
	"pop r29\n\t"
	"pop r28\n\t"
	"pop r16\n\t"
	"pop r25\n\t"
	"pop r24\n\t"
	"out __SREG__, r1\n\t"
	"pop r1\n\t"
	"reti\n\t"
	::
	);
}

void adc_init_ll(void) {
	cli();
	adc_isr_out_cnt=0;
	adc_isr_ch=0;
	adc_isr_sl=15;
	for(uint8_t i=0;i<ADC_MUX_CNT;i++) {
		adc_isr_out[i] = 0;
		adc_isr_supersample[i] = 0;
	}
	sei();
}

uint16_t adc_run_ll(uint16_t adc_values[ADC_MUX_CNT]) {
	uint32_t copy_out[ADC_MUX_CNT];
	uint16_t copy_count=0;
	cli();
	if (adc_isr_out_cnt) {
		copy_count = adc_isr_out_cnt;
		for (uint8_t i=0;i<ADC_MUX_CNT;i++) {
			copy_out[i] = adc_isr_out[i];
			adc_isr_out[i] = 0;
		}
		adc_isr_out_cnt = 0;
	}
	sei();
	if (copy_count) {
		for (uint8_t i=0;i<ADC_MUX_CNT;i++) {
			adc_values[i]= ((uint16_t)((uint32_t)(copy_out[i]/(uint32_t)copy_count)))>>2;
		}
	}
	return copy_count;
}

#if 0
void adc_isr_forloop(void) __attribute__((naked));
void adc_isr_forloop(void) {
	asm volatile (
	"ld r24, Z\n\t"
	"ldd r25, Z+1\n\t"
	"st Z, r16\n\t"
	"std Z+1, r16\n\t"
	"ld r30, Y\n\t"
	"ldd r31, Y+1\n\t"
	"add r30, r24\n\t"
	"adc r31, r25\n\t"
	"st Y, r30\n\t"
	"std Y+1, r31\n\t"
	"ldd r30, Y+2\n\t"
	"ldd r31, Y+3\n\t"
	"adc r30, r16\n\t"
	"adc r31, r16\n\t"
	"std Y+2, r30\n\t"
	"std Y+3, r31\n\t"
	"ret\n\t"
	::
	);
}
#endif