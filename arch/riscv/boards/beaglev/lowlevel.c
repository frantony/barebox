// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/barebox-riscv.h>
#include <debug_ll.h>
#include <asm/riscv_nmon.h>

ENTRY_FUNCTION(start_beaglev, a0, a1, a2)
{
	extern char __dtb_z_beaglev_start[];

	debug_ll_init();
	barebox_nmon_entry();
	putc_ll('>');

	barebox_riscv_entry(0x80000000, SZ_128M,
			    __dtb_z_beaglev_start + get_runtime_offset());
}
