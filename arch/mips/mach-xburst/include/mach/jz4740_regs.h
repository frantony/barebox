/*
 *  based on linux/include/asm-mips/mach-jz4740/regs.h
 *
 *  Ingenic's JZ4740 common include.
 *
 *  Copyright (C) 2006 - 2007 Ingenic Semiconductor Inc.
 *
 *  Author: <yliu@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JZ4740_REGS_H__
#define __JZ4740_REGS_H__

#define	TCU_BASE	0xb0002000
#define	WDT_BASE	0xb0002000
#define	UART0_BASE	0xb0030000
#define	JZ4740_LCD_BASE_ADDR	0xb3050000

/*************************************************************************
 * TCU (Timer Counter Unit)
 *************************************************************************/
#define TCU_TSR		(TCU_BASE + 0x1C) /* Timer Stop Register */
#define TCU_TSSR	(TCU_BASE + 0x2C) /* Timer Stop Set Register */
#define TCU_TSCR	(TCU_BASE + 0x3C) /* Timer Stop Clear Register */
#define TCU_TER		(TCU_BASE + 0x10) /* Timer Counter Enable Register */
#define TCU_TESR	(TCU_BASE + 0x14) /* Timer Counter Enable Set Register */
#define TCU_TECR	(TCU_BASE + 0x18) /* Timer Counter Enable Clear Register */
#define TCU_TFR		(TCU_BASE + 0x20) /* Timer Flag Register */
#define TCU_TFSR	(TCU_BASE + 0x24) /* Timer Flag Set Register */
#define TCU_TFCR	(TCU_BASE + 0x28) /* Timer Flag Clear Register */
#define TCU_TMR		(TCU_BASE + 0x30) /* Timer Mask Register */
#define TCU_TMSR	(TCU_BASE + 0x34) /* Timer Mask Set Register */
#define TCU_TMCR	(TCU_BASE + 0x38) /* Timer Mask Clear Register */
#define TCU_TDFR0	(TCU_BASE + 0x40) /* Timer Data Full Register */
#define TCU_TDHR0	(TCU_BASE + 0x44) /* Timer Data Half Register */
#define TCU_TCNT0	(TCU_BASE + 0x48) /* Timer Counter Register */
#define TCU_TCSR0	(TCU_BASE + 0x4C) /* Timer Control Register */
#define TCU_TDFR1	(TCU_BASE + 0x50)
#define TCU_TDHR1	(TCU_BASE + 0x54)
#define TCU_TCNT1	(TCU_BASE + 0x58)
#define TCU_TCSR1	(TCU_BASE + 0x5C)
#define TCU_TDFR2	(TCU_BASE + 0x60)
#define TCU_TDHR2	(TCU_BASE + 0x64)
#define TCU_TCNT2	(TCU_BASE + 0x68)
#define TCU_TCSR2	(TCU_BASE + 0x6C)
#define TCU_TDFR3	(TCU_BASE + 0x70)
#define TCU_TDHR3	(TCU_BASE + 0x74)
#define TCU_TCNT3	(TCU_BASE + 0x78)
#define TCU_TCSR3	(TCU_BASE + 0x7C)
#define TCU_TDFR4	(TCU_BASE + 0x80)
#define TCU_TDHR4	(TCU_BASE + 0x84)
#define TCU_TCNT4	(TCU_BASE + 0x88)
#define TCU_TCSR4	(TCU_BASE + 0x8C)
#define TCU_TDFR5	(TCU_BASE + 0x90)
#define TCU_TDHR5	(TCU_BASE + 0x94)
#define TCU_TCNT5	(TCU_BASE + 0x98)
#define TCU_TCSR5	(TCU_BASE + 0x9C)

