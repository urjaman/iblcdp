#include <avr/io.h>

.section .text

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
