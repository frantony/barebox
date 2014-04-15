/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <asm/io.h>
#include <linux/reboot.h>
#include "../../../lib/kexec/kexec.h"
#include <boot.h>

#define ENVP_ADDR	0x80002000l
#define ENVP_NB_ENTRIES	16
#define ENVP_ENTRY_SIZE	256

static int reserve_yamon_prom_env(void)
{
	request_sdram_region("yamon env",
		(unsigned long)ENVP_ADDR,
		ENVP_NB_ENTRIES * ENVP_ENTRY_SIZE);

	return 0;
}
late_initcall(reserve_yamon_prom_env);

static void prom_set(uint32_t *prom_buf, int index,
			const char *string, ...)
{
	va_list ap;
	int32_t table_addr;

	if (index >= ENVP_NB_ENTRIES)
		return;

	if (string == NULL) {
		prom_buf[index] = 0;
		return;
	}

	table_addr = sizeof(int32_t) * ENVP_NB_ENTRIES + index * ENVP_ENTRY_SIZE;
	prom_buf[index] = (ENVP_ADDR + table_addr);

	va_start(ap, string);
	vsnprintf((char *)prom_buf + table_addr, ENVP_ENTRY_SIZE, string, ap);
	va_end(ap);
}

static inline void yamon_jump(void *entry)
{
	int argnum;
	void (*kernel)(int a0, int a1, int a2, int a3);
	void *prom_buf;
	long prom_size;
	int prom_index = 0;

	kernel = entry;
	argnum = 2;

	/* Setup prom parameters. */
	prom_size = ENVP_NB_ENTRIES * (sizeof(int32_t) + ENVP_ENTRY_SIZE);
	prom_buf = (void *)ENVP_ADDR;

	prom_set(prom_buf, prom_index++, "%s", getenv("global.bootm.image"));
	prom_set(prom_buf, prom_index++, "%s", linux_bootargs_get());

	prom_set(prom_buf, prom_index++, "memsize");
	prom_set(prom_buf, prom_index++, "%i", 256 << 20);
	prom_set(prom_buf, prom_index++, "modetty0");
	prom_set(prom_buf, prom_index++, "38400n8r");
	prom_set(prom_buf, prom_index++, NULL);

	kernel(argnum,              /* number of arguments? */
		ENVP_ADDR,
		ENVP_ADDR + 8,
		0x10000000      /* no matter */
	);
}

int reboot(int cmd)
{
	if (cmd == LINUX_REBOOT_CMD_KEXEC) {
		extern unsigned long reboot_code_buffer;

		shutdown_barebox();
		yamon_jump(phys_to_virt(reboot_code_buffer));
	}

	return -1;
}
EXPORT_SYMBOL(reboot);
