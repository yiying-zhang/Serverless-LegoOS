#ifdef CONFIG_QUEUED_RWLOCKS
/*
 * Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 *
 * This file contains the spinlock/qrwlock implementations for
 * DEBUG_SPINLOCK.
 */

#include <linux/spinlock.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/debug_locks.h>
#include <linux/delay.h>
#include <linux/export.h>


void __qrwlock_init(qrwlock_t *lock, const char *name,
		   struct lock_class_key *key)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	/*
	 * Make sure we are not reinitializing a held lock:
	 */
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));
	lockdep_init_map(&lock->dep_map, name, key, 0);
#endif
	lock->raw_lock = (arch_qrwlock_t) __ARCH_QRW_LOCK_UNLOCKED;
	lock->magic = QRWLOCK_MAGIC;
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

EXPORT_SYMBOL(__qrwlock_init);


/* BEGIN_QRWLOCK */
static void qrwlock_bug(qrwlock_t *lock, const char *msg)
{
	if (!debug_locks_off())
		return;

	printk(KERN_EMERG "BUG: qrwlock %s on CPU#%d, %s/%d, %p\n",
		msg, raw_smp_processor_id(), current->comm,
		task_pid_nr(current), lock);
	dump_stack();
}

#define QRWLOCK_BUG_ON(cond, lock, msg) if (unlikely(cond)) qrwlock_bug(lock, msg)

void do_raw_qread_lock(qrwlock_t *lock)
{
	QRWLOCK_BUG_ON(lock->magic != QRWLOCK_MAGIC, lock, "bad magic");
	arch_qread_lock(&lock->raw_lock);
}

int do_raw_qread_trylock(qrwlock_t *lock)
{
	int ret = arch_qread_trylock(&lock->raw_lock);

#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	QRWLOCK_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_qread_unlock(qrwlock_t *lock)
{
	QRWLOCK_BUG_ON(lock->magic != QRWLOCK_MAGIC, lock, "bad magic");
	arch_qread_unlock(&lock->raw_lock);
}

static inline void debug_qwrite_lock_before(qrwlock_t *lock)
{
	QRWLOCK_BUG_ON(lock->magic != QRWLOCK_MAGIC, lock, "bad magic");
	QRWLOCK_BUG_ON(lock->owner == current, lock, "recursion");
	QRWLOCK_BUG_ON(lock->owner_cpu == raw_smp_processor_id(),
							lock, "cpu recursion");
}

static inline void debug_qwrite_lock_after(qrwlock_t *lock)
{
	lock->owner_cpu = raw_smp_processor_id();
	lock->owner = current;
}

static inline void debug_qwrite_unlock(qrwlock_t *lock)
{
	QRWLOCK_BUG_ON(lock->magic != QRWLOCK_MAGIC, lock, "bad magic");
	QRWLOCK_BUG_ON(lock->owner != current, lock, "wrong owner");
	QRWLOCK_BUG_ON(lock->owner_cpu != raw_smp_processor_id(),
							lock, "wrong CPU");
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

void do_raw_qwrite_lock(qrwlock_t *lock)
{
	debug_qwrite_lock_before(lock);
	arch_qwrite_lock(&lock->raw_lock);
	debug_qwrite_lock_after(lock);
}

int do_raw_qwrite_trylock(qrwlock_t *lock)
{
	int ret = arch_qwrite_trylock(&lock->raw_lock);

	if (ret)
		debug_qwrite_lock_after(lock);
#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	QRWLOCK_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_qwrite_unlock(qrwlock_t *lock)
{
	debug_qwrite_unlock(lock);
	arch_qwrite_unlock(&lock->raw_lock);
}
/* END_QRWLOCK */
#endif
