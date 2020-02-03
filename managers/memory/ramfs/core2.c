/*i  
 *   Copyright (c) 2016-2019 Wuklab, Purdue University. All rights reserved.
 *   
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */

/*
 * this file is the same as core.c, while it's initialized to read the code that
 * is statically linked with haolanpiggy.S, so we can have different ramfs opera * tor to different pcomponent request, and run different user space program in  * 2P
 */
#include <lego/mm.h>
#include <lego/kernel.h>
#include <lego/comp_memory.h>
#include <lego/comp_storage.h>
#include <lego/fit_ibapi.h>

#include <memory/vm.h>
#include <memory/file_ops.h>

extern char __ramfs_start2[], __ramfs_end2[];

static ssize_t ramfs_read2(struct lego_task_struct *tsk, struct lego_file *file,
			  char *buf, size_t count, loff_t *pos)
{
	char *start;

	start = __ramfs_start2 + *pos;
	memcpy(buf, start, count);
	*pos += count;

	return count;
}

static ssize_t ramfs_write2(struct lego_task_struct *tsk, struct lego_file *file,
			   const char *buf, size_t count, loff_t *pos)
{
	return -EINVAL;
}

static int ramfs_vma_fault2(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct lego_task_struct *tsk;
	struct lego_file *file;
	size_t count;
	loff_t pos;
	unsigned long page;

	page = __get_free_page(GFP_KERNEL);
	if (unlikely(!page))
		return VM_FAULT_OOM;

	tsk = vma->vm_mm->task;
	file = vma->vm_file;
	count = PAGE_SIZE;
	pos = vmf->pgoff << PAGE_SHIFT;

	ramfs_read2(tsk, file, (char *)page, count, &pos);

	vmf->page = page;

	return 0;
}

static struct vm_operations_struct ramfs_vma_ops2 = {
	.fault	= ramfs_vma_fault2,
};

static int ramfs_mmap2(struct lego_task_struct *tsk, struct lego_file *file,
		      struct vm_area_struct *vma)
{
	vma->vm_ops = &ramfs_vma_ops2;
	return 0;
}

struct lego_file_operations ramfs_file_ops2 = {
	.read	= ramfs_read2,
	.write	= ramfs_write2,
	.mmap	= ramfs_mmap2,
};
