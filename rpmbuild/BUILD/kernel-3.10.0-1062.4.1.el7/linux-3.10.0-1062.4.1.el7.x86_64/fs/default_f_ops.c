/*
 * fs/dax.c - Direct Access filesystem code
 * Copyright (c) 2013-2014 Intel Corporation
 * Author: Matthew Wilcox <matthew.r.wilcox@intel.com>
 * Author: Ross Zwisler <ross.zwisler@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/atomic.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/dax.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/pagevec.h>
#include <linux/sched.h>
#include <linux/uio.h>
#include <linux/socket.h>
#include <linux/vmstat.h>
#include <linux/pfn_t.h>
#include <linux/sizes.h>
#include <linux/mmu_notifier.h>
#include <linux/iomap.h>
#include "internal.h"

#ifdef CONFIG_DEBUG_FILE
#define file_debug(fmt, ...)	\
	pr_debug("%s(): " fmt "\n", __func__, __VA_ARGS__)
#else
static inline void file_debug(const char *fmt, ...) { }
#endif

static int p2s_open(struct file *f)
{
    int retval = 0;
    void *msg;
    u32 len_msg, *opcode;
    struct p2s_open_struct *payload;

    len_msg = sizeof(*opcode) + sizeof(*payload);
    msg = kmalloc(len_msg, GFP_KERNEL);
    if (!msg)
        return -ENOMEM;

    opcode = msg;
    *opcode = P2S_OPEN;

    payload = msg + sizeof(*opcode);
    payload->uid = current_uid();
    strncpy(payload->filename, f->f_name, MAX_FILENAME_LENGTH);
    payload->permission = f->f_mode;
    payload->flags = f->f_flags;

    file_debug("f_name: %s, mode: 0%o, flags: %x",
               payload->filename, payload->permission, payload->flags);

    ibapi_send_reply_imm(current_storage_home_node(), msg, len_msg,
                         &retval, sizeof(retval), false);

#ifdef CONFIG_DEBUG_FILE
    if (retval < 0)
		pr_debug("%s: %s\n", FUNC, ret_to_string(ERR_TO_LEGO_RET((long)retval)));
#endif

    kfree(msg);
    return retval;
}