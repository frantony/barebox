// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_ezzy_4_start[];

void __naked barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_ezzy_4_start + get_runtime_offset();

	barebox_arm_entry(0x14000000, SZ_4M, fdt);
}