#define REG_TCU_TSR	REG32(TCU_TSR)
#define REG_TCU_TSSR	REG32(TCU_TSSR)
#define REG_TCU_TSCR	REG32(TCU_TSCR)
#define REG_TCU_TER	REG8(TCU_TER)
#define REG_TCU_TESR	REG8(TCU_TESR)
#define REG_TCU_TECR	REG8(TCU_TECR)
#define REG_TCU_TFR	REG32(TCU_TFR)
#define REG_TCU_TFSR	REG32(TCU_TFSR)
#define REG_TCU_TFCR	REG32(TCU_TFCR)
#define REG_TCU_TMR	REG32(TCU_TMR)
#define REG_TCU_TMSR	REG32(TCU_TMSR)
#define REG_TCU_TMCR	REG32(TCU_TMCR)
#define REG_TCU_TDFR0	REG16(TCU_TDFR0)
#define REG_TCU_TDHR0	REG16(TCU_TDHR0)
#define REG_TCU_TCNT0	REG16(TCU_TCNT0)
#define REG_TCU_TCSR0	REG16(TCU_TCSR0)
#define REG_TCU_TDFR1	REG16(TCU_TDFR1)
#define REG_TCU_TDHR1	REG16(TCU_TDHR1)
#define REG_TCU_TCNT1	REG16(TCU_TCNT1)
#define REG_TCU_TCSR1	REG16(TCU_TCSR1)
#define REG_TCU_TDFR2	REG16(TCU_TDFR2)
#define REG_TCU_TDHR2	REG16(TCU_TDHR2)
#define REG_TCU_TCNT2	REG16(TCU_TCNT2)
#define REG_TCU_TCSR2	REG16(TCU_TCSR2)
#define REG_TCU_TDFR3	REG16(TCU_TDFR3)
#define REG_TCU_TDHR3	REG16(TCU_TDHR3)
#define REG_TCU_TCNT3	REG16(TCU_TCNT3)
#define REG_TCU_TCSR3	REG16(TCU_TCSR3)
#define REG_TCU_TDFR4	REG16(TCU_TDFR4)
#define REG_TCU_TDHR4	REG16(TCU_TDHR4)
#define REG_TCU_TCNT4	REG16(TCU_TCNT4)
#define REG_TCU_TCSR4	REG16(TCU_TCSR4)

// n = 0,1,2,3,4,5
#define TCU_TDFR(n)	(TCU_BASE + (0x40 + (n)*0x10)) /* Timer Data Full Reg */
#define TCU_TDHR(n)	(TCU_BASE + (0x44 + (n)*0x10)) /* Timer Data Half Reg */
#define TCU_TCNT(n)	(TCU_BASE + (0x48 + (n)*0x10)) /* Timer Counter Reg */
#define TCU_TCSR(n)	(TCU_BASE + (0x4C + (n)*0x10)) /* Timer Control Reg */

#define REG_TCU_TDFR(n)	REG16(TCU_TDFR((n)))
#define REG_TCU_TDHR(n)	REG16(TCU_TDHR((n)))
#define REG_TCU_TCNT(n)	REG16(TCU_TCNT((n)))
#define REG_TCU_TCSR(n)	REG16(TCU_TCSR((n)))

// Register definitions
#define TCU_TCSR_PWM_SD		(1 << 9)
#define TCU_TCSR_PWM_INITL_HIGH	(1 << 8)
#define TCU_TCSR_PWM_EN		(1 << 7)
#define TCU_TCSR_PRESCALE_BIT	3
#define TCU_TCSR_PRESCALE_MASK	(0x7 << TCU_TCSR_PRESCALE_BIT)
  #define TCU_TCSR_PRESCALE1	(0x0 << TCU_TCSR_PRESCALE_BIT)
  #define TCU_TCSR_PRESCALE4	(0x1 << TCU_TCSR_PRESCALE_BIT)
  #define TCU_TCSR_PRESCALE16	(0x2 << TCU_TCSR_PRESCALE_BIT)
  #define TCU_TCSR_PRESCALE64	(0x3 << TCU_TCSR_PRESCALE_BIT)
  #define TCU_TCSR_PRESCALE256	(0x4 << TCU_TCSR_PRESCALE_BIT)
  #define TCU_TCSR_PRESCALE1024	(0x5 << TCU_TCSR_PRESCALE_BIT)
#define TCU_TCSR_EXT_EN		(1 << 2)
#define TCU_TCSR_RTC_EN		(1 << 1)
#define TCU_TCSR_PCK_EN		(1 << 0)

#define TCU_TER_TCEN5		(1 << 5)
#define TCU_TER_TCEN4		(1 << 4)
#define TCU_TER_TCEN3		(1 << 3)
#define TCU_TER_TCEN2		(1 << 2)
#define TCU_TER_TCEN1		(1 << 1)
#define TCU_TER_TCEN0		(1 << 0)

