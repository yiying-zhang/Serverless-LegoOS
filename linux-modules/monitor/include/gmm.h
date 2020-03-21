/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _LEGO_GMM_H
#define _LEGO_GMM_H

/* information of each memory component */
struct mnode_struct {
	__u32 nid;
	unsigned long totalram;
	unsigned long freeram;
	unsigned long nr_request;
	struct list_head list;
};

int choose_node(void);
int handle_m2mm_consult(struct consult_info *, u64, struct common_header *);
void handle_m2mm_status_report(struct m2mm_status_report *payload, u64 desc);

/* state management: mnode lookup */
int handle_p2mm_state_lookup(struct p2mm_state_lookup *payload, u64 desc, struct common_header *hdr);

#endif /* _LEGO_GMM_H */
