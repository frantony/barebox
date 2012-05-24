/*
 * Lowlevel init code for MIPS malta
 *
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>

	.section ".text_bare_init"

LEAF(mach_init_lowlevel)

	/*
	 * Load BAR registers of GT64120 as done by YAMON
	 *
	 * based on write_bootloader() in qemu.git/hw/mips_malta.c
         * see GT64120 manual and qemu.git/hw/gt64xxx.c for details
	 *
	 * This is big-endian version of code!
	 */

	/* move GT64120 registers to 0x1be00000 */
	lui	t1, 0xb400
	lui	t0, 0xdf00
	sw	t0, 104(t1)

	/* setup MEM-to-PCI0 mapping */
	lui	t1, 0xbbe0

	/* setup PCI0 io window to 18000000-181fffff */
	lui	t0, 0xc000
	sw	t0, 72(t1) /* GT_PCI0IOLD */
	lui	t0, 0x4000
	sw	t0, 80(t1) /* GT_PCI0IOHD */

	/* setup PCI0 mem windows */
	lui	t0, 0x8000
	sw	t0, 88(t1) /* GT_PCI0M0LD */
	lui	t0, 0x3f00
	sw	t0, 96(t1) /* GT_PCI0M0HD */
	lui	t0, 0xc100
	sw	t0, 128(t1) /* GT_PCI0M1LD */
	lui	t0, 0x5e00
	sw	t0, 136(t1) /* GT_PCI0M1HD */

	/* make KSEG0 uncached */
	li	t0, CONF_CM_UNCACHED
	mtc0	t0, CP0_CONFIG

	/* return */
	b	mach_init_lowlevel_return
	 nop
END(mach_init_lowlevel)
