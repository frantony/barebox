/*
 *  based on linux/include/asm-mips/mach-jz4770/regs.h
 *
 *  Ingenic's JZ4770 common include.
 *
 *  Copyright (C) 2006 - 2007 Ingenic Semiconductor Inc.
 *
 *  Author: <yliu@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JZ4770_REGS_H__
#define __JZ4770_REGS_H__

#ifdef CFG_EXTAL
#define JZ_EXTAL		CFG_EXTAL
#else
#define JZ_EXTAL		12000000
#endif

#define CPM_BASE	0xb0000000
#define INTC_BASE	0xb0001000
#define GPIO_BASE	0xb0010000
#define UART2_BASE	0xb0032000
#define DDRC_BASE	0xb3020000
#define MDMAC_BASE	0xb3420000
#define DMAC_BASE	0xb3420000

/*************************************************************************
 * INTC (Interrupt Controller)
 *************************************************************************/
#define INTC_ISR(n)	(INTC_BASE + 0x00 + (n) * 0x20)
#define INTC_IMR(n)	(INTC_BASE + 0x04 + (n) * 0x20)
#define INTC_IMSR(n)	(INTC_BASE + 0x08 + (n) * 0x20)
#define INTC_IMCR(n)	(INTC_BASE + 0x0c + (n) * 0x20)
#define INTC_IPR(n)	(INTC_BASE + 0x10 + (n) * 0x20)

/*************************************************************************
 * GPIO (General-Purpose I/O Ports)
 *************************************************************************/
//n = 0,1,2,3,4,5
#define GPIO_PXPIN(n)	(GPIO_BASE + 0x00 + (n)*0x100) /* PIN Level Register */
#define GPIO_PXINT(n)	(GPIO_BASE + 0x10 + (n)*0x100) /* Port Interrupt Register */
#define GPIO_PXINTS(n)	(GPIO_BASE + 0x14 + (n)*0x100) /* Port Interrupt Set Register */
#define GPIO_PXINTC(n)	(GPIO_BASE + 0x18 + (n)*0x100) /* Port Interrupt Clear Register */
#define GPIO_PXMASK(n)	(GPIO_BASE + 0x20 + (n)*0x100) /* Port Interrupt Mask Register */
#define GPIO_PXMASKS(n)	(GPIO_BASE + 0x24 + (n)*0x100) /* Port Interrupt Mask Set Reg */
#define GPIO_PXMASKC(n)	(GPIO_BASE + 0x28 + (n)*0x100) /* Port Interrupt Mask Clear Reg */
#define GPIO_PXPAT1(n)	(GPIO_BASE + 0x30 + (n)*0x100) /* Port Pattern 1 Register */
#define GPIO_PXPAT1S(n)	(GPIO_BASE + 0x34 + (n)*0x100) /* Port Pattern 1 Set Reg. */
#define GPIO_PXPAT1C(n)	(GPIO_BASE + 0x38 + (n)*0x100) /* Port Pattern 1 Clear Reg. */
#define GPIO_PXPAT0(n)	(GPIO_BASE + 0x40 + (n)*0x100) /* Port Pattern 0 Register */
#define GPIO_PXPAT0S(n)	(GPIO_BASE + 0x44 + (n)*0x100) /* Port Pattern 0 Set Register */
#define GPIO_PXPAT0C(n)	(GPIO_BASE + 0x48 + (n)*0x100) /* Port Pattern 0 Clear Register */
#define GPIO_PXFLG(n)	(GPIO_BASE + 0x50 + (n)*0x100) /* Port Flag Register */
#define GPIO_PXFLGC(n)	(GPIO_BASE + 0x58 + (n)*0x100) /* Port Flag clear Register */
#define GPIO_PXPEN(n)	(GPIO_BASE + 0x70 + (n)*0x100) /* Port Pull Disable Register */
#define GPIO_PXPENS(n)	(GPIO_BASE + 0x74 + (n)*0x100) /* Port Pull Disable Set Register */
#define GPIO_PXPENC(n)	(GPIO_BASE + 0x78 + (n)*0x100) /* Port Pull Disable Clear Register */

/* Clock control register */
#define CPM_CPCCR_MEM			(0x01 << 30)
#define CPM_CPCCR_CE			(0x01 << 22)
#define CPM_CPCCR_PCS			(0x01 << 21)

#define CPM_CPCCR_H1DIV_BIT		24
#define CPM_CPCCR_H1DIV_MASK		(0x0f << CPM_CPCCR_H1DIV_BIT)
#define CPM_CPCCR_H2DIV_BIT		16
#define CPM_CPCCR_H2DIV_MASK		(0x0f << CPM_CPCCR_H2DIV_BIT)
#define CPM_CPCCR_C1DIV_BIT		12
#define CPM_CPCCR_C1DIV_MASK		(0x0f << CPM_CPCCR_C1DIV_BIT)
#define CPM_CPCCR_PDIV_BIT		8
#define CPM_CPCCR_PDIV_MASK		(0x0f << CPM_CPCCR_PDIV_BIT)
#define CPM_CPCCR_H0DIV_BIT		4
#define CPM_CPCCR_H0DIV_MASK		(0x0f << CPM_CPCCR_H0DIV_BIT)
#define CPM_CPCCR_CDIV_BIT		0
#define CPM_CPCCR_CDIV_MASK		(0x0f << CPM_CPCCR_CDIV_BIT)