#define TCU_TESR_TCST5		(1 << 5)
#define TCU_TESR_TCST4		(1 << 4)
#define TCU_TESR_TCST3		(1 << 3)
#define TCU_TESR_TCST2		(1 << 2)
#define TCU_TESR_TCST1		(1 << 1)
#define TCU_TESR_TCST0		(1 << 0)

#define TCU_TECR_TCCL5		(1 << 5)
#define TCU_TECR_TCCL4		(1 << 4)
#define TCU_TECR_TCCL3		(1 << 3)
#define TCU_TECR_TCCL2		(1 << 2)
#define TCU_TECR_TCCL1		(1 << 1)
#define TCU_TECR_TCCL0		(1 << 0)

#define TCU_TFR_HFLAG5		(1 << 21)
#define TCU_TFR_HFLAG4		(1 << 20)
#define TCU_TFR_HFLAG3		(1 << 19)
#define TCU_TFR_HFLAG2		(1 << 18)
#define TCU_TFR_HFLAG1		(1 << 17)
#define TCU_TFR_HFLAG0		(1 << 16)
#define TCU_TFR_FFLAG5		(1 << 5)
#define TCU_TFR_FFLAG4		(1 << 4)
#define TCU_TFR_FFLAG3		(1 << 3)
#define TCU_TFR_FFLAG2		(1 << 2)
#define TCU_TFR_FFLAG1		(1 << 1)
#define TCU_TFR_FFLAG0		(1 << 0)

#define TCU_TFSR_HFLAG5		(1 << 21)
#define TCU_TFSR_HFLAG4		(1 << 20)
#define TCU_TFSR_HFLAG3		(1 << 19)
#define TCU_TFSR_HFLAG2		(1 << 18)
#define TCU_TFSR_HFLAG1		(1 << 17)
#define TCU_TFSR_HFLAG0		(1 << 16)
#define TCU_TFSR_FFLAG5		(1 << 5)
#define TCU_TFSR_FFLAG4		(1 << 4)
#define TCU_TFSR_FFLAG3		(1 << 3)
#define TCU_TFSR_FFLAG2		(1 << 2)
#define TCU_TFSR_FFLAG1		(1 << 1)
#define TCU_TFSR_FFLAG0		(1 << 0)

#define TCU_TFCR_HFLAG5		(1 << 21)
#define TCU_TFCR_HFLAG4		(1 << 20)
#define TCU_TFCR_HFLAG3		(1 << 19)
#define TCU_TFCR_HFLAG2		(1 << 18)
#define TCU_TFCR_HFLAG1		(1 << 17)
#define TCU_TFCR_HFLAG0		(1 << 16)
#define TCU_TFCR_FFLAG5		(1 << 5)
#define TCU_TFCR_FFLAG4		(1 << 4)
#define TCU_TFCR_FFLAG3		(1 << 3)
#define TCU_TFCR_FFLAG2		(1 << 2)
#define TCU_TFCR_FFLAG1		(1 << 1)
#define TCU_TFCR_FFLAG0		(1 << 0)

#define TCU_TMR_HMASK5		(1 << 21)
#define TCU_TMR_HMASK4		(1 << 20)
#define TCU_TMR_HMASK3		(1 << 19)
#define TCU_TMR_HMASK2		(1 << 18)
#define TCU_TMR_HMASK1		(1 << 17)
#define TCU_TMR_HMASK0		(1 << 16)
#define TCU_TMR_FMASK5		(1 << 5)
#define TCU_TMR_FMASK4		(1 << 4)
#define TCU_TMR_FMASK3		(1 << 3)
#define TCU_TMR_FMASK2		(1 << 2)
#define TCU_TMR_FMASK1		(1 << 1)
#define TCU_TMR_FMASK0		(1 << 0)

