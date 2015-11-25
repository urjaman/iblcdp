/* UART MODULE HEADER */

#ifdef M1284
#include "sluart.h"
#else

uint8_t uart_isdata(void);
int uart_recv_to(uint8_t to);
uint8_t uart_recv(void);
void uart_send(uint8_t val);
void uart_init(void);
void uart_wait_txdone(void);
#define BAUD 115200

#define dprint(s) sendstr_P(PSTR(s))
//#define dprint(s)

#endif
