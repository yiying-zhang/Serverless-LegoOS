/*
 * Copyright (c) 2016-2020 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

//#ifdef CONFIG_COMP_PROCESSOR

#include <lego/syscalls.h>
#include <lego/comp_common.h>
#include <lego/fit_ibapi.h>

#include <processor/processor.h>

SYSCALL_DEFINE4(state_save, char*, name, unsigned long, name_size, unsigned long, state_size, const char*, state)
{
    ssize_t retval;
    ssize_t retlen;
    u32 len_msg;
    void* msg;
    struct common_header* hdr;
    struct p2m_state_save_payload* payload;

    len_msg = sizeof(*hdr)+sizeof(*payload);
    msg = kmalloc(len_msg, GFP_KERNEL);
    if(!msg)
        return -ENOMEM;

    hdr = (struct common_header *) msg;
    hdr->opcode = P2M_STATE_SAVE;
    hdr->src_nid = LEGO_LOCAL_NID;
    payload = to_payload(msg);

    if(copy_from_user(payload->name, name, name_size+1)){
        retval = -EFAULT;
        goto OUT;
    }

    if(copy_from_user(payload->state, state, state_size+1)){
        retval = -EFAULT;
        goto OUT;
    }

    payload->name_size = name_size+1;
    payload->state_size = state_size+1;

    retlen = ibapi_send_reply_imm(current_memory_home_node(), msg, len_msg, &retval, sizeof(retval),false);

    /* check return value */
    if(retlen == -ETIMEDOUT){
        retval = -ETIMEDOUT;
        goto OUT;
    }

OUT:
    /* free allocated memory */
    kfree(msg);
    return retval;

}

SYSCALL_DEFINE4(state_load, char*, name, unsigned long, name_size, unsigned long, state_size, char*, state)
{
    struct p2m_state_load_reply reply;
    ssize_t retval;
    ssize_t retlen;
    u32 len_msg;
    void *msg;
    struct common_header* hdr;
    struct p2m_state_load_payload* payload;

    len_msg = sizeof(*hdr) + sizeof(*payload);
    msg = kmalloc(len_msg, GFP_KERNEL);
    if(!msg)
        return -ENOMEM;

    hdr = (struct common_header *)msg;
    hdr->opcode = P2M_STATE_LOAD;
    hdr->src_nid = LEGO_LOCAL_NID;
    payload = to_payload(msg);

    if(copy_from_user(payload->name, name, name_size+1)){
        retval = -EFAULT;
        goto OUT;
    }
    payload->name_size = name_size+1;

    retlen = ibapi_send_reply_imm(current_memory_home_node(), msg, len_msg, &reply, sizeof(reply),false);

    /* check return value */
    if(retlen == -ETIMEDOUT){
        retval = -ETIMEDOUT;
        goto OUT;
    }


    /* reply, reply 0 means good */
    if(reply.retval == 0){
        /* copy data to user space */
        if(copy_to_user(state, (void*)reply.state, (unsigned long)strlen(reply.state)+1)){
            retval = -EFAULT;
            goto OUT;
        }

    }

    retval = reply.retval;

OUT:
    /* free allocated memory */
    kfree(msg);

    return retval;

}



//#endif /* _LEGO_PROCESSOR_NODE_H_ */