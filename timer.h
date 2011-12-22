void timer_run(void);
uint32_t timer_get(void);
uint8_t timer_get_1hzp(void);
uint8_t timer_get_todo(void);
uint8_t timer_get_5hzp(void);
void timer_set_waiting(void);
void timer_set_time24(uint8_t hours, uint8_t mins, uint8_t secs);
void timer_set_time28(uint8_t hours, uint8_t mins, uint8_t secs);
void timer_get_time24(uint8_t* hp, uint8_t* mp, uint8_t* sp);
void timer_get_time28(uint8_t* hp, uint8_t* mp, uint8_t* sp);

// Delay us and delay ms, both have a limit of 200ms (for 5hzp)
// and a granularity of US_PER_SSUNIT (32us)
void timer_delay_us(uint32_t us);
void timer_delay_ms(uint8_t ms);


/* timer-ll.c */
void timer_init(void);
uint16_t timer_get_subsectimer(void);
uint8_t timer_getdec_todo(void);
uint8_t timer_get_todo(void);
uint32_t timer_get_linear_ss_time(void);

/* timer-ll.c is used to consolidate access to todo and subsectimer 
   to a single object */

#define SSTC ((F_CPU+128)/256)
#define FCPUKHZ ((F_CPU+500)/1000)
#define US_PER_SSUNIT ((256000+(FCPUKHZ/2))/FCPUKHZ)