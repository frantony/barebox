// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Spreadtrum Communications Inc.
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <poller.h>
#include <kfifo.h>
#include <malloc.h>
#include <input/matrix_keypad.h>
#include <linux/err.h>
#include <input/input.h>

#define SPRD_KPD_CTRL			0x0
#define SPRD_KPD_INT_EN			0x4
#define SPRD_KPD_INT_RAW_STATUS		0x8
#define SPRD_KPD_INT_MASK_STATUS	0xc
#define SPRD_KPD_INT_CLR		0x10
#define SPRD_KPD_POLARITY		0x18
#define SPRD_KPD_DEBOUNCE_CNT		0x1c
#define SPRD_KPD_LONG_KEY_CNT		0x20
#define SPRD_KPD_SLEEP_CNT		0x24
#define SPRD_KPD_CLK_DIV_CNT		0x28
#define SPRD_KPD_KEY_STATUS		0x2c
#define SPRD_KPD_SLEEP_STATUS		0x30
#define SPRD_KPD_DEBUG_STATUS1		0x34
#define SPRD_KPD_DEBUG_STATUS2		0x38

#define SPRD_KPD_EN			BIT(0)
#define SPRD_KPD_SLEEP_EN		BIT(1)
#define SPRD_KPD_LONG_KEY_EN		BIT(2)

#define SPRD_KPD_ROWS_MSK		GENMASK(23, 16)
#define SPRD_KPD_COLS_MSK		GENMASK(15, 8)

#define SPRD_KPD_INT_ALL		GENMASK(11, 0)
#define SPRD_KPD_INT_DOWNUP		GENMASK(7, 0)
#define SPRD_KPD_INT_LONG		GENMASK(11, 8)

#define SPRD_KPD_ROW_POLARITY		GENMASK(7, 0)
#define SPRD_KPD_COL_POLARITY		GENMASK(15, 8)

#define SPRD_KPD_PRESS_INTX(X, V) \
	(((V) >> (X)) & GENMASK(0, 0))
#define SPRD_KPD_RELEASE_INTX(X, V) \
	(((V) >> ((X) + 4)) & GENMASK(0, 0))
#define SPRD_KPD_INTX_COL(X, V) \
	(((V) >> ((X) << 3)) & GENMASK(2, 0))
#define SPRD_KPD_INTX_ROW(X, V) \
	(((V) >> (((X) << 3) + 4)) & GENMASK(2, 0))
#define SPRD_KPD_INTX_DOWN(X, V) \
	(((V) >> (((X) << 3) + 7)) & GENMASK(0, 0))

#define SPRD_KPD_RTC_HZ			32768
#define SPRD_DEF_LONG_KEY_MS		1000
#define SPRD_DEF_DIV_CNT		1
#define SPRD_KPD_INT_CNT		4
#define SPRD_KPD_ROWS_MAX		8
#define SPRD_KPD_COLS_MAX		8
#define SPRD_KPD_ROWS_SHIFT		16
#define SPRD_KPD_COLS_SHIFT		8

#define SPRD_CAP_WAKEUP			BIT(0)
#define SPRD_CAP_LONG_KEY		BIT(1)
#define SPRD_CAP_REPEAT			BIT(2)

struct sprd_keypad_data {
	u32 rows_en; /* enabled rows bits */
	u32 cols_en; /* enabled cols bits */
	u32 num_rows;
	u32 num_cols;
	u32 capabilities;
	u32 debounce_ms;
	struct device *dev;
	void __iomem *base;
	struct poller_struct poller;
	struct input_device input;
//	struct clk *enable;
//	struct clk *rtc;

	unsigned short *keycodes;

	u32 int_status;
	u32 key_status;
};

static inline struct sprd_keypad_data *
poller_to_sprd_pdata(struct poller_struct *poller)
{
	return container_of(poller, struct sprd_keypad_data, poller);
}

#if 0
static int sprd_keypad_enable(struct sprd_keypad_data *data)
{
	struct device *dev = data->input_dev->dev.parent;
	int ret;

	ret = clk_prepare_enable(data->rtc);
	if (ret) {
		dev_err(dev, "enable rtc failed.\n");
		return ret;
	}

	ret = clk_prepare_enable(data->enable);
	if (ret) {
		dev_err(dev, "enable keypad failed.\n");
		clk_disable_unprepare(data->rtc);
		return ret;
	}

	return 0;
}

static void sprd_keypad_disable(struct sprd_keypad_data *data)
{
	clk_disable_unprepare(data->enable);
	clk_disable_unprepare(data->rtc);
}
#endif

