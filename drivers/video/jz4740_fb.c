/*
 *  Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 *	JZ4740 SoC LCD framebuffer driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of  the GNU General Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define DEBUG

#include <common.h>
#include <init.h>
#include <io.h>
#include <fb.h>
#include <malloc.h>
#include <errno.h>

//#include <asm/mach-jz4740/gpio.h>

#define JZ_REG_LCD_CFG		0x00
#define JZ_REG_LCD_VSYNC	0x04
#define JZ_REG_LCD_HSYNC	0x08
#define JZ_REG_LCD_VAT		0x0C
#define JZ_REG_LCD_DAH		0x10
#define JZ_REG_LCD_DAV		0x14
#define JZ_REG_LCD_PS		0x18
#define JZ_REG_LCD_CLS		0x1C
#define JZ_REG_LCD_SPL		0x20
#define JZ_REG_LCD_REV		0x24
#define JZ_REG_LCD_CTRL		0x30
#define JZ_REG_LCD_STATE	0x34
#define JZ_REG_LCD_IID		0x38
#define JZ_REG_LCD_DA0		0x40
#define JZ_REG_LCD_SA0		0x44
#define JZ_REG_LCD_FID0		0x48
#define JZ_REG_LCD_CMD0		0x4C
#define JZ_REG_LCD_DA1		0x50
#define JZ_REG_LCD_SA1		0x54
#define JZ_REG_LCD_FID1		0x58
#define JZ_REG_LCD_CMD1		0x5C

#define JZ_LCD_CFG_SLCD			BIT(31)
#define JZ_LCD_CFG_PS_DISABLE		BIT(23)
#define JZ_LCD_CFG_CLS_DISABLE		BIT(22)
#define JZ_LCD_CFG_SPL_DISABLE		BIT(21)
#define JZ_LCD_CFG_REV_DISABLE		BIT(20)
#define JZ_LCD_CFG_HSYNCM		BIT(19)
#define JZ_LCD_CFG_PCLKM		BIT(18)
#define JZ_LCD_CFG_INV			BIT(17)
#define JZ_LCD_CFG_SYNC_DIR		BIT(16)
#define JZ_LCD_CFG_PS_POLARITY		BIT(15)
#define JZ_LCD_CFG_CLS_POLARITY		BIT(14)
#define JZ_LCD_CFG_SPL_POLARITY		BIT(13)
#define JZ_LCD_CFG_REV_POLARITY		BIT(12)
#define JZ_LCD_CFG_HSYNC_ACTIVE_LOW	BIT(11)
#define JZ_LCD_CFG_PCLK_FALLING_EDGE	BIT(10)
#define JZ_LCD_CFG_DE_ACTIVE_LOW	BIT(9)
#define JZ_LCD_CFG_VSYNC_ACTIVE_LOW	BIT(8)
#define JZ_LCD_CFG_18_BIT		BIT(7)
#define JZ_LCD_CFG_PDW			(BIT(5) | BIT(4))
#define JZ_LCD_CFG_MODE_MASK 0xf

#define JZ_LCD_CTRL_BURST_4		(0x0 << 28)
#define JZ_LCD_CTRL_BURST_8		(0x1 << 28)
#define JZ_LCD_CTRL_BURST_16		(0x2 << 28)
#define JZ_LCD_CTRL_RGB555		BIT(27)
#define JZ_LCD_CTRL_OFUP		BIT(26)
#define JZ_LCD_CTRL_FRC_GRAYSCALE_16	(0x0 << 24)
#define JZ_LCD_CTRL_FRC_GRAYSCALE_4	(0x1 << 24)
#define JZ_LCD_CTRL_FRC_GRAYSCALE_2	(0x2 << 24)
#define JZ_LCD_CTRL_PDD_MASK		(0xff << 16)
#define JZ_LCD_CTRL_EOF_IRQ		BIT(13)
#define JZ_LCD_CTRL_SOF_IRQ		BIT(12)
#define JZ_LCD_CTRL_OFU_IRQ		BIT(11)
#define JZ_LCD_CTRL_IFU0_IRQ		BIT(10)
#define JZ_LCD_CTRL_IFU1_IRQ		BIT(9)
#define JZ_LCD_CTRL_DD_IRQ		BIT(8)
#define JZ_LCD_CTRL_QDD_IRQ		BIT(7)
#define JZ_LCD_CTRL_REVERSE_ENDIAN	BIT(6)
#define JZ_LCD_CTRL_LSB_FISRT		BIT(5)
#define JZ_LCD_CTRL_DISABLE		BIT(4)
#define JZ_LCD_CTRL_ENABLE		BIT(3)
#define JZ_LCD_CTRL_BPP_1		0x0
#define JZ_LCD_CTRL_BPP_2		0x1
#define JZ_LCD_CTRL_BPP_4		0x2
#define JZ_LCD_CTRL_BPP_8		0x3
#define JZ_LCD_CTRL_BPP_15_16		0x4
#define JZ_LCD_CTRL_BPP_18_24		0x5

#define JZ_LCD_CMD_SOF_IRQ BIT(15)
#define JZ_LCD_CMD_EOF_IRQ BIT(16)
#define JZ_LCD_CMD_ENABLE_PAL BIT(12)

#define JZ_LCD_SYNC_MASK 0x3ff

#define JZ_LCD_STATE_DISABLED BIT(0)

struct jzfb_framedesc {
	uint32_t next;
	uint32_t addr;
	uint32_t id;
	uint32_t cmd;
} __packed;

struct jzfb {
	struct fb_info *fb;
	struct platform_device *pdev;
	void __iomem *base;
	struct resource *mem;
	struct jz4740_fb_platform_data *pdata;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;
	struct jzfb_framedesc *framedesc;
	dma_addr_t framedesc_phys;

	struct clk *ldclk;
	struct clk *lpclk;

	unsigned is_enabled:1;

	uint32_t pseudo_palette[16];
};

#if 0
#if 0
static const struct jz_gpio_bulk_request jz_lcd_ctrl_pins[] = {
	JZ_GPIO_BULK_PIN(LCD_PCLK),
	JZ_GPIO_BULK_PIN(LCD_HSYNC),
	JZ_GPIO_BULK_PIN(LCD_VSYNC),
	JZ_GPIO_BULK_PIN(LCD_DE),
	JZ_GPIO_BULK_PIN(LCD_PS),
	JZ_GPIO_BULK_PIN(LCD_REV),
	JZ_GPIO_BULK_PIN(LCD_CLS),
	JZ_GPIO_BULK_PIN(LCD_SPL),
};

static const struct jz_gpio_bulk_request jz_lcd_data_pins[] = {
	JZ_GPIO_BULK_PIN(LCD_DATA0),
	JZ_GPIO_BULK_PIN(LCD_DATA1),
	JZ_GPIO_BULK_PIN(LCD_DATA2),
	JZ_GPIO_BULK_PIN(LCD_DATA3),
	JZ_GPIO_BULK_PIN(LCD_DATA4),
	JZ_GPIO_BULK_PIN(LCD_DATA5),
	JZ_GPIO_BULK_PIN(LCD_DATA6),
	JZ_GPIO_BULK_PIN(LCD_DATA7),
	JZ_GPIO_BULK_PIN(LCD_DATA8),
	JZ_GPIO_BULK_PIN(LCD_DATA9),
	JZ_GPIO_BULK_PIN(LCD_DATA10),
	JZ_GPIO_BULK_PIN(LCD_DATA11),
	JZ_GPIO_BULK_PIN(LCD_DATA12),
	JZ_GPIO_BULK_PIN(LCD_DATA13),
	JZ_GPIO_BULK_PIN(LCD_DATA14),
	JZ_GPIO_BULK_PIN(LCD_DATA15),
	JZ_GPIO_BULK_PIN(LCD_DATA16),
	JZ_GPIO_BULK_PIN(LCD_DATA17),
};
#endif

static unsigned int jzfb_num_ctrl_pins(struct jzfb *jzfb)
{
	unsigned int num;

	switch (jzfb->pdata->lcd_type) {
	case JZ_LCD_TYPE_GENERIC_16_BIT:
		num = 4;
		break;
	case JZ_LCD_TYPE_GENERIC_18_BIT:
		num = 4;
		break;
	case JZ_LCD_TYPE_8BIT_SERIAL:
		num = 3;
		break;
	case JZ_LCD_TYPE_SPECIAL_TFT_1:
	case JZ_LCD_TYPE_SPECIAL_TFT_2:
	case JZ_LCD_TYPE_SPECIAL_TFT_3:
		num = 8;
		break;
	default:
		num = 0;
		break;
	}
	return num;
}

static unsigned int jzfb_num_data_pins(struct jzfb *jzfb)
{
	unsigned int num;

	switch (jzfb->pdata->lcd_type) {
	case JZ_LCD_TYPE_GENERIC_16_BIT:
		num = 16;
		break;
	case JZ_LCD_TYPE_GENERIC_18_BIT:
		num = 18;
		break;
	case JZ_LCD_TYPE_8BIT_SERIAL:
		num = 8;
		break;
	case JZ_LCD_TYPE_SPECIAL_TFT_1:
	case JZ_LCD_TYPE_SPECIAL_TFT_2:
	case JZ_LCD_TYPE_SPECIAL_TFT_3:
		if (jzfb->pdata->bpp == 18)
			num = 18;
		else
			num = 16;
		break;
	default:
		num = 0;
		break;
	}
	return num;
}

/* Based on CNVT_TOHW macro from skeletonfb.c */
static inline uint32_t jzfb_convert_color_to_hw(unsigned val,
	struct fb_bitfield *bf)
{
	return (((val << bf->length) + 0x7FFF - val) >> 16) << bf->offset;
}

