#include "main.h"
#include "timer.h"
#include "cron.h"

static struct cron_task *chead = NULL;

static void list_rm_task(struct cron_task *t)
{
	static struct cron_task * s;
	static struct cron_task * p = NULL;
	for (s=chead;s;s = s->next) {
		if (s==t) {
			if (!p) {
				chead = s->next;
			} else {
				p->next = s->next;
			}
		}
		p = s;
	}
}

static void list_add_task(struct cron_task *t,uint16_t sstimer) {
	/* Assumed: task isnt in the list. */
	t->next_invoc = sstimer + t->ss_freq;
	static struct cron_task * s;
	static struct cron_task * p = NULL;
	for (s=chead;s;s = s->next) {
		if (s->next_invoc > t->next_invoc) {
			break;
		}
		p = s;
	}
	if (!p) {
		t->next = chead;
		chead = t;
	} else {
		s = p->next;
		p->next = t;
		t->next = s;
	}
}

void cron_add_task(struct cron_task *t) {
	list_rm_task(t); /* Check that it isnt in the list. */
	list_add_task(t,timer_get_subsectimer());
}

void cron_rm_task(struct cron_task *t) {
	list_rm_task(t);
}

void cron_initialize(void) {
	if (!chead) return;
	/* Second changed. Re-build our list. */
	struct cron_task *s = chead;
	chead = NULL;
	while (s) {
		struct cron_task *np = s->next;
		list_add_task(s,0);
		s = np;
	}
	/* And call everybody once at 0 ss units. */
	/* This is how SSTC freq tasks get their run force. */
	s = chead;
	while (s) {
		struct cron_task *np = s->next;
		s->taskf();
		s = np;
	}
}

/* Give timer the next time when to run a task. */
uint16_t cron_next_task(void) {
	if (!chead) return SSTC;
	return chead->next_invoc;
}

/* Run the next task. */
uint16_t cron_run_tasks(void) {
	if (!chead) return SSTC;
	uint16_t sstimer = timer_get_subsectimer();
	struct cron_task *nt = chead;
	nt->taskf();
	chead = nt->next;
	list_add_task(nt,sstimer);
	return chead->next_invoc;
}
