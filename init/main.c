/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <lego/mm.h>
#include <lego/pid.h>
#include <lego/smp.h>
#include <lego/bug.h>
#include <lego/tty.h>
#include <lego/irq.h>
#include <lego/pci.h>
#include <lego/net.h>
#include <lego/init.h>
#include <lego/list.h>
#include <lego/slab.h>
#include <lego/time.h>
#include <lego/wait.h>
#include <lego/timer.h>
#include <lego/delay.h>
#include <lego/sched.h>
#include <lego/string.h>
#include <lego/atomic.h>
#include <lego/kernel.h>
#include <lego/extable.h>
#include <lego/jiffies.h>
#include <lego/kthread.h>
#include <lego/cpumask.h>
#include <lego/nodemask.h>
#include <lego/spinlock.h>
#include <lego/irqdomain.h>
#include <lego/pci.h>
#include <lego/net.h>
#include <lego/memory.h>
#include <lego/workqueue.h>
#include <lego/completion.h>

#include <lego/comp_memory.h>
#include <lego/comp_processor.h>

#include <asm/io.h>
#include <asm/asm.h>
#include <asm/page.h>
#include <asm/numa.h>
#include <asm/traps.h>
#include <asm/setup.h>


enum system_states system_state __read_mostly;

/* Screen information used by kernel */
struct screen_info screen_info;

/* Builtin command line from kconfig */
#ifdef CONFIG_CMDLINE_BOOL
static char __initdata builtin_cmdline[COMMAND_LINE_SIZE] = CONFIG_CMDLINE;
#endif

/* Untouched command line saved by head, passed from boot loader */
char __initdata boot_command_line[COMMAND_LINE_SIZE];

/* Concatenated command line from boot and builtin */
static char command_line[COMMAND_LINE_SIZE];

/* Setup configured maximum number of CPUs to activate */
unsigned int setup_max_cpus = NR_CPUS;

extern void calibrate_delay(void);

/* Defined in linker scripts */
extern const struct obs_kernel_param __sinitsetup[], __einitsetup[];

static int __init parse_kernel_param(char *param, char *val)
{
	const struct obs_kernel_param *p;

	for (p = __sinitsetup; p < __einitsetup; p++) {
		if (parameq(param, p->str)) {
			if (p->setup_func(val) != 0) {
				pr_warn("Malformed option '%s'\n", param);
				return -ENOENT;
			}
		}
	}
	return 0;
}

static void inline setup_nr_cpu_ids(void)
{
	nr_cpu_ids = find_last_bit(cpumask_bits(cpu_possible_mask), NR_CPUS) + 1;
}

#define MAX_INIT_ARGS	CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS	CONFIG_INIT_ENV_ARG_LIMIT

#ifdef CONFIG_COMP_PROCESSOR
/* http://c-faq.com/decl/spiral.anderson.html */
static const char *argv_init[MAX_INIT_ARGS+2] = { "init", NULL, };
const char *envp_init[MAX_INIT_ENVS+2] = { "HOME=/", "TERM=linux", NULL, };

static int procmgmt(void *unused)
{
	const char *init_filename = "/etc/init";

	pr_info("Global thread pid: %u\n", current->pid);

	argv_init[0] = init_filename;
	return do_execve(init_filename,
		(const char *const *)argv_init,
		(const char *const *)envp_init);
}

static void run_global_thread(void)
{
	/*
	 * Must use kernel_thread instead of global_kthread_run
	 * because that one will call do_exit inside. So do_execve
	 * will not have any effect.
	 */
	kernel_thread(procmgmt, NULL, CLONE_GLOBAL_THREAD);
}
#endif

static __initdata DEFINE_COMPLETION(kthreadd_done);

/*
 * This is our first kernel thread (pid 1),
 */