/* PLL control register 0 */
#define CPM_CPPCR_PLLM_BS               (1 << 31) /* 1:high band 0:low band*/
#define CPM_CPPCR_PLLM_BIT		24
#define CPM_CPPCR_PLLM_MASK		(0x7f << CPM_CPPCR_PLLM_BIT)
#define CPM_CPPCR_PLLN_BIT		18
#define CPM_CPPCR_PLLN_MASK		(0x1f << CPM_CPPCR_PLLN_BIT)
#define CPM_CPPCR_PLLOD_BIT		16
#define CPM_CPPCR_PLLOD_MASK		(0x03 << CPM_CPPCR_PLLOD_BIT)
#define CPM_CPPCR_LOCK0			(1 << 15)
#define CPM_CPPCR_ENLOCK		(1 << 14)
#define CPM_CPPCR_PLLS			(1 << 10)
#define CPM_CPPCR_PLLBP			(1 << 9)
#define CPM_CPPCR_PLLEN			(1 << 8)
#define CPM_CPPCR_PLLST_BIT		0
#define CPM_CPPCR_PLLST_MASK		(0xff << CPM_CPPCR_PLLST_BIT)

/* PLL control register 1 */
#define CPM_CPPCR_PLL1M_BS               (1 << 31) /* 1:high band 0:low band*/
#define CPM_CPPCR1_PLL1M_BIT		24
#define CPM_CPPCR1_PLL1M_MASK		(0x7f << CPM_CPPCR1_PLL1M_BIT)
#define CPM_CPPCR1_PLL1N_BIT		18
#define CPM_CPPCR1_PLL1N_MASK		(0x1f << CPM_CPPCR1_PLL1N_BIT)
#define CPM_CPPCR1_PLL1OD_BIT		16
#define CPM_CPPCR1_PLL1OD_MASK		(0x03 << CPM_CPPCR1_PLL1OD_BIT)
#define CPM_CPPCR1_P1SCS		(1 << 15)
#define CPM_CPPCR1_P1SDIV_BIT		8
#define CPM_CPPCR1_P1SDIV_MASK		(0x3f << CPM_CPPCR1_P1SDIV_BIT)
#define CPM_CPPCR1_PLL1EN		(1 << 7)
#define CPM_CPPCR1_PLL1S		(1 << 6)
#define CPM_CPPCR1_LOCK1		(1 << 2)
#define CPM_CPPCR1_PLL1OFF		(1 << 1)
#define CPM_CPPCR1_PLL1ON		(1 << 0)

/*************************************************************************
 * CPM (Clock reset and Power control Management)
 *************************************************************************/
#define CPM_CPCCR		(CPM_BASE+0x00) /* Clock control register		*/
#define CPM_CPPCR		(CPM_BASE+0x10) /* PLL control register 0		*/
#define CPM_CPPSR		(CPM_BASE+0x14) /* PLL switch and status Register	*/
#define CPM_CPPCR1		(CPM_BASE+0x30) /* PLL control register 1		*/
#define CPM_CPSPR		(CPM_BASE+0x34) /* CPM scratch pad register		*/
#define CPM_CPSPPR		(CPM_BASE+0x38) /* CPM scratch protected register	*/
#define CPM_I2SCDR		(CPM_BASE+0x60) /* I2S device clock divider register	*/
#define CPM_MSCCDR		(CPM_BASE+0x68) /* MSC clock divider register		*/
#define CPM_UHCCDR		(CPM_BASE+0x6C) /* UHC 48M clock divider register	*/
#define CPM_CIMCDR		(CPM_BASE+0x7c) /* CIM MCLK clock divider register	*/
#define CPM_GPUCDR		(CPM_BASE+0x88) /* GPU clock divider register		*/
#define CPM_MSC1CDR             (CPM_BASE+0xa4) /* MSC1 clock divider register		*/
#define CPM_MSC2CDR             (CPM_BASE+0xa8) /* MSC2 clock divider register		*/
#define CPM_BCHCDR		(CPM_BASE+0xac) /* BCH clock divider register           */
#define CPM_INTR                (CPM_BASE+0xb0) /* CPM interrupt register               */
#define CPM_INTRE               (CPM_BASE+0xb4) /* CPM interrupt enable register        */

#define CPM_LCR			(CPM_BASE+0x04)
#define CPM_PSWCST(n)		(CPM_BASE+0x4*(n)+0x90)
#define CPM_CLKGR0		(CPM_BASE+0x20) /* Clock Gate Register0 */
#define CPM_CLKGR1		(CPM_BASE+0x28) /* Clock Gate Register1 */
#define CPM_OPCR		(CPM_BASE+0x24) /* Oscillator and Power Control Register */

/* Clock Gate Register0 */
#define CPM_CLKGR0_DDR          (1 << 30)
#define CPM_CLKGR0_DMAC         (1 << 21)
#define CPM_CLKGR0_UART2        (1 << 17)

#define __cpm_get_pllm() \
	((__raw_readl(CPM_CPPCR) & CPM_CPPCR_PLLM_MASK) >> CPM_CPPCR_PLLM_BIT)
#define __cpm_get_plln() \
	((__raw_readl(CPM_CPPCR) & CPM_CPPCR_PLLN_MASK) >> CPM_CPPCR_PLLN_BIT)
#define __cpm_get_pllod() \
	((__raw_readl(CPM_CPPCR) & CPM_CPPCR_PLLOD_MASK) >> CPM_CPPCR_PLLOD_BIT)
#define __cpm_get_cdiv() \
	((__raw_readl(CPM_CPCCR) & CPM_CPCCR_CDIV_MASK) >> CPM_CPCCR_CDIV_BIT)
#define __cpm_get_mdiv() \
	((__raw_readl(CPM_CPCCR) & CPM_CPCCR_H0DIV_MASK) >> CPM_CPCCR_H0DIV_BIT)

/*************************************************************************
 * DDRC (DDR Controller)
 *************************************************************************/
#define DDR_MEM_PHY_BASE	0x20000000

