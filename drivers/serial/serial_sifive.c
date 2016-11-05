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
#include <kfifo.h>

#define UART_TXR	0x00
#define UART_RXR	0x04
#define UART_TXC	0x08
# define UART_TXEN	0x1
#define UART_RXC	0x0c
# define UART_RXEN	0x1
#define UART_DIV	0x18

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
	/* FIXME: no baudrate setup at the moment :( */

	return 0;
}

#define BUFFER_SIZE	16

static struct kfifo *rxfifo;

static void sifive_serial_putc(struct console_device *cdev, char c)
{
	while (sifive_serial_readl(cdev, UART_TXR) & 0x80000000)
		;

	sifive_serial_writel(cdev, c, UART_TXR);
}

static int sifive_serial_tstc(struct console_device *cdev)
{
	uint32_t t;

	if (kfifo_len(rxfifo)) {
		return 1;
	}

	t = sifive_serial_readl(cdev, UART_RXR);

	if (t & 0x80000000) {
		return 0;
	}

	kfifo_putc(rxfifo, t);

	return 1;
}

static int sifive_serial_getc(struct console_device *cdev)
{
	unsigned char ch;

	if (!kfifo_len(rxfifo)) {
		return -1;
	}

	kfifo_getc(rxfifo, &ch);

	return ch;
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
	cdev->putc = &sifive_serial_putc;
	cdev->tstc = &sifive_serial_tstc;
	cdev->getc = &sifive_serial_getc;
	cdev->setbrg = &sifive_serial_setbaudrate;

	rxfifo = kfifo_alloc(BUFFER_SIZE);

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
