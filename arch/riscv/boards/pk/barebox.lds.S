#include <asm-generic/barebox.lds.h>

SECTIONS
{
	_stext = .;
	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*) }

	. = ALIGN(64);
	__barebox_initcalls_start = .;
	__barebox_initcalls : { INITCALLS }
	__barebox_initcalls_end = .;

	. = ALIGN(64);
	__barebox_exitcalls_start = .;
	__barebox_exitcalls : { EXITCALLS }
	__barebox_exitcalls_end = .;

	. = ALIGN(64);
	__barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	__barebox_magicvar_end = .;

	. = ALIGN(64);
	__barebox_cmd_start = .;
	__barebox_cmd : { BAREBOX_CMDS }
	__barebox_cmd_end = .;

	_edata = .;
	. = ALIGN(8);
	__bss_start = .;
	.bss : { *(.bss*) }
	__bss_stop = .;
	_end = .;
}

INSERT BEFORE .rodata;