#define DDRC_ST		(DDRC_BASE + 0x0) /* DDR Status Register */
#define DDRC_CFG	(DDRC_BASE + 0x4) /* DDR Configure Register */
#define DDRC_CTRL	(DDRC_BASE + 0x8) /* DDR Control Register */
#define DDRC_LMR	(DDRC_BASE + 0xc) /* DDR Load-Mode-Register */
#define DDRC_TIMING1	(DDRC_BASE + 0x10) /* DDR Timing Config Register 1 */
#define DDRC_TIMING2	(DDRC_BASE + 0x14) /* DDR Timing Config Register 2 */
#define DDRC_REFCNT	(DDRC_BASE + 0x18) /* DDR  Auto-Refresh Counter */
#define DDRC_DQS	(DDRC_BASE + 0x1c) /* DDR DQS Delay Control Register */
#define DDRC_DQS_ADJ	(DDRC_BASE + 0x20) /* DDR DQS Delay Adjust Register */
#define DDRC_MMAP0	(DDRC_BASE + 0x24) /* DDR Memory Map Config Register */
#define DDRC_MMAP1	(DDRC_BASE + 0x28) /* DDR Memory Map Config Register */
#define DDRC_MDELAY	(DDRC_BASE + 0x2c) /* DDR Memory Map Config Register */
#define DDRC_CKEL	(DDRC_BASE + 0x30) /* DDR CKE Low if it was set to 0 */
#define DDRC_PMEMCTRL0	(DDRC_BASE + 0x54)
#define DDRC_PMEMCTRL1	(DDRC_BASE + 0x50)
#define DDRC_PMEMCTRL2	(DDRC_BASE + 0x58)
#define DDRC_PMEMCTRL3	(DDRC_BASE + 0x5c)

/* DDRC Status Register */
#define DDRC_ST_ENDIAN	(1 << 7) /* 0 Little data endian
					    1 Big data endian */
#define DDRC_ST_DPDN		(1 << 5) /* 0 DDR memory is NOT in deep-power-down state
					    1 DDR memory is in deep-power-down state */
#define DDRC_ST_PDN		(1 << 4) /* 0 DDR memory is NOT in power-down state
					    1 DDR memory is in power-down state */
#define DDRC_ST_AREF		(1 << 3) /* 0 DDR memory is NOT in auto-refresh state
					    1 DDR memory is in auto-refresh state */
#define DDRC_ST_SREF		(1 << 2) /* 0 DDR memory is NOT in self-refresh state
					    1 DDR memory is in self-refresh state */
#define DDRC_ST_CKE1		(1 << 1) /* 0 CKE1 Pin is low
					    1 CKE1 Pin is high */
#define DDRC_ST_CKE0		(1 << 0) /* 0 CKE0 Pin is low
					    1 CKE0 Pin is high */

#define DDRC_CFG_MPRT		(1 << 15)  /* mem protect */

#define DDRC_CFG_ROW1_BIT	27 /* Row Address width. */
#define DDRC_CFG_COL1_BIT	25 /* Col Address width. */
#define DDRC_CFG_BA1		(1 << 24)
#define DDRC_CFG_IMBA		(1 << 23)
#define DDRC_CFG_BTRUN		(1 << 21)

#define DDRC_CFG_TYPE_BIT	12
#define DDRC_CFG_TYPE_MASK	(0x7 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR1	(2 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_MDDR	(3 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR2	(4 << DDRC_CFG_TYPE_BIT)

#define DDRC_CFG_ROW_BIT	10 /* Row Address width. */
#define DDRC_CFG_ROW_MASK	(0x3 << DDRC_CFG_ROW_BIT)
  #define DDRC_CFG_ROW_12	(0 << DDRC_CFG_ROW_BIT) /* 13-bit row address is used */
  #define DDRC_CFG_ROW_13	(1 << DDRC_CFG_ROW_BIT) /* 14-bit row address is used */
  #define DDRC_CFG_ROW_14	(2 << DDRC_CFG_ROW_BIT)

#define DDRC_CFG_COL_BIT	8 /* Column Address width.
				     Specify the Column address width of external DDR. */
#define DDRC_CFG_COL_MASK	(0x3 << DDRC_CFG_COL_BIT)
  #define DDRC_CFG_COL_8	(0 << DDRC_CFG_COL_BIT) /* 9-bit Column address is used */
  #define DDRC_CFG_COL_9	(1 << DDRC_CFG_COL_BIT) /* 9-bit Column address is used */
  #define DDRC_CFG_COL_10	(2 << DDRC_CFG_COL_BIT) /* 9-bit Column address is used */
  #define DDRC_CFG_COL_11	(3 << DDRC_CFG_COL_BIT) /* 10-bit Column address is used */

#define DDRC_CFG_CS1EN	(1 << 7) /* 0 DDR Pin CS1 un-used
					    1 There're DDR memory connected to CS1 */
#define DDRC_CFG_CS0EN	(1 << 6) /* 0 DDR Pin CS0 un-used
					    1 There're DDR memory connected to CS0 */


#define DDRC_CFG_TSEL_BIT	18 /* Read delay select */
#define DDRC_CFG_TSEL_MASK	(0x3 << DDRC_CFG_TSEL_BIT)
#define DDRC_CFG_TSEL_0	(0 << DDRC_CFG_TSEL_BIT) /* No delay */
#define DDRC_CFG_TSEL_1	(1 << DDRC_CFG_TSEL_BIT) /* delay 1 tCK */
#define DDRC_CFG_TSEL_2	(2 << DDRC_CFG_TSEL_BIT) /* delay 2 tCK */
#define DDRC_CFG_TSEL_3	(3 << DDRC_CFG_TSEL_BIT) /* delay 3 tCK */