static void sprd_keypad_poller(struct poller_struct *poller)
{
	struct sprd_keypad_data *data = poller_to_sprd_pdata(poller);
	struct device *dev = data->dev;
	u32 int_status = readl_relaxed(data->base +
						SPRD_KPD_INT_RAW_STATUS);
	u32 key_status = readl_relaxed(data->base +
						SPRD_KPD_KEY_STATUS);
	unsigned short *keycodes = data->keycodes;
	u32 row_shift = get_count_order(data->num_cols);
	unsigned short key;
	int col, row;
	u32 i;

	if (int_status == data->int_status && key_status == data->key_status) {
		return;
	}

	data->int_status = int_status;
	data->key_status = key_status;

	//dev_err(dev, "int_status=%08x key_status=%08x\n", int_status, key_status);

	writel_relaxed(SPRD_KPD_INT_ALL, data->base + SPRD_KPD_INT_CLR);

	for (i = 0; i < SPRD_KPD_INT_CNT; i++) {
		if (SPRD_KPD_PRESS_INTX(i, int_status)) {
			col = SPRD_KPD_INTX_COL(i, key_status);
			row = SPRD_KPD_INTX_ROW(i, key_status);
			key = keycodes[MATRIX_SCAN_CODE(row, col, row_shift)];
			input_report_key_event(&data->input, key, 1);
			dev_dbg(dev, "%dD\n", key);
			dev_dbg(dev, "%d %d down\n", col, row);
		}
		if (SPRD_KPD_RELEASE_INTX(i, int_status)) {
			col = SPRD_KPD_INTX_COL(i, key_status);
			row = SPRD_KPD_INTX_ROW(i, key_status);
			key = keycodes[MATRIX_SCAN_CODE(row, col, row_shift)];
			input_report_key_event(&data->input, key, 0);
			dev_dbg(dev, "%dU\n", key);
			dev_dbg(dev, "%d %d up\n", col, row);
		}
	}
}

static u32 sprd_keypad_time_to_counter(u32 array_size, u32 time_ms)
{
	u32 value;

	/*
	 * y(ms) = (x + 1) * array_size
	 *		/ (32.768 / (clk_div_num + 1))
	 * y means time in ms
	 * x means counter
	 * array_size equal to rows * columns
	 * clk_div_num is devider to keypad source clock
	 **/
	value = SPRD_KPD_RTC_HZ * time_ms;
	value = value / (1000 * array_size *
			(SPRD_DEF_DIV_CNT + 1));
	if (value >= 1)
		value -= 1;

	return value;
}

// FIXME
static void sprd_keypad_ll_hw_init(void)
{
	//// sprd_keypad_enable()
	// keypad_init
	// APB_PWR_ON(0x80040);
	writel(0x00080040, 0x8b0010a8);

	writel(0x0000208a, 0x8c000034);
	writel(0x0000208a, 0x8c000038);
	writel(0x0000208a, 0x8c00003c);

	writel(0x00002001, 0x8c000048); // (row=2?)
	writel(0x00002001, 0x8c00004c); // row=3
	writel(0x00002001, 0x8c000050); // row=4
}

static int sprd_keypad_hw_init(struct sprd_keypad_data *data)
{
	u32 value;

	sprd_keypad_ll_hw_init();

	writel_relaxed(SPRD_KPD_INT_ALL, data->base + SPRD_KPD_INT_CLR);
	writel_relaxed(SPRD_KPD_ROW_POLARITY | SPRD_KPD_COL_POLARITY,
			data->base + SPRD_KPD_POLARITY);
	writel_relaxed(SPRD_DEF_DIV_CNT, data->base + SPRD_KPD_CLK_DIV_CNT);

	value = sprd_keypad_time_to_counter(data->num_rows * data->num_cols,
						SPRD_DEF_LONG_KEY_MS);
	writel_relaxed(value, data->base + SPRD_KPD_LONG_KEY_CNT);

	value = sprd_keypad_time_to_counter(data->num_rows * data->num_cols,
						data->debounce_ms);
	writel_relaxed(value, data->base + SPRD_KPD_DEBOUNCE_CNT);

	value = SPRD_KPD_INT_DOWNUP;
	if (data->capabilities & SPRD_CAP_LONG_KEY)
		value |= SPRD_KPD_INT_LONG;

	writel_relaxed(value, data->base + SPRD_KPD_INT_EN);

	value = SPRD_KPD_RTC_HZ - 1;
	writel_relaxed(value, data->base + SPRD_KPD_SLEEP_CNT);

	/* set enabled rows and columns */
	value = (((data->rows_en << SPRD_KPD_ROWS_SHIFT)
		| (data->cols_en << SPRD_KPD_COLS_SHIFT))
		& (SPRD_KPD_ROWS_MSK | SPRD_KPD_COLS_MSK))
		| SPRD_KPD_EN | SPRD_KPD_SLEEP_EN;
	if (data->capabilities & SPRD_CAP_LONG_KEY)
		value |= SPRD_KPD_LONG_KEY_EN;
	writel_relaxed(value, data->base + SPRD_KPD_CTRL);
	pr_err("%s:%d ctrl value =%08x\n", __FUNCTION__, __LINE__, value);

	return 0;
}

