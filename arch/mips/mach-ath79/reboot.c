/*
 * Copyright (C) 2018 Antony Pavlov <antonynpavlov@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <memory.h>
#include <boot.h>
#include <linux/reboot.h>
#include <bootm.h>
#include "../../../lib/kexec/kexec.h"
#include <asm/io.h>

int reboot(int cmd, void *opaque)
{
	if (cmd == LINUX_REBOOT_CMD_KEXEC) {
		extern unsigned long reboot_code_buffer;
//		extern unsigned long kexec_args[4];
		void (*kexec_code_buffer)(void);
//		struct image_data *data = opaque;

		shutdown_barebox();

		kexec_code_buffer = phys_to_virt(reboot_code_buffer);

#if 0
		kexec_args[0] = 2; /* number of arguments? */
		kexec_args[1] = ENVP_ADDR;
		kexec_args[2] = ENVP_ADDR + 8;
		kexec_args[3] = 0x10000000; /* no matter */
#endif

		kexec_code_buffer();
	}

	return -1;
}
EXPORT_SYMBOL(reboot);