static int jzfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			unsigned blue, unsigned transp, struct fb_info *fb)
{
	uint32_t color;

	if (regno >= 16)
		return -EINVAL;

	color = jzfb_convert_color_to_hw(red, &fb->var.red);
	color |= jzfb_convert_color_to_hw(green, &fb->var.green);
	color |= jzfb_convert_color_to_hw(blue, &fb->var.blue);
	color |= jzfb_convert_color_to_hw(transp, &fb->var.transp);

	((uint32_t *)(fb->pseudo_palette))[regno] = color;

	return 0;
}

static int jzfb_get_controller_bpp(struct jzfb *jzfb)
{
	switch (jzfb->pdata->bpp) {
	case 18:
	case 24:
		return 32;
	case 15:
		return 16;
	default:
		return jzfb->pdata->bpp;
	}
}

static struct fb_videomode *jzfb_get_mode(struct jzfb *jzfb,
	struct fb_var_screeninfo *var)
{
	size_t i;
	struct fb_videomode *mode = jzfb->pdata->modes;

	for (i = 0; i < jzfb->pdata->num_modes; ++i, ++mode) {
		if (mode->xres == var->xres && mode->yres == var->yres)
			return mode;
	}

	return NULL;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct jzfb *jzfb = fb->par;
	struct fb_videomode *mode;

