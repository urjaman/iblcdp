#include "main.h"
#include "timer.h"
#include "cron.h"
#include "uart.h"

/* This part is the non-calendar/date/time-related part. Just uptimer, etc. */
uint8_t timer_waiting=0;
static uint8_t timer_1hzp=0; // 1 HZ Pulse, 0/1
static uint8_t timer_5hzp=0; // 5 HZ Pulse
static uint32_t secondstimer=0;
static uint8_t timer5hz=0; // Linear 8-bit counter at 5hz, rolls over every 51s.
static uint8_t timer5hz_todo=0; // Used to fix linear counter if a 5hz pulse is missed.

static uint16_t timer_gen_5hzp(void) {
	static uint8_t state=0;
	timer_5hzp=0;
	if (timer_1hzp) {
		state=0;
	}
	uint16_t rv;
	if ((rv=timer_get_lin_ss_u16())>=((SSTC/5)*state)) {
		timer_5hzp=1;
		state++;
	}
	return rv;
}

void timer_delay_us(uint24_t us) {
	uint24_t ss_start = timer_get_linear_ss_time();
	if (us>200000) us = 200000; // Safety Limit for 5hzP
	uint24_t ss_end = ss_start + (us/US_PER_SSUNIT) + 1;
	uint24_t ss_now;
	uint16_t ncront = cron_next_task();
	while ((ss_now=timer_get_linear_ss_time())<ss_end) {
		if (ss_now>=ncront) ncront = cron_run_tasks();
		sleep_mode();
	}
}

void timer_delay_ms(uint8_t ms) {
	timer_delay_us((uint24_t)ms*1000);
}

void timer_set_waiting(void) {
	timer_waiting=1;
}

/* This is the interface from non-calendar time to calendar time functions. */
static void timer_time_tick();

void timer_run(void) {
	uint16_t ncront = cron_next_task();
	timer_1hzp=0;
	for (;;) {
		if (timer_getdec_todo()) {
			secondstimer++;
			timer_1hzp=1;
			timer5hz += timer5hz_todo;
			timer5hz_todo = 5;
			cron_initialize();
			timer_time_tick();
			ncront = cron_next_task();
		}
		uint16_t ss = timer_gen_5hzp();
		if (timer_5hzp) {
			timer5hz++;
			timer5hz_todo--;
		}
		if (ss>=ncront) ncront = cron_run_tasks();
		if ((timer_5hzp)||(timer_1hzp)||(timer_waiting)||uart_isdata()) {
			timer_waiting=0;
			break;
		}
		sleep_mode();
	}
}


uint32_t timer_get(void) {
	return secondstimer;
}

uint8_t timer_get_1hzp(void) {
	return timer_1hzp;
}

uint8_t timer_get_5hzp(void) {
	return timer_5hzp;
}

uint8_t timer_get_5hz_cnt(void) {
	return timer5hz;
}

/*******************************************/
/* Here start the calendar time functions. */
/*******************************************/

// Time ticking without RTC is considered valid for this time.
//#define TIME_NONRTC_VALID_TIME (365*24*60*60)

uint32_t timer_time_last_valid_moment=0;
uint8_t timer_time_valid=0;
static struct mtm timer_tm_now = { 0,1,1,0,0,0 };

void timer_set_time(struct mtm *tm) {
	timer_tm_now = *tm;
	timer_time_valid = 1;
	timer_time_last_valid_moment = secondstimer;
//	rtc_write(tm); // If there is an RTC, set time into it.
}


void timer_get_time(struct mtm *tm) {
	*tm = timer_tm_now;
}

uint8_t timer_time_isvalid(void) {
	return timer_time_valid;
}

static void timer_time_tick(void) {
#if 0
	uint8_t rv;
	struct mtm rtctime;
	if ((rv=rtc_read(&rtctime))==0) { // We have RTC and it is valid, take it as the absolute truth.
		timer_tm_now = rtctime;
		timer_time_valid = 1;
		timer_time_last_valid_moment = secondstimer;
		return;
	}
#endif
	// We have no RTC and have to wing it on our own.
	uint24_t tmp = timer_tm_now.sec+1;
	if (tmp>=60) {
		tmp = timer_tm_now.min+1;
		if (tmp>=60) {
			tmp = timer_tm_now.hour+1;
			if (tmp>=24) {
				tmp = mtm2lindate(&timer_tm_now)+1;
				lindate2mtm(&timer_tm_now,tmp);
				tmp = 0;
			}
			timer_tm_now.hour = tmp;
			tmp = 0;
		}
		timer_tm_now.min = tmp;
		tmp = 0;
	}
	timer_tm_now.sec = tmp;
#if 0
	// Time incremented. Check if it is valid and whether it should still be valid.
	if (timer_time_valid) {
		uint32_t passed = secondstimer - timer_time_last_valid_moment;
		if (passed>TIME_NONRTC_VALID_TIME) timer_time_valid = 0;
	}
	if ((timer_time_valid)&&(rv==2)) { // RTC did exist but had no time, and our time is still valid, set RTC.
		rtc_write(&timer_tm_now);
	}
#endif
}
