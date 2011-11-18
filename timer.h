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

/* timer-ll.c */
void timer_init(void);
uint16_t timer_get_subsectimer(void);
uint8_t timer_getdec_todo(void);
uint8_t timer_get_todo(void);
/* timer-ll.c is used to consolidate access to todo and subsectimer 
   to a single object */