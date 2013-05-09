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
//#include <gpio.h>
#include <mach/pinctrl.h>

#define JZ_REG_GPIO_PULL		0x30
#define JZ_REG_GPIO_PULL_SET		0x34
#define JZ_REG_GPIO_PULL_CLEAR		0x38
#define JZ_REG_GPIO_FUNC		0x40
#define JZ_REG_GPIO_FUNC_SET		0x44
#define JZ_REG_GPIO_FUNC_CLEAR		0x48
#define JZ_REG_GPIO_SELECT		0x50
#define JZ_REG_GPIO_SELECT_SET		0x54
#define JZ_REG_GPIO_SELECT_CLEAR	0x58
#define JZ_REG_GPIO_TRIGGER		0x70
#define JZ_REG_GPIO_TRIGGER_SET		0x74
#define JZ_REG_GPIO_TRIGGER_CLEAR	0x78

#define GPIO_TO_BIT(gpio) BIT(gpio & 0x1f)
#define GPIO_TO_REG(gpio, reg) (gpio_to_jz_gpio_base(gpio) + (reg))

static void __iomem *base;
/* static int maxbank; */
/* static int bank_reg_size; */

static inline void *gpio_to_jz_gpio_base(unsigned int gpio)
{
	return base + 0x100 * (gpio >> 5);
}

static inline void jz_gpio_write_bit(unsigned int gpio, unsigned int reg)
{
	writel(GPIO_TO_BIT(gpio), GPIO_TO_REG(gpio, reg));
}

int jz_gpio_set_function(int gpio, enum jz_gpio_function function)
{
	if (function == JZ_GPIO_FUNC_NONE) {
		jz_gpio_write_bit(gpio, JZ_REG_GPIO_FUNC_CLEAR);
		jz_gpio_write_bit(gpio, JZ_REG_GPIO_SELECT_CLEAR);
		jz_gpio_write_bit(gpio, JZ_REG_GPIO_TRIGGER_CLEAR);
	} else {
		jz_gpio_write_bit(gpio, JZ_REG_GPIO_FUNC_SET);
		jz_gpio_write_bit(gpio, JZ_REG_GPIO_TRIGGER_CLEAR);
		switch (function) {
		case JZ_GPIO_FUNC1:
			jz_gpio_write_bit(gpio, JZ_REG_GPIO_SELECT_CLEAR);
			break;
		case JZ_GPIO_FUNC3:
			jz_gpio_write_bit(gpio, JZ_REG_GPIO_TRIGGER_SET);
		case JZ_GPIO_FUNC2: /* Falltrough */
			jz_gpio_write_bit(gpio, JZ_REG_GPIO_SELECT_SET);
			break;
		default:
			BUG();
			break;
		}
	}

	return 0;
}

void jz_gpio_enable_pullup(unsigned gpio)
{
	jz_gpio_write_bit(gpio, JZ_REG_GPIO_PULL_CLEAR);
}

void jz_gpio_disable_pullup(unsigned gpio)
{
	jz_gpio_write_bit(gpio, JZ_REG_GPIO_PULL_SET);
}

#if 0
int jz_gpio_port_direction_input(int port, uint32_t mask)
{
	writel(mask, GPIO_TO_REG(port, JZ_REG_GPIO_DIRECTION_CLEAR));

	return 0;
}

int jz_gpio_port_direction_output(int port, uint32_t mask)
{
	writel(mask, GPIO_TO_REG(port, JZ_REG_GPIO_DIRECTION_SET));

	return 0;
}

void jz_gpio_port_set_value(int port, uint32_t value, uint32_t mask)
{
	writel(~value & mask, GPIO_TO_REG(port, JZ_REG_GPIO_DATA_CLEAR));
	writel(value & mask, GPIO_TO_REG(port, JZ_REG_GPIO_DATA_SET));
}

uint32_t jz_gpio_port_get_value(int port, uint32_t mask)
{
	uint32_t value = readl(GPIO_TO_REG(port, JZ_REG_GPIO_PIN));

	return value & mask;
}
#endif

void jz_pinctrl_init(void __iomem *jz_base)
{
	base = jz_base;
}
