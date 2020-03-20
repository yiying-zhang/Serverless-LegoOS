/*
 * Copyright (c) 2016-2020 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) "PCOMP_MSG_HANDLER: " fmt

#include <lego/slab.h>
#include <lego/string.h>
#include <lego/fit_ibapi.h>
#include <lego/kthread.h>
#include <lego/pid.h>
#include <processor/processor.h>
#include <processor/vnode.h>
#include <monitor/common.h>

#define MAX_INIT_ARGS	CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS	CONFIG_INIT_ENV_ARG_LIMIT

#define MAX_RXBUF_SIZE	(20 * PAGE_SIZE)

#define MAX_P2P_MSG_SIZE 2048

const int ECHO_LEN = 49;
const char* ECHO = "~~~WASSUP THIS IS UNA Echo MSG from Receiver~~~\n";

const int SUCCESS_ENQUEUED_LEN = 29;
const char* SUCCESS_ENQUEUED = "~~~SUCCESSFULLY ENQUEUE!~~~\n";

const int FAIL_ENQUEUED_LEN = 25;
const char* FAIL_ENQUEUED = "~~~FAILED TO ENQUEUE~~~\n";

static int enqueue_msg(pid_t dst_pid, void * msg_body, unsigned int msg_size);
// static int dequeue_msg(pid_t dst_pid, void * recv_buf, unsigned int recv_size);

struct info_struct {
	uintptr_t desc;
	char msg[MAX_RXBUF_SIZE];
};

struct remote_msg {
	unsigned int msg_size;
	char msg[MAX_P2P_MSG_SIZE];

	/* I don't understand why in victim_flush this ptr is called next */
	struct list_head next;
};

// TODO: we should consolidate the handlers
static void handle_echo(struct common_header *hdr, u64 desc)
{
	pr_info("Receiving echo: %u, from node: %u\n",
		hdr->opcode, hdr->src_nid);

	ibapi_reply_message(ECHO, ECHO_LEN, desc);
}

static void handle_remote_send(struct info_struct *info)
{
	struct p2p_msg_struct * msg = (struct p2p_msg_struct *)(info->msg);
	struct p2p_msg_hdr * hdr = to_p2p_msg_header(msg);
	void * msg_body = to_p2p_msg_body(msg);

	// pr_info("~Receiving remote send: %u, from node: %u, pid: %d\n", hdr->opcode, hdr->src_nid, hdr->src_pid);
	// pr_info("~Targeting node: %u, pid: %d\n", hdr->dst_nid, hdr->dst_pid);
	// pr_info("---MSG LEN: %u---\n", hdr->msg_size);
	// pr_info("%s", msg_body);
	// pr_info("\n---MSG END---\n");

	int ret = enqueue_msg(hdr->dst_pid, msg_body, hdr->msg_size);

	if (ret) {
		ibapi_reply_message(SUCCESS_ENQUEUED, SUCCESS_ENQUEUED_LEN, info->desc);
	} else {
		ibapi_reply_message(FAIL_ENQUEUED, FAIL_ENQUEUED_LEN, info->desc);
	}
}

static int enqueue_msg(pid_t dst_pid, void * msg_body, unsigned int msg_size) {
	
	struct task_struct * p = find_task_by_pid(dst_pid);

	// Destination not exist
	if ((void *)p == NULL) {
		return 0;
	}

	struct remote_msg * r_msg = kmalloc(sizeof(struct remote_msg), GFP_KERNEL);
	if (unlikely(!r_msg)) {
		WARN(1, "OOM");
		return -ENOMEM;
	}

	/* Fill in the list node */
	r_msg->msg_size = msg_size;
	memcpy(r_msg->msg, msg_body, MAX_P2P_MSG_SIZE);

	spin_lock(&(p->msg_list_lock));
	list_add_tail(&(r_msg->next), &(p->remote_msg_list));
	atomic_inc(&(p->nr_msg_available));
	spin_unlock(&(p->msg_list_lock));

	return 1;
}

// static int dequeue_msg(pid_t dst_pid, void * recv_buf, unsigned int recv_size) {
	
// 	struct task_struct * p = find_task_by_pid(dst_pid);
	
// 	while (!atomic_read(&(p->nr_msg_available))) {
// 		cpu_relax();
// 	}

// 	spin_lock(&(p->msg_list_lock));
// 	struct remote_msg * r_msg = list_entry((p->remote_msg_list).next,
// 				 struct remote_msg, next);
// 	list_del(&(r_msg->next));
// 	atomic_dec(&(p->nr_msg_available));
// 	spin_unlock(&(p->msg_list_lock));

// 	copy_to_user(recv_buf, r_msg->msg, r_msg->msg_size);

// 	return r_msg->msg_size;
// }


static int msg_dispatcher(struct info_struct *info)
{
	uintptr_t desc = info->desc;
	void *payload;
	struct common_header *hdr;

	hdr = to_common_header(info->msg);
	payload = to_payload(info->msg);

	// pr_info("~~~~~~~~~~Within Msg dispacher~~~~~~~~~~~~~\n");

	/*
	 * BIG FAT NOTE:
	 * 1) Handler MUST call reply message
	 * 2) Handler CAN NOT free payload and hdr
	 * 3) Handler SHOULD NOT call exit()
	 */
	switch (hdr->opcode) {
		case P2P_RECHO:
			pr_info("~~~~Extracted P2P_RECHO opcode. following through to handler~~~~\n");
			handle_echo(hdr, desc);
			break;
		case P2P_RSEND_REPLY:
			// pr_info("~~~~Extracted P2P_RSEND_REPLY opcode. following through to handler~~~~\n");
			handle_remote_send(info);
			break;
		// default:
		
		// pr_info("~~~~~~~~~~msg dispacher done~~~~~~~~~~~~~\n");
	}

	return 0;
}

static int msg_handler(void *unused)
{
	struct info_struct *info;
	int port = 0;
	int retlen;

	info = kmalloc(sizeof(struct info_struct), GFP_KERNEL);
	if (unlikely(!info)) {
		WARN_ON(1);
		do_exit(-1);
	}
	pr_info("~~~~MSG handler is up and running~~~~~~~\n");

	while (1) {
		/*
		 * This function is blocking,
		 * will return until FIT gets a messages:
		 */
		memset(info, 0, sizeof(struct info_struct));
		retlen = ibapi_receive_message(port,
				info->msg, MAX_RXBUF_SIZE,
				&info->desc);

		if (unlikely(retlen >= MAX_RXBUF_SIZE))
			panic("retlen: %d,maxlen: %lu", retlen, MAX_RXBUF_SIZE);

		uintptr_t desc;
		void *payload;
		struct common_header *hdr;

		desc = info->desc;
		hdr = to_common_header(info->msg);
		payload = to_payload(info->msg);

		// pr_info("~~~~MSG handler GOT A NEW MSG!~~~~~~~\n");
		// pr_info("~~~~OPCODE %d, Source nid:%d~~~~\n", hdr->opcode ,hdr->src_nid);

		// if (hdr->src_nid == 0 || hdr->src_nid == 1) {
		// 	pr_info("~~~~Reading source nid:%d from header~~~~\n", hdr->src_nid);
		// }

		msg_dispatcher(info);
	}
	return 0;
}

void msg_handler_init(void)
{
#ifdef CONFIG_COMP_PROCESSOR
	struct task_struct *ret __maybe_unused;

	ret = kthread_run(msg_handler, NULL, "msg_handler");
	if (IS_ERR(ret)) {
		panic("Fail to create msg handler thread");
	}
	pr_info("processor msg handler is up\n");
#endif
}