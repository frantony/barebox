/*
 * Copyright (C) 2009-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
  */

#define __LOWLEVEL_INIT__

#include <common.h>
#include <asm/system.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <init.h>
#include <sizes.h>

#include <mach/debug_ll.h>

#define DAVINCI_SRAM_BASE 0x82000000
#define DAVINCI_SRAM_SIZE SZ_16M
void __naked __bare_init barebox_arm_reset_vector(void)
{
	//arm_cpu_lowlevel_init();
	PUTC_LL('A');

	//arm_setup_stack(DAVINCI_SRAM_BASE + DAVINCI_SRAM_SIZE - 16);

	barebox_arm_entry(DAVINCI_SRAM_BASE, DAVINCI_SRAM_SIZE, 0);
}
