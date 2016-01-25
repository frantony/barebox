/*
 * Copyright (C) 2013, 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
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
#include <driver.h>
#include <spi/spi.h>
#include <io.h>
#include <clock.h>

enum ath79_spi_state {
	ATH79_SPI_STATE_WAIT_CMD = 0,
	ATH79_SPI_STATE_WAIT_READ,
};

struct ath79_spi {
	struct spi_master	master;
	void __iomem		*regs;
	u32			val;
	u32			reg_ctrl;

	enum ath79_spi_state	state;
	u32			clk_div;
	unsigned long 		read_addr;
	unsigned long		ahb_rate;
};

#define AR71XX_SPI_REG_FS	0x00	/* Function Select */
#define AR71XX_SPI_REG_CTRL	0x04	/* SPI Control */
#define AR71XX_SPI_REG_IOC	0x08	/* SPI I/O Control */
#define AR71XX_SPI_REG_RDS	0x0c	/* Read Data Shift */

#define AR71XX_SPI_FS_GPIO	BIT(0)	/* Enable GPIO mode */

#define AR71XX_SPI_IOC_DO	BIT(0)	/* Data Out pin */
#define AR71XX_SPI_IOC_CLK	BIT(8)	/* CLK pin */
#define AR71XX_SPI_IOC_CS(n)	BIT(16 + (n))
#define AR71XX_SPI_IOC_CS0	AR71XX_SPI_IOC_CS(0)
#define AR71XX_SPI_IOC_CS1	AR71XX_SPI_IOC_CS(1)
#define AR71XX_SPI_IOC_CS2	AR71XX_SPI_IOC_CS(2)
#define AR71XX_SPI_IOC_CS_ALL	(AR71XX_SPI_IOC_CS0 | AR71XX_SPI_IOC_CS1 | \
				 AR71XX_SPI_IOC_CS2)

static inline u32 ath79_spi_rr(struct ath79_spi *sp, int reg)
{
	return __raw_readl(sp->regs + reg);
}

static inline void ath79_spi_wr(struct ath79_spi *sp, u32 val, int reg)
{
	__raw_writel(val, sp->regs + reg);
}

static inline void setbits(struct ath79_spi *sp, int bits, int on)
{
	/*
	 * We are the only user of SCSPTR so no locking is required.
	 * Reading bit 2 and 0 in SCSPTR gives pin state as input.
	 * Writing the same bits sets the output value.
	 * This makes regular read-modify-write difficult so we
	 * use sp->val to keep track of the latest register value.
	 */

	if (on)
		sp->val |= bits;
	else
		sp->val &= ~bits;

	ath79_spi_wr(sp, sp->val, AR71XX_SPI_REG_IOC);
}

static inline struct ath79_spi *ath79_spidev_to_sp(struct spi_device *spi)
{
	return container_of(spi->master, struct ath79_spi, master);
}

static inline void setsck(struct spi_device *spi, int on)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);

	setbits(sc, AR71XX_SPI_IOC_CLK, on);
}

static inline void setmosi(struct spi_device *spi, int on)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);

	setbits(sc, AR71XX_SPI_IOC_DO, on);
}

static inline u32 getmiso(struct spi_device *spi)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);

	return !!((ath79_spi_rr(sc, AR71XX_SPI_REG_RDS) & 1));
}

#define spidelay(nsecs) udelay(nsecs/1000)

#include "spi-bitbang-txrx.h"

static inline void ath79_spi_chipselect(struct ath79_spi *sp, int chipselect)
{
	int off_bits;

	off_bits = 0xffffffff;

	switch (chipselect) {
	case 0:
		off_bits &= ~AR71XX_SPI_IOC_CS0;
		break;

	case 1:
		off_bits &= ~AR71XX_SPI_IOC_CS1;
		break;

	case 2:
		off_bits &= ~AR71XX_SPI_IOC_CS2;
		break;

	case 3:
		break;
	}

	/* by default inactivate chip selects */
	sp->val |= AR71XX_SPI_IOC_CS_ALL;
	sp->val &= off_bits;

	ath79_spi_wr(sp, sp->val, AR71XX_SPI_REG_IOC);
}

