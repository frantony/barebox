/*
 * based on linux.git/drivers/tty/serial/serial_ar933x.c
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>

#define AR933X_UART_DATA_REG            0x00
#define AR933X_UART_DATA_TX_RX_MASK     0xff
#define AR933X_UART_DATA_RX_CSR         BIT(8)
#define AR933X_UART_DATA_TX_CSR         BIT(9)

static inline void ar933x_serial_writel(struct console_device *cdev,
	u32 b, int offset)
{
	void *serial_base = cdev->dev->priv;

	cpu_writel(b, serial_base + offset);
}

static inline u32 ar933x_serial_readl(struct console_device *cdev,
	int offset)
{
	void *serial_base = cdev->dev->priv;

	return cpu_readl(serial_base + offset);
}

static int ar933x_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	/* FIXME: empty */

	return 0;
}

static void ar933x_serial_putc(struct console_device *cdev, char ch)
{
	u32 data;

	/* wait transmitter ready */
	data = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);
	while (!(data & AR933X_UART_DATA_TX_CSR)) {
		data = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);
	}

	data = (ch & AR933X_UART_DATA_TX_RX_MASK) | AR933X_UART_DATA_TX_CSR;
	ar933x_serial_writel(cdev, data, AR933X_UART_DATA_REG);
}

static int ar933x_serial_tstc(struct console_device *cdev)
{
	u32 rdata;

	rdata = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);

	return rdata & AR933X_UART_DATA_RX_CSR;
}

static int ar933x_serial_getc(struct console_device *cdev)
{
	u32 rdata;

	while (!ar933x_serial_tstc(cdev))
		;

	rdata = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);

	/* remove the character from the FIFO */
	ar933x_serial_writel(cdev, AR933X_UART_DATA_RX_CSR,
		AR933X_UART_DATA_REG);

	return rdata & AR933X_UART_DATA_TX_RX_MASK;
}

static int ar933x_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	dev->priv = dev_request_mem_region(dev, 0);
	cdev->dev = dev;
	cdev->tstc = ar933x_serial_tstc;
	cdev->putc = ar933x_serial_putc;
	cdev->getc = ar933x_serial_getc;
	cdev->setbrg = ar933x_serial_setbaudrate;

	/* FIXME: need ar933x_serial_init_port(cdev); */

	console_register(cdev);

	return 0;
}

static struct driver_d ar933x_serial_driver = {
	.name  = "ar933x_serial",
	.probe = ar933x_serial_probe,
};
console_platform_driver(ar933x_serial_driver);
