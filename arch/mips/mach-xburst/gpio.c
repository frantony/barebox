/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Based on Linux JZ4740 platform GPIO support:
 * Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
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
#include <init.h>
#include <io.h>
#include <gpio.h>

#define JZ_REG_GPIO_PIN			0x00
#define JZ_REG_GPIO_DATA		0x10
#define JZ_REG_GPIO_DATA_SET		0x14
#define JZ_REG_GPIO_DATA_CLEAR		0x18
#define JZ_REG_GPIO_DIRECTION		0x60
#define JZ_REG_GPIO_DIRECTION_SET	0x64
#define JZ_REG_GPIO_DIRECTION_CLEAR	0x68

#define GPIO_TO_BIT(gpio) BIT(gpio & 0x1f)
#define CHIP_TO_REG(chip, reg) (gpio_chip_to_jz_gpio_chip(chip)->base + (reg))

struct jz_gpio_chip {
	void __iomem *base;
	struct gpio_chip chip;
};

static inline struct jz_gpio_chip *gpio_chip_to_jz_gpio_chip(struct gpio_chip *chip)
{
	return container_of(chip, struct jz_gpio_chip, chip);
}

static int jz_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	return !!(readl(CHIP_TO_REG(chip, JZ_REG_GPIO_PIN)) & BIT(gpio));
}

static void jz_gpio_set_value(struct gpio_chip *chip, unsigned gpio, int value)
{
	uint32_t __iomem *reg = CHIP_TO_REG(chip, JZ_REG_GPIO_DATA_SET);
	reg += !value;
	writel(BIT(gpio), reg);
}

static int jz_gpio_direction_output(struct gpio_chip *chip, unsigned gpio,
	int value)
{
	writel(BIT(gpio), CHIP_TO_REG(chip, JZ_REG_GPIO_DIRECTION_SET));
	jz_gpio_set_value(chip, gpio, value);

	return 0;
}

static int jz_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	writel(BIT(gpio), CHIP_TO_REG(chip, JZ_REG_GPIO_DIRECTION_CLEAR));

	return 0;
}

static struct gpio_ops jz_gpio_ops = {
	.direction_input = jz_gpio_direction_input,
	.direction_output = jz_gpio_direction_output,
	.get = jz_gpio_get_value,
	.set = jz_gpio_set_value,
};

static int jz_gpio_probe(struct device_d *dev)
{
	struct jz_gpio_chip *jz_gpio;

	jz_gpio = xzalloc(sizeof(*jz_gpio));
	jz_gpio->base = dev_request_mem_region(dev, 0);
	jz_gpio->chip.ops = &jz_gpio_ops;
	jz_gpio->chip.base = dev->id * 32;
	jz_gpio->chip.ngpio = 32;
	jz_gpio->chip.dev = dev;
	gpiochip_add(&jz_gpio->chip);

	dev_dbg(dev, "probed gpiochip%d with base %d\n",
				dev->id, jz_gpio->chip.base);

	return 0;
}

static struct driver_d jz_gpio_driver = {
	.name = "jz-gpio",
	.probe = jz_gpio_probe,
};

static int jz_gpio_add(void)
{
	return platform_driver_register(&jz_gpio_driver);
}
coredevice_initcall(jz_gpio_add);
