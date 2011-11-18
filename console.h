unsigned char getline_mc(unsigned char *buf, uint8_t len);
void sendstr_P(PGM_P str);
void sendstr(const unsigned char * str);
unsigned char* scanfor_notspace(unsigned char *buf);
unsigned char* scanfor_space(unsigned char *buf);
void tokenize(unsigned char *rcvbuf,unsigned char** ptrs, uint8_t* tkcntptr);
#define RECEIVE() uart_recv()
#define SEND(n) uart_send(n)
