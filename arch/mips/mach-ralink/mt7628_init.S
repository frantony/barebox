/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * Author:  Weijie Gao <weijie.gao@mediatek.com>
 */

#include <config.h>
#include <asm/cacheops.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/asm.h>
#include <mach/mt7628.h>

/* FIXME */
#define CONFIG_SYS_ICACHE_SIZE		(64 << 10)
#define CONFIG_SYS_ICACHE_LINE_SIZE	32
#define CONFIG_SYS_DCACHE_SIZE		(32 << 10)
#define	CONFIG_SYS_DCACHE_LINE_SIZE	32

#ifndef MIPS_CONF_IMPL
# define MIPS_CONF_IMPL		(_ULCAST_(0x1ff) << 16)
#endif

#define SYS_INIT_SP_OFFSET	0x400000

#define CONFIG_SYS_INIT_SP_ADDR	(KSEG0ADDR(SYS_INIT_SP_OFFSET))

#define CACHE_STACK_SIZE	0x4000
#define CACHE_STACK_BASE	(CONFIG_SYS_INIT_SP_ADDR - CACHE_STACK_SIZE)

#define DELAY_USEC(us)		((58 * (us)) / 3)

	.macro setup_stack
	li	t0, -16
	PTR_LI	t1, CONFIG_SYS_INIT_SP_ADDR
	and	sp, t1, t0		# force 16 byte alignment
	move	fp, sp
	.endm


	.set noreorder

LEAF(mt7628_sram_init)
	/* Setup CPU PLL */
	li	t0, DELAY_USEC(1000000)
	li	t1, KSEG1ADDR(SYSCTL_BASE + SYSCTL_ROM_STATUS_REG)
	li	t2, KSEG1ADDR(SYSCTL_BASE + SYSCTL_CLKCFG0_REG)

_check_rom_status:
	lw	t3, 0(t1)
	andi	t3, t3, 1
	bnez	t3, _rom_normal
	subu	t0, t0, 1
	bnez	t0, _check_rom_status
	 nop

	lw	t3, 0(t2)
	ori	t3, (CPU_PLL_FROM_BBP | CPU_PLL_FROM_XTAL)
	xori	t3, CPU_PLL_FROM_BBP
	b	_cpu_pll_done
	 nop

_rom_normal:
	lw	t3, 0(t2)
	ori	t3, (CPU_PLL_FROM_BBP | CPU_PLL_FROM_XTAL | \
		    DIS_BBP_SLEEP | EN_BBP_CLK)
	xori	t3, (CPU_PLL_FROM_BBP | CPU_PLL_FROM_XTAL)

_cpu_pll_done:
	sw	t3, 0(t2)

	li	t2, KSEG1ADDR(RBUSCTL_BASE + RBUSCTL_DYN_CFG0_REG)
	lw	t3, 0(t2)
	ori	t3, t3, (CPU_FDIV_M | CPU_FFRAC_M)
	xori	t3, t3, (CPU_FDIV_M | CPU_FFRAC_M)
	ori	t3, t3, ((1 << CPU_FDIV_S) | (1 << CPU_FFRAC_S))
	sw	t3, 0(t2)

	/* Clear WST & SPR bits in ErrCtl */
	mfc0	t0, CP0_ECC
	ins	t0, zero, 30, 2
	mtc0	t0, CP0_ECC
	ehb

	/* Simply initialize I-Cache */
	li	a0, 0
	li	a1, CONFIG_SYS_ICACHE_SIZE

	mtc0	zero, CP0_TAGLO		/* Zero to DDataLo */

1:	cache	Index_Store_Tag_I, 0(a0)
	addiu	a0, CONFIG_SYS_ICACHE_LINE_SIZE
	bne	a0, a1, 1b
	 nop

	/* Simply initialize D-Cache */
	li	a0, 0
	li	a1, CONFIG_SYS_DCACHE_SIZE

	mtc0	zero, CP0_TAGLO, 2

2:	cache	Index_Store_Tag_D, 0(a0)
	addiu	a0, CONFIG_SYS_DCACHE_LINE_SIZE
	bne	a0, a1, 2b
	 nop

	/* Set KSEG0 Cachable */
	mfc0	t0, CP0_CONFIG
	and	t0, t0, MIPS_CONF_IMPL
	or	t0, t0, CONF_CM_CACHABLE_NONCOHERENT
	mtc0	t0, CP0_CONFIG
	ehb

	/* Lock D-Cache */
	PTR_LI	a0, CACHE_STACK_BASE		/* D-Cache lock base */
	li	a1, CACHE_STACK_SIZE		/* D-Cache lock size */
	li	a2, 0x1ffff800			/* Mask of DTagLo[PTagLo] */

3:
	/* Lock one cacheline */
	and	t0, a0, a2
	ori	t0, 0xe0			/* Valid & Dirty & Lock bits */
	mtc0	t0, CP0_TAGLO, 2		/* Write to DTagLo */
	ehb
	cache	Index_Store_Tag_D, 0(a0)

	addiu	a0, CONFIG_SYS_DCACHE_LINE_SIZE
	sub	a1, CONFIG_SYS_DCACHE_LINE_SIZE
	bnez	a1, 3b
	 nop

	setup_stack

	jr	ra
	 nop
	END(mt7628_sram_init)

NESTED(mt7628_lowlevel_init, 0, ra)
	/* Save ra and do real lowlevel initialization */
	move	s0, ra

	PTR_LA	t9, mt7628_ddr_init
	jalr	t9
	 nop

	move	ra, s0

	/* Write back data in locked cache to DRAM */
	PTR_LI	a0, CACHE_STACK_BASE		/* D-Cache unlock base */
	li	a1, CACHE_STACK_SIZE		/* D-Cache unlock size */

1:
	cache	Hit_Writeback_Inv_D, 0(a0)
	addiu	a0, CONFIG_SYS_DCACHE_LINE_SIZE
	sub	a1, CONFIG_SYS_DCACHE_LINE_SIZE
	bnez	a1, 1b
	 nop

	/* Set KSEG0 Uncached */
	mfc0	t0, CP0_CONFIG
	and	t0, t0, MIPS_CONF_IMPL
	or	t0, t0, CONF_CM_UNCACHED
	mtc0	t0, CP0_CONFIG
	ehb

	jr	ra
	 nop
	END(mt7628_lowlevel_init)
