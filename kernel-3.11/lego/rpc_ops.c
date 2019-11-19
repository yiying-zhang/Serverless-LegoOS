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
#include <lego/struct_common.h>
#include <lego/fit_ibapi.h>

struct p2s_open_struct{
    int	uid;
    char filename[MAX_FILENAME_LENGTH];
    fmode_t	permission;
    int	flags;
};

// TODO: to change the function signature
asmlinkage int f2s_send(struct file *f, int storage_node)
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
    //strncpy(payload->filename, f->f_name, MAX_FILENAME_LENGTH); // no need for the file name
    payload->permission = f->f_mode;
    payload->flags = f->f_flags;

    ibapi_send_reply_imm(storage_node, msg, len_msg,
                         &retval, sizeof(retval), false);

    kfree(msg);
    return retval;
}