static int sprd_keypad_parse_dt(struct sprd_keypad_data *data)
{
	struct device *dev = data->dev;
	struct device_node *np = dev->of_node;
	int ret;

	ret = matrix_keypad_parse_properties(dev,
						&data->num_rows,
						&data->num_cols);
	if (ret)
		return ret;

	if (data->num_rows > SPRD_KPD_ROWS_MAX
		|| data->num_cols > SPRD_KPD_COLS_MAX) {
		dev_err(dev, "invalid num_rows or num_cols\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "debounce-interval", &data->debounce_ms);
	if (ret) {
		data->debounce_ms = 5;
		dev_warn(dev, "parse debounce-interval failed.\n");
	}

	if (of_get_property(np, "linux,repeat", NULL))
		data->capabilities |= SPRD_CAP_REPEAT;
	if (of_get_property(np, "sprd,support_long_key", NULL))
		data->capabilities |= SPRD_CAP_LONG_KEY;

#if 0
	data->enable = devm_clk_get(dev, "enable");
	if (IS_ERR(data->enable)) {
		if (PTR_ERR(data->enable) != -EPROBE_DEFER)
			dev_err(dev, "get enable clk failed.\n");
		return PTR_ERR(data->enable);
	}

	data->rtc = devm_clk_get(dev, "rtc");
	if (IS_ERR(data->rtc)) {
		if (PTR_ERR(data->enable) != -EPROBE_DEFER)
			dev_err(dev, "get rtc clk failed.\n");
		return PTR_ERR(data->rtc);
	}
#endif

	return 0;
}

static int sprd_keypad_probe(struct device *dev)
{
	struct sprd_keypad_data *data;
	struct resource *res;
	int ret, i, j, row_shift;
	unsigned long rows, cols;
	unsigned short *keycodes;

	data = xzalloc(sizeof(*data));

	data->dev = dev;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);
	data->base = IOMEM(res->start);

	ret = sprd_keypad_parse_dt(data);
	if (ret)
		return ret;

	row_shift = get_count_order(data->num_cols);

	keycodes = xzalloc(MATRIX_SCAN_CODE(SPRD_KPD_ROWS_MAX + 1, SPRD_KPD_COLS_MAX + 1, row_shift) * sizeof(*keycodes));
	data->keycodes = keycodes;

	ret = matrix_keypad_build_keymap(dev, NULL, row_shift, keycodes);
	if (ret)
		return ret;

	rows = cols = 0;

	for (i = 0; i < data->num_rows; i++) {
		for (j = 0; j < data->num_cols; j++) {
			if (!!keycodes[MATRIX_SCAN_CODE(i, j, row_shift)]) {
				set_bit(i, &rows);
				set_bit(j, &cols);
			}
		}
	}

	data->rows_en = rows;
	data->cols_en = cols;

#if 0
	ret = sprd_keypad_enable(data);
	if (ret)
		return ret;
#endif

	ret = sprd_keypad_hw_init(data);
	if (ret) {
		//sprd_keypad_disable(data);
		return ret;
	}

	data->poller.func = sprd_keypad_poller;

	ret = poller_register(&data->poller, dev_name(dev));
	if (ret)
		return ret;

	ret = input_device_register(&data->input);
	if (ret) {
		// FIXME: poller is registered
		//dev_err(&pdev->dev, "register input dev failed\n");
		//sprd_keypad_disable(data);
		return ret;
	}

	return 0;
}

static const struct of_device_id sprd_keypad_dt_ids[] = {
	{ .compatible = "sprd,s9820e-keypad", },
	{},
};

static struct driver sprd_keypad_driver = {
	.name = "sprd-keypad",
	.probe = sprd_keypad_probe,
	.of_compatible = DRV_OF_COMPAT(sprd_keypad_dt_ids),
};
device_platform_driver(sprd_keypad_driver);

MODULE_DESCRIPTION("Spreadtrum KPD Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Neo Hou <neo.hou@unisoc.com>");
