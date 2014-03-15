/*
 * TI DaVinci GPIO Support
 *
 * Copyright (c) 2006-2007 David Brownell
 * Copyright (c) 2007, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>

#include <linux/err.h>
//#include <linux/of.h>
//#include <linux/of_device.h>
//#include <linux/platform_device.h>
#include <platform_data/gpio-davinci.h>

#define readb_relaxed                   readb
#define readw_relaxed                   readw
#define readl_relaxed                   readl
#define readq_relaxed                   readq

#define writeb_relaxed                  writeb
#define writew_relaxed                  writew
#define writel_relaxed                  writel
#define writeq_relaxed                  writeq

struct davinci_gpio_regs {
	u32	dir;
	u32	out_data;
	u32	set_data;
	u32	clr_data;
	u32	in_data;
	u32	set_rising;
	u32	clr_rising;
	u32	set_falling;
	u32	clr_falling;
	u32	intstat;
};

#define BINTEN	0x8 /* GPIO Interrupt Per-Bank Enable Register */

#define chip2controller(chip)	\
	container_of(chip, struct davinci_gpio_controller, chip)

static struct davinci_gpio_regs __iomem *gpio2regs(void __iomem *gpio_base, unsigned gpio)
{
	void __iomem *ptr;

	if (gpio < 32 * 1)
		ptr = gpio_base + 0x10;
	else if (gpio < 32 * 2)
		ptr = gpio_base + 0x38;
	else if (gpio < 32 * 3)
		ptr = gpio_base + 0x60;
	else if (gpio < 32 * 4)
		ptr = gpio_base + 0x88;
	else if (gpio < 32 * 5)
		ptr = gpio_base + 0xb0;
	else
		ptr = NULL;
	return ptr;
}

/* board setup code *MUST* setup pinmux and enable the GPIO clock. */
static inline int __davinci_direction(struct gpio_chip *chip,
			unsigned offset, bool out, int value)
{
#if 0
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = d->regs;
	unsigned long flags;
	u32 temp;
	u32 mask = 1 << offset;

	temp = readl_relaxed(&g->dir);
	if (out) {
		temp &= ~mask;
		writel_relaxed(mask, value ? &g->set_data : &g->clr_data);
	} else {
		temp |= mask;
	}
	writel_relaxed(temp, &g->dir);
#endif

	return 0;
}

static int davinci_direction_in(struct gpio_chip *chip, unsigned offset)
{
	return __davinci_direction(chip, offset, false, 0);
}

static int
davinci_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	return __davinci_direction(chip, offset, true, value);
}

/*
 * Read the pin's value (works even if it's set up as output);
 * returns zero/nonzero.
 *
 * Note that changes are synched to the GPIO clock, so reading values back
 * right after you've set them may give old values.
 */
static int davinci_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = d->regs;

	return (1 << offset) & readl_relaxed(&g->in_data);
}

/*
 * Assuming the pin is muxed as a gpio output, set its output value.
 */
static void
davinci_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct davinci_gpio_controller *d = chip2controller(chip);
	struct davinci_gpio_regs __iomem *g = d->regs;

	writel_relaxed((1 << offset), value ? &g->set_data : &g->clr_data);
}

static int davinci_gpio_probe(struct device_d *dev)
{
	void __iomem *gpio_base;

	int ret;
	u32 val;
	int i, base;
	unsigned ngpio;
	struct davinci_gpio_controller *chips;
	struct davinci_gpio_regs __iomem *regs;
	struct device_node *dn = dev->device_node;

	dev->id = of_alias_get_id(dn, "gpio");
	if (dev->id < 0)
		return dev->id;

	ret = of_property_read_u32(dn, "ti,ngpio", &val);
	if (ret) {
		dev_err(dev, "could not read 'ti,ngpio' property\n");
		return -EINVAL;
	}

	ngpio = val;

	if (WARN_ON(ARCH_NR_GPIOS < ngpio))
		ngpio = ARCH_NR_GPIOS;

	chips = xzalloc((ngpio / 32 + 1) * sizeof(struct davinci_gpio_controller));

	gpio_base = dev_request_mem_region(dev, 0);
	if (!gpio_base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	for (i = 0, base = 0; base < ngpio; i++, base += 32) {
		chips[i].chip.ops->direction_input = davinci_direction_in;
		chips[i].chip.ops->get = davinci_gpio_get;
		chips[i].chip.ops->direction_output = davinci_direction_out;
		chips[i].chip.ops->set = davinci_gpio_set;

		chips[i].chip.base = base;
		chips[i].chip.ngpio = ngpio - base;
		if (chips[i].chip.ngpio > 32)
			chips[i].chip.ngpio = 32;

		regs = gpio2regs(gpio_base, base);
		chips[i].regs = regs;
		chips[i].set_data = &regs->set_data;
		chips[i].clr_data = &regs->clr_data;
		chips[i].in_data = &regs->in_data;

		gpiochip_add(&chips[i].chip);
	}

	return 0;
}

static struct of_device_id davinci_gpio_ids[] = {
	{ .compatible = "ti,dm6441-gpio", },
	{ /* sentinel */ },
};

static struct driver_d davinci_gpio_driver = {
	.name		= "davinci_gpio",
	.probe		= davinci_gpio_probe,
	.of_compatible	= DRV_OF_COMPAT(davinci_gpio_ids),
};

/**
 * GPIO driver registration needs to be done before machine_init functions
 * access GPIO. Hence davinci_gpio_drv_reg() is a postcore_initcall.
 */
static int davinci_gpio_drv_reg(void)
{
	return platform_driver_register(&davinci_gpio_driver);
}
coredevice_initcall(davinci_gpio_drv_reg);
