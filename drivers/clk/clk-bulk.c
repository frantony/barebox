// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2017 NXP
 *
 * Dong Aisheng <aisheng.dong@nxp.com>
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <stringlist.h>
#include <complete.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clk/clk-conf.h>

void clk_bulk_put(int num_clks, struct clk_bulk_data *clks)
{
	while (--num_clks >= 0) {
		clk_put(clks[num_clks].clk);
		clks[num_clks].clk = NULL;
	}
}
EXPORT_SYMBOL_GPL(clk_bulk_put);

static int __clk_bulk_get(struct device_d *dev, int num_clks,
			      struct clk_bulk_data *clks,
			      bool optional)
{
	int ret;
	int i;

	for (i = 0; i < num_clks; i++)
		clks[i].clk = NULL;

	for (i = 0; i < num_clks; i++) {
		clks[i].clk = clk_get(dev, clks[i].id);
		if (IS_ERR(clks[i].clk)) {
			ret = PTR_ERR(clks[i].clk);
			clks[i].clk = NULL;

			if (ret == -ENOENT && optional)
				continue;

			if (ret != -EPROBE_DEFER)
				dev_err(dev, "Failed to get clk '%s': %d\n",
					clks[i].id, ret);

			goto err;
		}
	}

	return 0;

err:
	clk_bulk_put(i, clks);

	return ret;
}

int __must_check clk_bulk_get(struct device_d *dev, int num_clks,
			      struct clk_bulk_data *clks)
{
	return __clk_bulk_get(dev, num_clks, clks, false);
}
EXPORT_SYMBOL(clk_bulk_get);

int __must_check clk_bulk_get_optional(struct device_d *dev, int num_clks,
				       struct clk_bulk_data *clks)
{
	return __clk_bulk_get(dev, num_clks, clks, true);
}
EXPORT_SYMBOL(clk_bulk_get_optional);

static int __must_check of_clk_bulk_get(struct device_node *np, int num_clks,
					struct clk_bulk_data *clks)
{
	int ret;
	int i;

	for (i = 0; i < num_clks; i++) {
		clks[i].id = NULL;
		clks[i].clk = NULL;
	}

	for (i = 0; i < num_clks; i++) {
		of_property_read_string_index(np, "clock-names", i, &clks[i].id);
		clks[i].clk = of_clk_get(np, i);
		if (IS_ERR(clks[i].clk)) {
			ret = PTR_ERR(clks[i].clk);
			pr_err("%pOF: Failed to get clk index: %d ret: %d\n",
				np, i, ret);
			clks[i].clk = NULL;
			goto err;
		}
	}

	return 0;

err:
	clk_bulk_put(i, clks);

	return ret;
}

static int __must_check of_clk_bulk_get_all(struct device_node *np,
					    struct clk_bulk_data **clks)
{
	struct clk_bulk_data *clk_bulk;
	int num_clks;
	int ret;

	num_clks = of_clk_get_parent_count(np);
	if (!num_clks)
		return 0;

	clk_bulk = kmalloc_array(num_clks, sizeof(*clk_bulk), GFP_KERNEL);
	if (!clk_bulk)
		return -ENOMEM;

	ret = of_clk_bulk_get(np, num_clks, clk_bulk);
	if (ret) {
		kfree(clk_bulk);
		return ret;
	}

	*clks = clk_bulk;

	return num_clks;
}

void clk_bulk_put_all(int num_clks, struct clk_bulk_data *clks)
{
	if (IS_ERR_OR_NULL(clks))
		return;

	clk_bulk_put(num_clks, clks);

	kfree(clks);
}
EXPORT_SYMBOL(clk_bulk_put_all);

int __must_check clk_bulk_get_all(struct device_d *dev,
				  struct clk_bulk_data **clks)
{
	struct device_node *np = dev->device_node;

	if (!np)
		return 0;

	return of_clk_bulk_get_all(np, clks);
}
EXPORT_SYMBOL(clk_bulk_get_all);

/**
 * clk_bulk_disable - gate a set of clocks
 * @num_clks: the number of clk_bulk_data
 * @clks: the clk_bulk_data table being gated
 *
 * clk_bulk_disable must not sleep, which differentiates it from
 * clk_bulk_unprepare. clk_bulk_disable must be called before
 * clk_bulk_unprepare.
 */
void clk_bulk_disable(int num_clks, const struct clk_bulk_data *clks)
{

	while (--num_clks >= 0)
		clk_disable(clks[num_clks].clk);
}
EXPORT_SYMBOL_GPL(clk_bulk_disable);

/**
 * clk_bulk_enable - ungate a set of clocks
 * @num_clks: the number of clk_bulk_data
 * @clks: the clk_bulk_data table being ungated
 *
 * clk_bulk_enable must not sleep
 * Returns 0 on success, -EERROR otherwise.
 */
int __must_check clk_bulk_enable(int num_clks, const struct clk_bulk_data *clks)
{
	int ret;
	int i;

	for (i = 0; i < num_clks; i++) {
		ret = clk_enable(clks[i].clk);
		if (ret) {
			pr_err("Failed to enable clk '%s': %d\n",
				clks[i].id, ret);
			goto err;
		}
	}

	return 0;

err:
	clk_bulk_disable(i, clks);

	return  ret;
}
EXPORT_SYMBOL_GPL(clk_bulk_enable);