	if (var->bits_per_pixel != jzfb_get_controller_bpp(jzfb) &&
		var->bits_per_pixel != jzfb->pdata->bpp)
		return -EINVAL;

	mode = jzfb_get_mode(jzfb, var);
	if (mode == NULL)
		return -EINVAL;

	fb_videomode_to_var(var, mode);

	switch (jzfb->pdata->bpp) {
	case 8:
		break;
	case 15:
		var->red.offset = 10;
		var->red.length = 5;
		var->green.offset = 6;
		var->green.length = 5;
		var->blue.offset = 0;
		var->blue.length = 5;
		break;
	case 16:
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		break;
	case 18:
		var->red.offset = 16;
		var->red.length = 6;
		var->green.offset = 8;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 6;
		var->bits_per_pixel = 32;
		break;
	case 32:
	case 24:
		var->transp.offset = 24;
		var->transp.length = 8;
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->bits_per_pixel = 32;
		break;
	default:
		break;
	}

	return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct jz4740_fb_platform_data *pdata = jzfb->pdata;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_videomode *mode;
	uint16_t hds, vds;
	uint16_t hde, vde;
	uint16_t ht, vt;
	uint32_t ctrl;
	uint32_t cfg;
	unsigned long rate;

	mode = jzfb_get_mode(jzfb, var);
	if (mode == NULL)
		return -EINVAL;

	if (mode == info->mode)
		return 0;

	info->mode = mode;

	hds = mode->hsync_len + mode->left_margin;
	hde = hds + mode->xres;
	ht = hde + mode->right_margin;

	vds = mode->vsync_len + mode->upper_margin;
	vde = vds + mode->yres;
	vt = vde + mode->lower_margin;

	ctrl = JZ_LCD_CTRL_OFUP | JZ_LCD_CTRL_BURST_16;

	switch (pdata->bpp) {
	case 1:
		ctrl |= JZ_LCD_CTRL_BPP_1;
		break;
	case 2:
		ctrl |= JZ_LCD_CTRL_BPP_2;
		break;
	case 4:
		ctrl |= JZ_LCD_CTRL_BPP_4;
		break;
	case 8:
		ctrl |= JZ_LCD_CTRL_BPP_8;
	break;
	case 15:
		ctrl |= JZ_LCD_CTRL_RGB555; /* Falltrough */
	case 16:
		ctrl |= JZ_LCD_CTRL_BPP_15_16;
		break;
	case 18:
	case 24:
	case 32:
		ctrl |= JZ_LCD_CTRL_BPP_18_24;
		break;
	default:
		break;
	}

	cfg = pdata->lcd_type & 0xf;

	if (!(mode->sync & FB_SYNC_HOR_HIGH_ACT))
		cfg |= JZ_LCD_CFG_HSYNC_ACTIVE_LOW;

	if (!(mode->sync & FB_SYNC_VERT_HIGH_ACT))
		cfg |= JZ_LCD_CFG_VSYNC_ACTIVE_LOW;

	if (pdata->pixclk_falling_edge)
		cfg |= JZ_LCD_CFG_PCLK_FALLING_EDGE;

