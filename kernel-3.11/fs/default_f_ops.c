/*
 * Copyright (c) 2016-2019 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <lego/stat.h>
#include <lego/slab.h>
#include <lego/uaccess.h>
#include <lego/files.h>
#include <lego/syscalls.h>
#include <lego/comp_common.h>
#include <lego/comp_storage.h>
#include <lego/seq_file.h>
#include <lego/timer.h>
#include <lego/fit_ibapi.h>
#include <lego/kernel.h>
#include <processor/fs.h>
#include <processor/processor.h>

#ifdef CONFIG_DEBUG_FILE
#define file_debug(fmt, ...)	\
	pr_debug("%s(): " fmt "\n", __func__, __VA_ARGS__)
#else
static inline void file_debug(const char *fmt, ...) { }
#endif

// TODO: to change the function signature
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
