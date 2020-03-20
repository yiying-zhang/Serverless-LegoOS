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

#ifdef CONFIG_DEBUG_STATE
#define state_debug(fmt, ...) \
	pr_debug("%s():%d " fmt, __func__, __LINE__, __VA_ARGS__)
#else
static inline void state_debug(const char *fmt, ...) { }
#endif

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
    while (*key)
    {
        h = (o * h + *key++) % table_size;
        o = o * t % (table_size - 1);
    }
    return h;
}

struct hlist_head * state_md; /* Create state_metadata as a hash table */
struct rw_semaphore * md_sems; /* Semaphore for each chain (hlist) in state_md */

#define STATE_MD_BITS 10
#define STATE_MD_SIZE (1 << STATE_MD_BITS)

DEFINE_SPINLOCK(md_lock); /* Lock related to metadata store creation and deletion */
//struct rw_semaphore md_sems[STATE_MD_SIZE]; /* Semaphore for each chain (hlist) in state_md */

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
    struct p2m_state_save_reply * retbuf;
    struct hlist_head * state_md_cand;
    struct rw_semaphore * md_sems_cand;
	struct md_entry * entry, * curr;
	char * state, * name;
    ssize_t retval;
    unsigned long flags, hashval;
    struct rw_semaphore * sem;
	int i, found;

    pr_debug("[Function] state_save\n");

	retval = 0;

    if (!state_md) { /* If state_md is initialized, then there is no need to acquire md_lock */
        pr_debug("[Warning] state_md doesn't exist. Initializing candidate.\n");
        state_md_cand = kmalloc(STATE_MD_SIZE * sizeof(struct hlist_head), GFP_KERNEL);
        if (!state_md_cand) {
			pr_err("[Error] Failed to create state metadata!\n");
            retval = -ENOMEM;
            goto out;
        }
        for (i = 0; i < STATE_MD_SIZE; i++) {
            INIT_HLIST_HEAD(&state_md_cand[i]);
        }
        pr_debug("[Success] state_md_cand initialized.\n");

		md_sems_cand = kmalloc(STATE_MD_SIZE * sizeof(struct rw_semaphore), GFP_KERNEL);
		if (!md_sems_cand) {
			pr_err("[Error] Failed to create state metadata!\n");
			kfree(state_md_cand);
			retval = -ENOMEM;
			goto out;
		}
		for (i = 0; i < STATE_MD_SIZE; i++) {
			init_rwsem(&md_sems_cand[i]);
		}

        spin_lock_irqsave(&md_lock, flags); /* Lock acquired for initialization */
        if (!state_md) {
            state_md = state_md_cand;
			md_sems = md_sems_cand;
            spin_unlock_irqrestore(&md_lock, flags);
            pr_debug("[Success] state_md and md_sems initialized.\n");
        } else {
            spin_unlock_irqrestore(&md_lock, flags);
            kfree(state_md_cand);
            kfree(md_sems_cand);
			pr_debug("[Warning] state_md already initialized by other thread. Aborting.\n");

        }
    }

    entry = kmalloc(sizeof(struct md_entry), GFP_KERNEL);
    if (!entry) {
		pr_err("[Error] Failed to create data entry!\n");
        retval = -ENOMEM;
        goto out;
    }
	pr_debug("[Success] entry initialized.\n");

    state = kmalloc(payload->state_size, GFP_KERNEL);
    if (!state) {
		pr_err("[Error] Failed to allocate memory for state data!\n");
        retval = -ENOMEM;
        goto free_entry;
    }
    memcpy(state, payload->state, payload->state_size);
	pr_debug("[Success] state initialized. {%s}\n", state);

    name = kmalloc(payload->name_size, GFP_KERNEL);
    if (!name) {
		pr_err("[Error] Failed to allocate memory for state name!\n");
        retval = -ENOMEM;
        goto free_state;
    }
    memcpy(name, payload->name, payload->name_size);
	pr_debug("[Success] name initialized. {%s}\n", name);

    entry->name = name;
    entry->data.addr = state;
    entry->data.size = payload->state_size;

    hashval = hash_func(payload->name, STATE_MD_SIZE);
    sem = &md_sems[hashval];

    found = 0;
    down_write(sem); /* Acquire WRITE lock */
    hlist_for_each_entry(curr, &state_md[hashval], node) {
		pr_debug("[Log] data=%s\n", curr->name);
        if (!strcmp(curr->name, payload->name)){
			pr_debug("[Log] Found a matching state\n");
            found = 1;
            break;
        }
    }
    if (found) {
        curr->data.addr = entry->data.addr;
        curr->data.size = entry->data.size;
    }
    else {
        hlist_add_head(&(entry->node), &state_md[hashval]);
    }
    up_write(sem); /* Release WRITE lock */

    if (found){
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
    // construct reply
    ssize_t retval;
    struct p2m_state_load_reply *retbuf;
	struct md_entry * curr;
	unsigned long hashval;
    struct rw_semaphore * sem;

	pr_debug("[Function] state_load\n");

    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));

    retval = -EINVAL;
    retbuf->state_size = 0;


    if (!state_md) {
		pr_err("[Error] state_md doesn't exist. Stop.\n");
        goto out;
    }

    hashval = hash_func(payload->name, STATE_MD_SIZE);
    sem = &md_sems[hashval];

    down_read(sem); /* Acquire READ lock */

	hlist_for_each_entry(curr, &state_md[hashval], node) {
		pr_debug("[Log] data=%s\n", curr->name);
        if (!strcmp(curr->name, payload->name)){
			pr_debug("[Log] Found a matching state\n");
            memcpy(retbuf->state, curr->data.addr, curr->data.size);
            retbuf->state_size = curr->data.size;
            retval = 0;
            break;
        }
    }
    up_read(sem); /* Release READ lock */

