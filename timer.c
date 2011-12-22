#include "main.h"
#include "timer.h"
#include "buttons.h"


uint8_t timer_waiting=0;

static uint8_t timer_1hzp=0;
static uint8_t timer_5hzp=0;
static uint8_t seconds=0;
static uint8_t minutes=0;
static uint8_t hours_24=0;
static uint8_t hours_28=0;
static uint32_t secondstimer=0;
static uint32_t last_time_set=0;

static void timer_gen_5hzp(void) {
	static uint8_t state=0;
	timer_5hzp=0;
	if (timer_1hzp) {
		state=0;
	}
	if (timer_get_subsectimer()>=((SSTC/5)*state)) {
		timer_5hzp=1;
		state++;
	}
}

	

void timer_delay_us(uint32_t us) {
	uint32_t ss_start = timer_get_linear_ss_time();
	if (us>200000) us = 200000; // Safety Limit for 5hzP
	uint32_t ss_end = ss_start + (us/US_PER_SSUNIT) + 1;
	while (timer_get_linear_ss_time()<ss_end) sleep_mode();
}

void timer_delay_ms(uint8_t ms) {
	timer_delay_us((uint32_t)ms*1000);
}

void timer_set_waiting(void) {
	timer_waiting=1;
}

void timer_run(void) {
	timer_1hzp=0;
	for (;;) {
		if (timer_getdec_todo()) {
			uint8_t tmp = seconds + 1;
			secondstimer++;
			timer_1hzp=1;
			if (tmp == 60) {
				tmp = minutes + 1;
					if (tmp == 60) {
						tmp = hours_24 + 1;
						if (tmp == 24) tmp = 0;
						hours_24 = tmp;
						tmp = hours_28 + 1;
						if (tmp == 28) tmp = 0;
						hours_28 = tmp;
						tmp = 0;
					}
				minutes = tmp;
				tmp = 0;
			}
			seconds = tmp;
		}
		timer_gen_5hzp();
		if ((timer_5hzp)||(timer_1hzp)||(timer_waiting)||(buttons_get_v())) {
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

static int32_t time_diff(uint8_t h1, uint8_t m1, uint8_t s1, uint8_t h2, uint8_t m2, uint8_t s2) { // new, old
	int32_t t1,t2, t3;
	t1 = ((h1*60+m1)*60)+s1;
	t2 = ((h2*60+m2)*60)+s2;
	t3 = t1 - t2;
	if (t3 > 43200) t3 -= 86400;
	return t3;
}

void timer_set_time24(uint8_t hours, uint8_t mins, uint8_t secs) {
	uint32_t days = (secondstimer - last_time_set)/86400;
	if (hours > 23) hours = 0;
	if (mins > 59) mins = 0;
	if (secs > 59) secs = 0; 
	if ((days)&&(days<10)) {
		uint32_t min_diff = days*60; // if differense is less than a minute per day, we have achieved 0.07% accuracy.
					     // Wont try to ask more from an RC osc.
		uint32_t max_diff = (57*60); // allow changing to/from daylight saving time without calibrating
		int32_t diff = time_diff(hours,mins,secs,hours_24,minutes,seconds);
		if ((abs(diff) < max_diff)&&(abs(diff) > min_diff)) {
			uint8_t cal = OSCCAL;
			if (diff>0) { 
				// We're too slow
				if ((cal!=255)&&(cal!=127)) {
					cal++;
					OSCCAL = cal;
				}
			}
			if (diff<0) {
				// We're too fast
				if ((cal!=128)&&(cal!=0)) {
					cal--;
					OSCCAL = cal;
				}
			}
		}
	}
	last_time_set = secondstimer;
	hours_24 = hours;
	minutes = mins;
	seconds = secs;
}

void timer_set_time28(uint8_t hours, uint8_t mins, uint8_t secs) {
	if (hours > 27) hours = 0;
	hours_28 = hours;
	mins=mins;secs=secs;
}

void timer_get_time24(uint8_t* hp, uint8_t* mp, uint8_t* sp) {
	uint8_t h,m,s;
	h = hours_24;
	m = minutes;
	s = seconds;
	*hp = h;
	*mp = m;
	*sp = s;
}

void timer_get_time28(uint8_t* hp, uint8_t* mp, uint8_t* sp) {
	uint8_t h,m,s;
	h = hours_28;
	m = minutes;
	s = seconds;
	*hp = h;
	*mp = m;
	*sp = s;
}