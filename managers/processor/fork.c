/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <lego/sched.h>
#include <lego/comp_processor.h>

/* Return 0 on success, other on failure */
int p2m_fork(struct task_struct *p)
{
	struct p2m_fork_struct payload;
	int ret, retbuf;

	pr_info("%s: pid: %d\n", __func__, p->pid);
	BUG_ON(!p);
	payload.pid = p->pid;
	memcpy(payload.comm, p->comm, TASK_COMM_LEN);

	ret = net_send_reply_timeout(DEF_MEM_HOMENODE, P2M_FORK, &payload,
				sizeof(payload), &retbuf, sizeof(retbuf), false,
				DEF_NET_TIMEOUT);

	WARN(retbuf, ret_to_string(retbuf));
	return retbuf;
}