	if (pdata->date_enable_active_low)
		cfg |= JZ_LCD_CFG_DE_ACTIVE_LOW;

	if (pdata->lcd_type == JZ_LCD_TYPE_GENERIC_18_BIT)
		cfg |= JZ_LCD_CFG_18_BIT;

	if (mode->pixclock) {
		rate = PICOS2KHZ(mode->pixclock) * 1000;
		mode->refresh = rate / vt / ht;
	} else {
		if (pdata->lcd_type == JZ_LCD_TYPE_8BIT_SERIAL)
			rate = mode->refresh * (vt + 2 * mode->xres) * ht;
		else
			rate = mode->refresh * vt * ht;

		mode->pixclock = KHZ2PICOS(rate / 1000);
	}

	if (!jzfb->is_enabled)
		clk_enable(jzfb->ldclk);
	else
		ctrl |= JZ_LCD_CTRL_ENABLE;

	switch (pdata->lcd_type) {
	case JZ_LCD_TYPE_SPECIAL_TFT_1:
	case JZ_LCD_TYPE_SPECIAL_TFT_2:
	case JZ_LCD_TYPE_SPECIAL_TFT_3:
		writel(pdata->special_tft_config.spl, jzfb->base + JZ_REG_LCD_SPL);
		writel(pdata->special_tft_config.cls, jzfb->base + JZ_REG_LCD_CLS);
		writel(pdata->special_tft_config.ps, jzfb->base + JZ_REG_LCD_PS);
		writel(pdata->special_tft_config.ps, jzfb->base + JZ_REG_LCD_REV);
		break;
	default:
		cfg |= JZ_LCD_CFG_PS_DISABLE;
		cfg |= JZ_LCD_CFG_CLS_DISABLE;
		cfg |= JZ_LCD_CFG_SPL_DISABLE;
		cfg |= JZ_LCD_CFG_REV_DISABLE;
		break;
	}

	writel(mode->hsync_len, jzfb->base + JZ_REG_LCD_HSYNC);
	writel(mode->vsync_len, jzfb->base + JZ_REG_LCD_VSYNC);

	writel((ht << 16) | vt, jzfb->base + JZ_REG_LCD_VAT);

	writel((hds << 16) | hde, jzfb->base + JZ_REG_LCD_DAH);
	writel((vds << 16) | vde, jzfb->base + JZ_REG_LCD_DAV);

	writel(cfg, jzfb->base + JZ_REG_LCD_CFG);

	writel(ctrl, jzfb->base + JZ_REG_LCD_CTRL);

	if (!jzfb->is_enabled)
		clk_disable(jzfb->ldclk);

	clk_set_rate(jzfb->lpclk, rate);
	clk_set_rate(jzfb->ldclk, rate * 3);

	return 0;
}

static void jzfb_enable(struct jzfb *jzfb)
{
	uint32_t ctrl;

	clk_enable(jzfb->ldclk);

	jz_gpio_bulk_resume(jz_lcd_ctrl_pins, jzfb_num_ctrl_pins(jzfb));
	jz_gpio_bulk_resume(jz_lcd_data_pins, jzfb_num_data_pins(jzfb));

	writel(0, jzfb->base + JZ_REG_LCD_STATE);

	writel(jzfb->framedesc->next, jzfb->base + JZ_REG_LCD_DA0);

	ctrl = readl(jzfb->base + JZ_REG_LCD_CTRL);
	ctrl |= JZ_LCD_CTRL_ENABLE;
	ctrl &= ~JZ_LCD_CTRL_DISABLE;
	writel(ctrl, jzfb->base + JZ_REG_LCD_CTRL);
}

static void jzfb_disable(struct jzfb *jzfb)
{
	uint32_t ctrl;

	ctrl = readl(jzfb->base + JZ_REG_LCD_CTRL);
	ctrl |= JZ_LCD_CTRL_DISABLE;
	writel(ctrl, jzfb->base + JZ_REG_LCD_CTRL);
	do {
		ctrl = readl(jzfb->base + JZ_REG_LCD_STATE);
	} while (!(ctrl & JZ_LCD_STATE_DISABLED));

	jz_gpio_bulk_suspend(jz_lcd_ctrl_pins, jzfb_num_ctrl_pins(jzfb));
	jz_gpio_bulk_suspend(jz_lcd_data_pins, jzfb_num_data_pins(jzfb));

	clk_disable(jzfb->ldclk);
}

static int jzfb_blank(int blank_mode, struct fb_info *info)
{
	struct jzfb *jzfb = info->par;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (jzfb->is_enabled) {
			return 0;
		}

		jzfb_enable(jzfb);
		jzfb->is_enabled = 1;

		break;
	default:
		if (!jzfb->is_enabled) {
			return 0;
		}

		jzfb_disable(jzfb);
		jzfb->is_enabled = 0;

		break;
	}

	return 0;
}

