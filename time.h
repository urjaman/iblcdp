 // Linear format is seconds since 00:00 1.1.2000, mtm format as below:
struct mtm {
	uint8_t year; // EPOCH==0
	uint8_t month; // 1-12
	uint8_t day; // 1-31
	uint8_t hour; // 0-23
	uint8_t min; // 0-59
	uint8_t sec; // 0-59
};
uint8_t month_days(uint8_t year, uint8_t month);
uint32_t mtm2linear(struct mtm * tm);
void linear2mtm(struct mtm*tm, uint32_t lintime);
void linear_date_string(unsigned char* buf, uint32_t lintime);

#define TIME_EPOCH_YEAR 2000
// Change that once per century or so :P, this system should work for enough years to overflow the 32-bit time counting from boot in this century...