#define DDRC_CFG_CL_BIT	2 /* CAS Latency */
#define DDRC_CFG_CL_MASK	(0xf << DDRC_CFG_CL_BIT)
#define DDRC_CFG_CL_3		(0 << DDRC_CFG_CL_BIT) /* CL = 3 tCK */
#define DDRC_CFG_CL_4		(1 << DDRC_CFG_CL_BIT) /* CL = 4 tCK */
#define DDRC_CFG_CL_5		(2 << DDRC_CFG_CL_BIT) /* CL = 5 tCK */
#define DDRC_CFG_CL_6		(3 << DDRC_CFG_CL_BIT) /* CL = 6 tCK */

#define DDRC_CFG_BA		(1 << 1) /* 0 4 bank device, Pin ba[1:0] valid, ba[2] un-used
					    1 8 bank device, Pin ba[2:0] valid*/
#define DDRC_CFG_DW		(1 << 0) /*0 External memory data width is 16-bit
					   1 External memory data width is 32-bit */

/* DDRC Control Register */
#define DDRC_CTRL_ACTPD	(1 << 15) /* 0 Precharge all banks before entering power-down
					     1 Do not precharge banks before entering power-down */
#define DDRC_CTRL_PDT_BIT	12 /* Power-Down Timer */
#define DDRC_CTRL_PDT_MASK	(0x7 << DDRC_CTRL_PDT_BIT)
  #define DDRC_CTRL_PDT_DIS	(0 << DDRC_CTRL_PDT_BIT) /* power-down disabled */
  #define DDRC_CTRL_PDT_8	(1 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 8 tCK idle */
  #define DDRC_CTRL_PDT_16	(2 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 16 tCK idle */
  #define DDRC_CTRL_PDT_32	(3 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 32 tCK idle */
  #define DDRC_CTRL_PDT_64	(4 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 64 tCK idle */
  #define DDRC_CTRL_PDT_128	(5 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 128 tCK idle */

#define DDRC_CTRL_PRET_BIT	8 /* Precharge Timer */
#define DDRC_CTRL_PRET_MASK	(0x7 << DDRC_CTRL_PRET_BIT) /*  */
  #define DDRC_CTRL_PRET_DIS	(0 << DDRC_CTRL_PRET_BIT) /* PRET function Disabled */
  #define DDRC_CTRL_PRET_8	(1 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 8 tCK idle */
  #define DDRC_CTRL_PRET_16	(2 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 16 tCK idle */
  #define DDRC_CTRL_PRET_32	(3 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 32 tCK idle */
  #define DDRC_CTRL_PRET_64	(4 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 64 tCK idle */
  #define DDRC_CTRL_PRET_128	(5 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 128 tCK idle */

#define DDRC_CTRL_DPD		(1 << 6) /* 1 Drive external DDR device entering self-refresh mode */

#define DDRC_CTRL_SR		(1 << 5) /* 1 Drive external DDR device entering self-refresh mode
					    0 Drive external DDR device exiting self-refresh mode */
#define DDRC_CTRL_UNALIGN	(1 << 4) /* 0 Disable unaligned transfer on AXI BUS
					    1 Enable unaligned transfer on AXI BUS */
#define DDRC_CTRL_ALH		(1 << 3) /* Advanced Latency Hiding:
					    0 Disable ALH
					    1 Enable ALH */
#define DDRC_CTRL_RDC		(1 << 2) /* 0 dclk clock frequency is lower than 60MHz
					    1 dclk clock frequency is higher than 60MHz */
#define DDRC_CTRL_CKE		(1 << 1) /* 0 Not set CKE Pin High
					    1 Set CKE Pin HIGH */
#define DDRC_CTRL_RESET	(1 << 0) /* 0 End resetting ddrc_controller
					    1 Resetting ddrc_controller */


/* DDRC Load-Mode-Register */
#define DDRC_LMR_DDR_ADDR_BIT	16 /* When performing a DDR command, DDRC_ADDR[13:0]
					      corresponding to external DDR address Pin A[13:0] */
#define DDRC_LMR_DDR_ADDR_MASK	(0x3fff << DDRC_LMR_DDR_ADDR_BIT)

#define DDRC_LMR_BA_BIT		8 /* When performing a DDR command, BA[2:0]
				     corresponding to external DDR address Pin BA[2:0]. */
#define DDRC_LMR_BA_MASK	(0x7 << DDRC_LMR_BA_BIT)
  /* For DDR2 */
  #define DDRC_LMR_BA_MRS	(0 << DDRC_LMR_BA_BIT) /* Mode Register set */
  #define DDRC_LMR_BA_EMRS1	(1 << DDRC_LMR_BA_BIT) /* Extended Mode Register1 set */
  #define DDRC_LMR_BA_EMRS2	(2 << DDRC_LMR_BA_BIT) /* Extended Mode Register2 set */
  #define DDRC_LMR_BA_EMRS3	(3 << DDRC_LMR_BA_BIT) /* Extended Mode Register3 set */
  /* For mobile DDR */
  #define DDRC_LMR_BA_M_MRS	(0 << DDRC_LMR_BA_BIT) /* Mode Register set */
  #define DDRC_LMR_BA_M_EMRS	(2 << DDRC_LMR_BA_BIT) /* Extended Mode Register set */
  #define DDRC_LMR_BA_M_SR	(1 << DDRC_LMR_BA_BIT) /* Status Register set */
  /* For Normal DDR1 */
  #define DDRC_LMR_BA_N_MRS	(0 << DDRC_LMR_BA_BIT) /* Mode Register set */
  #define DDRC_LMR_BA_N_EMRS	(1 << DDRC_LMR_BA_BIT) /* Extended Mode Register set */

