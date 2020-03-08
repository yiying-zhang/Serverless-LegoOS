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

#include <lego/spinlock.h>
#include <lego/rwsem.h>
#include <lego/hashtable.h>

#include <memory/thread_pool.h>

/**
 * hash_func - hash a string to unsigned long given table size
 * @key: key string to be hashed
 * @table_size: a positive number specifying table size
 */
unsigned long hash_func(const char * key, const unsigned int table_size)
{
    unsigned int h = 0;
    unsigned int o = 31415;
    const unsigned int t = 27183;
    char * key_copy = key;
    while (*key)
    {
        h = (o * h + *key++) % table_size;
        o = o * t % (table_size - 1);
    }
    // Verbose debugging info for now
    printk ("[Success] hashing {%s} to %d\n", key_copy, h);
    return h;
}

struct hlist_head * state_md; /* Create state_metadata as a hash table */
#define STATE_MD_BITS 8
#define STATE_MD_SIZE (1 << STATE_MD_BITS)

DEFINE_SPINLOCK(md_lock); /* Lock related to metadata store creation and deletion */
struct rw_semaphore md_sems[STATE_MD_SIZE]; /* Semaphore for each chain (hlist) in state_md */

/*
 * metadata entry
 */
struct md_entry {
    char * name; /* state name as the key for hashing */
    struct {
        void * addr;
        size_t size;
    } data; /* saved state address and size */
    struct hlist_node node; /* the node linking next entry along the hlist */
};

/**
 * handle_p2m_state_save - create state_md if not exist and save state with name as the key (tentatively)
 * @payload: payload struct storing name and state data
 * @hdr: header struct for getting caller identifier
 * @tb: output buffer for constructing reply
 */
void handle_p2m_state_save(struct p2m_state_save_payload * payload, struct common_header *hdr, struct thpool_buffer *tb)
{
    printk("[Function] state_save\n");
    struct p2m_state_save_reply *retbuf;
    struct hlist_head * state_md_cand;
    ssize_t retval;
    unsigned long flags;
    unsigned long hashval;
    struct rw_semaphore * sem;

    retval = 0;

    if (!state_md) { /* If state_md is initialized, then there is no need to acquire md_lock */
        printk("[Warning] state_md doesn't exist. Initializing candidate.\n");
        state_md_cand = kmalloc(STATE_MD_SIZE * sizeof(struct hlist_head), GFP_KERNEL);
        if (!state_md_cand) {
            printk("[Error] Failed to create state metadata!\n");
            retval = -ENOMEM;
            goto out;
        }
        int i;
        for (i = 0; i < STATE_MD_SIZE; i++) {
            INIT_HLIST_HEAD(&state_md_cand[i]);
        }
        printk("[Success] state_md_cand initialized.\n");

        spin_lock_irqsave(&md_lock, flags); /* Lock acquired for initialization */
        if (!state_md) {
            state_md = state_md_cand;
            /* Also initialize all hlist semaphores. This should only be done once. */
            for (i = 0; i < STATE_MD_SIZE; i++) {
                init_rwsem(&md_sems[i]);
            }
            spin_unlock_irqrestore(&md_lock, flags);
            printk("[Success] state_md and md_sems initialized.\n");
        } else {
            spin_unlock_irqrestore(&md_lock, flags);
            kfree(state_md_cand);
            printk("[Warning] state_md already initialized by other thread. Aborting.\n");

        }
    }

    struct md_entry * entry = kmalloc(sizeof(struct md_entry), GFP_KERNEL);
    if (!entry) {
        printk("[Error] Failed to create data entry!\n");
        retval = -ENOMEM;
        goto out;
    }
    printk("[Success] entry initialized.\n");

    char * state = kmalloc(payload->state_size, GFP_KERNEL);
    if (!state) {
        printk("[Error] Failed to allocate memory for state data!\n");
        retval = -ENOMEM;
        goto free_entry;
    }
    memcpy(state, payload->state, payload->state_size);
    printk("[Success] state initialized. {%s}\n", state);

    char * name = kmalloc(payload->name_size, GFP_KERNEL);
    if (!name) {
        printk("[Error] Failed to allocate memory for state name!\n");
        retval = -ENOMEM;
        goto free_state;
    }
    memcpy(name, payload->name, payload->name_size);
    printk("[Success] name initialized. {%s}\n", name);

    entry->name = name;
    entry->data.addr = state;
    entry->data.size = payload->state_size;

    hashval = hash_func(payload->name, STATE_MD_SIZE);
    sem = &md_sems[hashval];

    struct md_entry * curr;
    down_write(sem); /* Acquire WRITE lock */
    hlist_for_each_entry(curr, &state_md[hashval], node) {
        printk("[Log] data=%s\n", curr->name);
        if (!strcmp(curr->name, payload->name)){
            printk("[Log] Found a matching state\n");
            break;
        }
    }
    if (curr != NULL) {
        curr->data.addr = entry->data.addr;
        curr->data.size = entry->data.size;
    }
    else {
        hlist_add_head(&(entry->node), &state_md[hashval]);
    }
    up_write(sem); /* Release WRITE lock */

    if (curr != NULL){
        kfree(name);
        kfree(entry);
    }

    goto out;

free_state:
    kfree(state);
free_entry:
    kfree(entry);
out:
    // construct reply
    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));
    retbuf->retval = retval;
}

/**
 * handle_p2m_state_load - load state data to output buffer referenced by name (assume no duplicates, tentatively)
 * @payload: payload struct storing name
 * @hdr: header struct for getting caller identifier
 * @tb: output buffer for constructing reply
 */
void handle_p2m_state_load(struct p2m_state_load_payload * payload, struct common_header *hdr, struct thpool_buffer *tb)
{
    printk("[Function] state_load\n");
    // construct reply
    ssize_t retval;
    struct p2m_state_load_reply *retbuf;
    unsigned long hashval;
    struct rw_semaphore * sem;

    retval = 0;
    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));

    retbuf->retval = retval;
    strcpy(retbuf->state, "Reply Placeholder");

    if (!state_md) {
        printk("[Error] state_md doesn't exist. Stop.\n");
        retval = -EINVAL;
        return;
    }

    hashval = hash_func(payload->name, STATE_MD_SIZE);
    sem = &md_sems[hashval];

    struct md_entry * curr;
    down_read(sem); /* Acquire READ lock */
    hlist_for_each_entry(curr, &state_md[hashval], node) {
        printk("[Log] data=%s\n", curr->name);
        if (!strcmp(curr->name, payload->name)){
            printk("[Log] Found a matching state\n");
            memcpy(retbuf->state, curr->data.addr, curr->data.size);
            break;
        }
    }
    up_read(sem); /* Release READ lock */

}