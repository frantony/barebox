// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>

/* Reset Control */
#define SYSC_REG_RESET_CTRL	0x034

#define RSTCTL_RESET_SYSTEM	BIT(0)

static void __noreturn ralink_restart_soc(struct restart_handler *rst)
{
	/* rt_sysc_membase = plat_of_remap_node("ralink,mt7620a-sysc"); */
	void __iomem *rt_sysc_membase = (void *)0xb0000000;

	__raw_writel(RSTCTL_RESET_SYSTEM, rt_sysc_membase + SYSC_REG_RESET_CTRL);

	hang();
	/* NOTREACHED */
}

static int restart_register_feature(void)
{
	restart_handler_register_fn(ralink_restart_soc);

	return 0;
}
coredevice_initcall(restart_register_feature);