#define DDRC_LMR_CMD_BIT	4
#define DDRC_LMR_CMD_MASK	(0x3 << DDRC_LMR_CMD_BIT)
  #define DDRC_LMR_CMD_PREC	(0 << DDRC_LMR_CMD_BIT)/* Precharge one bank/All banks */
  #define DDRC_LMR_CMD_AUREF	(1 << DDRC_LMR_CMD_BIT)/* Auto-Refresh */
  #define DDRC_LMR_CMD_LMR	(2 << DDRC_LMR_CMD_BIT)/* Load Mode Register */

#define DDRC_LMR_START		(1 << 0) /* 0 No command is performed
						    1 On the posedge of START, perform a command
						    defined by CMD field */

/* DDRC Mode Register Set */
#define DDR2_MRS_PD_BIT		10 /* Active power down exit time */
#define DDR2_MRS_PD_MASK	(1 << DDR_MRS_PD_BIT) 
  #define DDR2_MRS_PD_FAST_EXIT	(0 << 10)
  #define DDR2_MRS_PD_SLOW_EXIT	(1 << 10)
#define DDR2_MRS_WR_BIT		9 /* Write Recovery for autoprecharge */
#define DDR2_MRS_WR_MASK	(7 << DDR_MRS_WR_BIT)
#define DDR2_MRS_DLL_RST	(1 << 8) /* DLL Reset */
#define DDR2_MRS_TM_BIT		7        /* Operating Mode */
#define DDR2_MRS_TM_MASK	(1 << DDR_MRS_TM_BIT) 
  #define DDR2_MRS_TM_NORMAL	(0 << DDR_MRS_TM_BIT)
  #define DDR2_MRS_TM_TEST	(1 << DDR_MRS_TM_BIT)
#define DDR_MRS_CAS_BIT		4        /* CAS Latency */
#define DDR_MRS_CAS_MASK	(7 << DDR_MRS_CAS_BIT)
#define DDR_MRS_BT_BIT		3        /* Burst Type */
#define DDR_MRS_BT_MASK		(1 << DDR_MRS_BT_BIT)
  #define DDR_MRS_BT_SEQ	(0 << DDR_MRS_BT_BIT) /* Sequential */
  #define DDR_MRS_BT_INT	(1 << DDR_MRS_BT_BIT) /* Interleave */
#define DDR_MRS_BL_BIT		0        /* Burst Length */
#define DDR_MRS_BL_MASK		(7 << DDR_MRS_BL_BIT)
  #define DDR_MRS_BL_4		(2 << DDR_MRS_BL_BIT)
  #define DDR_MRS_BL_8		(3 << DDR_MRS_BL_BIT)

/* DDR2 Extended Mode Register1 Set */
#define DDR_EMRS1_QOFF		(1<<12) /* 0 Output buffer enabled
					   1 Output buffer disabled */
#define DDR_EMRS1_RDQS_EN	(1<<11) /* 0 Disable
					   1 Enable */
#define DDR_EMRS1_DQS_DIS	(1<<10) /* 0 Enable
					   1 Disable */
#define DDR_EMRS1_OCD_BIT	7 /* Additive Latency 0 -> 6 */
#define DDR_EMRS1_OCD_MASK	(0x7 << DDR_EMRS1_OCD_BIT)
  #define DDR_EMRS1_OCD_EXIT		(0 << DDR_EMRS1_OCD_BIT)
  #define DDR_EMRS1_OCD_D0		(1 << DDR_EMRS1_OCD_BIT)
  #define DDR_EMRS1_OCD_D1		(2 << DDR_EMRS1_OCD_BIT)
  #define DDR_EMRS1_OCD_ADJ		(4 << DDR_EMRS1_OCD_BIT)
  #define DDR_EMRS1_OCD_DFLT		(7 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_AL_BIT	3 /* Additive Latency 0 -> 6 */
#define DDR_EMRS1_AL_MASK	(7 << DDR_EMRS1_AL_BIT)
#define DDR_EMRS1_RTT_BIT	2 /*  */
#define DDR_EMRS1_RTT_MASK	(0x11 << DDR_EMRS1_DIC_BIT) /* Bit 6, Bit 2 */
#define DDR_EMRS1_DIC_BIT	1        /* Output Driver Impedence Control */
#define DDR_EMRS1_DIC_MASK	(1 << DDR_EMRS1_DIC_BIT) /* 100% */
  #define DDR_EMRS1_DIC_NORMAL	(0 << DDR_EMRS1_DIC_BIT) /* 60% */
  #define DDR_EMRS1_DIC_HALF	(1 << DDR_EMRS1_DIC_BIT)
#define DDR_EMRS1_DLL_BIT	0        /* DLL Enable  */
#define DDR_EMRS1_DLL_MASK	(1 << DDR_EMRS1_DLL_BIT)
  #define DDR_EMRS1_DLL_EN	(0 << DDR_EMRS1_DLL_BIT)
  #define DDR_EMRS1_DLL_DIS	(1 << DDR_EMRS1_DLL_BIT)

/* Mobile SDRAM Extended Mode Register */
#define DDR_EMRS_DS_BIT		5	/* Driver strength */
#define DDR_EMRS_DS_MASK	(3 << DDR_EMRS_DS_BIT)
  #define DDR_EMRS_DS_FULL	(0 << DDR_EMRS_DS_BIT)	/*Full*/
  #define DDR_EMRS_DS_HALF	(1 << DDR_EMRS_DS_BIT)	/*1/2 Strength*/
  #define DDR_EMRS_DS_QUTR	(2 << DDR_EMRS_DS_BIT)	/*1/4 Strength*/