static int jzfb_alloc_devmem(struct jzfb *jzfb)
{
	int max_videosize = 0;
	struct fb_videomode *mode = jzfb->pdata->modes;
	void *page;
	int i;

	for (i = 0; i < jzfb->pdata->num_modes; ++mode, ++i) {
		if (max_videosize < mode->xres * mode->yres)
			max_videosize = mode->xres * mode->yres;
	}

	max_videosize *= jzfb_get_controller_bpp(jzfb) >> 3;

	jzfb->framedesc = dma_alloc_coherent(&jzfb->pdev->dev,
					sizeof(*jzfb->framedesc),
					&jzfb->framedesc_phys, GFP_KERNEL);

	if (!jzfb->framedesc)
		return -ENOMEM;

	jzfb->vidmem_size = PAGE_ALIGN(max_videosize);
	jzfb->vidmem = dma_alloc_coherent(&jzfb->pdev->dev,
					jzfb->vidmem_size,
					&jzfb->vidmem_phys, GFP_KERNEL);

	if (!jzfb->vidmem)
		goto err_free_framedesc;

	for (page = jzfb->vidmem;
		 page < jzfb->vidmem + PAGE_ALIGN(jzfb->vidmem_size);
		 page += PAGE_SIZE) {
		SetPageReserved(virt_to_page(page));
	}

	jzfb->framedesc->next = jzfb->framedesc_phys;
	jzfb->framedesc->addr = jzfb->vidmem_phys;
	jzfb->framedesc->id = 0xdeafbead;
	jzfb->framedesc->cmd = 0;
	jzfb->framedesc->cmd |= max_videosize / 4;

	return 0;

err_free_framedesc:
	dma_free_coherent(&jzfb->pdev->dev, sizeof(*jzfb->framedesc),
				jzfb->framedesc, jzfb->framedesc_phys);
	return -ENOMEM;
}

static void jzfb_free_devmem(struct jzfb *jzfb)
{
	dma_free_coherent(&jzfb->pdev->dev, jzfb->vidmem_size,
				jzfb->vidmem, jzfb->vidmem_phys);
	dma_free_coherent(&jzfb->pdev->dev, sizeof(*jzfb->framedesc),
				jzfb->framedesc, jzfb->framedesc_phys);
}

static struct  fb_ops jzfb_ops = {
	.fb_activate_var = jzfb_check_var,
//	.fb_enable = jzfb_blank,
//	.fb_disable = jzfb_setcolreg,
};

static int __devinit jzfb_probe(struct platform_device *pdev)
{
	int ret;
	struct jzfb *jzfb;
	struct fb_info *fb;
	struct jz4740_fb_platform_data *pdata = pdev->dev.platform_data;
	struct resource *mem;

	if (!pdata) {
		dev_err(&pdev->dev, "Missing platform data\n");
		return -ENXIO;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "Failed to get register memory resource\n");
		return -ENXIO;
	}

	mem = request_mem_region(mem->start, resource_size(mem), pdev->name);
	if (!mem) {
		dev_err(&pdev->dev, "Failed to request register memory region\n");
		return -EBUSY;
	}

	fb = framebuffer_alloc(sizeof(struct jzfb), &pdev->dev);
	if (!fb) {
		dev_err(&pdev->dev, "Failed to allocate framebuffer device\n");
		ret = -ENOMEM;
		goto err_release_mem_region;
	}

	fb->fbops = &jzfb_ops;
	fb->flags = FBINFO_DEFAULT;

	jzfb = fb->par;
	jzfb->pdev = pdev;
	jzfb->pdata = pdata;
	jzfb->mem = mem;

	jzfb->ldclk = clk_get(&pdev->dev, "lcd");
	if (IS_ERR(jzfb->ldclk)) {
		ret = PTR_ERR(jzfb->ldclk);
		dev_err(&pdev->dev, "Failed to get lcd clock: %d\n", ret);
		goto err_framebuffer_release;
	}

	jzfb->lpclk = clk_get(&pdev->dev, "lcd_pclk");
	if (IS_ERR(jzfb->lpclk)) {
		ret = PTR_ERR(jzfb->lpclk);
		dev_err(&pdev->dev, "Failed to get lcd pixel clock: %d\n", ret);
		goto err_put_ldclk;
	}

	jzfb->base = ioremap(mem->start, resource_size(mem));
	if (!jzfb->base) {
		dev_err(&pdev->dev, "Failed to ioremap register memory region\n");
		ret = -EBUSY;
		goto err_put_lpclk;
	}

	platform_set_drvdata(pdev, jzfb);

	fb_videomode_to_modelist(pdata->modes, pdata->num_modes,
				 &fb->modelist);
	fb_videomode_to_var(&fb->var, pdata->modes);
	fb->var.bits_per_pixel = pdata->bpp;
	jzfb_check_var(&fb->var, fb);

	ret = jzfb_alloc_devmem(jzfb);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate video memory\n");
		goto err_iounmap;
	}

	fb->screen_base = jzfb->vidmem;
	fb->pseudo_palette = jzfb->pseudo_palette;

	fb_alloc_cmap(&fb->cmap, 256, 0);

	clk_enable(jzfb->ldclk);
	jzfb->is_enabled = 1;

	writel(jzfb->framedesc->next, jzfb->base + JZ_REG_LCD_DA0);

	fb->mode = NULL;
	jzfb_set_par(fb);

	jz_gpio_bulk_request(jz_lcd_ctrl_pins, jzfb_num_ctrl_pins(jzfb));
	jz_gpio_bulk_request(jz_lcd_data_pins, jzfb_num_data_pins(jzfb));

	ret = register_framebuffer(fb);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register framebuffer: %d\n", ret);
		goto err_free_devmem;
	}

	jzfb->fb = fb;

	return 0;

