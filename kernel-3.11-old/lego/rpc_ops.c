/*
 * Copyright (c) 2016-2019 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stat.h>
#include <linux/slab.h>
#include <processor/fs.h>
#include <processor/processor.h>

// TODO: to change the function signature
asmlinkage int f2s_send(struct file *f)
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
    
    kfree(msg);
    return retval;
}