static int ath79_spi_setup(struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	struct device_d spi_dev = spi->dev;

	if (spi->bits_per_word != 8) {
		dev_err(master->dev, "master doesn't support %d bits per word requested by %s\n",
			spi->bits_per_word, spi_dev.name);
		return -EINVAL;
	}

	if ((spi->mode & (SPI_CPHA | SPI_CPOL)) != SPI_MODE_0) {
		dev_err(master->dev, "master doesn't support SPI_MODE%d requested by %s\n",
			spi->mode & (SPI_CPHA | SPI_CPOL), spi_dev.name);
		return -EINVAL;
	}

	return 0;
}

static int ath79_spi_read(struct spi_device *spi, void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	u8 *rxf_buf = buf;

	while (cnt < nbyte) {
		*rxf_buf = bitbang_txrx_be_cpha1(spi, 1000, 1, 0, 8);
		rxf_buf++;
		cnt++;
	}

	return cnt;
}

static int ath79_spi_write(struct spi_device *spi,
				const void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	const u8 *txf_buf = buf;

	while (cnt < nbyte) {
		bitbang_txrx_be_cpha1(spi, 1000, 1, (u32)*txf_buf, 8);
		txf_buf++;
		cnt++;
	}

	return 0;
}


static void ath79_spi_enable(struct ath79_spi *sp)
{
	/* enable GPIO mode */
	ath79_spi_wr(sp, AR71XX_SPI_FS_GPIO, AR71XX_SPI_REG_FS);

	/* save CTRL register */
	sp->reg_ctrl = ath79_spi_rr(sp, AR71XX_SPI_REG_CTRL);
	sp->val = ath79_spi_rr(sp, AR71XX_SPI_REG_IOC);
}

static void ath79_spi_disable(struct ath79_spi *sp)
{
	/* restore CTRL register */
	ath79_spi_wr(sp, sp->reg_ctrl, AR71XX_SPI_REG_CTRL);
	/* disable GPIO mode */
	ath79_spi_wr(sp, 0, AR71XX_SPI_REG_FS);
}

static int ath79_spi_do_read_flash_data(struct spi_device *spi,
					struct spi_transfer *t)
{
	struct ath79_spi *sp = ath79_spidev_to_sp(spi);

	/* disable GPIO mode */
	ath79_spi_wr(sp, 0, AR71XX_SPI_REG_FS);

	memcpy_fromio(t->rx_buf, sp->regs + sp->read_addr, t->len);

	/* enable GPIO mode */
	ath79_spi_wr(sp, AR71XX_SPI_FS_GPIO, AR71XX_SPI_REG_FS);

	/* restore IOC register */
	ath79_spi_wr(sp, sp->val, AR71XX_SPI_REG_IOC);

	return t->len;
}

static int ath79_spi_do_read_flash_cmd(struct spi_device *spi,
				       struct spi_transfer *t)
{
	struct ath79_spi *sp = ath79_spidev_to_sp(spi);
	int len;
	const u8 *p;

	sp->read_addr = 0;

	len = t->len - 1;

	if (t->dummy)
		len -= 1;

	p = t->tx_buf;

	while (len--) {
		p++;
		sp->read_addr <<= 8;
		sp->read_addr |= *p;
	}

	return t->len;
}

static bool ath79_spi_is_read_cmd(struct spi_device *spi,
				 struct spi_transfer *t)
{
	return t->type == SPI_TRANSFER_FLASH_READ_CMD;
}

static bool ath79_spi_is_data_read(struct spi_device *spi,
				  struct spi_transfer *t)
{
	return t->type == SPI_TRANSFER_FLASH_READ_DATA;
}

