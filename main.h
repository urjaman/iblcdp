/* GENERIC DEFINITIONS FOR PROJECT */
// Nominal HZ:
//#define F_CPU 8000000
// Measured HZ (iter 1):
//#define F_CPU 7967744
// Measured HZ (iter 2):
//#define F_CPU 8032512
// Measured HZ (iter 3):
#define F_CPU 8045824
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
void mini_mainloop(void);
#define MAXTOKENS 16
extern unsigned char token_count;
extern unsigned char* tokenptrs[];


//#define ENABLE_UARTIF
//#define ENABLE_UARTMODULE

