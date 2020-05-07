/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2018 Oleksij Rempel <o.rempel@pengutronix.de>, Pengutronix
 * (C) Copyright 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <asm-generic/barebox.lds.h>
#include <asm-generic/memory_layout.h>
#include <linux/sizes.h>


#define RAM_BASE	(TEXT_BASE - SZ_2M)
MEMORY { .ram : ORIGIN = RAM_BASE, LENGTH = SZ_2M }

#if (CONFIG_ARCH_TEXT_BASE != 0)
# define BASE    CONFIG_ARCH_TEXT_BASE
MEMORY { .rom : ORIGIN = BASE, LENGTH = SZ_1M }
#else
# define BASE    RAM_BASE
REGION_ALIAS(".rom", .ram)
#endif

OUTPUT_ARCH("mips")
SECTIONS
{
	. = 0;

	PRE_IMAGE

	. = ALIGN(4);
	.text      :
	{
		_stext = .;
		_text = .;
		*(.text_head_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
	} > .rom

	BAREBOX_BARE_INIT_SIZE

	. = ALIGN(4);
	.rodata : { *(.rodata*) } > .rom

	.barebox_imd : { BAREBOX_IMD } > .rom

	_etext = .;			/* End of text and rodata section */

	. = ALIGN(4);
	.data : { *(.data*) } > .rom

	pbl_code_size =  . - BASE;

	. = ALIGN(4);
	__piggydata_start = .;
	.piggydata : {
		*(.piggydata)
	} > .rom
	__piggydata_end = .;

	pbl_image_size =  . - BASE;

	pbl_memory_size =  . - BASE;
	_end = .;

	.bss (NOLOAD) : {
		. = ALIGN(4);
		__bss_start = .;
		*(.bss*)
		__bss_stop = .;
	} > .ram
}
