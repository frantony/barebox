/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 * Used code copyrighted (C) 2009 by Shinya Kuribayashi <skuribay@pobox.com>
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

#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm-generic/memory_layout.h>
#include <mach/jz4770_regs.h>

	.set noreorder
	.text
	.section ".flash_header_start"
	.align	4

	.org	0x0
	.byte	0x0

	/* skip MBR */
	.align	4
	.org	0x200

EXPORT(_start_flash)

#define JZ_MSCBOOT_CFG       0x4d53504c
	.word	JZ_MSCBOOT_CFG

	/*
	 * Disable all interrupts
	 */
	/* Hmmm, for this register reset value is 0xFFFFFFFF */
	/* FIXME: we set IE=0 in CP0_STATUS, so ... */
	la	t0, INTC_IMR(0)
	li	t1, 0xffffffff
	sw	t1, 0(t0)

	/*
	 * CU0=UM=EXL=IE=0, BEV=ERL=1, IP2~7=1
	 */
	li	k0, 0x0040FC04
	mtc0	k0, CP0_STATUS

	/* IV=1, use the specical interrupt vector (0x200) */
	/* FIXME: we set IE=0 in CP0_STATUS, so ... */
	li	k0, CAUSEF_IV
	mtc0	k0, CP0_CAUSE

	/* Setup stack pointer (use cache!) and jump to C routine */
	la	sp, 0x80004000
	b	spl_main
	 nop

#if 0
	/* RAM loadable entry point */

	/* Skip MBR and SPL */
	.org	0x2200
#if (STACK_BASE + STACK_SIZE) % 16 != 0
#error stack pointer must be 16-byte-aligned
#endif

stack_setup:
	la	sp, STACK_BASE + STACK_SIZE

	/* reserve four 32-bit argument slots */
	addiu	sp, -16

	la	v0, main_entry
	jal     v0
	 nop
#endif

	/* No return */

__error1:
	b	__error1
	 nop
