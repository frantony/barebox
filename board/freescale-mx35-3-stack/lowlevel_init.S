/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <mach/imx-regs.h>
#include "board-mx35_3stack.h"

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,	[r0];

#define writeb(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	strb		r1,	[r0];

#define IIM_SREV		0x24
#define ROMPATCH_REV		0x40

#define CHIP_REV_1_0		0x10
#define CHIP_REV_2_0		0x20

#define BOARD_REV_1_0		0x0
#define BOARD_REV_2_0		0x1

#define CLKMODE_AUTO            0
#define CLKMODE_CONSUMER        1

#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)

#define L2CC_BASE_ADDR		0x30000000
#define ROMPATCH_BASE_ADDR      0x60000000

#define CSD0_BASE_ADDR		0x80000000
#define CSD1_BASE_ADDR		0x90000000

#define NFC_BASE_ADDR		0xBB000000
#define NFC_BUF_SIZE		0x1000


/* L210 */
#define L2CC_BASE_ADDR                  0x30000000
#define L2_CACHE_LINE_SIZE              32
#define L2_CACHE_CTL_REG                0x100
#define L2_CACHE_AUX_CTL_REG            0x104
#define L2_CACHE_SYNC_REG               0x730
#define L2_CACHE_INV_LINE_REG           0x770
#define L2_CACHE_INV_WAY_REG            0x77C
#define L2_CACHE_CLEAN_LINE_REG         0x7B0
#define L2_CACHE_CLEAN_INV_LINE_REG     0x7F0
#define L2_CACHE_DBG_CTL_REG            0xF40

/*
 * AIPS 1
 */
#define AIPS1_BASE_ADDR         0x43F00000
#define AIPS1_CTRL_BASE_ADDR    AIPS1_BASE_ADDR
#define MAX_BASE_ADDR           0x43F04000
#define EVTMON_BASE_ADDR        0x43F08000
#define CLKCTL_BASE_ADDR        0x43F0C000
#define I2C_BASE_ADDR           0x43F80000
#define I2C3_BASE_ADDR          0x43F84000
#define ATA_BASE_ADDR           0x43F8C000
#define UART1_BASE_ADDR         0x43F90000
#define UART2_BASE_ADDR         0x43F94000
#define I2C2_BASE_ADDR          0x43F98000
#define CSPI1_BASE_ADDR         0x43FA4000
#define IOMUXC_BASE_ADDR        0x43FAC000

/*
 * AIPS 2
 */
#define AIPS2_BASE_ADDR         0x53F00000
#define AIPS2_CTRL_BASE_ADDR    AIPS2_BASE_ADDR
#define CCM_BASE_ADDR           0x53F80000
#define GPT1_BASE_ADDR          0x53F90000
#define EPIT1_BASE_ADDR         0x53F94000
#define EPIT2_BASE_ADDR         0x53F98000
#define GPIO3_BASE_ADDR         0x53FA4000
#define MMC_SDHC1_BASE_ADDR 	0x53FB4000
#define MMC_SDHC2_BASE_ADDR 	0x53FB8000
#define MMC_SDHC3_BASE_ADDR 	0x53FBC000
#define IPU_CTRL_BASE_ADDR      0x53FC0000
#define GPIO3_BASE_ADDR         0x53FA4000
#define GPIO1_BASE_ADDR         0x53FCC000
#define GPIO2_BASE_ADDR         0x53FD0000
#define SDMA_BASE_ADDR          0x53FD4000
#define RTC_BASE_ADDR           0x53FD8000
#define WDOG_BASE_ADDR          0x53FDC000
#define PWM_BASE_ADDR           0x53FE0000
#define RTIC_BASE_ADDR          0x53FEC000
#define IIM_BASE_ADDR           0x53FF0000

