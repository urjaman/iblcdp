
#define CHANLEN(c,l) (((c)<<4)|(l))
#define CHAN(x) ((x)>>4)
#define LEN(x) ((x) & 0xF)

// Implementation varies (slave/master)
uint8_t sl_add_tx(uint8_t ch, uint8_t l, uint8_t *buf);
