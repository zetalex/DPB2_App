


#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#ifndef TIMER_H_   /* Include guard */
#define TIMER_H_

struct periodic_info {
	int sig;
	sigset_t alarm_sig;
};

static int make_periodic(int unsigned period, struct periodic_info *info);
static void wait_period(struct periodic_info *info);

#endif

