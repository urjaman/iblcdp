struct cron_task {
	struct cron_task *next;
	void(*taskf)(void);
	uint16_t ss_freq;
	uint16_t next_invoc;
};
/* These are the public functions for everybody. */
void cron_add_task(struct cron_task *t);
void cron_rm_task(struct cron_task *t);
/* These are the cron API for timer. */
void cron_initialize(void);
uint16_t cron_next_task(void);
uint16_t cron_run_tasks(void);
