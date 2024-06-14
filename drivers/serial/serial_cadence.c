// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (c) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <malloc.h>
#include <notifier.h>
#include <serial/cadence.h>

/*
 * create default values for different platforms
 */
struct cadence_serial_devtype_data {
	u32 ctrl;
	u32 mode;
};

static struct cadence_serial_devtype_data cadence_r1p08_data = {
	.ctrl = CADENCE_CTRL_RXEN | CADENCE_CTRL_TXEN,
	.mode = CADENCE_MODE_CLK_REF | CADENCE_MODE_CHRL_8 | CADENCE_MODE_PAR_NONE,
};

struct cadence_serial_priv {
	struct console_device cdev;
	int baudrate;
	struct notifier_block notify;
	void __iomem *regs;
	struct clk *clk;
	struct cadence_serial_devtype_data *devtype;
};

static int cadence_serial_reset(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	/* Soft-Reset Tx/Rx paths */
	writel(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES, priv->regs +
		CADENCE_UART_CONTROL);

	while (readl(priv->regs + CADENCE_UART_CONTROL) &
		(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES))
		;

	return 0;
}

static int cadence_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);
	unsigned int gen, div;
	int calc_rate;
	unsigned long clk;
	int error;
	int val;

	clk = clk_get_rate(priv->clk);
	priv->baudrate = baudrate;

	/* disable transmitter and receiver */
	val = readl(priv->regs + CADENCE_UART_CONTROL);
	val &= ~CADENCE_CTRL_TXEN & ~CADENCE_CTRL_RXEN;
	writel(val, priv->regs + CADENCE_UART_CONTROL);

	/*
	 *	      clk
	 * rate = -----------
	 *	  gen*(div+1)
	 */

	for (div = 4; div < 256; div++) {
		gen = clk / (baudrate * (div + 1));

		if (gen < 1 || gen > 65535)
			continue;

		calc_rate = clk / (gen * (div + 1));
		error = baudrate - calc_rate;
		if (error < 0)
			error *= -1;
		if (((error * 100) / baudrate) < 3)
			break;
	}

	writel(gen, priv->regs + CADENCE_UART_BAUD_GEN);
	writel(div, priv->regs + CADENCE_UART_BAUD_DIV);

	/* Soft-Reset Tx/Rx paths */
	writel(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES, priv->regs +
		CADENCE_UART_CONTROL);

	while (readl(priv->regs + CADENCE_UART_CONTROL) &
		(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES))
		;

	/* Enable UART */
	writel(priv->devtype->ctrl, priv->regs + CADENCE_UART_CONTROL);

	return 0;
}

static int cadence_serial_init_port(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	cadence_serial_reset(cdev);

	/* Enable UART */
	writel(priv->devtype->ctrl, priv->regs + CADENCE_UART_CONTROL);
	writel(priv->devtype->mode, priv->regs + CADENCE_UART_MODE);

	return 0;
}

static void cadence_serial_putc(struct console_device *cdev, char c)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	while ((readl(priv->regs + CADENCE_UART_CHANNEL_STS) &
		CADENCE_STS_TFUL) != 0)
		;

	writel(c, priv->regs + CADENCE_UART_RXTXFIFO);
}

static int cadence_serial_tstc(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	return ((readl(priv->regs + CADENCE_UART_CHANNEL_STS) &
		 CADENCE_STS_REMPTY) == 0);
}

static int cadence_serial_getc(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	while (!cadence_serial_tstc(cdev))
		;

	return readl(priv->regs + CADENCE_UART_RXTXFIFO);
}

static void cadence_serial_flush(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	while ((readl(priv->regs + CADENCE_UART_CHANNEL_STS) &
		CADENCE_STS_TEMPTY) == 0)
		;
}

static int cadence_clocksource_clock_change(struct notifier_block *nb,
			unsigned long event, void *data)
{
	struct cadence_serial_priv *priv = container_of(nb,
					struct cadence_serial_priv, notify);

	cadence_serial_setbaudrate(&priv->cdev, priv->baudrate);

	return 0;
}

static int cadence_serial_probe(struct device *dev)
{
	struct resource *iores;
	struct console_device *cdev;
	struct cadence_serial_priv *priv;
	struct cadence_serial_devtype_data *devtype;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));
	priv->devtype = devtype;
	cdev = &priv->cdev;
	dev->priv = priv;

	priv->clk = clk_get_for_console(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = -ENODEV;
		goto err_free;
	}

	if (priv->clk && (devtype->mode & CADENCE_MODE_CLK_REF_DIV))
		clk_set_rate(priv->clk, clk_get_rate(priv->clk) / 8);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	priv->regs = IOMEM(iores->start);

	cdev->dev = dev;
	cdev->tstc = cadence_serial_tstc;
	cdev->putc = cadence_serial_putc;
	cdev->getc = cadence_serial_getc;
	cdev->flush = cadence_serial_flush;
	cdev->setbrg = priv->clk ? cadence_serial_setbaudrate : NULL;

	cadence_serial_init_port(cdev);

	console_register(cdev);

	if (priv->clk) {
		priv->notify.notifier_call = cadence_clocksource_clock_change;
		clock_register_client(&priv->notify);
	}

	return 0;

err_free:
	free(priv);
	return ret;
}

static __maybe_unused struct of_device_id cadence_serial_dt_ids[] = {
	{
		.compatible = "xlnx,xuartps",
		.data = &cadence_r1p08_data,
	}, {
		.compatible = "cdns,uart-r1p12",
		.data = &cadence_r1p08_data,
	}, {
		.compatible = "xlnx,zynqmp-uart",
		.data = &cadence_r1p08_data,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, cadence_serial_dt_ids);

static struct platform_device_id cadence_serial_ids[] = {
	{
		.name = "cadence-uart",
		.driver_data = (unsigned long)&cadence_r1p08_data,
	}, {
		/* sentinel */
	},
};

static struct driver cadence_serial_driver = {
	.name   = "cadence_serial",
	.probe  = cadence_serial_probe,
	.of_compatible = DRV_OF_COMPAT(cadence_serial_dt_ids),
	.id_table = cadence_serial_ids,
};

console_platform_driver(cadence_serial_driver);
