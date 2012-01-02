/* The RTC is an PCF8563. This module is minimalistic, should work for lots of RTC's with little change. */
#include "main.h"
#include "time.h"
#include "i2c.h"
#include "rtc.h"
// main.c
#define PCF8563_I2C_ADDR 0xA2
#define RTC_I2C_ADDR PCF8563_I2C_ADDR

static uint8_t readbcd(uint8_t bcd) {
	return (bcd>>4)*10+(bcd&0xF);
}
static uint8_t writebcd(uint8_t bin) {
	uint8_t bcd = bin%10;
	bcd |= (bin/10) << 4;
	return bcd;
}

static uint8_t rtc_is_ok=0;

// 0 = OK, nonzero = not ok
uint8_t rtc_read(struct mtm* tm) {
	uint8_t buf[7];
	if (i2c_read_regs(RTC_I2C_ADDR,2,7,buf)) {
		rtc_is_ok=0;
		return 1; // Not OK
	}
	if (buf[0]&0x80) {
		rtc_is_ok = 0;
		return 2; // VL, not OK
	}
	tm->sec = readbcd(buf[0]);
	tm->min = readbcd(buf[1]&0x7F);
	tm->hour = readbcd(buf[2]&0x3F);
	uint8_t tmp = readbcd(buf[6]);
	if (buf[5]&0x80) tmp += 100;
	tm->year = tmp;
	tm->month = readbcd(buf[5]&0x1F);
	tm->day = readbcd(buf[3]&0x3F);
	rtc_is_ok = 1;
	return 0; // OK
}


void rtc_write(struct mtm* tm) {
	uint8_t buf[7];
	buf[0] = writebcd(tm->sec);
	buf[1] = writebcd(tm->min);
	buf[2] = writebcd(tm->hour);
	buf[3] = writebcd(tm->day);
	buf[4] = 0; // WeekDay, unused.
	buf[5] = writebcd(tm->month);
	uint8_t tmp = tm->year;
	if (tmp>99) {
		buf[5] |= 0x80;
		tmp -= 100;
	}
	buf[6] = writebcd(tmp);
	if (i2c_write_regs(RTC_I2C_ADDR,2,7,buf)) {
		rtc_is_ok = 0;
		return; // Not OK
	}
	rtc_is_ok = 1;
	return;
}



uint8_t rtc_valid(void) {
	return rtc_is_ok;
}