out:
    retbuf->retval = retval;

}

/**
 * handle_p2m_state_delete - delete state data referenced by name
 * @payload: payload struct storing name
 * @hdr: header struct for getting caller identifier
 * @tb: output buffer for constructing reply
 */
void handle_p2m_state_delete(struct p2m_state_delete_payload * payload, struct common_header *hdr, struct thpool_buffer *tb)
{
    // construct reply
    ssize_t retval;
    struct p2m_state_delete_reply *retbuf;
	struct md_entry * curr;
	unsigned long hashval;
    struct rw_semaphore * sem;

	pr_debug("[Function] state_delete\n");

    retval = -EINVAL;
    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));

    if (!state_md) {
		pr_err("[Error] state_md doesn't exist. Stop.\n");
        retval = -EINVAL;
		goto out;
    }

    hashval = hash_func(payload->name, STATE_MD_SIZE);
    sem = &md_sems[hashval];

    down_write(sem); /* Acquire READ lock */
    hlist_for_each_entry(curr, &state_md[hashval], node) {
		pr_debug("[Log] data=%s\n", curr->name);
        if (!strcmp(curr->name, payload->name)){
			pr_debug("[Log] Found a matching state\n");
            kfree(curr->data.addr);
            kfree(curr->name);
            hash_del(&(curr->node));
            retval = 0;
            break;
        }
    }
    up_write(sem); /* Release READ lock */
out:
    retbuf->retval = retval;

}

/**
 * handle_p2m_state_check - check if state data referenced by name exists
 * @payload: payload struct storing name
 * @hdr: header struct for getting caller identifier
 * @tb: output buffer for constructing reply
 */
void handle_p2m_state_check(struct p2m_state_check_payload * payload, struct common_header *hdr, struct thpool_buffer *tb)
{
    // construct reply
    ssize_t retval;
    struct p2m_state_check_reply *retbuf;
	struct md_entry * curr;
	unsigned long hashval;
    struct rw_semaphore * sem;

	pr_debug("[Function] state_check\n");

    retval = -EINVAL;
    retbuf = thpool_buffer_tx(tb);
    tb_set_tx_size(tb, sizeof(*retbuf));

    if (!state_md) {
		pr_err("[Error] state_md doesn't exist. Stop.\n");
        retval = -EINVAL;
		goto out;
    }

    hashval = hash_func(payload->name, STATE_MD_SIZE);
    sem = &md_sems[hashval];

    down_read(sem); /* Acquire READ lock */
	hlist_for_each_entry(curr, &state_md[hashval], node) {
		pr_debug("[Log] data=%s\n", curr->name);
        if (!strcmp(curr->name, payload->name)){
			pr_debug("[Log] Found a matching state\n");
            retval = 0;
            break;
        }
    }
    up_read(sem); /* Release READ lock */
out:
    retbuf->retval = retval;

}