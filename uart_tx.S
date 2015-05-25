#include <avr/io.h>

.section .text

#if 0

/* AVR305 equation: #define UART_B_VALUE (((F_CPU/BAUD_RATE)-23)/6) */
/* Adding 3 to numerator simulates nearest rounding for more accurate baud rates */
#define UART_B_VALUE (((16000000/115200)-20)/6)
#if UART_B_VALUE > 255
#error Baud rate too slow for soft UART
#endif

uartDelay:
	ldi r25, UART_B_VALUE
1:	dec r25
	brne 1b
	ret

.global uart_send
uart_send:
	ldi r18, 10
	com r24
	sec
	cli
1:	brcc 2f
	cbi _SFR_IO_ADDR(PORTD), 2
	rjmp 3f
2:	sbi _SFR_IO_ADDR(PORTD), 2
	nop
3:	rcall uartDelay
	rcall uartDelay
	lsr r24
	dec r18
	brne 1b
	reti

#else
/* 2Mbaud, here we come. */
.global uart_send
uart_send:
	cli
	cbi _SFR_IO_ADDR(PORTD), 2
	in r18, _SFR_IO_ADDR(PORTD)
	bst r24, 0
	bld r18, 2
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 1
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 2
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 3
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 4
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 5
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 6
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	bst r24, 7
	bld r18, 2
	nop
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	out _SFR_IO_ADDR(PORTD), r18
	adiw r30, 0 /* 2 cycles, 1 word. */
	adiw r30, 0
	adiw r30, 0 /* 2 cycles, 1 word. */
	sei
	sbi _SFR_IO_ADDR(PORTD), 2
	nop
	ret /* ret 4 + rjmp 2 + cli 1 + nop => 8 cyles before start of next minimum. */
#endif
