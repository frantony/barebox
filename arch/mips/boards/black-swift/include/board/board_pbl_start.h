/*
 * Copyright (C) 2013, 2015 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
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
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

#ifndef CONFIG_SYS_MIPS_CACHE_MODE
#define CONFIG_SYS_MIPS_CACHE_MODE CONF_CM_CACHABLE_NONCOHERENT
#endif

#define INDEX_BASE	CKSEG0

#define INDEX_STORE_TAG_I	0x08
#define INDEX_STORE_TAG_D	0x09

/*
 * Bits in the MIPS32/64 PRA coprocessor 0 config registers 1 and above.
 */
#define MIPS_CONF1_DA_SHIFT	7
#define MIPS_CONF1_DA		(_ULCAST_(7) <<  7)
#define MIPS_CONF1_DL_SHIFT	10
#define MIPS_CONF1_DL		(_ULCAST_(7) << 10)
#define MIPS_CONF1_DS_SHIFT	13
#define MIPS_CONF1_DS		(_ULCAST_(7) << 13)
#define MIPS_CONF1_IA_SHIFT	16

//#define CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD

	.macro	f_fill64 dst, offset, val
	LONG_S	\val, (\offset +  0 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  1 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  2 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  3 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  4 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  5 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  6 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  7 * LONGSIZE)(\dst)
#if LONGSIZE == 4
	LONG_S	\val, (\offset +  8 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  9 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 10 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 11 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 12 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 13 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 14 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 15 * LONGSIZE)(\dst)
#endif
	.endm

	.macro cache_loop	curr, end, line_sz, op
10:	cache		\op, 0(\curr)
	PTR_ADDU	\curr, \curr, \line_sz
	bne		\curr, \end, 10b
	.endm

	.macro	l1_info		sz, line_sz, off
	.set	push
	.set	noat

	mfc0	$1, CP0_CONFIG, 1

	/* detect line size */
	srl	\line_sz, $1, \off + MIPS_CONF1_DL_SHIFT - MIPS_CONF1_DA_SHIFT
	andi	\line_sz, \line_sz, (MIPS_CONF1_DL >> MIPS_CONF1_DL_SHIFT)
	move	\sz, zero
	beqz	\line_sz, 10f
	li	\sz, 2
	sllv	\line_sz, \sz, \line_sz

	/* detect associativity */
	srl	\sz, $1, \off + MIPS_CONF1_DA_SHIFT - MIPS_CONF1_DA_SHIFT
	andi	\sz, \sz, (MIPS_CONF1_DA >> MIPS_CONF1_DA_SHIFT)
	addi	\sz, \sz, 1

	/* sz *= line_sz */
	mul	\sz, \sz, \line_sz

	/* detect log32(sets) */
	srl	$1, $1, \off + MIPS_CONF1_DS_SHIFT - MIPS_CONF1_DA_SHIFT
	andi	$1, $1, (MIPS_CONF1_DS >> MIPS_CONF1_DS_SHIFT)
	addiu	$1, $1, 1
	andi	$1, $1, 0x7

	/* sz <<= log32(sets) */
	sllv	\sz, \sz, $1

	/* sz *= 32 */
	li	$1, 32
	mul	\sz, \sz, $1
10:
	.set	pop
	.endm
/*
 * mips_cache_reset - low level initialisation of the primary caches
 *
 * This routine initialises the primary caches to ensure that they have good
 * parity.  It must be called by the ROM before any cached locations are used
 * to prevent the possibility of data with bad parity being written to memory.
 *
 * To initialise the instruction cache it is essential that a source of data
 * with good parity is available. This routine will initialise an area of
 * memory starting at location zero to be used as a source of parity.
 *
 * RETURNS: N/A
 *
 */
	.macro	mips_cache_reset
#ifdef CONFIG_SYS_ICACHE_SIZE
	li	t2, CONFIG_SYS_ICACHE_SIZE
	li	t8, CONFIG_SYS_CACHELINE_SIZE
