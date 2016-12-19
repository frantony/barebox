/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
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

#define UART_RX_OFFSET 0
#define UART_TX_OFFSET 0
#define UART_TX_COUNT_OFFSET 0x4
#define UART_RX_COUNT_OFFSET 0x8
#define UART_DIVIDER_OFFSET  0xC

static inline uint32_t sifive_serial_readl(struct console_device *cdev,
						uint32_t offset)
{
	void __iomem *base = cdev->dev->priv;

	return readl(base + offset);
}

static inline void sifive_serial_writel(struct console_device *cdev,
					uint32_t value, uint32_t offset)
{
	void __iomem *base = cdev->dev->priv;

	writel(value, base + offset);
}

static int sifive_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	/* FIXME: no baudrate setup at the momement :( */

	return 0;
}

static void sifive_serial_putc(struct console_device *cdev, char c)
{
//	while (sifive_serial_readl(cdev, UART_TX_COUNT_OFFSET) > 0)
//		;

	sifive_serial_writel(cdev, c, UART_TX_OFFSET);
}

static int sifive_serial_getc(struct console_device *cdev)
{
	uint32_t rxcnt;

	do {
		rxcnt = sifive_serial_readl(cdev, UART_RX_COUNT_OFFSET);
	} while (!rxcnt);

	return sifive_serial_readl(cdev, UART_RX_OFFSET);
}

static int sifive_serial_tstc(struct console_device *cdev)
{
	uint32_t rxcnt = sifive_serial_readl(cdev, UART_RX_COUNT_OFFSET);

	return (rxcnt != 0);
}

static int sifive_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	dev->priv = IOMEM(iores->start);
	cdev->dev = dev;
	cdev->tstc = &sifive_serial_tstc;
	cdev->putc = &sifive_serial_putc;
	cdev->getc = &sifive_serial_getc;
	cdev->setbrg = &sifive_serial_setbaudrate;

	console_register(cdev);

	return 0;
}

static __maybe_unused struct of_device_id sifive_serial_dt_ids[] = {
	{
		.compatible = "sifive,uart",
	}, {
		/* sentinel */
	}
};

static struct driver_d sifive_serial_driver = {
	.name  = "sifive-uart",
	.probe = sifive_serial_probe,
	.of_compatible = DRV_OF_COMPAT(sifive_serial_dt_ids),
};
console_platform_driver(sifive_serial_driver);
