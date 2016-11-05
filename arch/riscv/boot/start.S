/*
 * Startup Code for MIPS CPU
 *
 * based on coreboot/src/arch/riscv/bootblock.S
 *
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
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
 */

#include <asm-generic/memory_layout.h>

	.text
	.section ".text_bare_init"
	.align 2

.globl _start
_start:
	li sp, STACK_BASE + STACK_SIZE

	# make room for HLS and initialize it
	addi sp, sp, -64 /* MENTRY_FRAME_SIZE */

	# poison the stack
	li t1, STACK_BASE
	li t0, 0xdeadbeef
	sw t0, 0(t1)

	# clear any pending interrupts
	csrwi mip, 0

	tail main_entry
