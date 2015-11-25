// Binary values to number strings, return string length.
uint8_t uint2str(unsigned char *buf, uint16_t val);
uint8_t uint2xstr(unsigned char *buf, uint16_t val);
uint8_t uchar2str(unsigned char *buf, uint8_t val);
uint8_t uchar2xstr(unsigned char *buf, uint8_t val);
uint8_t luint2str(unsigned char *buf, uint32_t val);
uint8_t luint2xstr(unsigned char*buf, uint32_t val);
// Number string to binary value.
uint8_t str2uchar(unsigned char *buf);
uint8_t xstr2uchar(unsigned char *buf);
uint32_t astr2luint(unsigned char *buf);

void dprint_hb(uint8_t b);
void dprint_hl(uint32_t b);

void luint2outdual(uint32_t val);