static int ath79_spi_txrx_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct ath79_spi *sp = ath79_spidev_to_sp(spi);
	int ret;

	switch (sp->state) {
	case ATH79_SPI_STATE_WAIT_CMD:
		if (ath79_spi_is_read_cmd(spi, t)) {
			ret = ath79_spi_do_read_flash_cmd(spi, t);
			sp->state = ATH79_SPI_STATE_WAIT_READ;
		} else {
			if (t->tx_buf)
				ath79_spi_write(spi, t->tx_buf, t->len);

			if (t->rx_buf)
				ath79_spi_read(spi, t->rx_buf, t->len);
		}
		break;

	case ATH79_SPI_STATE_WAIT_READ:
		if (ath79_spi_is_data_read(spi, t)) {
			ret = ath79_spi_do_read_flash_data(spi, t);
		} else {
			dev_warn(&spi->dev, "flash data read expected\n");
			ret = -EIO;
		}
		sp->state = ATH79_SPI_STATE_WAIT_CMD;
		break;

	default:
		BUG();
	}

	return ret;
}

static int ath79_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct ath79_spi *sc = ath79_spidev_to_sp(spi);
	struct spi_transfer *t;

	mesg->actual_length = 0;

	/* activate chip select signal */
	ath79_spi_chipselect(sc, spi->chip_select);

	list_for_each_entry(t, &mesg->transfers, transfer_list) {

		ath79_spi_txrx_bufs(spi, t);

		mesg->actual_length += t->len;
	}

	/* inactivate chip select signal */
	ath79_spi_chipselect(sc, -1);

	return 0;
}
#if 0
static int ath79_spi_setup_transfer(struct spi_device *spi,
				    struct spi_transfer *t)
{
	struct ath79_spi *sp = ath79_spidev_to_sp(spi);
	int ret;

	ret = spi_bitbang_setup_transfer(spi, t);
	if (ret)
		return ret;

	sp->bitbang.txrx_bufs = ath79_spi_txrx_bufs;

	return ret;
}
#endif

static int ath79_spi_probe(struct device_d *dev)
{
	struct spi_master *master;
	struct ath79_spi *ath79_spi;

	ath79_spi = xzalloc(sizeof(*ath79_spi));
	dev->priv = ath79_spi;

	master = &ath79_spi->master;
	master->dev = dev;

	master->bus_num = dev->id;
	master->setup = ath79_spi_setup;
	master->transfer = ath79_spi_transfer;
	master->num_chipselect = 3;

	if (IS_ENABLED(CONFIG_OFDEVICE)) {
		struct device_node *node = dev->device_node;
		u32 num_cs;
		int ret;

		ret = of_property_read_u32(node, "num-chipselects", &num_cs);
		if (ret)
			num_cs = 3;

		if (num_cs > 3) {
			dev_err(dev, "master doesn't support num-chipselects > 3\n");
		}

		master->num_chipselect = num_cs;
	}

	ath79_spi->regs = dev_request_mem_region(dev, 0);
	ath79_spi->state = ATH79_SPI_STATE_WAIT_CMD;

	/* enable gpio mode */
	ath79_spi_enable(ath79_spi);

	/* inactivate chip select signal */
	ath79_spi_chipselect(ath79_spi, -1);

	spi_register_master(master);

	return 0;
}

static void ath79_spi_remove(struct device_d *dev)
{
	struct ath79_spi *sp = dev->priv;

	ath79_spi_disable(sp);
}

static __maybe_unused struct of_device_id ath79_spi_dt_ids[] = {
	{
		.compatible = "qca,ath79-spi",
	},
	{
		/* sentinel */
	}
};

static struct driver_d ath79_spi_driver = {
	.name  = "ath79-spi",
	.probe = ath79_spi_probe,
	.remove = ath79_spi_remove,
	.of_compatible = DRV_OF_COMPAT(ath79_spi_dt_ids),
};
device_platform_driver(ath79_spi_driver);