#define DDR_EMRS_PRSR_BIT	0	/* Partial Array Self Refresh */
#define DDR_EMRS_PRSR_MASK	(7 << DDR_EMRS_PRSR_BIT)
  #define DDR_EMRS_PRSR_ALL	(0 << DDR_EMRS_PRSR_BIT) /*All Banks*/
  #define DDR_EMRS_PRSR_HALF_TL	(1 << DDR_EMRS_PRSR_BIT) /*Half of Total Bank*/
  #define DDR_EMRS_PRSR_QUTR_TL	(2 << DDR_EMRS_PRSR_BIT) /*Quarter of Total Bank*/
  #define DDR_EMRS_PRSR_HALF_B0	(5 << DDR_EMRS_PRSR_BIT) /*Half of Bank0*/
  #define DDR_EMRS_PRSR_QUTR_B0	(6 << DDR_EMRS_PRSR_BIT) /*Quarter of Bank0*/

/* DDR1 Mode Register */
#define DDR1_MRS_OM_BIT		7        /* Operating Mode */
#define DDR1_MRS_OM_MASK	(0x3f << DDR1_MRS_OM_BIT) 
  #define DDR1_MRS_OM_NORMAL	(0 << DDR1_MRS_OM_BIT)
  #define DDR1_MRS_OM_TEST	(1 << DDR1_MRS_OM_BIT)
  #define DDR1_MRS_OM_DLLRST	(2 << DDR1_MRS_OM_BIT)

/* DDR1 Extended Mode Register */
#define DDR1_EMRS_OM_BIT	2	/* Partial Array Self Refresh */
#define DDR1_EMRS_OM_MASK	(0x3ff << DDR1_EMRS_OM_BIT)
  #define DDR1_EMRS_OM_NORMAL	(0 << DDR1_EMRS_OM_BIT) /*All Banks*/

#define DDR1_EMRS_DS_BIT	1	/* Driver strength */
#define DDR1_EMRS_DS_MASK	(1 << DDR1_EMRS_DS_BIT)
  #define DDR1_EMRS_DS_FULL	(0 << DDR1_EMRS_DS_BIT)	/*Full*/
  #define DDR1_EMRS_DS_HALF	(1 << DDR1_EMRS_DS_BIT)	/*1/2 Strength*/

#define DDR1_EMRS_DLL_BIT	0	/* Driver strength */
#define DDR1_EMRS_DLL_MASK	(1 << DDR1_EMRS_DLL_BIT)
  #define DDR1_EMRS_DLL_EN	(0 << DDR1_EMRS_DLL_BIT)	/*Full*/
  #define DDR1_EMRS_DLL_DIS	(1 << DDR1_EMRS_DLL_BIT)	/*1/2 Strength*/

/* DDRC Timing Config Register 1 */
#define DDRC_TIMING1_TRAS_BIT 	28 /* ACTIVE to PRECHARGE command period (2 * tRAS + 1) */
#define DDRC_TIMING1_TRAS_MASK 	(0xf << DDRC_TIMING1_TRAS_BIT)

#define DDRC_TIMING1_TRTP_BIT		24 /* READ to PRECHARGE command period. */
#define DDRC_TIMING1_TRTP_MASK	(0x3 << DDRC_TIMING1_TRTP_BIT)

#define DDRC_TIMING1_TRP_BIT		20 /* PRECHARGE command period. */
#define DDRC_TIMING1_TRP_MASK 	(0x7 << DDRC_TIMING1_TRP_BIT)

#define DDRC_TIMING1_TRCD_BIT		16 /* ACTIVE to READ or WRITE command period. */
#define DDRC_TIMING1_TRCD_MASK	(0x7 << DDRC_TIMING1_TRCD_BIT)

#define DDRC_TIMING1_TRC_BIT 		12 /* ACTIVE to ACTIVE command period. */
#define DDRC_TIMING1_TRC_MASK 	(0xf << DDRC_TIMING1_TRC_BIT)

#define DDRC_TIMING1_TRRD_BIT		8 /* ACTIVE bank A to ACTIVE bank B command period. */
#define DDRC_TIMING1_TRRD_MASK	(0x3 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_DISABLE	(0 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_2		(1 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_3		(2 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_4		(3 << DDRC_TIMING1_TRRD_BIT)

#define DDRC_TIMING1_TWR_BIT 		4 /* WRITE Recovery Time defined by register MR of DDR2 memory */
#define DDRC_TIMING1_TWR_MASK		(0x7 << DDRC_TIMING1_TWR_BIT)
  #define DDRC_TIMING1_TWR_1		(0 << DDRC_TIMING1_TWR_BIT)
  #define DDRC_TIMING1_TWR_2		(1 << DDRC_TIMING1_TWR_BIT)
  #define DDRC_TIMING1_TWR_3		(2 << DDRC_TIMING1_TWR_BIT)
  #define DDRC_TIMING1_TWR_4		(3 << DDRC_TIMING1_TWR_BIT)
  #define DDRC_TIMING1_TWR_5		(4 << DDRC_TIMING1_TWR_BIT)
  #define DDRC_TIMING1_TWR_6		(5 << DDRC_TIMING1_TWR_BIT)

#define DDRC_TIMING1_TWTR_BIT		0 /* WRITE to READ command delay. */
#define DDRC_TIMING1_TWTR_MASK	(0x3 << DDRC_TIMING1_TWTR_BIT)
  #define DDRC_TIMING1_TWTR_1		(0 << DDRC_TIMING1_TWTR_BIT)
  #define DDRC_TIMING1_TWTR_2		(1 << DDRC_TIMING1_TWTR_BIT)
  #define DDRC_TIMING1_TWTR_3		(2 << DDRC_TIMING1_TWTR_BIT)
  #define DDRC_TIMING1_TWTR_4		(3 << DDRC_TIMING1_TWTR_BIT)

