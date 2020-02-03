/*
 * Copyright (c) 2016-2019 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* configure the user space program you want to run in another pComponent here

 */

#include <lego/linkage.h>

	.section ".ramfshaolan","ax"
GLOBAL(ramfs_start2)
#ifdef CONFIG_USE_RAMFS
.incbin "usr/pipe.o"
#endif
GLOBAL(ramfs_end2)
