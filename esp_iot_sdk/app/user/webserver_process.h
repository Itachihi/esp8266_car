#ifndef __WEBSERVER_PROCESS_H__
#define __WEBSERVER_PROCESS_H__

#include "espconn.h"

#define ARRAY_SIZE(a)	sizeof(a)/sizeof(a[0])

struct ProcCommand_t {
	const char *const name;
	void (*func)(struct espconn *ctx, const char *const parm);
};


struct ProcSelect_t {
	const char *const name;
	const struct ProcCommand_t *commands;
	int count;
};

void *getCommandHandler(const char *const select, const char *const command);

#endif
