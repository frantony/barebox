/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

//#define DEBUG

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <io.h>
#include <clock.h>

struct spi_ar933x_master {
	int	num_chipselect;
	int	spi_mode;
	int	databits;
	int	speed;
	int	bus_num;
};

struct ar933x_spi {
	struct spi_master	master;
	int			databits;
	int			speed;
	int			mode;
	void __iomem		*regs;
	u32			val;

	u32			reg_ctrl;
};

/*
 * SPI block
 */
#define AR71XX_SPI_REG_FS	0x00	/* Function Select */
#define AR71XX_SPI_REG_CTRL	0x04	/* SPI Control */
#define AR71XX_SPI_REG_IOC	0x08	/* SPI I/O Control */
#define AR71XX_SPI_REG_RDS	0x0c	/* Read Data Shift */

#define AR71XX_SPI_FS_GPIO	BIT(0)	/* Enable GPIO mode */

#define AR71XX_SPI_CTRL_RD	BIT(6)	/* Remap Disable */
#define AR71XX_SPI_CTRL_DIV_MASK 0x3f

#define AR71XX_SPI_IOC_DO	BIT(0)	/* Data Out pin */
#define AR71XX_SPI_IOC_CLK	BIT(8)	/* CLK pin */
#define AR71XX_SPI_IOC_CS(n)	BIT(16 + (n))
#define AR71XX_SPI_IOC_CS0	AR71XX_SPI_IOC_CS(0)
#define AR71XX_SPI_IOC_CS1	AR71XX_SPI_IOC_CS(1)
#define AR71XX_SPI_IOC_CS2	AR71XX_SPI_IOC_CS(2)
#define AR71XX_SPI_IOC_CS_ALL	(AR71XX_SPI_IOC_CS0 | AR71XX_SPI_IOC_CS1 | \
				 AR71XX_SPI_IOC_CS2)


static inline u32 ath79_spi_rr(struct ar933x_spi *ar933x_spi, int reg)
{
	u32 ret = cpu_readl(ar933x_spi->regs + reg);

	//printf("ath79_spi_rr: addr=0x%08x, result=0x%08x\n", ar933x_spi->regs + reg, ret);

	return ret;
}

static inline void ath79_spi_wr(struct ar933x_spi *ar933x_spi, u32 val, int reg)
{
	//printf("ath79_spi_wr: addr=0x%08x, val=0x%08x\n", ar933x_spi->regs + reg, val);

	cpu_writel(val, ar933x_spi->regs + reg);
}

static inline void setbits(struct ar933x_spi *sp, int bits, int on)
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

static inline void setsck(struct spi_device *spi, int on)
{
	struct ar933x_spi *sc = container_of(spi->master, struct ar933x_spi, master);

	setbits(sc, AR71XX_SPI_IOC_CLK, on);
}

static inline void setmosi(struct spi_device *spi, int on)
{
	struct ar933x_spi *sc = container_of(spi->master, struct ar933x_spi, master);

	setbits(sc, AR71XX_SPI_IOC_DO, on);
}

static inline u32 getmiso(struct spi_device *spi)
{
	struct ar933x_spi *sc = container_of(spi->master, struct ar933x_spi, master);

	return !!((ath79_spi_rr(sc, AR71XX_SPI_REG_RDS) & 1));
}

#include "spi-bitbang-txrx.h"

static inline void ar933x_chipselect(struct ar933x_spi *sp, int chipselect)
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

static int ar933x_spi_setup(struct spi_device *spi)
{
	struct spi_master *master = spi->master;
	//struct device_d spi_dev = spi->dev;
	//struct ar933x_spi *sc = container_of(master, struct ar933x_spi, master);

#if 0
	dev_dbg(master->dev, "%s\n", __func__);
	printf("%s chipsel %d mode 0x%08x bits_per_word: %d speed: %d\n",
			__FUNCTION__, spi->chip_select, spi->mode, spi->bits_per_word,
			spi->max_speed_hz);
#endif

#if 0 // Open
	int mode;
	int timing = 6;

	/* end transmitting and receiving, clear FIFOs */
ATH933X_SPI_CONTROL_SEL_BB
	/* set up mode, and active chip select signal */
	/* set up timing */
	/* clear interrupt status */
	/* disable interrupt */
#endif

	return 0;
}

static int spibr_read(struct spi_device *spi, void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	volatile u8 *rxf_buf = buf;

	while (cnt < nbyte) {
		*rxf_buf = bitbang_txrx_be_cpha1(spi, 1000, 1, 0, 8);
		rxf_buf++;
		cnt++;
	}

	return cnt;
}