err_free_devmem:
	jz_gpio_bulk_free(jz_lcd_ctrl_pins, jzfb_num_ctrl_pins(jzfb));
	jz_gpio_bulk_free(jz_lcd_data_pins, jzfb_num_data_pins(jzfb));

	fb_dealloc_cmap(&fb->cmap);
	jzfb_free_devmem(jzfb);
err_iounmap:
	iounmap(jzfb->base);
err_put_lpclk:
	clk_put(jzfb->lpclk);
err_put_ldclk:
	clk_put(jzfb->ldclk);
err_framebuffer_release:
	framebuffer_release(fb);
err_release_mem_region:
	release_mem_region(mem->start, resource_size(mem));
	return ret;
}

static int __devexit jzfb_remove(struct platform_device *pdev)
{
	struct jzfb *jzfb = platform_get_drvdata(pdev);

	jzfb_blank(FB_BLANK_POWERDOWN, jzfb->fb);

	jz_gpio_bulk_free(jz_lcd_ctrl_pins, jzfb_num_ctrl_pins(jzfb));
	jz_gpio_bulk_free(jz_lcd_data_pins, jzfb_num_data_pins(jzfb));

	iounmap(jzfb->base);
	release_mem_region(jzfb->mem->start, resource_size(jzfb->mem));

	fb_dealloc_cmap(&jzfb->fb->cmap);
	jzfb_free_devmem(jzfb);

	platform_set_drvdata(pdev, NULL);

	clk_put(jzfb->lpclk);
	clk_put(jzfb->ldclk);

	framebuffer_release(jzfb->fb);

	return 0;
}

#define JZFB_PM_OPS NULL

static struct platform_driver jzfb_driver = {
	.probe = jzfb_probe,
	.remove = __devexit_p(jzfb_remove),
	.driver = {
		.name = "jz4740-fb",
		.pm = JZFB_PM_OPS,
	},
};
#endif

/*
 * Framebuffer driver for Ingenic JZ4740 SoC
 *
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Copyright (C) 2009 - 2010 NVIDIA Corporation
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <common.h>
#include <init.h>
#include <fb.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <mach/jz4740_fb.h>

#define	DMAC_BASE	0xB3020000

#define MAX_DMA_NUM	6  /* max 6 channels */

#define DMAC_DSAR(n)	(0x00 + (n) * 0x20) /* DMA source address */
#define DMAC_DTAR(n)	(0x04 + (n) * 0x20) /* DMA target address */
#define DMAC_DTCR(n)	(0x08 + (n) * 0x20) /* DMA transfer count */
#define DMAC_DRSR(n)	(0x0c + (n) * 0x20) /* DMA request source */
#define DMAC_DCCSR(n)	(0x10 + (n) * 0x20) /* DMA control/status */
#define DMAC_DCMD(n)	(0x14 + (n) * 0x20) /* DMA command */
#define DMAC_DDA(n)	(0x18 + (n) * 0x20) /* DMA descriptor address */
#define DMAC_DMACR	(0x0300)              /* DMA control register */
#define DMAC_DMAIPR	(0x0304)              /* DMA interrupt pending */
#define DMAC_DMADBR	(0x0308)              /* DMA doorbell */
#define DMAC_DMADBSR	(0x030C)              /* DMA doorbell set */

// DMA request source register
#define DMAC_DRSR_RS_BIT	0
#define DMAC_DRSR_RS_MASK	(0x1f << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AUTO	(8 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART0OUT	(20 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART0IN	(21 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SSIOUT	(22 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SSIIN	(23 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AICOUT	(24 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AICIN	(25 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSCOUT	(26 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSCIN	(27 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_TCU	(28 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SADC	(29 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SLCD	(30 << DMAC_DRSR_RS_BIT)

