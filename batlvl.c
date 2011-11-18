#include "main.h"
#include "adc.h"
#include "timer.h"
#include "batlvl.h"

/* This is the most pseudo-science module yet. */
/* It attempts to estimate battery charge levels, so they 
   can be used to guide relay usage. */

#define BATTERY_COOLOFF_PERIOD (2*60*60)
#define CHGLVL_DEPTH 3
#define BATTERY_CHGLEVEL_READ_INTERVAL (10*60)
#define IGNON_MB_VOLTAGE 808
#define MB_WAS_CHARGING 1
#define FB_WAS_CHARGING 2

static uint8_t mb_chglvl[CHGLVL_DEPTH];
static uint8_t fb_chglvl[CHGLVL_DEPTH];
static uint8_t was_charging=0;
static uint32_t been_ignoff_since=0;
static uint32_t next_mb_charge_sec;
static uint32_t next_fb_charge_sec;
static uint32_t next_read_sec;

batlvl_setting_t batlvl_settings = { 
	.charge_hours_fast = 8,
	.charge_hours_slow = 12,
	.charge_voltage_slow = 845, //13.2
	.charge_voltage_fast = 922  //14.4
};

uint8_t batlvl_get_mb(void) {
	return mb_chglvl[CHGLVL_DEPTH-1];
}
uint8_t batlvl_get_fb(void) {
	return fb_chglvl[CHGLVL_DEPTH-1];
}

static uint16_t chargehours_to_seconds(uint8_t hours) {
	// Seconds per % of SoC
	uint16_t r = hours*36;
	return r;
}


static uint16_t voltage_soc_to_chgtime(uint16_t v, uint8_t soc) {
	uint16_t charge_time;
	uint16_t charge_time_slow, charge_time_fast;
	// 16 is 1xtime, 1 section is 25% of SoC (0-24,25-49,50-74,75-100)
	uint8_t time_factor;
	if (soc>=75) {
		time_factor = 28;
	} else if (soc>=50) {
		time_factor = 16;
	} else if (soc>=25) {
		time_factor = 12;
	} else {
		time_factor = 8;
	}
	charge_time_slow = chargehours_to_seconds(batlvl_settings.charge_hours_slow);
	charge_time_fast = chargehours_to_seconds(batlvl_settings.charge_hours_fast);
	uint16_t count_ct = charge_time_slow-charge_time_fast;
	uint16_t count_cv = batlvl_settings.charge_voltage_fast-batlvl_settings.charge_voltage_slow;
	uint16_t val_cv = v-batlvl_settings.charge_voltage_slow;
	uint16_t val_ct = ((val_cv*count_ct*4)+2) / (count_cv*4);
	if (val_ct > count_ct) val_ct = count_ct;
	charge_time = charge_time_slow-val_ct;
	charge_time = ((charge_time*time_factor)+8) / 16;
	return charge_time;
}

static uint8_t voltage_to_chglvl(uint16_t v) {
	if (v<672) return 0;
	if (v>807) return 100;
	// 0-10% == 10.5-11.3 => 9-0 vs 723-672
 	// 10-90% == 11.3-12.5 => 89-10 vs 799 - 724
	// 90%-100% ==  12.5-12.6 => 100-90 vs 807 - 800
	uint16_t base_v,count_v;
	uint8_t base_p,count_p;
	if (v<724) {
		base_v = 672;
		count_v = 51;
		base_p = 0;
		count_p = 9;
	} else if (v<800) {
		base_v = 724;
		count_v = 75;
		base_p = 10;
		count_p = 79;
	} else {
		base_v = 800;
		count_v = 7;
		base_p = 90;
		count_p = 10;
	}
	// logically, val_p = (val_v*count_p*8)+4 / (count_v*8)
	// test that 10 = ((7*10*8)+4) / (7*8)
	//           79 = ((75*79*8)+4) / (75*8)
        //            9 = ((51*9*8)+4) / (51*8)
	uint16_t val_v = v-base_v;
	uint16_t val_p = ((val_v*count_p*8)+4) / (count_v*8);
	if (val_p > count_p) val_p = count_p;
	return (uint8_t)(val_p+base_p);
}

