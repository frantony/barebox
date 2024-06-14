// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * Based on analogous driver from U-Boot
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <notifier.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <serial/lpuart.h>

struct lpuart {
	struct console_device cdev;
	int baudrate;
	int dte_mode;
	struct notifier_block notify;
	struct resource *io;
	void __iomem *base;
	struct clk *clk;
};

static struct lpuart *cdev_to_lpuart(struct console_device *cdev)
{
	return container_of(cdev, struct lpuart, cdev);
}

static struct lpuart *nb_to_lpuart(struct notifier_block *nb)
{
	return container_of(nb, struct lpuart, notify);
}

static void lpuart_enable(struct lpuart *lpuart, bool on)
{
	u8 ctrl;

	ctrl = readb(lpuart->base + UARTCR2);
	if (on)
		ctrl |= UARTCR2_TE | UARTCR2_RE;
	else
		ctrl &= ~(UARTCR2_TE | UARTCR2_RE);
	writeb(ctrl, lpuart->base + UARTCR2);
}

static int lpuart_serial_setbaudrate(struct console_device *cdev,
				     int baudrate)
{
	struct lpuart *lpuart = cdev_to_lpuart(cdev);

	lpuart_enable(lpuart, false);

	/*
	 * We treat baudrate of 0 as a request to disable UART
	 */
	if (baudrate) {
		lpuart_setbrg(lpuart->base, clk_get_rate(lpuart->clk),
			      baudrate);
		lpuart_enable(lpuart, true);
	}

	lpuart->baudrate = baudrate;

	return 0;
}

static int lpuart_serial_getc(struct console_device *cdev)
{
	bool ready;
	struct lpuart *lpuart = cdev_to_lpuart(cdev);

	do {
		const u8 sr1 = readb(lpuart->base + UARTSR1);
		ready = !!(sr1 & (UARTSR1_OR | UARTSR1_RDRF));
	} while (!ready);

	return readb(lpuart->base + UARTDR);
}

static void lpuart_serial_putc(struct console_device *cdev, char c)
{
	lpuart_putc(cdev_to_lpuart(cdev)->base, c);
}

/* Test whether a character is in the RX buffer */
static int lpuart_serial_tstc(struct console_device *cdev)
{
	return !!readb(cdev_to_lpuart(cdev)->base + UARTRCFIFO);
}

static void lpuart_serial_flush(struct console_device *cdev)
{
	bool tx_empty;
	struct lpuart *lpuart = cdev_to_lpuart(cdev);

	do {
		const u8 sr1 = readb(lpuart->base + UARTSR1);
		tx_empty = !!(sr1 & UARTSR1_TDRE);
	} while (!tx_empty);
}

static int lpuart_clocksource_clock_change(struct notifier_block *nb,
					   unsigned long event, void *data)
{
	struct lpuart *lpuart = nb_to_lpuart(nb);

	return lpuart_serial_setbaudrate(&lpuart->cdev, lpuart->baudrate);
}

static int lpuart_serial_probe(struct device *dev)
{
	int ret;
	struct console_device *cdev;
	struct lpuart *lpuart;
	const char *devname;

	lpuart    = xzalloc(sizeof(*lpuart));
	cdev      = &lpuart->cdev;
	dev->priv = lpuart;

	lpuart->io = dev_request_mem_resource(dev, 0);
	if (IS_ERR(lpuart->io)) {
		ret = PTR_ERR(lpuart->io);
		goto err_free;
	}
	lpuart->base = IOMEM(lpuart->io->start);

	lpuart->clk = clk_get_for_console(dev, NULL);
	if (IS_ERR(lpuart->clk)) {
		ret = PTR_ERR(lpuart->clk);
		dev_err(dev, "Failed to get UART clock %d\n", ret);
		goto io_release;
	}

	ret = clk_enable(lpuart->clk);
	if (ret) {
		dev_err(dev, "Failed to enable UART clock %d\n", ret);
		goto io_release;
	}

	cdev->dev    = dev;
	cdev->tstc   = lpuart_serial_tstc;
	cdev->putc   = lpuart_serial_putc;
	cdev->getc   = lpuart_serial_getc;
	cdev->flush  = lpuart_serial_flush;
	cdev->setbrg = lpuart->clk ? lpuart_serial_setbaudrate : NULL;

	if (dev->of_node) {
		devname = of_alias_get(dev->of_node);
		if (devname) {
			cdev->devname = xstrdup(devname);
			cdev->devid   = DEVICE_ID_SINGLE;
		}
	}

	cdev->linux_console_name = "ttyLP";
	cdev->linux_earlycon_name = "lpuart";
	cdev->phys_base = lpuart->base;

	if (lpuart->clk)
		lpuart_setup(lpuart->base, clk_get_rate(lpuart->clk));

	ret = console_register(cdev);
	if (!ret) {
		if (lpuart->clk) {
			lpuart->notify.notifier_call = lpuart_clocksource_clock_change;
			clock_register_client(&lpuart->notify);
		}

		return 0;
	}

	clk_put(lpuart->clk);
io_release:
	release_region(lpuart->io);
err_free:
	free(lpuart);

	return ret;
}

static struct of_device_id lpuart_serial_dt_ids[] = {
	{ .compatible = "fsl,vf610-lpuart" },
	{}
};
MODULE_DEVICE_TABLE(of, lpuart_serial_dt_ids);

static struct driver lpuart_serial_driver = {
	.name   = "lpuart-serial",
	.probe  = lpuart_serial_probe,
	.of_compatible = DRV_OF_COMPAT(lpuart_serial_dt_ids),
};
console_platform_driver(lpuart_serial_driver);