// DMA control register
#define DMAC_DMACR_PR_BIT	8  /* channel priority mode */
#define DMAC_DMACR_PR_MASK	(0x03 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_012345	(0 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_023145	(1 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_201345	(2 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_RR	(3 << DMAC_DMACR_PR_BIT) /* round robin */
#define DMAC_DMACR_HLT		(1 << 3)  /* DMA halt flag */
#define DMAC_DMACR_AR		(1 << 2)  /* address error flag */
#define DMAC_DMACR_DMAE		(1 << 0)  /* DMA enable bit */

// DMA channel control/status register
#define DMAC_DCCSR_NDES		(1 << 31) /* descriptor (0) or not (1) ? */
#define DMAC_DCCSR_CDOA_BIT	16        /* copy of DMA offset address */
#define DMAC_DCCSR_CDOA_MASK	(0xff << DMAC_DCCSR_CDOA_BIT)
#define DMAC_DCCSR_INV		(1 << 6)  /* descriptor invalid */
#define DMAC_DCCSR_AR		(1 << 4)  /* address error */
#define DMAC_DCCSR_TT		(1 << 3)  /* transfer terminated */
#define DMAC_DCCSR_HLT		(1 << 2)  /* DMA halted */
#define DMAC_DCCSR_CT		(1 << 1)  /* count terminated */
#define DMAC_DCCSR_EN		(1 << 0)  /* channel enable bit */

// DMA channel command register
#define DMAC_DCMD_SAI		(1 << 23) /* source address increment */
#define DMAC_DCMD_DAI		(1 << 22) /* dest address increment */
#define DMAC_DCMD_RDIL_BIT	16        /* request detection interval length */
#define DMAC_DCMD_RDIL_MASK	(0x0f << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_IGN	(0 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_2	(1 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_4	(2 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_8	(3 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_12	(4 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_16	(5 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_20	(6 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_24	(7 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_28	(8 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_32	(9 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_48	(10 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_60	(11 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_64	(12 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_124	(13 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_128	(14 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_200	(15 << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_SWDH_BIT	14  /* source port width */
#define DMAC_DCMD_SWDH_MASK	(0x03 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_32	(0 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_8	(1 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_16	(2 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_DWDH_BIT	12  /* dest port width */
#define DMAC_DCMD_DWDH_MASK	(0x03 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_32	(0 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_8	(1 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_16	(2 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DS_BIT	8  /* transfer data size of a data unit */
#define DMAC_DCMD_DS_MASK	(0x07 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_32BIT	(0 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_8BIT	(1 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_16BIT	(2 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_16BYTE	(3 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_32BYTE	(4 << DMAC_DCMD_DS_BIT)
#define DMAC_DCMD_TM		(1 << 7)  /* transfer mode: 0-single 1-block */
#define DMAC_DCMD_DES_V		(1 << 4)  /* descriptor valid flag */
#define DMAC_DCMD_DES_VM	(1 << 3)  /* descriptor valid mask: 1:support V-bit */
#define DMAC_DCMD_DES_VIE	(1 << 2)  /* DMA valid error interrupt enable */
#define DMAC_DCMD_TIE		(1 << 1)  /* DMA transfer interrupt enable */
#define DMAC_DCMD_LINK		(1 << 0)  /* descriptor link enable */


/*
 * Descriptor structure for JZ4740 DMA engine
 * Note: this structure must always be aligned to a 16-bytes boundary.
 */
struct jz_fb_dma_descriptor {
	u32 dcmd;	/* DCMD value for the current transfer */
	u32 dsadr;	/* DSAR value for the current transfer */
	u32 dtadr;	/* DTAR value for the current transfer */
	u32 ddadr;	/* Points to the next descriptor + transfer count */
};

struct jz4740_fb_info {
	struct device_d *dev;
	void __iomem *base;
	struct fb_info info;
	struct fb_videomode *mode;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;
	struct jzfb_framedesc *framedesc;
	dma_addr_t framedesc_phys;
};

#define SLCD_FIFO       (0xB0)  /* SLCD FIFO Register */