/* CCM */
#define CLKCTL_CCMR                     0x00
#define CLKCTL_PDR0                     0x04
#define CLKCTL_PDR1                     0x08
#define CLKCTL_PDR2                     0x0C
#define CLKCTL_PDR3                     0x10
#define CLKCTL_PDR4                     0x14
#define CLKCTL_RCSR                     0x18
#define CLKCTL_MPCTL                    0x1C
#define CLKCTL_PPCTL                    0x20
#define CLKCTL_ACMR                     0x24
#define CLKCTL_COSR                     0x28
#define CLKCTL_CGR0                     0x2C
#define CLKCTL_CGR1                     0x30
#define CLKCTL_CGR2                     0x34
#define CLKCTL_CGR3                     0x38

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define EXT_MEM_CTRL_BASE       0xB8000000
#define ESDCTL_BASE_ADDR        0xB8001000
#define WEIM_BASE_ADDR          0xB8002000
#define WEIM_CTRL_CS0           WEIM_BASE_ADDR
#define WEIM_CTRL_CS1           (WEIM_BASE_ADDR + 0x10)
#define WEIM_CTRL_CS2           (WEIM_BASE_ADDR + 0x20)
#define WEIM_CTRL_CS3           (WEIM_BASE_ADDR + 0x30)
#define WEIM_CTRL_CS4           (WEIM_BASE_ADDR + 0x40)
#define WEIM_CTRL_CS5           (WEIM_BASE_ADDR + 0x50)
#define M3IF_BASE_ADDR		0xB8003000
#define EMI_BASE_ADDR		0xB8004000


/*
 * return soc version
 * 	0x10:  TO1
 *	0x20:  TO2
 *	0x30:  TO3
 */
