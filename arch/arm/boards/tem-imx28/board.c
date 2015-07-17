/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <kernel@pengutronix.de>
 * Copyright (C) 2011 Marc Kleine-Budde, Pengutronix <mkl@pengutronix.de>
 * Copyright (C) 2011 Wolfram Sang, Pengutronix <w.sang@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <environment.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <envfs.h>

#include <mach/clock.h>
#include <mach/imx-regs.h>
#include <mach/iomux-imx28.h>
#include <mach/iomux.h>
#include <mach/ocotp.h>
#include <mach/devices.h>
#include <bbu.h>
#include <usb/fsl_usb2.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

static void tem_get_ethaddr(void)
{
	u8 mac_ocotp[3], mac[6];
	int ret;

	ret = mxs_ocotp_read(mac_ocotp, 3, 0);
	if (ret != 3) {
		pr_err("Reading MAC from OCOTP failed!\n");
		return;
	}

	mac[0] = 0x00;
	mac[1] = 0x23;
	mac[2] = 0x43;
	mac[3] = mac_ocotp[2];
	mac[4] = mac_ocotp[1];
	mac[5] = mac_ocotp[0];

	eth_register_ethaddr(0, mac);
}

static int tem_devices_init(void)
{
	tem_get_ethaddr(); /* must be after registering ocotp */

	return 0;
}
fs_initcall(tem_devices_init);

static int tem_console_init(void)
{
	barebox_set_model("TEM i.MX28");
	barebox_set_hostname("tem");

	imx28_bbu_nand_register_handler("nand", BBU_HANDLER_FLAG_DEFAULT);

	defaultenv_append_directory(defaultenv_tem_imx28);

	return 0;
}
console_initcall(tem_console_init);
