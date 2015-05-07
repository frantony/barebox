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

static int model_hostname_init(void)
{
	barebox_set_hostname("mr3020");

	return 0;
}
postcore_initcall(model_hostname_init);

#include <asm/addrspace.h>
#include <mach/ath79.h>
#include <usb/ehci.h>

#define AR933X_RESET_REG_RESET_MODULE           0x1c
#define AR933X_RESET_USB_HOST           BIT(5)
#define AR933X_RESET_USB_PHY            BIT(4)
#define AR933X_RESET_USBSUS_OVERRIDE    BIT(3)

#define AR933X_EHCI_BASE	0x1b000000
#define AR933X_EHCI_SIZE	0x1000

static void ath79_device_reset_set(u32 mask)
{
	u32 reg;
	u32 t;

	reg = AR933X_RESET_REG_RESET_MODULE;
	t = ath79_reset_rr(reg);
	ath79_reset_wr(reg, t | mask);
}

static void ath79_device_reset_clear(u32 mask)
{
	u32 reg;
	u32 t;

	reg = AR933X_RESET_REG_RESET_MODULE;
	t = ath79_reset_rr(reg);
	ath79_reset_wr(reg, t & ~mask);
}

static struct ehci_platform_data ehci_pdata = {
	.flags = EHCI_HAS_TT | EHCI_BE_MMIO,
};

static int ehci_init(void)
{
	ath79_device_reset_set(AR933X_RESET_USBSUS_OVERRIDE);
	mdelay(10); mdelay(10); mdelay(10);

	ath79_device_reset_clear(AR933X_RESET_USB_HOST);
	mdelay(10); mdelay(10); mdelay(10);

	ath79_device_reset_clear(AR933X_RESET_USB_PHY);
	mdelay(10); mdelay(10); mdelay(10);

	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC,
			KSEG1ADDR(AR933X_EHCI_BASE),
			&ehci_pdata);

	return 0;
}
device_initcall(ehci_init);
