/*
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

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <io.h>

/* Serial interface registers offsets */
#define UART_TX	0x0
#define UART_RX	0x4
#define UART_ST	0x14
 #define UART_ST_RX_RDY	1
 #define UART_ST_TX_RDY	2

/*
 * This driver is based on the "Serial terminal" docs here:
 *  http://magiclantern.wikia.com/wiki/Register_Map#Misc_Registers
 *
 * See also disassembler output for Canon A1100IS firmware
 * (version a1100_100c):
 *   * a outc-like function can be found at address 0xffff18f0;
 *   * a getc-like function can be found at address 0xffff192c.
 */

static inline uint32_t digic_serial_readl(struct console_device *cdev, uint32_t off)
{
	void *digic = cdev->dev->priv;

	return readl(digic + off);
}

static inline void digic_serial_writel(struct console_device *cdev, uint32_t val,
			  uint32_t off)
{
	void *digic = cdev->dev->priv;

	writel(val, digic + off);
}

static int digic_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	/* FIXME: empty */

	return 0;
}

static void digic_serial_putc(struct console_device *cdev, char c)
{
	while (!(digic_serial_readl(cdev, UART_ST) & UART_ST_TX_RDY))
		; /* noop */

	digic_serial_writel(cdev, 0x06, UART_ST);
	digic_serial_writel(cdev, c, UART_TX);
}

static int digic_serial_getc(struct console_device *cdev)
{
	while (!(digic_serial_readl(cdev, UART_ST) & UART_ST_RX_RDY))
		; /* noop */

	digic_serial_writel(cdev, 0x01, UART_ST);

	return digic_serial_readl(cdev, UART_RX);
}

static int digic_serial_tstc(struct console_device *cdev)
{
	return ((digic_serial_readl(cdev, UART_ST) & UART_ST_RX_RDY) != 0);

	/*
	 * Canon folks use additional check, something like this:
	 *
	 *		if (digic_serial_readl(cdev, UART_ST) & 0x38) {
	 *			digic_serial_writel(cdev, 0x38, UART_ST);
	 *			return 0;
	 *		}
	 *
	 * But I know nothing about these magic bits in the status regster...
	 *
	*/
}

static int digic_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	dev->priv = dev_request_mem_region(dev, 0);
	cdev->dev = dev;
	cdev->f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
	cdev->tstc = &digic_serial_tstc;
	cdev->putc = &digic_serial_putc;
	cdev->getc = &digic_serial_getc;
	cdev->setbrg = &digic_serial_setbaudrate;

	/* FIXME: need digic_init_port(cdev); */

	console_register(cdev);

	return 0;
}

static __maybe_unused struct of_device_id digic_serial_dt_ids[] = {
	{
		.compatible = "canon,digic-serial",
	}, {
		/* sentinel */
	}
};

static struct driver_d digic_serial_driver = {
	.name  = "digic-serial",
	.probe = digic_serial_probe,
	.of_compatible = DRV_OF_COMPAT(digic_serial_dt_ids),
};
console_platform_driver(digic_serial_driver);
