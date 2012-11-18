/*
 * nano-monitor for MIPS CPU
 *
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <board/debug_ll.h>
#include <asm/debug_ll_ns16550.h>

#define CODE_ENTER	0x0d
#define CODE_ESC	0x1b
#define CODE_BACKSPACE	0x7f

	.macro	nmon_outs msg
	.set	push
	.set	noreorder

	move	t1, ra			# preserve ra beforehand
	bal	0f
	 nop

0:
	addiu	a1, ra, \msg - 0b
	move	ra, t1

	bal	_nmon_outs
	 nop

	.set	pop
	.endm


	.macro	nmon_gethexw
	.set	push
	.set	noreorder

	bal	_nmon_gethexw
	 nop

	.set	pop
	.endm


	.macro	mips_nmon
	.set	push
	.set	noreorder

#ifdef CONFIG_DEBUG_LL
#ifdef CONFIG_NMON_USER_START

#if CONFIG_NMON_USER_START_DELAY < 1
#error CONFIG_NMON_USER_START_DELAY must be >= 1!
#endif

	nmon_outs msg_nmon_press_any_key

	li	s0, CONFIG_NMON_USER_START_DELAY
	move	s1, s0

1:
	debug_ll_ns16550_outc '.'
	bnez	s1, 1b
	 addi	s1, s1, -1

	move	s1, s0

1:
	/* delay_loop */
	li	s2, 0x400000
2:
	bnez	s2, 2b
	 addi	s2, s2, -1
	/* /delay_loop */

	nmon_outs msg_bsp

	debug_ll_ns16550_check_char

	bnez	v0, 3f
	 nop

	bnez	s1, 1b
	 addi	s1, s1, -1

	nmon_outs msg_skipping_nmon

	b	nmon_exit
	 nop

msg_nmon_press_any_key:
	.asciz "\r\npress any key to start nmon\r\n"

	.align	4
3:
	/* get received char from ns16550's buffer */
	debug_ll_ns16550_getc
#endif /* CONFIG_NMON_USER_START */

nmon_main_help:
#ifdef CONFIG_NMON_HELP
	nmon_outs msg_nmon_help
#endif /* CONFIG_NMON_HELP */

nmon_main:
	nmon_outs msg_prompt

	debug_ll_ns16550_getc

	move a0, v0 /* prepare a0 for debug_ll_ns16550_outc_a0 */

	li	v1, 'q'
	bne	v0, v1, 3f
	 nop

	debug_ll_ns16550_outc_a0

	b	nmon_exit
	 nop

3:
	li	v1, 'd'
	beq	v0, v1, nmon_cmd_d
	 nop

	li	v1, 'w'
	beq	v0, v1, nmon_cmd_w
	 nop

	li	v1, 'g'
	beq	v0, v1, nmon_cmd_g
	 nop

	b	nmon_main_help
	 nop

nmon_cmd_d:
	debug_ll_ns16550_outc_a0

	debug_ll_ns16550_outc ' '

	nmon_gethexw

	nmon_outs msg_nl

	lw	a0, (v0)
	debug_ll_ns16550_outhexw

	b	nmon_main
	 nop

nmon_cmd_w:
	debug_ll_ns16550_outc_a0

	debug_ll_ns16550_outc ' '
	nmon_gethexw
	move s0, v0

	debug_ll_ns16550_outc ' '
	nmon_gethexw

	b	nmon_main
	 sw	v0, (s0)

nmon_cmd_g:
	debug_ll_ns16550_outc_a0

	debug_ll_ns16550_outc ' '

	nmon_gethexw

	nmon_outs msg_nl

	jal	v0
	 nop
	b	nmon_main
	 nop

_nmon_outs:
	lbu	a0, 0(a1)
	addi	a1, a1, 1
	beqz	a0, _nmon_jr_ra_exit
	 nop

	debug_ll_ns16550_outc_a0

	b	_nmon_outs
	 nop

_nmon_gethexw:

	li	t3, 8
	li	t2, 0

_get_hex_digit:
	debug_ll_ns16550_getc

	li	v1, CODE_ESC
	beq	v0, v1, nmon_main
	 nop

	li	v1, '0'
	bge	v0, v1, 0f
	 nop
	b	_get_hex_digit
	 nop

0:
	li	v1, '9'
	ble	v0, v1, 9f
	 nop

	li	v1, 'f'
	ble	v0, v1, 1f
	 nop
	b	_get_hex_digit
	 nop

1:
	li	v1, 'a'
	bge	v0, v1, 8f
	 nop

	b	_get_hex_digit
	 nop

8: /* v0 \in {'a', 'b' ... 'f'} */
	sub	a3, v0, v1
	b	0f  /* FIXME: can we drop branch? */
	 addi	a3, 0xa

9: /* v0 \in {'0', '1' ... '9'} */
	li	a3, '0'
	sub	a3, v0, a3

0:	move a0, v0
	debug_ll_ns16550_outc_a0

	sll	t2, t2, 4
	or	t2, t2, a3
	sub	t3, t3, 1

	beqz	t3, 0f
	 nop

	b	_get_hex_digit
	 nop

0:
	move	v0, t2

_nmon_jr_ra_exit:
	jr	ra
	 nop

msg_prompt:
	.asciz "\r\nnmon> "

msg_nl:
	.asciz "\r\n"

msg_bsp:
	.asciz "\b \b"

msg_skipping_nmon:
	.asciz "skipping nmon..."

#ifdef CONFIG_NMON_HELP
msg_nmon_help:
	.ascii "\r\n\r\nnmon commands:\r\n"
	.ascii " q - quit\r\n"
	.ascii " d <addr> - read 32-bit word from addr\r\n"
	.ascii " w <addr> <val> - write 32-bit word to addr\r\n"
	.ascii " g <addr> - jump to <addr>\r\n"
	.asciz "   use <ESC> key to interrupt current command\r\n"
#endif /* CONFIG_NMON_HELP */

	.align	4

nmon_exit:

	nmon_outs msg_nl

#endif /* CONFIG_DEBUG_LL */
	.set	pop
	.endm
