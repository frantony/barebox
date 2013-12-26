#ifndef __ASM_MACH_AR9331_PBL_MACROS_H
#define __ASM_MACH_AR9331_PBL_MACROS_H

#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <mach/ar71xx_regs.h>

/* replicated define because linux/bitops.h cannot be included in assembly */
#define BIT(nr)  (1 << (nr))

#define	PLL_CPU_CONFIG_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_CPU_CONFIG_REG)
#define PLL_CPU_CONFIG2_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_CPU_CONFIG2_REG)
#define PLL_CLOCK_CTRL_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_CLOCK_CTRL_REG)
#define PLL_DITHER_FRAC_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_DITHER_FRAC_REG)
#define PLL_DITHER_REG		(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_DITHER_REG)

#define DEF_25MHZ_PLL_CLOCK_CTRL \
				((2 - 1) << AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT \
				| (1 - 1) << AR933X_PLL_CLOCK_CTRL_DDR_DIV_SHIFT \
				| (1 - 1) << AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT)
#define DEF_25MHZ_SETTLE_TIME	(34000 / 40)
#define DEF_25MHZ_PLL_CONFIG	( 1 << AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT \
				| 1 << AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT \
				| 32 << AR933X_PLL_CPU_CONFIG_NINT_SHIFT)

.macro	pbl_ar9331_pll
	.set	push
	.set	noreorder

	/* 25MHz config */
	pbl_reg_writel (DEF_25MHZ_PLL_CLOCK_CTRL | AR933X_PLL_CLOCK_CTRL_BYPASS), \
		PLL_CLOCK_CTRL_REG
	pbl_reg_writel DEF_25MHZ_SETTLE_TIME, PLL_CPU_CONFIG2_REG
	pbl_reg_writel (DEF_25MHZ_PLL_CONFIG | AR933X_PLL_CPU_CONFIG_PLLPWD), \
		PLL_CPU_CONFIG_REG

	/* power on CPU PLL */
	pbl_reg_clr	AR933X_PLL_CPU_CONFIG_PLLPWD, PLL_CPU_CONFIG_REG
	/* disable PLL bypass */
	pbl_reg_clr	AR933X_PLL_CLOCK_CTRL_BYPASS, PLL_CLOCK_CTRL_REG

	pbl_sleep	t2, 40

	.set	pop
.endm

#define DDR_BASE		(KSEG1 | AR71XX_DDR_CTRL_BASE)
#define	DDR_REG_CONFIG		(DDR_BASE | AR933X_DDR_REG_CONFIG)
#define	DDR_REG_CONFIG2		(DDR_BASE | AR933X_DDR_REG_CONFIG2)
#define	DDR_REG_MODE		(DDR_BASE | AR933X_DDR_REG_MODE)
#define	DDR_REG_MODE_EXT	(DDR_BASE | AR933X_DDR_REG_MODE_EXT)
#define	DDR_REG_CTRL		(DDR_BASE | AR933X_DDR_REG_CTRL)
#define	DDR_REG_REFRESH		(DDR_BASE | AR933X_DDR_REG_REFRESH)
#define	DDR_REG_RD_DAT		(DDR_BASE | AR933X_DDR_REG_RD_DAT)
#define	DDR_REG_TAP_CTRL0	(DDR_BASE | AR933X_DDR_REG_TAP_CTRL0)
#define	DDR_REG_TAP_CTRL1	(DDR_BASE | AR933X_DDR_REG_TAP_CTRL1)

#define DDR_CTRL_EMR3		BIT(5)
#define DDR_CTRL_EMR2		BIT(4)
#define DDR_CTRL_PREA		BIT(3) /* Forces a PRECHARGE ALL cycle */
#define DDR_CTRL_REF		BIT(2) /* Forces an AUTO REFRESH cycle */
#define DDR_CTRL_EMRS		BIT(1)
#define DDR_CTRL_MRS		BIT(0)

.macro	pbl_ar9331_ram
	.set	push
	.set	noreorder

	pbl_reg_writel	0x7fbc8cd0, DDR_REG_CONFIG
	pbl_reg_writel	0x9dd0e6a8, DDR_REG_CONFIG2

	pbl_reg_writel	0x8, DDR_REG_CTRL
	pbl_reg_writel	0x133, DDR_REG_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_REG_CTRL
	pbl_reg_writel	0x2, DDR_REG_MODE_EXT

	pbl_reg_writel	DDR_CTRL_EMRS, DDR_REG_CTRL
	pbl_reg_writel	DDR_CTRL_PREA, DDR_REG_CTRL
	pbl_reg_writel	0x33, DDR_REG_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_REG_CTRL
	pbl_reg_writel	0x4186, DDR_REG_REFRESH
	pbl_reg_writel	0x8, DDR_REG_TAP_CTRL0

	pbl_reg_writel	0x9, DDR_REG_TAP_CTRL1
	pbl_reg_writel	0xff, DDR_REG_RD_DAT

	.set	pop
.endm

#endif /* __ASM_MACH_AR9331_PBL_MACROS_H */
