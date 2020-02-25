/*
 * Copyright (c) 2016-2020 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <lego/kernel.h>
#include <lego/comp_common.h>
#include <lego/printk.h>
#include <lego/hashtable.h>

#include <memory/thread_pool.h>

//TODO: handle_p2m_state_dummy_get
struct p2m_state_reply {
    ssize_t		retval;
};

void handle_p2m_state_dummy_get(struct p2m_state_struct *payload, struct thpool_buffer *tb)
{
    // Print number from payload
    printk("HEYYY! Handling message for state management: %ld", payload->number);
//    pr_info("handling message: %ld\n", payload->number);

    ssize_t retval = 6666;
    void *buf;
    struct p2m_state_reply *retbuf;
    retbuf = thpool_buffer_tx(tb);
    buf = (char *)retbuf;
    tb_set_tx_size(tb, sizeof(retval));

    retbuf->retval = retval;
}


/*
 *  state_save
 */
struct hlist_head * state_md; /* Create state_metadata as a hash table */
#define STATE_MD_SIZE 10

struct md_entry {
    char * name;
    struct {
        void * addr;
        size_t size;
    } data;
    struct hlist_node * node;
}

void handle_p2m_state_save(struct p2m_state_save_payload * payload, struct thpool_buffer *tb)
{
    printk("[Function] state_save\n");

    if (!state_md) {
        printk("[Warning] state_md doesn't exist. Attempt to initialize.\n");
        state_md = kmalloc(STATE_MD_SIZE * sizeof(struct hlist_head), GFP_KERNEL);
        if (!state_md){
            printk("[Error] Failed to create state metadata!\n");
            return -ENOMEN;
        }
        for (int i =0; i < STATE_MD_SIZE; i++)
            INIT_HLIST_HEAD(&state_md[i]);
    }

    struct md_entry entry = kmalloc(sizeof(struct md_entry), GFP_KERNEL);
    if (!entry){
        printk("[Error] Failed to create data entry!\n");
        return -ENOMEN;
    }


    // kmalloc saved state
    state = kmalloc(payload->state_size, GFP_KERNEL);
    if (!state){
        printk("[Error] Failed to allocate memory for state data!\n");
        return -ENOMEN;
    }
    name = kmalloc(payload->name_size, GFP_KERNEL);
    if (!name){
        printk("[Error] Failed to allocate memory for state name!\n");
        return -ENOMEN;
    }
    memcpy(state, payload->state, payload->state_size);
    memcpy(name, payload->name, payload->name_size);
    entry->name = name;
    entry->data->addr = state;
    entry->data->size = payload->state_size;

    hash_add(*state_md, &state_md->node, state_md->name);

    // construct reply
    ssize_t retval = 0;
    struct p2m_state_save_reply *retbuf;
    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));
    retbuf->retval = retval;
}


/*
 *  state_load
 */

void handle_p2m_state_load(struct p2m_state_load_payload * payload, struct thpool_buffer *tb)
{
    printk("[Function] state_load\n");


    // construct reply
    ssize_t retval = 0;
    struct p2m_state_load_reply *retbuf;
    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));

    retbuf->retval = retval;
    strcpy(retbuf->state, "Reply Placeholder");

}