/*
 * Startup Code for MIPS CPU
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <asm/pbl_macros.h>

#define GT_PCI0IOLD_OFS		0x048
#define GT_PCI0IOHD_OFS		0x050
#define GT_PCI0M0LD_OFS		0x058
#define GT_PCI0M0HD_OFS		0x060
#define GT_ISD_OFS		0x068

#define GT_PCI0M1LD_OFS		0x080
#define GT_PCI0M1HD_OFS		0x088

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	b       __start
	 nop

	/*
	   MIPS_REVISION_REG located at 0x1fc00010
	   see the MIPS_REVISION_CORID macro in linux kernel sources
	   set up it to 0x420 (Malta Board with CoreLV) as qemu does
	*/
	.org    0x10
	.word   0x00000420

	.align 4
__start:

	mips_disable_interrupts

	/*
	 * Load BAR registers of GT64120 as done by YAMON
	 *
	 * based on write_bootloader() in qemu.git/hw/mips_malta.c
	 * see GT64120 manual and qemu.git/hw/gt64xxx.c for details
	 *
	 * This is big-endian version of code!
	 */

	/* move GT64120 registers to 0x1be00000 */
	li	t1, 0xb4000000
	li	t0, 0xdf000000
	sw	t0, GT_ISD_OFS(t1)

	/* setup MEM-to-PCI0 mapping */
	li	t1, 0xbbe00000

	/* setup PCI0 io window to 0x18000000-0x181fffff */
	li	t0, 0xc0000000
	sw	t0, GT_PCI0IOLD_OFS(t1)
	li	t0, 0x40000000
	sw	t0, GT_PCI0IOHD_OFS(t1)

	/* setup PCI0 mem windows */
	li	t0, 0x80000000
	sw	t0, GT_PCI0M0LD_OFS(t1)
	li	t0, 0x3f000000
	sw	t0, GT_PCI0M0HD_OFS(t1)
	li	t0, 0xc1000000
	sw	t0, GT_PCI0M1LD_OFS(t1)
	li	t0, 0x5e000000
	sw	t0, GT_PCI0M1HD_OFS(t1)

	/* cpu specific setup ... */
	/* ... absent */

	copy_to_link_location	pbl_start

	.set	pop
	.endm