.macro check_soc_version ret, tmp
	ldr \tmp, =IIM_BASE_ADDR
	ldr \ret, [\tmp, #IIM_SREV]
	cmp \ret, #0x00
	moveq \tmp, #ROMPATCH_REV
	ldreq \ret, [\tmp]
	moveq \ret, \ret, lsl #4
	addne \ret, \ret, #0x10
.endm

/*
 * L2CC Cache setup/invalidation/disable
 */
.macro init_l2cc
    /* Disable L2 cache first */
    mov r0, #L2CC_BASE_ADDR
    ldr r1, [r0, #L2_CACHE_CTL_REG]
    bic r1, r1, #0x1
    str r1, [r0, #L2_CACHE_CTL_REG]

    /*
     * Configure L2 Cache:
     * - 128k size(16k way)
     * - 8-way associativity
     * - 0 ws TAG/VALID/DIRTY
     * - 4 ws DATA R/W
     */
    ldr r1, [r0, #L2_CACHE_AUX_CTL_REG]
    and r1, r1, #0xFE000000
    ldr r2, =L2CC_AUX_CTL_CONFIG
    orr r1, r1, r2
    str r1, [r0, #L2_CACHE_AUX_CTL_REG]

    /* Workaournd for TO1 DDR issue:WT*/
    check_soc_version r1, r2
    cmp r1, #CHIP_REV_2_0
    ldrlo r1, [r0, #L2_CACHE_DBG_CTL_REG]
    orrlo r1, r1, #2
    strlo r1, [r0, #L2_CACHE_DBG_CTL_REG]

   /* Invalidate L2 */
    mov r1, #0x000000FF
    str r1, [r0, #L2_CACHE_INV_WAY_REG]
1:
    /* Poll Invalidate By Way register */
    ldr r2, [r0, #L2_CACHE_INV_WAY_REG]
    cmp r2, #0
    bne 1b
.endm /* init_l2cc */

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr r0, =AIPS1_BASE_ADDR
	ldr r1, =AIPS_MPR_CONFIG
	str r1, [r0, #0x00]
	str r1, [r0, #0x04]
	ldr r0, =AIPS2_BASE_ADDR
	str r1, [r0, #0x00]
	str r1, [r0, #0x04]

	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
	ldr r0, =AIPS1_BASE_ADDR
	ldr r1, =AIPS_OPACR_CONFIG
	str r1, [r0, #0x40]
	str r1, [r0, #0x44]
	str r1, [r0, #0x48]
	str r1, [r0, #0x4C]
	str r1, [r0, #0x50]
	ldr r0, =AIPS2_BASE_ADDR
	str r1, [r0, #0x40]
	str r1, [r0, #0x44]
	str r1, [r0, #0x48]
	str r1, [r0, #0x4C]
	str r1, [r0, #0x50]
.endm /* init_aips */

/* MAX (Multi-Layer AHB Crossbar Switch) setup */
.macro init_max
	ldr r0, =MAX_BASE_ADDR
	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
	ldr r1, =MAX_MPR_CONFIG
	str r1, [r0, #0x000]        /* for S0 */
	str r1, [r0, #0x100]        /* for S1 */
	str r1, [r0, #0x200]        /* for S2 */
	str r1, [r0, #0x300]        /* for S3 */
	str r1, [r0, #0x400]        /* for S4 */
	/* SGPCR - always park on last master */
	ldr r1, =MAX_SGPCR_CONFIG
	str r1, [r0, #0x010]        /* for S0 */
	str r1, [r0, #0x110]        /* for S1 */
	str r1, [r0, #0x210]        /* for S2 */
	str r1, [r0, #0x310]        /* for S3 */
	str r1, [r0, #0x410]        /* for S4 */
	/* MGPCR - restore default values */
	ldr r1, =MAX_MGPCR_CONFIG
	str r1, [r0, #0x800]        /* for M0 */
	str r1, [r0, #0x900]        /* for M1 */
	str r1, [r0, #0xA00]        /* for M2 */
	str r1, [r0, #0xB00]        /* for M3 */
	str r1, [r0, #0xC00]        /* for M4 */
	str r1, [r0, #0xD00]        /* for M5 */
.endm /* init_max */

/* M3IF setup */
.macro init_m3if
	/* Configure M3IF registers */
	ldr r1, =M3IF_BASE_ADDR
	/*
	* M3IF Control Register (M3IFCTL)
	* MRRP[0] = L2CC0 not on priority list (0 << 0)	= 0x00000000
	* MRRP[1] = L2CC1 not on priority list (0 << 0)	= 0x00000000
	* MRRP[2] = MBX not on priority list (0 << 0)	= 0x00000000
	* MRRP[3] = MAX1 not on priority list (0 << 0)	= 0x00000000
	* MRRP[4] = SDMA not on priority list (0 << 0)	= 0x00000000
	* MRRP[5] = MPEG4 not on priority list (0 << 0)	= 0x00000000
	* MRRP[6] = IPU1 on priority list (1 << 6)	= 0x00000040
	* MRRP[7] = IPU2 not on priority list (0 << 0)	= 0x00000000
	*						------------
	*						  0x00000040
	*/
	ldr r0, =M3IF_CONFIG
	str r0, [r1]  /* M3IF control reg */
.endm /* init_m3if */

/* To support 133MHz DDR */
.macro  init_drive_strength
/*
	mov r0, #0x2
	ldr r1, =IOMUXC_BASE_ADDR
	add r1, r1, #0x368
        add r2, r1, #0x4C8 - 0x368
1:      str r0, [r1], #4
	cmp r1, r2
        ble 1b
*/
.endm /* init_drive_strength */

/* CPLD on CS5 setup */
.macro init_debug_board
	ldr r0, =DBG_BASE_ADDR
	ldr r1, =DBG_CSCR_U_CONFIG
	str r1, [r0, #0x00]
	ldr r1, =DBG_CSCR_L_CONFIG
	str r1, [r0, #0x04]
	ldr r1, =DBG_CSCR_A_CONFIG
	str r1, [r0, #0x08]
.endm /* init_debug_board */

/* clock setup */
.macro init_clock
	ldr r0, =CCM_BASE_ADDR

        /* default CLKO to 1/32 of the ARM core*/
        ldr r1, [r0, #CLKCTL_COSR]
        bic r1, r1, #0x00000FF00
        bic r1, r1, #0x0000000FF
        mov r2, #0x00006C00
        add r2, r2, #0x67
        orr r1, r1, r2
        str r1, [r0, #CLKCTL_COSR]

        ldr r2, =CCM_CCMR_CONFIG
        str r2, [r0, #CLKCTL_CCMR]

    	check_soc_version r1, r2
    	cmp r1, #CHIP_REV_2_0
	ldrhs r3, =CCM_MPLL_399_HZ
	bhs 1f
	ldr r2, [r0, #CLKCTL_PDR0]
        tst r2, #CLKMODE_CONSUMER
        ldrne r3, =CCM_MPLL_532_HZ  /* consumer path*/
        ldreq r3, =CCM_MPLL_399_HZ  /* auto path*/
1:
	str r3, [r0, #CLKCTL_MPCTL]

        ldr r1, =CCM_PPLL_300_HZ
        str r1, [r0, #CLKCTL_PPCTL]

        ldr r1, [r0, #CLKCTL_PDR0]
        orr r1, r1, #0x800000
        str r1, [r0, #CLKCTL_PDR0]

        ldr r1, =CCM_PDR0_CONFIG
        str r1, [r0, #CLKCTL_PDR0]

        ldr r1, [r0, #CLKCTL_CGR0]
        orr r1, r1, #0x00300000
        str r1, [r0, #CLKCTL_CGR0]

        ldr r1, [r0, #CLKCTL_CGR1]
        orr r1, r1, #0x00000C00
        orr r1, r1, #0x00000003
        str r1, [r0, #CLKCTL_CGR1]
.endm /* init_clock */

.macro init_sdram
        writel(0x0000d843, 0xB8002050)
        writel(0x22252521, 0xB8002054)
        writel(0x22220a00, 0xB8002058)
        writel(0x00000304, 0xB8001010)
        writel(0x0000030C, 0xB8001010)
        writel(0x007ffc3f, 0xB8001004)
        writel(0x92220000, 0xB8001000)
        writel(0x12345678, 0x80000400)
        writel(0xA2220000, 0xB8001000)
        writel(0x87654321, 0x80000000)
        writel(0x87654321, 0x80000000)
        writel(0xB2220000, 0xB8001000)
        writeb(0xda, 0x80000233)
        writeb(0xda, 0x82000780)
        writeb(0xda, 0x82000400)
        writel(0x82226080, 0xB8001000)
        writel(0x007ffc3f, 0xB8001004)
        writel(0x007ffc3f, 0xB800100C)
        writel(0x00000304, 0xB8001010)
        writel(0x00002000, 0xB8001008)
.endm /* init_sdram */
	
.macro setup_sdram
        ldr r0, =ESDCTL_BASE_ADDR
        mov r3, #0x2000
        str r3, [r0, #0x0]
        str r3, [r0, #0x8]

	/*ip(r12) has used to save lr register in upper calling*/
        mov fp, lr

	mov r5, #0x00
        mov r2, #0x00
        mov r1, #CSD0_BASE_ADDR
        bl setup_sdram_bank
        cmp r3, #0x0
        orreq r5, r5, #1
        eorne r2, r2, #0x1
        blne setup_sdram_bank

        mov lr, fp

	check_soc_version r3, r4
	cmp r1, #CHIP_REV_2_0
	bhs 1f
	cmp r5, #0
        movne r3, #L2CC_BASE_ADDR
        ldrne r4, [r3, #L2_CACHE_AUX_CTL_REG]
        orrne r4, r4, #0x1000
        strne r4, [r3, #L2_CACHE_AUX_CTL_REG]
1:
        ldr r3, =ESDCTL_DELAY_LINE5
        str r3, [r0, #0x30]
.endm /* setup_sdram */

.section ".text.init", "x"

.globl board_init_lowlevel
board_init_lowlevel:
	mov     r10, lr

	/* Platform CHIP level init*/
#ifdef TURN_OFF_IMPRECISE_ABORT
	mrs r0, cpsr
	bic r0, r0, #0x100
	msr cpsr, r0
#endif

	mrc 15, 0, r1, c1, c0, 0

#ifndef BRANCH_PREDICTION_ENABLE
    	mrc 15, 0, r0, c1, c0, 1
    	bic r0, r0, #7
    	mcr 15, 0, r0, c1, c0, 1
#else
    	mrc 15, 0, r0, c1, c0, 1
    	orr r0, r0, #7
    	mcr 15, 0, r0, c1, c0, 1
    	orr r1, r1, #(1<<11)
#endif

#ifdef UNALIGNED_ACCESS_ENABLE
    	orr r1, r1, #(1<<22)
#endif

#ifdef LOW_INT_LATENCY_ENABLE
    	orr r1, r1, #(1<<21)
#endif
    	mcr 15, 0, r1, c1, c0, 0

    	mov r0, #0
#ifdef BRANCH_PREDICTION_ENABLE
   	mcr 15, 0, r0, c15, c2, 4
#endif
	mcr 15, 0, r0, c7, c10, 4       /* Drain the write buffer */

	/* initializes very early AIPS, what for?
	 * Then it also initializes Multi-Layer AHB Crossbar Switch,
	 * M3IF */
	/* Also setup the Peripheral Port Remap register inside the core */
	ldr r0, =0x40000015        /* start from AIPS 2GB region */
	mcr p15, 0, r0, c15, c2, 4

	init_l2cc

	init_aips

	init_max

	init_m3if

	init_drive_strength

	cmp pc, #PHYS_SDRAM_1
	blo init_clock_start
	cmp pc, #(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE)
	blo skip_sdram_setup

init_clock_start:
	init_clock
	init_debug_board
	init_sdram
	setup_sdram

ret:
	mov     pc,r10

skip_sdram_setup:
	mov r0, #NFC_BASE_ADDR
	add r1, r0, #NFC_BUF_SIZE
	cmp pc, r0
	movlo pc, lr
	cmp pc, r1
	movhi pc, lr
	/* return from mxc_nand_load */
	/* r12 saved upper lr*/
	//b mxc_nand_load

/*
 * r0: ESDCTL control base, r1: sdram slot base
 * r2: DDR type(0:DDR2, 1:MDDR) r3, r4:working base
 */
setup_sdram_bank:
        mov r3, #0xE /*0xA + 0x4*/
        tst r2, #0x1
        orreq r3, r3, #0x300 /*DDR2*/
        str r3, [r0, #0x10]
        bic r3, r3, #0x00A
        str r3, [r0, #0x10]
        beq 2f

        mov r3, #0x20000
1:      subs r3, r3, #1
        bne 1b

2:      tst r2, #0x1
        ldreq r3, =ESDCTL_DDR2_CONFIG
        ldrne r3, =ESDCTL_MDDR_CONFIG
        cmp r1, #CSD1_BASE_ADDR
        strlo r3, [r0, #0x4]
        strhs r3, [r0, #0xC]

        ldr r3, =ESDCTL_0x92220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
        mov r3, #0xDA
        ldr r4, =ESDCTL_PRECHARGE
        strb r3, [r1, r4]

        tst r2, #0x1
        bne skip_set_mode

        cmp r1, #CSD1_BASE_ADDR
        ldr r3, =ESDCTL_0xB2220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
        mov r3, #0xDA
        ldr r4, =ESDCTL_DDR2_EMR2
        strb r3, [r1, r4]
        ldr r4, =ESDCTL_DDR2_EMR3
        strb r3, [r1, r4]
        ldr r4, =ESDCTL_DDR2_EN_DLL
        strb r3, [r1, r4]
        ldr r4, =ESDCTL_DDR2_RESET_DLL
        strb r3, [r1, r4]

        ldr r3, =ESDCTL_0x92220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
        mov r3, #0xDA
        ldr r4, =ESDCTL_PRECHARGE
        strb r3, [r1, r4]

skip_set_mode:
        cmp r1, #CSD1_BASE_ADDR
        ldr r3, =ESDCTL_0xA2220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
        mov r3, #0xDA
        strb r3, [r1]
        strb r3, [r1]

        ldr r3, =ESDCTL_0xB2220000
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]
        tst r2, #0x1
        ldreq r4, =ESDCTL_DDR2_MR
        ldrne r4, =ESDCTL_MDDR_MR
       	mov r3, #0xDA
        strb r3, [r1, r4]
        ldreq r4, =ESDCTL_DDR2_OCD_DEFAULT
        streqb r3, [r1, r4]
        ldreq r4, =ESDCTL_DDR2_EN_DLL
        ldrne r4, =ESDCTL_MDDR_EMR
        strb r3, [r1, r4]

        cmp r1, #CSD1_BASE_ADDR
        ldr r3, =ESDCTL_0x82228080
        strlo r3, [r0, #0x0]
        strhs r3, [r0, #0x8]

        tst r2, #0x1
        moveq r4, #0x20000
        movne r4, #0x200
1:      subs r4, r4, #1
        bne 1b

       	str r3, [r1, #0x100]
        ldr r4, [r1, #0x100]
        cmp r3, r4
        movne r3, #1
        moveq r3, #0

        mov pc, lr