static uint8_t is_charging(uint16_t v) {
	if (v>=batlvl_settings.charge_voltage_slow) return 1;
	return 0;
}

static void batlvl_direct_estimate(void) {
	uint8_t i;
	for(i=CHGLVL_DEPTH-1;i;i--) {
		mb_chglvl[i] = mb_chglvl[i-1];
	}
	mb_chglvl[0] = voltage_to_chglvl(adc_read_mb());
	if (!is_charging(adc_read_fb())) {
		for(i=CHGLVL_DEPTH-1;i;i--) {
			fb_chglvl[i] = fb_chglvl[i-1];
		}
		fb_chglvl[0] = voltage_to_chglvl(adc_read_fb());
	}
}

void batlvl_init(void) {
	uint8_t i;
	if ((adc_read_mb()<IGNON_MB_VOLTAGE)&&(adc_read_ign() < 384)) {
		batlvl_direct_estimate();
	} else { 
		// Making an ass out of you and me...	
		mb_chglvl[0] = 80;
		fb_chglvl[0] = 50;
	}
	for(i=1;i<CHGLVL_DEPTH;i++) {
		mb_chglvl[i] = mb_chglvl[0];
		fb_chglvl[i] = fb_chglvl[0];
	}
}


void batlvl_run(void) {
	if (!timer_get_1hzp()) return; // We run at max 1Hz
	if ((adc_read_mb()>=IGNON_MB_VOLTAGE)||(adc_read_ign() >= 384)) {
		uint8_t i;
		// "Ign=On"
		been_ignoff_since = timer_get(); // == not yet
		next_read_sec = been_ignoff_since+BATTERY_COOLOFF_PERIOD;
		// Flush recent chglvl data
		for(i=0;i<CHGLVL_DEPTH-1;i++) {
			mb_chglvl[i] = mb_chglvl[CHGLVL_DEPTH-1];
			fb_chglvl[i] = fb_chglvl[CHGLVL_DEPTH-1];
		}
	}
	if ((been_ignoff_since+BATTERY_COOLOFF_PERIOD) <= timer_get()) {
		if (timer_get()>=next_read_sec) {
			batlvl_direct_estimate();
			next_read_sec = timer_get() + BATTERY_CHGLEVEL_READ_INTERVAL;
		}
	}

	if (is_charging(adc_read_mb())) {
		if (was_charging&MB_WAS_CHARGING) {
			if (timer_get()>=next_mb_charge_sec) {
				if (mb_chglvl[CHGLVL_DEPTH-1]<100) {
					mb_chglvl[CHGLVL_DEPTH-1]++;
				}
				next_mb_charge_sec = timer_get() +
				voltage_soc_to_chgtime(adc_read_mb(),mb_chglvl[CHGLVL_DEPTH-1]);
			}
		} else { 
			was_charging |= MB_WAS_CHARGING;
			next_mb_charge_sec = timer_get() +
			voltage_soc_to_chgtime(adc_read_mb(),mb_chglvl[CHGLVL_DEPTH-1]);
		}
	} else {
		was_charging &= ~MB_WAS_CHARGING;
	}
	if (is_charging(adc_read_fb())) {
		if (was_charging&FB_WAS_CHARGING) {
			if (timer_get()>=next_fb_charge_sec) {
				if (fb_chglvl[CHGLVL_DEPTH-1]<100) {
					fb_chglvl[CHGLVL_DEPTH-1]++;
				}
				next_fb_charge_sec = timer_get() +
				voltage_soc_to_chgtime(adc_read_fb(),fb_chglvl[CHGLVL_DEPTH-1]);
			}
		} else { 
			was_charging |= FB_WAS_CHARGING;
			next_mb_charge_sec = timer_get() +
			voltage_soc_to_chgtime(adc_read_fb(),fb_chglvl[CHGLVL_DEPTH-1]);
		}
	} else {
		was_charging &= ~FB_WAS_CHARGING;
	}
}