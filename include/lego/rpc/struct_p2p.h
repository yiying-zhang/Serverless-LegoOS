/*
 * Copyright (c) 2016-2020 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _LEGO_RPC_STRUCT_P2P_H
#define _LEGO_RPC_STRUCT_P2P_H

#include <lego/rpc/struct_common.h>
#include <lego/types.h>
#include <lego/kernel.h>

#define MAX_P2P_MSG_LEN 100

struct p2p_msg_hdr {
	unsigned int opcode;		/* OPCODE should be useless in our case */
	unsigned int src_nid;
	pid_t src_pid;
	unsigned int dst_nid;
	pid_t dst_pid;
	unsigned int msg_len;
};

struct p2p_msg_struct {
	struct p2p_msg_hdr hdr;
	char msg_content[MAX_P2P_MSG_LEN];
};


static inline struct p2p_msg_hdr *to_p2p_msg_header(void *msg)
{
	return (struct p2p_msg_hdr *)(msg);
}

static inline void *to_p2p_msg_body(void *msg)
{
	return (void *)(msg + sizeof(struct p2p_msg_hdr));
}

static inline void print_p2p_msg_header(struct p2p_msg_hdr * hdr)
{
	pr_info(">>>p2p_msg_hdr:\n");
	pr_info("\tOPCODE:%d\n", hdr->opcode);
	pr_info("\tsrc_nid:%d\n", hdr->src_nid);
	pr_info("\tsrc_pid:%d\n", hdr->src_pid);
	pr_info("\tdst_nid:%d\n", hdr->dst_nid);
	pr_info("\tdst_pid:%d\n", hdr->dst_pid);
	pr_info("\tmsg_len:%u\n", hdr->msg_len);
}

#endif