static void jz4740_fb_enable_controller2(struct jz4740_fb_info *fbi)
{
	int dma_chan;
	struct jz_fb_dma_descriptor *desc;
	dma_addr_t next;

	desc = xmemalign(16, sizeof(struct jz_fb_dma_descriptor));

	desc->dcmd = DMAC_DCMD_SAI
		| DMAC_DCMD_RDIL_IGN
		| DMAC_DCMD_SWDH_32
		| DMAC_DCMD_DWDH_16
		| DMAC_DCMD_DS_16BYTE
		| DMAC_DCMD_TM
		| DMAC_DCMD_DES_V
		| DMAC_DCMD_DES_VIE
		| DMAC_DCMD_LINK;

	desc->dsadr = fbi->vidmem_phys;
	desc->dtadr = (0x1fffffff & (dma_addr_t)(fbi->base + SLCD_FIFO));
	next = (0x1fffffff & (dma_addr_t)desc) >> 4;

	desc->ddadr = ((next << 24) | ((fbi->vidmem_size / 16) & 0xffffff));

	dma_chan = 1;

	/* Init the SLCD DMA and Enable */
	writel(DMAC_DRSR_RS_SLCD, DMAC_BASE + DMAC_DRSR(dma_chan));
	writel(DMAC_DMACR_DMAE, DMAC_BASE + DMAC_DMACR);
	writel(DMAC_DCCSR_EN, DMAC_BASE + DMAC_DCCSR(dma_chan));

	writel((0x1fffffff & (dma_addr_t) desc), DMAC_BASE + DMAC_DDA(dma_chan));

	/* Start DMA */
	writel((1 << (dma_chan)), DMAC_BASE + DMAC_DMADBSR);
	printf("jz4740_fb_enable_controller (screen = %p)\n", fbi->vidmem);
}

/**
 * @param fb_info Framebuffer information
 */
static void jz4740_fb_enable_controller(struct fb_info *fb_info)
{
	struct jz4740_fb_info *fbi = fb_info->priv;
	uint32_t ctrl;

	writel(0, fbi->base + JZ_REG_LCD_STATE);

	writel(fbi->framedesc->next, fbi->base + JZ_REG_LCD_DA0);

	ctrl = readl(fbi->base + JZ_REG_LCD_CTRL);
	ctrl |= JZ_LCD_CTRL_ENABLE;
	ctrl &= ~JZ_LCD_CTRL_DISABLE;
	writel(ctrl, fbi->base + JZ_REG_LCD_CTRL);
	jz4740_fb_enable_controller2(fbi);
}

/**
 * @param fb_info Framebuffer information
 */
static void jz4740_fb_disable_controller(struct fb_info *fb_info)
{
}

/**
 * Prepare the video hardware for a specified video mode
 * @param fb_info Framebuffer information
 * @return 0 on success
 */
static int jz4740_fb_activate_var(struct fb_info *fb_info)
{
	//struct fb_videomode *mode = fb_info->mode;

	return 0;
}

/*
 * There is only one video hardware instance available.
 * It makes no sense to dynamically allocate this data
 */
static struct fb_ops jz4740_fb_ops = {
	.fb_activate_var = jz4740_fb_activate_var,
	.fb_enable = jz4740_fb_enable_controller,
	.fb_disable = jz4740_fb_disable_controller,
};

static int jz4740_fb_probe(struct device_d *dev)
{
	struct jz4740_fb_platform_data *pdata = dev->platform_data;
	struct jz4740_fb_info *fbi;
	struct fb_info *info;
	int ret;
	int max_videosize;

	if (! pdata)
		return -ENODEV;

	fbi = xzalloc(sizeof(*fbi));
	fbi->base = dev_request_mem_region(dev, 0);

	info = &fbi->info;

	info->priv = fbi;

	info->mode = pdata->mode;
	info->xres = 320;
	info->yres = 240;
	info->bits_per_pixel = 16;
	info->fbops = &jz4740_fb_ops;

	dev_info(dev, "JZ4740 framebuffer driver\n");

//	if (pdata->framebuffer)
//		info->screen_base = pdata->framebuffer;
//	else

	fbi->vidmem_size = info->xres * info->yres * (info->bits_per_pixel >> 3);
	max_videosize = fbi->vidmem_size;
	fbi->vidmem = xzalloc(fbi->vidmem_size);
	info->screen_base = fbi->vidmem;

	fbi->framedesc = xzalloc(sizeof(struct jzfb_framedesc));

	/* HACK */
	fbi->vidmem_phys = 0x1fffffff & (dma_addr_t) fbi->vidmem;
	fbi->framedesc_phys = 0x1fffffff & (dma_addr_t) fbi->framedesc;

	fbi->framedesc->next = fbi->framedesc_phys;
	fbi->framedesc->addr = fbi->vidmem_phys;
	fbi->framedesc->id = 0xdeafbead;
	fbi->framedesc->cmd = 0;
	fbi->framedesc->cmd |= max_videosize / 4;

	jz4740_fb_activate_var(info);

	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(dev, "failed to register framebuffer\n");
		return ret;
	}

	return 0;
}

static struct driver_d jz4740_fb_driver = {
	.name	= "jz4740_fb",
	.probe	= jz4740_fb_probe,
};

static int jz4740_fb_init(void)
{
	return register_driver(&jz4740_fb_driver);
}
device_initcall(jz4740_fb_init);