/* DDRC Timing Config Register 2 */
#define DDRC_TIMING2_TRFC_BIT         24 /* AUTO-REFRESH command period. */
#define DDRC_TIMING2_TRFC_MASK        (0x3f << DDRC_TIMING2_TRFC_BIT)
#define DDRC_TIMING2_RWCOV_BIT        19 /* Equal to Tsel of MDELAY. */
#define DDRC_TIMING2_RWCOV_MASK       (0x3 << DDRC_TIMING2_RWCOV_BIT)
#define DDRC_TIMING2_TMINSR_BIT       8  /* Minimum Self-Refresh / Deep-Power-Down time */
#define DDRC_TIMING2_TMINSR_MASK      (0xf << DDRC_TIMING2_TMINSR_BIT)
#define DDRC_TIMING2_TXP_BIT          4  /* EXIT-POWER-DOWN to next valid command period. */
#define DDRC_TIMING2_TXP_MASK         (0x7 << DDRC_TIMING2_TXP_BIT)
#define DDRC_TIMING2_TMRD_BIT         0  /* Load-Mode-Register to next valid command period. */
#define DDRC_TIMING2_TMRD_MASK        (0x3 << DDRC_TIMING2_TMRD_BIT)

/* DDRC  Auto-Refresh Counter */
#define DDRC_REFCNT_CON_BIT           16 /* Constant value used to compare with CNT value. */
#define DDRC_REFCNT_CON_MASK          (0xff << DDRC_REFCNT_CON_BIT)
#define DDRC_REFCNT_CNT_BIT           8  /* 8-bit counter */
#define DDRC_REFCNT_CNT_MASK          (0xff << DDRC_REFCNT_CNT_BIT)
#define DDRC_REFCNT_CLKDIV_BIT        1  /* Clock Divider for auto-refresh counter. */
#define DDRC_REFCNT_CLKDIV_MASK       (0x7 << DDRC_REFCNT_CLKDIV_BIT)
#define DDRC_REFCNT_REF_EN            (1 << 0) /* Enable Refresh Counter */

/* DDRC DQS Delay Control Register */
#define DDRC_DQS_ERROR                (1 << 29) /* ahb_clk Delay Detect ERROR, read-only. */
#define DDRC_DQS_READY                (1 << 28) /* ahb_clk Delay Detect READY, read-only. */
#define DDRC_DQS_AUTO                 (1 << 23) /* Hardware auto-detect & set delay line */
#define DDRC_DQS_DET                  (1 << 24) /* Start delay detecting. */
#define DDRC_DQS_SRDET                (1 << 25) /* Hardware auto-redetect & set delay line. */
#define DDRC_DQS_CLKD_BIT             16 /* CLKD is reference value for setting WDQS and RDQS.*/
#define DDRC_DQS_CLKD_MASK            (0x3f << DDRC_DQS_CLKD_BIT) 
#define DDRC_DQS_WDQS_BIT             8  /* Set delay element number to write DQS delay-line. */
#define DDRC_DQS_WDQS_MASK            (0x3f << DDRC_DQS_WDQS_BIT) 
#define DDRC_DQS_RDQS_BIT             0  /* Set delay element number to read DQS delay-line. */
#define DDRC_DQS_RDQS_MASK            (0x3f << DDRC_DQS_RDQS_BIT) 

/* DDRC DQS Delay Adjust Register */
#define DDRC_DQS_ADJWSIGN             (1 << 13) /* The sign of WDQS value for WRITE DQS delay */
#define DDRC_DQS_ADJWDQS_BIT          8 /* The adjust value for WRITE DQS delay */
#define DDRC_DQS_ADJWDQS_MASK         (0x1f << DDRC_DQS_ADJWDQS_BIT)
#define DDRC_DQS_ADJRSIGN             (1 << 5) /* The sign of RDQS value for READ DQS delay */
#define DDRC_DQS_ADJRDQS_BIT          0 /* The adjust value for READ DQS delay */
#define DDRC_DQS_ADJRDQS_MASK         (0x1f << DDRC_DQS_ADJRDQS_BIT)

/* DDRC Memory Map Config Register */
#define DDRC_MMAP_BASE_BIT            8 /* base address */
#define DDRC_MMAP_BASE_MASK           (0xff << DDRC_MMAP_BASE_BIT)
#define DDRC_MMAP_MASK_BIT            0 /* address mask */
#define DDRC_MMAP_MASK_MASK           (0xff << DDRC_MMAP_MASK_BIT)         

#define DDRC_MMAP0_BASE		     (0x20 << DDRC_MMAP_BASE_BIT)
#define DDRC_MMAP1_BASE_64M	(0x24 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/
#define DDRC_MMAP1_BASE_128M	(0x28 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/
#define DDRC_MMAP1_BASE_256M	(0x30 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/

#define DDRC_MMAP_MASK_64_64	(0xfc << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/
#define DDRC_MMAP_MASK_128_128	(0xf8 << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/
#define DDRC_MMAP_MASK_256_256	(0xf0 << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/

#define DDRC_MDELAY_MAUTO_BIT (6)
#define DDRC_MDELAY_MAUTO  (1 << DDRC_MDELAY_MAUTO_BIT)
#define DDR_GET_VALUE(x, y)			      \
({						      \
	unsigned long value, tmp;	              \
	tmp = x * 1000;				      \
	value = (tmp % y == 0) ? (tmp / y) : (tmp / y + 1); \
		value;                                \
})

/*************************************************************************
 * DMAC (DMA Controller)
 *************************************************************************/

#define HALF_DMA_NUM	6   /* the number of one dma controller's channels */

/* m is the DMA controller index (0, 1), n is the DMA channel index (0 - 11) */

#define DMAC_DSAR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x00 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA source address */
#define DMAC_DTAR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x04 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA target address */
#define DMAC_DTCR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x08 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA transfer count */
#define DMAC_DRSR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x0c + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA request source */
#define DMAC_DCCSR(n) (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x10 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA control/status */
#define DMAC_DCMD(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x14 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA command */
#define DMAC_DDA(n)   (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x18 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA descriptor address */
#define DMAC_DSD(n)   (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0xc0 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x04)) /* DMA Stride Address */

