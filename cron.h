struct cron_task {
	struct cron_task *next;
	void(*taskf)(void);
	uint16_t ss_freq;
	uint16_t next_invoc;
};
void cron_add_task(struct cron_task *t);
void cron_rm_task(struct cron_task *t);
void cron_initialize(void);
uint16_t cron_next_task(void);
uint16_t cron_run_tasks(void);