#define TCU_TMSR_HMST5		(1 << 21)
#define TCU_TMSR_HMST4		(1 << 20)
#define TCU_TMSR_HMST3		(1 << 19)
#define TCU_TMSR_HMST2		(1 << 18)
#define TCU_TMSR_HMST1		(1 << 17)
#define TCU_TMSR_HMST0		(1 << 16)
#define TCU_TMSR_FMST5		(1 << 5)
#define TCU_TMSR_FMST4		(1 << 4)
#define TCU_TMSR_FMST3		(1 << 3)
#define TCU_TMSR_FMST2		(1 << 2)
#define TCU_TMSR_FMST1		(1 << 1)
#define TCU_TMSR_FMST0		(1 << 0)

#define TCU_TMCR_HMCL5		(1 << 21)
#define TCU_TMCR_HMCL4		(1 << 20)
#define TCU_TMCR_HMCL3		(1 << 19)
#define TCU_TMCR_HMCL2		(1 << 18)
#define TCU_TMCR_HMCL1		(1 << 17)
#define TCU_TMCR_HMCL0		(1 << 16)
#define TCU_TMCR_FMCL5		(1 << 5)
#define TCU_TMCR_FMCL4		(1 << 4)
#define TCU_TMCR_FMCL3		(1 << 3)
#define TCU_TMCR_FMCL2		(1 << 2)
#define TCU_TMCR_FMCL1		(1 << 1)
#define TCU_TMCR_FMCL0		(1 << 0)

#define TCU_TSR_WDTS		(1 << 16)
#define TCU_TSR_STOP5		(1 << 5)
#define TCU_TSR_STOP4		(1 << 4)
#define TCU_TSR_STOP3		(1 << 3)
#define TCU_TSR_STOP2		(1 << 2)
#define TCU_TSR_STOP1		(1 << 1)
#define TCU_TSR_STOP0		(1 << 0)

#define TCU_TSSR_WDTSS		(1 << 16)
#define TCU_TSSR_STPS5		(1 << 5)
#define TCU_TSSR_STPS4		(1 << 4)
#define TCU_TSSR_STPS3		(1 << 3)
#define TCU_TSSR_STPS2		(1 << 2)
#define TCU_TSSR_STPS1		(1 << 1)
#define TCU_TSSR_STPS0		(1 << 0)

#define TCU_TSSR_WDTSC		(1 << 16)
#define TCU_TSSR_STPC5		(1 << 5)
#define TCU_TSSR_STPC4		(1 << 4)
#define TCU_TSSR_STPC3		(1 << 3)
#define TCU_TSSR_STPC2		(1 << 2)
#define TCU_TSSR_STPC1		(1 << 1)
#define TCU_TSSR_STPC0		(1 << 0)


/*************************************************************************
 * WDT (WatchDog Timer)
 *************************************************************************/
#define WDT_TDR		(WDT_BASE + 0x00)
#define WDT_TCER	(WDT_BASE + 0x04)
#define WDT_TCNT	(WDT_BASE + 0x08)
#define WDT_TCSR	(WDT_BASE + 0x0C)

#define REG_WDT_TDR	REG16(WDT_TDR)
#define REG_WDT_TCER	REG8(WDT_TCER)
#define REG_WDT_TCNT	REG16(WDT_TCNT)
#define REG_WDT_TCSR	REG16(WDT_TCSR)

// Register definition
#define WDT_TCSR_PRESCALE_BIT	3
#define WDT_TCSR_PRESCALE_MASK	(0x7 << WDT_TCSR_PRESCALE_BIT)
  #define WDT_TCSR_PRESCALE1	(0x0 << WDT_TCSR_PRESCALE_BIT)
  #define WDT_TCSR_PRESCALE4	(0x1 << WDT_TCSR_PRESCALE_BIT)
  #define WDT_TCSR_PRESCALE16	(0x2 << WDT_TCSR_PRESCALE_BIT)
  #define WDT_TCSR_PRESCALE64	(0x3 << WDT_TCSR_PRESCALE_BIT)
  #define WDT_TCSR_PRESCALE256	(0x4 << WDT_TCSR_PRESCALE_BIT)
  #define WDT_TCSR_PRESCALE1024	(0x5 << WDT_TCSR_PRESCALE_BIT)
#define WDT_TCSR_EXT_EN		(1 << 2)
#define WDT_TCSR_RTC_EN		(1 << 1)
#define WDT_TCSR_PCK_EN		(1 << 0)

#define WDT_TCER_TCEN		(1 << 0)

#endif /* __JZ4740_REGS_H__ */
