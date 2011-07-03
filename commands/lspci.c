/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <init.h>
#include <linux/pci.h>

#include <command.h>

extern struct pci_bus *pci_root;

static int do_lspci(struct command *cmdtp, int argc, char *argv[])
{
	struct pci_dev *dev;

	if (!pci_present()) {
		printf("no pci!\n");
		return 1;
	}

	list_for_each_entry(dev, &pci_root->devices, bus_list) {
		printf("%02x: %04x: %04x:%04x (rev %02x)\n",
			      dev->devfn,
			      (dev->class >> 8) & 0xffff,
			      dev->vendor,
			      dev->device,
			      dev->revision);
	}

	return 0;
}

BAREBOX_CMD_START(lspci)
	.cmd            = do_lspci,
	.usage          = "Show PCI info",
BAREBOX_CMD_END
