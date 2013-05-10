#ifndef __MACH_PINCTRL_H__
#define __MACH_PINCTRL_H__

/* Just all modern Ingenic SoCs have port A-D */
#define	JZ_GPIO_PORTA(pin)	(0x00 + pin)
#define	JZ_GPIO_PORTB(pin)	(0x20 + pin)
#define	JZ_GPIO_PORTC(pin)	(0x40 + pin)
#define	JZ_GPIO_PORTD(pin)	(0x60 + pin)

/* JZ4755 and JZ4770 have ports E and F */
#define	JZ_GPIO_PORTE(pin)	(0x80 + pin)
#define	JZ_GPIO_PORTF(pin)	(0xa0 + pin)

enum jz_gpio_function {
	JZ_GPIO_FUNC_NONE,
	JZ_GPIO_FUNC1,
	JZ_GPIO_FUNC2,
	JZ_GPIO_FUNC3,
};

void jz_pinctrl_init(void __iomem *jz_base);

/*
 Usually a driver for a SoC component has to request several gpio pins and
 configure them as funcion pins.
 jz_gpio_bulk_request can be used to ease this process.
 Usually one would do something like:

 static const struct jz_gpio_bulk_request i2c_pins[] = {
	JZ_GPIO_BULK_PIN(I2C_SDA),
	JZ_GPIO_BULK_PIN(I2C_SCK),
 };

 inside the probe function:

    ret = jz_gpio_bulk_request(i2c_pins, ARRAY_SIZE(i2c_pins));
    if (ret) {
	...

 inside the remove function:

    jz_gpio_bulk_free(i2c_pins, ARRAY_SIZE(i2c_pins));

*/

struct jz_gpio_bulk_request {
	int gpio;
	const char *name;
	enum jz_gpio_function function;
};

#define JZ_GPIO_BULK_PIN(pin) { \
    .gpio = JZ_GPIO_ ## pin, \
    .name = #pin, \
    .function = JZ_GPIO_FUNC_ ## pin \
}

int jz_gpio_bulk_request(const struct jz_gpio_bulk_request *request, size_t num);
void jz_gpio_bulk_free(const struct jz_gpio_bulk_request *request, size_t num);
void jz_gpio_enable_pullup(unsigned gpio);
void jz_gpio_disable_pullup(unsigned gpio);
int jz_gpio_set_function(int gpio, enum jz_gpio_function function);

#if 0
int jz_gpio_port_direction_input(int port, uint32_t mask);
int jz_gpio_port_direction_output(int port, uint32_t mask);
void jz_gpio_port_set_value(int port, uint32_t value, uint32_t mask);
uint32_t jz_gpio_port_get_value(int port, uint32_t mask);
#endif

#endif /* __MACH_PINCTRL_H__ */
