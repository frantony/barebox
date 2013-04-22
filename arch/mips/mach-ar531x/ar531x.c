/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003 Atheros Communications, Inc.,  All Rights Reserved.
 * Copyright (C) 2006 FON Technology, SL.
 * Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 * Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 */

/*
 * Platform devices for Atheros SoCs
 */


#include <common.h>
#include <io.h>
#include <mach/ar531x_regs.h>
#include <mach/ar231x_platform.h>
#include <mach/debug_ll.h>
#include <ns16550.h>
#include <init.h>
#include <driver.h>
#include <linux/types.h>

struct ar231x_board_data ar231x_board;

void __noreturn reset_cpu(ulong addr)
{
	printf("reseting cpu\n");
	__raw_writel(0x10000,
			(char *)KSEG1ADDR(AR531X_WD_TIMER));
	__raw_writel(AR531X_WD_CTRL_RESET,
			(char *)KSEG1ADDR(AR531X_WD_CTRL));
	while(1);
}
EXPORT_SYMBOL(reset_cpu);

/*
 * This table is indexed by bits 5..4 of the CLOCKCTL1 register
 * to determine the predevisor value.
 */
static int CLOCKCTL1_PREDIVIDE_TABLE[4] = { 1, 2, 4, 5 };


static unsigned int
ar5312_cpu_frequency(void)
{
	unsigned int predivide_mask, predivide_shift;
	unsigned int multiplier_mask, multiplier_shift;
	unsigned int clockCtl1, preDivideSelect, preDivisor, multiplier;
	unsigned int doubler_mask;
	u32 devid;

	devid = __raw_readl((char *)KSEG1ADDR(AR531X_REV));
	devid &= AR531X_REV_MAJ;
	devid >>= AR531X_REV_MAJ_S;
	if (devid == AR531X_REV_MAJ_AR2313) {
		predivide_mask = AR2313_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2313_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2313_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2313_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2313_CLOCKCTL1_DOUBLER_MASK;
	} else { /* AR5312 and AR2312 */
		predivide_mask = AR2312_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2312_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2312_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2312_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2312_CLOCKCTL1_DOUBLER_MASK;
	}


	/*
	 * Clocking is derived from a fixed 40MHz input clock.
	 *
	 *  cpuFreq = InputClock * MULT (where MULT is PLL multiplier)
	 *  sysFreq = cpuFreq / 4	   (used for APB clock, serial,
	 *							   flash, Timer, Watchdog Timer)
	 *
	 *  cntFreq = cpuFreq / 2	   (use for CPU count/compare)
	 *
	 * So, for example, with a PLL multiplier of 5, we have
	 *
	 *  cpuFreq = 200MHz
	 *  sysFreq = 50MHz
	 *  cntFreq = 100MHz
	 *
	 * We compute the CPU frequency, based on PLL settings.
	 */

	clockCtl1 = __raw_readl((char *)KSEG1ADDR(AR5312_CLOCKCTL1));
	preDivideSelect = (clockCtl1 & predivide_mask) >> predivide_shift;
	preDivisor = CLOCKCTL1_PREDIVIDE_TABLE[preDivideSelect];
	multiplier = (clockCtl1 & multiplier_mask) >> multiplier_shift;

	if (clockCtl1 & doubler_mask) {
		multiplier = multiplier << 1;
	}
	return (40000000 / preDivisor) * multiplier;
}

static unsigned int
ar5312_sys_frequency(void)
{
	return ar5312_cpu_frequency() / 4;
}

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	printf("Disable watchdog.\n");
	__raw_writeb(AR531X_WD_CTRL_IGNORE_EXPIRATION,
					(char *)KSEG1ADDR(AR531X_WD_CTRL));
        return 0;
}

static void flash_init(void)
{
	u32 flash_size;
	u32 ctl, old_ctl;

    /*
     * Configure flash bank 0.
     * Assume 8M window size. Flash will be aliased if it's smaller
     */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR531X_FLASHCTL0));
	ctl = FLASHCTL_E | FLASHCTL_AC_8M | FLASHCTL_RBLE |
			(0x01 << FLASHCTL_IDCY_S) |
			(0x07 << FLASHCTL_WST1_S) |
			(0x07 << FLASHCTL_WST2_S) | (old_ctl & FLASHCTL_MW);

	__raw_writel(ctl, (char *)KSEG1ADDR(AR531X_FLASHCTL0));
	/* Disable other flash banks */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR531X_FLASHCTL1));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR531X_FLASHCTL1));

	old_ctl = __raw_readl((char *)KSEG1ADDR(AR531X_FLASHCTL2));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR531X_FLASHCTL2));

	/* TODO: precompile option for different sices */
	/* find out the flash size */
	//__raw_writel(0x000d3ce1, (char *)KSEG1ADDR(AR531X_FLASHCTL));
	//__raw_readl((char *)KSEG1ADDR(AR531X_FLASHCTL));
	flash_size = 0x400000;
	/* check the bus width? cfi routine seems to detect it automatically */
	ar231x_find_config((char *)KSEG1ADDR(AR531X_FLASH + flash_size));
//	add_cfi_flash_device(0, KSEG1ADDR(AR531X_FLASH), flash_size, 0);
}

static int ether_init(void)
{
	struct ar231x_eth_platform_data *eth = &ar231x_board.eth_pdata;
	//TODO: check board config to find which eth card we should enable
	//now we will take some cart
	eth->base_eth = KSEG1ADDR(AR531X_ENET1);
	eth->reset_mac = AR531X_RESET_ENET1;
	eth->reset_phy = AR531X_RESET_EPHY0;
	eth->base_reset = KSEG1ADDR(AR531X_RESET);
	eth->base_phy = KSEG1ADDR(AR531X_ENET0);
	eth->mac = ar231x_board.config->enet0_mac;

	// and some base addresses here
	add_generic_device("ar231x_eth", 0, NULL,
		(resource_size_t)KSEG1ADDR(AR531X_ENET0), 0x1000, IORESOURCE_MEM, eth);
	return 0;
}

static int platform_init(void)
{
	watchdog_init();
	flash_init();
	ether_init();
	return 0;
}
late_initcall(platform_init);

static struct NS16550_plat serial_plat = {
	.shift = AR531X_UART_SHIFT,
};

static int ar531x_console_init(void)
{
	u32 reset;

	/* reset UART0 */
	reset = __raw_readl((char *)KSEG1ADDR(AR531X_RESET));
	reset = ((reset & ~AR531X_RESET_APB) | AR531X_RESET_UART0);
	__raw_writel(reset, (char *)KSEG1ADDR(AR531X_RESET));

	reset &= ~AR531X_RESET_UART0;
	__raw_writel(reset, (char *)KSEG1ADDR(AR531X_RESET));

	/* Register the serial port */
	serial_plat.clock = ar5312_sys_frequency();
	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(AR531X_UART0),
		8 << AR531X_UART_SHIFT, IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}
console_initcall(ar531x_console_init);