#define DMAC_DMACR(m)	(DMAC_BASE + 0x0300 + 0x100 * (m))              /* DMA control register */
#define DMAC_DMAIPR(m)	(DMAC_BASE + 0x0304 + 0x100 * (m))              /* DMA interrupt pending */
#define DMAC_DMADBR(m)	(DMAC_BASE + 0x0308 + 0x100 * (m))              /* DMA doorbell */
#define DMAC_DMADBSR(m)	(DMAC_BASE + 0x030C + 0x100 * (m))              /* DMA doorbell set */
#define DMAC_DMACKE(m)  (DMAC_BASE + 0x0310 + 0x100 * (m))

// DMA request source register
#define DMAC_DRSR_RS_BIT	0
#define DMAC_DRSR_RS_MASK	(0x1f << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_EXT	(0 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_NAND	(1 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_BCH_ENC	(2 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_BCH_DEC	(3 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AUTO	(8 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART3OUT	(14 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART3IN	(15 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART2OUT	(16 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART2IN	(17 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART1OUT	(18 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART1IN	(19 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART0OUT	(20 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART0IN	(21 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AICOUT	(24 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AICIN	(25 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC0OUT	(26 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC0IN	(27 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_TCU	(28 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SADC	(29 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC1OUT	(30 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC1IN	(31 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_PMOUT	(34 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_PMIN	(35 << DMAC_DRSR_RS_BIT)

// DMA channel control/status register
#define DMAC_DCCSR_NDES		(1 << 31) /* descriptor (0) or not (1) ? */
#define DMAC_DCCSR_DES8		(1 << 30) /* Descriptor 8 Word */
#define DMAC_DCCSR_DES4		(0 << 30) /* Descriptor 4 Word */
#define DMAC_DCCSR_CDOA_BIT	16        /* copy of DMA offset address */
#define DMAC_DCCSR_CDOA_MASK	(0xff << DMAC_DCCSR_CDOA_BIT)
#define DMAC_DCCSR_BERR		(1 << 7)  /* BCH error within this transfer, Only for channel 0 */
#define DMAC_DCCSR_INV		(1 << 6)  /* descriptor invalid */
#define DMAC_DCCSR_AR		(1 << 4)  /* address error */
#define DMAC_DCCSR_TT		(1 << 3)  /* transfer terminated */
#define DMAC_DCCSR_HLT		(1 << 2)  /* DMA halted */
#define DMAC_DCCSR_CT		(1 << 1)  /* count terminated */
#define DMAC_DCCSR_EN		(1 << 0)  /* channel enable bit */

// DMA channel command register
#define DMAC_DCMD_EACKS_LOW	(1 << 31) /* External DACK Output Level Select, active low */
#define DMAC_DCMD_EACKS_HIGH	(0 << 31) /* External DACK Output Level Select, active high */
#define DMAC_DCMD_EACKM_WRITE	(1 << 30) /* External DACK Output Mode Select, output in write cycle */
#define DMAC_DCMD_EACKM_READ	(0 << 30) /* External DACK Output Mode Select, output in read cycle */
#define DMAC_DCMD_ERDM_BIT      28        /* External DREQ Detection Mode Select */
#define DMAC_DCMD_ERDM_MASK     (0x03 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_LOW    (0 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_FALL   (1 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_HIGH   (2 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_RISE   (3 << DMAC_DCMD_ERDM_BIT)
#define DMAC_DCMD_BLAST		(1 << 25) /* BCH last */
#define DMAC_DCMD_SAI		(1 << 23) /* source address increment */
#define DMAC_DCMD_DAI		(1 << 22) /* dest address increment */
#define DMAC_DCMD_RDIL_BIT	16        /* request detection interval length */
#define DMAC_DCMD_RDIL_MASK	(0x0f << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_IGN	(0 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_2	(1 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_4	(2 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_8	(3 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_12	(4 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_16	(5 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_20	(6 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_24	(7 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_28	(8 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_32	(9 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_48	(10 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_60	(11 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_64	(12 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_124	(13 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_128	(14 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_200	(15 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_SWDH_BIT	14  /* source port width */
#define DMAC_DCMD_SWDH_MASK	(0x03 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_32	(0 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_8	(1 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_16	(2 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_DWDH_BIT	12  /* dest port width */
#define DMAC_DCMD_DWDH_MASK	(0x03 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_32	(0 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_8	(1 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_16	(2 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DS_BIT	8  /* transfer data size of a data unit */
#define DMAC_DCMD_DS_MASK	(0x07 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_32BIT	(0 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_8BIT	(1 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_16BIT	(2 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_16BYTE	(3 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_32BYTE	(4 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_STDE		(1 << 5) /* Stride Disable/Enable */
#define DMAC_DCMD_DES_V		(1 << 4)  /* descriptor valid flag */
#define DMAC_DCMD_DES_VM	(1 << 3)  /* descriptor valid mask: 1:support V-bit */
#define DMAC_DCMD_DES_VIE	(1 << 2)  /* DMA valid error interrupt enable */
#define DMAC_DCMD_TIE		(1 << 1)  /* DMA transfer interrupt enable */
#define DMAC_DCMD_LINK		(1 << 0)  /* descriptor link enable */

// DMA control register
#define DMAC_DMACR_DMAE		(1 << 0)  /* DMA enable bit */

#define MDMAC_DMACKES           (MDMAC_BASE + 0x0314)

/*************************************************************************
* EMC (External SDR Controller)
*************************************************************************/
#define EMC_LOW_SDRAM_SPACE_SIZE        0x10000000 /* 256M */

#endif /* __JZ4770_REGS_H__ */
