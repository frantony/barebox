// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * Copyright (C) 2008-2009 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 * Copyright (C) 2013 John Crispin <john@phrozen.org>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/reset-controller.h>

struct rt2880_reset_data {
	void __iomem			*membase;
	struct reset_controller_dev	rcdev;
};

static int rt2880_assert_device(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct rt2880_reset_data *data = container_of(rcdev,
						     struct rt2880_reset_data,
						     rcdev);
	u32 val;

	pr_err("rt2880_assert_device id=%d\n", id);
	if (id < 8)
		return -1;

	val = __raw_readl(data->membase);
	val |= BIT(id);
	__raw_writel(val, data->membase);

	return 0;
}

static int rt2880_deassert_device(struct reset_controller_dev *rcdev,
				  unsigned long id)
{
	struct rt2880_reset_data *data = container_of(rcdev,
						     struct rt2880_reset_data,
						     rcdev);
	u32 val;

	pr_err("rt2880_deassert_device id=%d\n", id);
	if (id < 8)
		return -1;

	val = __raw_readl(data->membase);
	val &= ~BIT(id);
	__raw_writel(val, data->membase);

	return 0;
}

static int rt2880_reset_device(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	rt2880_assert_device(rcdev, id);
	return rt2880_deassert_device(rcdev, id);
}

static const struct reset_control_ops rt2880_reset_ops = {
	.reset = rt2880_reset_device,
	.assert = rt2880_assert_device,
	.deassert = rt2880_deassert_device,
};

static int rt2880_reset_probe(struct device_d *dev)
{
	struct rt2880_reset_data *data;
	struct resource *res;
	struct device_node *np = dev->device_node;

	data = xzalloc(sizeof(*data));

	res = dev_request_mem_resource(dev, 0);
	data->membase = IOMEM(res->start);
	if (IS_ERR(data->membase))
		return PTR_ERR(data->membase);

	data->rcdev.nr_resets = 32;
	data->rcdev.ops = &rt2880_reset_ops;
	data->rcdev.of_node = np;

	return reset_controller_register(&data->rcdev);
}

static const struct of_device_id rt2880_reset_dt_ids[] = {
	{ .compatible = "ralink,rt2880-reset", },
	{ /* sentinel */ },
};

static struct driver_d rt2880_reset_driver = {
	.name = "rt2880-reset",
	.probe	= rt2880_reset_probe,
	.of_compatible	= DRV_OF_COMPAT(rt2880_reset_dt_ids),
};

static int rt2880_reset_init(void)
{
	return platform_driver_register(&rt2880_reset_driver);
}
postcore_initcall(rt2880_reset_init);