static int kernel_init(void *unused)
{
	/* Wait until kthreadd is all set-up. */
	wait_for_completion(&kthreadd_done);
	set_task_comm(current, "kernel_init");

	init_workqueues();

#ifdef CONFIG_INFINIBAND
	ib_mad_init();
	pr_info("after ib_mad_init\n");
#endif

	pr_info("before pci init\n");
	pci_init();

#ifdef CONFIG_INFINIBAND
	pr_info("before cmb init\n");
	ib_cm_init();
	pr_info("before lego ib init\n");
	kthread_run(lego_ib_init, NULL, "ib-initd");

	/* wait until ib finished initialization */
	wait_for_completion(&ib_init_done);
#endif

	panic("asd");
	/* Final step towards a running component.. */
#ifdef CONFIG_COMP_PROCESSOR
	processor_component_init();
	run_global_thread();
#elif defined(CONFIG_COMP_MEMORY)
	memory_component_init();
#endif
	/*
	 * For memory component, there is no need to run user-program,
	 * kernel_init can just exit.
	 * For processor component, it has to create a new global
	 * thread, so kernel is useless too.
	 */
	do_exit(0);

	return 0;
}

static void rest_init(void)
{
	int pid;

	/*
	 * We need to spawn init first so that it obtains pid 1, however
	 * the init task will end up wanting to create kthreads, which, if
	 * we schedule it before we create kthreadd, will OOPS.
	 */
	kernel_thread(kernel_init, NULL, CLONE_FS);

	pid = kernel_thread(kthreadd, NULL, CLONE_FS);
	kthreadd_task = find_task_by_pid(pid);
	complete(&kthreadd_done);

	/*
	 * The boot idle thread must execute schedule()
	 * at least once to get things moving (the preempt
	 * count must be 1 here):
	 */
	setup_init_idleclass(current);
	schedule_preempt_disabled();

	/* Call into cpu_idle with preempt disabled */
	cpu_idle();
}

asmlinkage void __init start_kernel(void)
{
	local_irq_disable();

	setup_task_stack_end_magic(&init_task);

	boot_cpumask_init();

	/* Prepare output first */
	tty_init();
	pr_info("%s", lego_banner);

#ifdef CONFIG_CMDLINE_BOOL
	if (builtin_cmdline[0]) {
		/* append boot loader cmdline to builtin */
		strlcat(builtin_cmdline, " ", COMMAND_LINE_SIZE);
		strlcat(builtin_cmdline, boot_command_line, COMMAND_LINE_SIZE);
		strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
	}
#endif
	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	pr_info("Command line: %s\n", command_line);

	/* Things to be done before parsing cmdline */
	early_setup_arch();

	/* Parse setup parameters */
	parse_args(command_line, parse_kernel_param);

	/* Architecture-Specific Initialization */
	setup_arch();
	setup_nr_cpu_ids();
	setup_nr_node_ids();
	setup_per_cpu_areas();

	/*
	 * Init CPU0
	 * cpu_init() will use smp_processor_id(), which
	 * MUST come after setting up per-cpu areas
	 */
	cpu_init();

	/* Allocate pid mapping array */
	pid_init();

	/*
	 * JUST A NOTE:
	 * If we have any large bootmem allocations later (e.g. printk logbuf),
	 * they should go right before memory_init(), because memory_init()
	 * will reserve the reserved memblock, and free the free memblock
	 * to buddy allocator. No more large allocations will be possible.
	 */

	/*
	 * Build all memory managment data structures,
	 * buddy allocator is avaiable afterwards:
	 */
	memory_init();

	sort_main_extable();

	irq_init();

	timekeeping_init();
	time_init();
	calibrate_delay();

	/*
	 * Set up the scheduler prior starting any interrupts,
	 * such as timer interrupt. Full topology setup happens
	 * at smp_init(), but meanwhile we still have a functioning
	 * scheduler:
	 */
	sched_init();

	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();

	if (WARN(!irqs_disabled(),
		 "Interrupts were enabled *very* early, fixing it\n"))
		local_irq_disable();


	init_timers();

	/*
	 * Boot all possible CPUs
	 */
	smp_prepare_cpus(setup_max_cpus);
	local_irq_enable();
	smp_init();

	/* STOP! WE ARE ALIVE NOW */
	rest_init();
}