#else
	l1_info	t2, t8, MIPS_CONF1_IA_SHIFT
#endif

#ifdef CONFIG_SYS_DCACHE_SIZE
	li	t3, CONFIG_SYS_DCACHE_SIZE
	li	t9, CONFIG_SYS_CACHELINE_SIZE
#else
	l1_info	t3, t9, MIPS_CONF1_DA_SHIFT
#endif

#ifdef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD

	/* Determine the largest L1 cache size */
#if defined(CONFIG_SYS_ICACHE_SIZE) && defined(CONFIG_SYS_DCACHE_SIZE)
#if CONFIG_SYS_ICACHE_SIZE > CONFIG_SYS_DCACHE_SIZE
	li	v0, CONFIG_SYS_ICACHE_SIZE
#else
	li	v0, CONFIG_SYS_DCACHE_SIZE
#endif
#else
	move	v0, t2
	sltu	t1, t2, t3
	movn	v0, t3, t1
#endif
	/*
	 * Now clear that much memory starting from zero.
	 */
	PTR_LI		a0, CKSEG1
	PTR_ADDU	a1, a0, v0
2:	PTR_ADDIU	a0, 64
	f_fill64	a0, -64, zero
	bne		a0, a1, 2b

#endif /* CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD */

	/*
	 * The TagLo registers used depend upon the CPU implementation, but the
	 * architecture requires that it is safe for software to write to both
	 * TagLo selects 0 & 2 covering supported cases.
	 */
	mtc0		zero, CP0_TAGLO
	mtc0		zero, CP0_TAGLO, 2

	/*
	 * The caches are probably in an indeterminate state, so we force good
	 * parity into them by doing an invalidate for each line. If
	 * CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD is set then we'll proceed to
	 * perform a load/fill & a further invalidate for each line, assuming
	 * that the bottom of RAM (having just been cleared) will generate good
	 * parity for the cache.
	 */

	/*
	 * Initialize the I-cache first,
	 */
	blez		t2, 1f
	PTR_LI		t0, INDEX_BASE
	PTR_ADDU	t1, t0, t2
	/* clear tag to invalidate */
	cache_loop	t0, t1, t8, INDEX_STORE_TAG_I
#ifdef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD
	/* fill once, so data field parity is correct */
	PTR_LI		t0, INDEX_BASE
	cache_loop	t0, t1, t8, FILL
	/* invalidate again - prudent but not strictly neccessary */
	PTR_LI		t0, INDEX_BASE
	cache_loop	t0, t1, t8, INDEX_STORE_TAG_I
#endif

	/*
	 * then initialize D-cache.
	 */
1:	blez		t3, 3f
	PTR_LI		t0, INDEX_BASE
	PTR_ADDU	t1, t0, t3
	/* clear all tags */
	cache_loop	t0, t1, t9, INDEX_STORE_TAG_D
#ifdef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD
	/* load from each line (in cached space) */
	PTR_LI		t0, INDEX_BASE
2:	LONG_L		zero, 0(t0)
	PTR_ADDU	t0, t9
	bne		t0, t1, 2b
	/* clear all tags */
	PTR_LI		t0, INDEX_BASE
	cache_loop	t0, t1, t9, INDEX_STORE_TAG_D
#endif

3:	nop

	.endm

	.macro	dcache_enable
	mfc0	t0, CP0_CONFIG
	ori	t0, CONF_CM_CMASK
	xori	t0, CONF_CM_CMASK
	ori	t0, CONFIG_SYS_MIPS_CACHE_MODE
	mtc0	t0, CP0_CONFIG
	.endm

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	pbl_ar9331_pll
	pbl_ar9331_ddr2_config

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

skip_pll_ram_config:
	pbl_ar9331_uart_enable
	debug_ll_ar9331_init
	mips_nmon

	copy_to_link_location	pbl_start

	.set	pop
	.endm
