/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * based on relocate_kernel.S for kexec
 * Created by <nschichan@corp.free.fr> on Thu Oct 12 17:49:57 2006
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

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>

LEAF(relocate_new_kernel)
	.set	push
	.set	reorder
	PTR_L		s0, kexec_segments
	PTR_L		s1, kexec_nr_segments
	PTR_L		s2, kexec_start_address

process_segment:
	PTR_L		s4, (s0) /* buf */
	PTR_L		s5, SZREG (s0) /* bufsz */
	PTR_L		s6, 2*SZREG (s0) /* mem */

copy_segment:
	/* copy segment word by word */
	REG_L		s7, (s4)
	REG_S		s7, (s6)
	PTR_ADD		s4, s4, SZREG
	PTR_ADD		s6, s6, SZREG
	LONG_SUB	s5, s5, 1
	bne		s5, zero, copy_segment

	LONG_SUB	s1, s1, 1
	beq		s1, zero, done

	PTR_ADD		s0, s0, 4*SZREG

	b		process_segment

done:
	/* jump to kexec_start_address */
	j		s2
	END(relocate_new_kernel)

kexec_start_address:
	EXPORT(kexec_start_address)
	PTR		0x0
	.size		kexec_start_address, PTRSIZE

kexec_segments:
	EXPORT(kexec_segments)
	PTR		0x0
	.size		kexec_segments, PTRSIZE

kexec_nr_segments:
	EXPORT(kexec_nr_segments)
	PTR		0x0
	.size		kexec_nr_segments, PTRSIZE

relocate_new_kernel_end:

relocate_new_kernel_size:
	EXPORT(relocate_new_kernel_size)
	PTR		relocate_new_kernel_end - relocate_new_kernel
	.size		relocate_new_kernel_size, PTRSIZE
	.set	pop
