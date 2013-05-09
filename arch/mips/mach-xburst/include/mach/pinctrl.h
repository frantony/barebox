#ifndef __MACH_PINCTRL_H__
#define __MACH_PINCTRL_H__

enum jz_gpio_function {
	JZ_GPIO_FUNC_NONE,
	JZ_GPIO_FUNC1,
	JZ_GPIO_FUNC2,
	JZ_GPIO_FUNC3,
};

void jz_pinctrl_init(void __iomem *jz_base);

#endif /* __MACH_PINCTRL_H__ */
