#include <util/crc16.h>
void dallas_init(void);
void dallas_run(void);
int16_t dallas_temp_get(uint8_t idx);