uint8_t uart_isdata(void);
uint8_t uart_recv(void);
void uart_send(uint8_t c);
void sluart_run(void);
void uart_init(void);
void uart_wait_txdone(void);
