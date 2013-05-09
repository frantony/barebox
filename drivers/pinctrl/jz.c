#define JZ4740_GPIO_BASE_A (32*0)
#define JZ4740_GPIO_BASE_B (32*1)
#define JZ4740_GPIO_BASE_C (32*2)
#define JZ4740_GPIO_BASE_D (32*3)

#define JZ4740_GPIO_NUM_A 32
#define JZ4740_GPIO_NUM_B 32
#define JZ4740_GPIO_NUM_C 31
#define JZ4740_GPIO_NUM_D 32

#define JZ_REG_GPIO_MASK		0x20
#define JZ_REG_GPIO_MASK_SET		0x24
#define JZ_REG_GPIO_MASK_CLEAR		0x28
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
#define JZ_REG_GPIO_FLAG		0x80
#define JZ_REG_GPIO_FLAG_CLEAR		0x14

#define GPIO_TO_BIT(gpio) BIT(gpio & 0x1f)
#define GPIO_TO_REG(gpio, reg) (gpio_to_jz_gpio_chip(gpio)->base + (reg))

struct jz_gpio_chip {
	uint32_t edge_trigger_both;

	void __iomem *base;

	struct gpio_chip gpio_chip;
};

static struct jz_gpio_chip jz4740_gpio_chips[];

static inline struct jz_gpio_chip *gpio_to_jz_gpio_chip(unsigned int gpio)
{
	return &jz4740_gpio_chips[gpio >> 5];
}

static inline struct jz_gpio_chip *gpio_chip_to_jz_gpio_chip(struct gpio_chip *gpio_chip)
{
	return container_of(gpio_chip, struct jz_gpio_chip, gpio_chip);
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
EXPORT_SYMBOL_GPL(jz_gpio_disable_pullup);

int jz_gpio_port_direction_input(int port, uint32_t mask)
{
	writel(mask, GPIO_TO_REG(port, JZ_REG_GPIO_DIRECTION_CLEAR));

	return 0;
}
EXPORT_SYMBOL(jz_gpio_port_direction_input);

int jz_gpio_port_direction_output(int port, uint32_t mask)
{
	writel(mask, GPIO_TO_REG(port, JZ_REG_GPIO_DIRECTION_SET));

	return 0;
}
EXPORT_SYMBOL(jz_gpio_port_direction_output);

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

#define JZ4740_GPIO_CHIP(_bank) { \
	.gpio_chip = { \
		.label = "Bank " # _bank, \
		.base = JZ4740_GPIO_BASE_ ## _bank, \
		.ngpio = JZ4740_GPIO_NUM_ ## _bank, \
	}, \
}

static struct jz_gpio_chip jz4740_gpio_chips[] = {
	JZ4740_GPIO_CHIP(A),
	JZ4740_GPIO_CHIP(B),
	JZ4740_GPIO_CHIP(C),
	JZ4740_GPIO_CHIP(D),
};