static int spibr_write(struct spi_device *spi, const void *buf, size_t nbyte)
{
	ssize_t cnt = 0;
	const u8 *txf_buf = buf;

	while (cnt < nbyte) {
		#if 0
		printf("spibr_write %02x\n", *txf_buf);
		#endif
		bitbang_txrx_be_cpha1(spi, 1000, 1, (u32)*txf_buf, 8);
		txf_buf++;
		cnt++;
	}

	return 0;
}

static int ar933x_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
	struct ar933x_spi *sc = container_of(spi->master, struct ar933x_spi, master);
	struct spi_master *master = spi->master;
	struct spi_transfer *t;
	unsigned int cs_change;

	dev_dbg(master->dev, "%s\n", __func__);

	cs_change = 0;

	mesg->actual_length = 0;
	ar933x_chipselect(sc, spi->chip_select);

	/* end transmitting and receiving, clear FIFOs */

	/* activate chip select */

	list_for_each_entry(t, &mesg->transfers, transfer_list) {

#if 0
		printf("t:\n");
		printf("  tx_buf = %p\n", t->tx_buf);
		printf("  rx_buf = %p\n", t->rx_buf);
		printf("  len = %d\n", t->len);
		printf("  cs_change = %d\n", t->cs_change);
		printf("  bits_per_word = %d\n", t->delay_usecs);
		printf("  speed_hz = %d\n", t->speed_hz);
#endif

		if (cs_change) {
			printf("cs_change\n");
		}

		cs_change = t->cs_change;

		if (t->tx_buf)
			spibr_write(spi, t->tx_buf, t->len);

		if (t->rx_buf)
			spibr_read(spi, t->rx_buf, t->len);

		// FIXME
		mesg->actual_length += t->len;
//		mesg->actual_length += altera_spi_do_xfer(spi, t);

//		if (cs_change) {
//			altera_spi_cs_active(spi);
//		}
	}

	/* set up mode and inactive chip select signal */
	ar933x_chipselect(sc, -1);

	return 0;
}

static void ath79_spi_enable(struct ar933x_spi *sp)
{
	/* enable GPIO mode */
	ath79_spi_wr(sp, AR71XX_SPI_FS_GPIO, AR71XX_SPI_REG_FS);

	/* save CTRL register */
	sp->reg_ctrl = ath79_spi_rr(sp, AR71XX_SPI_REG_CTRL);
//	sp->ioc_base = ath79_spi_rr(sp, AR71XX_SPI_REG_IOC);
	sp->val = ath79_spi_rr(sp, AR71XX_SPI_REG_IOC);

	/* TODO: setup speed? */
	ath79_spi_wr(sp, 0x43, AR71XX_SPI_REG_CTRL);
}

static void ath79_spi_disable(struct ar933x_spi *sp)
{
	/* restore CTRL register */
	ath79_spi_wr(sp, sp->reg_ctrl, AR71XX_SPI_REG_CTRL);
	/* disable GPIO mode */
	ath79_spi_wr(sp, 0, AR71XX_SPI_REG_FS);
}

static int ar933x_spi_probe(struct device_d *dev)
{
	struct spi_master *master;
	struct ar933x_spi *ar933x_spi;
	//struct spi_ar933x_master *pdata = dev->platform_data;

	//printf("%s() \n", __func__);

	ar933x_spi = xzalloc(sizeof(*ar933x_spi));
	dev->priv = ar933x_spi;

	master = &ar933x_spi->master;
	master->dev = dev;

	master->setup = ar933x_spi_setup;
	master->transfer = ar933x_spi_transfer;
//	master->num_chipselect = pdata->num_chipselect;
//	master->bus_num = pdata->bus_num;

	/* FIXME: chipselect = 2 register_device: already registered m25p800 */
	master->num_chipselect = 3;
	master->bus_num = 0;

	ar933x_spi->regs = dev_request_mem_region(dev, 0);

	/* enable gpio mode */

//	ar933x_spi->databits = pdata->databits;
//	ar933x_spi->speed = pdata->speed;
//	ar933x_spi->mode = pdata->spi_mode;

	ath79_spi_enable(ar933x_spi);

	ar933x_chipselect(ar933x_spi, -1);

	spi_register_master(master);

	return 0;
}

static struct driver_d ar933x_spi_driver = {
	.name  = "ar933x_spi",
	.probe = ar933x_spi_probe,
};

static int ar933x_spi_driver_init(void)
{
	return platform_driver_register(&ar933x_spi_driver);
}
device_initcall(ar933x_spi_driver_init);
