// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
 */

#include <common.h>
#include <dma.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <driver.h>
#include <platform_data/eth-fec.h>
#include <io.h>
#include <clock.h>
#include <xfuncs.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <of_net.h>
#include <of_gpio.h>
#include <regulator.h>
#include <gpio.h>
#include <linux/iopoll.h>

#include "fec_imx.h"

static int fec_set_promisc(struct eth_device *edev, bool enable)
{
	struct fec_priv *fec = (struct fec_priv *)edev->priv;
	u32 rcntl;

	rcntl = readl(fec->regs + FEC_R_CNTRL);

	if (enable)
		rcntl |= FEC_R_CNTRL_PROMISC;
	else
		rcntl &= ~FEC_R_CNTRL_PROMISC;

	writel(rcntl, fec->regs + FEC_R_CNTRL);

	return 0;
}

/*
 * MII-interface related functions
 */
static int fec_miibus_read(struct mii_bus *bus, int phyAddr, int regAddr)
{
	struct fec_priv *fec = (struct fec_priv *)bus->priv;

	uint32_t reg;		/* convenient holder for the PHY register */
	uint32_t phy;		/* convenient holder for the PHY */

	writel(((clk_get_rate(fec->clk[FEC_CLK_IPG]) >> 20) / 5) << 1,
			fec->regs + FEC_MII_SPEED);
	/*
	 * reading from any PHY's register is done by properly
	 * programming the FEC's MII data register.
	 */
	writel(FEC_IEVENT_MII, fec->regs + FEC_IEVENT);
	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	writel(FEC_MII_DATA_ST | FEC_MII_DATA_OP_RD | FEC_MII_DATA_TA | phy | reg, fec->regs + FEC_MII_DATA);

	/*
	 * wait for the related interrupt
	 */
	if (readl_poll_timeout(fec->regs + FEC_IEVENT, reg,
			       reg & FEC_IEVENT_MII, USEC_PER_MSEC)) {
		dev_err(&fec->edev.dev, "Read MDIO failed...\n");
		return -1;
	}

	/*
	 * clear mii interrupt bit
	 */
	writel(FEC_IEVENT_MII, fec->regs + FEC_IEVENT);

	/*
	 * it's now safe to read the PHY's register
	 */
	return readl(fec->regs + FEC_MII_DATA) & 0xffff;
}

static int fec_miibus_write(struct mii_bus *bus, int phyAddr,
	int regAddr, u16 data)
{
	struct fec_priv *fec = (struct fec_priv *)bus->priv;

	uint32_t reg;		/* convenient holder for the PHY register */
	uint32_t phy;		/* convenient holder for the PHY */

	writel(((clk_get_rate(fec->clk[FEC_CLK_IPG]) >> 20) / 5) << 1,
			fec->regs + FEC_MII_SPEED);

	reg = regAddr << FEC_MII_DATA_RA_SHIFT;
	phy = phyAddr << FEC_MII_DATA_PA_SHIFT;

	writel(FEC_MII_DATA_ST | FEC_MII_DATA_OP_WR |
		FEC_MII_DATA_TA | phy | reg | data, fec->regs + FEC_MII_DATA);

	/*
	 * wait for the MII interrupt
	 */
	if (readl_poll_timeout(fec->regs + FEC_IEVENT, reg,
			       reg & FEC_IEVENT_MII, USEC_PER_MSEC)) {
		dev_err(&fec->edev.dev, "Write MDIO failed...\n");
		return -1;
	}

	/*
	 * clear MII interrupt bit
	 */
	writel(FEC_IEVENT_MII, fec->regs + FEC_IEVENT);

	return 0;
}

static int fec_rx_task_enable(struct fec_priv *fec)
{
	writel(1 << 24, fec->regs + FEC_R_DES_ACTIVE);
	return 0;
}

static int fec_rx_task_disable(struct fec_priv *fec)
{
	return 0;
}

static int fec_tx_task_enable(struct fec_priv *fec)
{
	writel(1 << 24, fec->regs + FEC_X_DES_ACTIVE);
	return 0;
}

static int fec_tx_task_disable(struct fec_priv *fec)
{
	return 0;
}

/**
 * Swap endianess to send data on an i.MX28 based platform
 * @param buf Pointer to little endian data
 * @param len Size in words (max. 1500 bytes)
 * @return Pointer to the big endian data
 */
static void *imx28_fix_endianess_wr(uint32_t *buf, unsigned wlen)
{
	unsigned u;
	static uint32_t data[376];	/* = 1500 bytes + 4 bytes */

	for (u = 0; u < wlen; u++, buf++)
		data[u] = __swab32(*buf);

	return data;
}

/**
 * Swap endianess to read data on an i.MX28 based platform
 * @param buf Pointer to little endian data
 * @param len Size in words (max. 1500 bytes)
 *
 * TODO: Check for the risk of destroying some other data behind the buffer
 * if its size is not a multiple of 4.
 */
static void imx28_fix_endianess_rd(uint32_t *buf, unsigned wlen)
{
	unsigned u;

	for (u = 0; u < wlen; u++, buf++)
		*buf = __swab32(*buf);
}

/**
 * Initialize receive task's buffer descriptors
 * @param[in] fec all we know about the device yet
 * @param[in] count receive buffer count to be allocated
 * @param[in] size size of each receive buffer
 * @return 0 on success
 *
 * For this task we need additional memory for the data buffers. And each
 * data buffer requires some alignment. Thy must be aligned to a specific
 * boundary each (DB_DATA_ALIGNMENT).
 */
static int fec_rbd_init(struct fec_priv *fec, int count, int size)
{
	int ix;

	for (ix = 0; ix < count; ix++) {
		writew(FEC_RBD_EMPTY, &fec->rbd_base[ix].status);
		writew(0, &fec->rbd_base[ix].data_length);
	}
	/*
	 * mark the last RBD to close the ring
	 */
	writew(FEC_RBD_WRAP | FEC_RBD_EMPTY, &fec->rbd_base[ix - 1].status);
	fec->rbd_index = 0;

	return 0;
}

/**
 * Initialize transmit task's buffer descriptors
 * @param[in] fec all we know about the device yet
 *
 * Transmit buffers are created externally. We only have to init the BDs here.\n
 * Note: There is a race condition in the hardware. When only one BD is in
 * use it must be marked with the WRAP bit to use it for every transmit.
 * This bit in combination with the READY bit results into double transmit
 * of each data buffer. It seems the state machine checks READY earlier then
 * resetting it after the first transfer.
 * Using two BDs solves this issue.
 */
static void fec_tbd_init(struct fec_priv *fec)
{
	writew(0x0000, &fec->tbd_base[0].status);
	writew(FEC_TBD_WRAP, &fec->tbd_base[1].status);
	fec->tbd_index = 0;
}

/**
 * Mark the given read buffer descriptor as free
 * @param[in] last 1 if this is the last buffer descriptor in the chain, else 0
 * @param[in] pRbd buffer descriptor to mark free again
 */
static void fec_rbd_clean(int last, struct buffer_descriptor __iomem *pRbd)
{
	/*
	 * Reset buffer descriptor as empty
	 */
	if (last)
		writew(FEC_RBD_WRAP | FEC_RBD_EMPTY, &pRbd->status);
	else
		writew(FEC_RBD_EMPTY, &pRbd->status);
	/*
	 * no data in it
	 */
	writew(0, &pRbd->data_length);
}

static int fec_get_hwaddr(struct eth_device *dev, unsigned char *mac)
{
	return -1;
}

static int fec_set_hwaddr(struct eth_device *dev, const unsigned char *mac)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;

	/*
	 * Set physical address
	 */
	writel((mac[0] << 24) + (mac[1] << 16) + (mac[2] << 8) + mac[3], fec->regs + FEC_PADDR1);
	writel((mac[4] << 24) + (mac[5] << 16) + 0x8808, fec->regs + FEC_PADDR2);

	return 0;
}

static int fec_init(struct eth_device *dev)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;
	u32 rcntl, xwmrk;

	/*
	 * Clear FEC-Lite interrupt event register(IEVENT)
	 */
	writel(0xffffffff, fec->regs + FEC_IEVENT);

	/*
	 * Set interrupt mask register
	 */
	writel(0x00000000, fec->regs + FEC_IMASK);

	rcntl = readl(fec->regs + FEC_R_CNTRL);

	/* Keep promisc setting */
	rcntl &= FEC_R_CNTRL_PROMISC;

	/*
	 * Set FEC-Lite receive control register(R_CNTRL):
	 */
	rcntl |= FEC_R_CNTRL_MAX_FL(1518);

	rcntl |= FEC_R_CNTRL_MII_MODE;
	/*
	 * Set MII_SPEED = (1/(mii_speed * 2)) * System Clock
	 * and do not drop the Preamble.
	 */
	writel(((clk_get_rate(fec->clk[FEC_CLK_IPG]) >> 20) / 5) << 1,
			fec->regs + FEC_MII_SPEED);

	if (fec->interface == PHY_INTERFACE_MODE_RMII) {
		if (fec_is_imx28(fec) || fec_is_imx6(fec)) {
			rcntl |= FEC_R_CNTRL_RMII_MODE | FEC_R_CNTRL_FCE |
				FEC_R_CNTRL_NO_LGTH_CHECK;
		} else {
			/* disable the gasket and wait */
			writel(0, fec->regs + FEC_MIIGSK_ENR);
			while (readl(fec->regs + FEC_MIIGSK_ENR) & FEC_MIIGSK_ENR_READY)
				udelay(1);

			/* configure the gasket for RMII, 50 MHz, no loopback, no echo */
			writel(FEC_MIIGSK_CFGR_IF_MODE_RMII, fec->regs + FEC_MIIGSK_CFGR);

			/* re-enable the gasket */
			writel(FEC_MIIGSK_ENR_EN, fec->regs + FEC_MIIGSK_ENR);
		}
	}

	if (fec->interface == PHY_INTERFACE_MODE_RGMII ||
	    fec->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    fec->interface == PHY_INTERFACE_MODE_RGMII_TXID ||
	    fec->interface == PHY_INTERFACE_MODE_RGMII_RXID)
		rcntl |= 1 << 6;

	writel(rcntl, fec->regs + FEC_R_CNTRL);

	/*
	 * Set Opcode/Pause Duration Register
	 */
	writel(0x00010020, fec->regs + FEC_OP_PAUSE);

	xwmrk = 0x2;

	/* set ENET tx at store and forward mode */
	if (fec_is_imx6(fec))
		xwmrk |= 1 << 8;

	writel(xwmrk, fec->regs + FEC_X_WMRK);

	/*
	 * Set multicast address filter
	 */
	writel(0, fec->regs + FEC_IADDR1);
	writel(0, fec->regs + FEC_IADDR2);
	writel(0, fec->regs + FEC_GADDR1);
	writel(0, fec->regs + FEC_GADDR2);

	/* size of each buffer */
	writel(FEC_MAX_PKT_SIZE, fec->regs + FEC_EMRBR);

	/* set rx and tx buffer descriptor base address */
	writel(virt_to_phys(fec->tbd_base), fec->regs + FEC_ETDSR);
	writel(virt_to_phys(fec->rbd_base), fec->regs + FEC_ERDSR);

	return 0;
}

static void fec_update_linkspeed(struct eth_device *edev)
{
	struct fec_priv *fec = (struct fec_priv *)edev->priv;
	int speed = edev->phydev->speed;
	u32 rcntl = readl(fec->regs + FEC_R_CNTRL) & ~FEC_R_CNTRL_RMII_10T;
	u32 ecntl = readl(fec->regs + FEC_ECNTRL) & ~FEC_ECNTRL_SPEED;

	if (speed == SPEED_10)
		rcntl |= FEC_R_CNTRL_RMII_10T;

	if (speed == SPEED_1000)
		ecntl |= FEC_ECNTRL_SPEED;

	writel(rcntl, fec->regs + FEC_R_CNTRL);
	writel(ecntl, fec->regs + FEC_ECNTRL);
}

/**
 * Start the FEC engine
 * @param[in] edev Our device to handle
 */
static int fec_open(struct eth_device *edev)
{
	struct fec_priv *fec = (struct fec_priv *)edev->priv;
	int ret;
	u32 ecr;

	ret = phy_device_connect(edev, &fec->miibus, fec->phy_addr,
				 fec_update_linkspeed, fec->phy_flags,
				 fec->interface);
	if (ret)
		return ret;

	if (fec->phy_init)
		fec->phy_init(edev->phydev);

	fec_init(edev);

	/*
	 * Initialize RxBD/TxBD rings
	 */
	fec_rbd_init(fec, FEC_RBD_NUM, FEC_MAX_PKT_SIZE);
	fec_tbd_init(fec);

	/* full-duplex, heartbeat disabled */
	writel(1 << 2, fec->regs + FEC_X_CNTRL);
	fec->rbd_index = 0;

	/*
	 * Enable FEC-Lite controller
	 */
	ecr = FEC_ECNTRL_ETHER_EN;

	/* Enable Swap to support little-endian device */
	if (fec_is_imx6(fec))
		ecr |= 0x100;

	writel(ecr, fec->regs + FEC_ECNTRL);
	/*
	 * Enable SmartDMA receive task
	 */
	fec_rx_task_enable(fec);

	return 0;
}

/**
 * Halt the FEC engine
 * @param[in] dev Our device to handle
 */
static void fec_halt(struct eth_device *dev)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;
	uint32_t reg;

	/*
	 * Only halt if fec has been started. Otherwise we would have to wait
	 * for the timeout below.
	 */
	if (!(readl(fec->regs + FEC_ECNTRL) & FEC_ECNTRL_ETHER_EN))
		return;

	/* issue graceful stop command to the FEC transmitter if necessary */
	writel(readl(fec->regs + FEC_X_CNTRL) | FEC_ECNTRL_RESET,
			fec->regs + FEC_X_CNTRL);

	/* wait for graceful stop to register */
	if (readl_poll_timeout(fec->regs + FEC_IEVENT, reg,
			       reg & FEC_IEVENT_GRA, USEC_PER_SEC))
		dev_err(&dev->dev, "graceful stop timeout\n");

	/* Disable SmartDMA tasks */
	fec_tx_task_disable(fec);
	fec_rx_task_disable(fec);

	/*
	 * Disable the Ethernet Controller
	 * Note: this will also reset the BD index counter!
	 */
	writel(0, fec->regs + FEC_ECNTRL);
	fec->rbd_index = 0;
	fec->tbd_index = 0;
}

/**
 * Transmit one frame
 * @param[in] dev Our ethernet device to handle
 * @param[in] eth_data Pointer to the data to be transmitted
 * @param[in] data_length Data count in bytes
 * @return 0 on success
 */
static int fec_send(struct eth_device *dev, void *eth_data, int data_length)
{
	unsigned int status;
	dma_addr_t dma;

	/*
	 * This routine transmits one frame.  This routine only accepts
	 * 6-byte Ethernet addresses.
	 */
	struct fec_priv *fec = (struct fec_priv *)dev->priv;

	/* Check for valid length of data. */
	if ((data_length > 1500) || (data_length <= 0)) {
		dev_err(&dev->dev, "Payload (%d) to large!\n", data_length);
		return -1;
	}

	if (!IS_ALIGNED((unsigned long)eth_data, DB_DATA_ALIGNMENT))
		dev_warn(&dev->dev, "Transmit data not aligned: %p!\n", eth_data);

	/*
	 * Setup the transmit buffer
	 * Note: We are always using the first buffer for transmission,
	 * the second will be empty and only used to stop the DMA engine
	 */
	if (fec_is_imx28(fec))
		eth_data = imx28_fix_endianess_wr(eth_data, (data_length + 3) >> 2);

	writew(data_length, &fec->tbd_base[fec->tbd_index].data_length);

	dma = dma_map_single(fec->dev, eth_data, data_length, DMA_TO_DEVICE);
	if (dma_mapping_error(fec->dev, dma))
		return -EFAULT;

	writel((uint32_t)(dma), &fec->tbd_base[fec->tbd_index].data_pointer);

	/*
	 * update BD's status now
	 * This block:
	 * - is always the last in a chain (means no chain)
	 * - should transmitt the CRC
	 * - might be the last BD in the list, so the address counter should
	 *   wrap (-> keep the WRAP flag)
	 */
	status = readw(&fec->tbd_base[fec->tbd_index].status) & FEC_TBD_WRAP;
	status |= FEC_TBD_LAST | FEC_TBD_TC | FEC_TBD_READY;
	writew(status, &fec->tbd_base[fec->tbd_index].status);
	/* Enable SmartDMA transmit task */
	fec_tx_task_enable(fec);

	if (readw_poll_timeout(&fec->tbd_base[fec->tbd_index].status,
			       status, !(status & FEC_TBD_READY), USEC_PER_SEC))
		dev_err(&dev->dev, "transmission timeout\n");

	dma_unmap_single(fec->dev, dma, data_length, DMA_TO_DEVICE);

	/* for next transmission use the other buffer */
	if (fec->tbd_index)
		fec->tbd_index = 0;
	else
		fec->tbd_index = 1;

	return 0;
}

/**
 * Pull one frame from the card
 * @param[in] dev Our ethernet device to handle
 * @return Length of packet read
 */
static void fec_recv(struct eth_device *dev)
{
	struct fec_priv *fec = (struct fec_priv *)dev->priv;
	struct buffer_descriptor __iomem *rbd = &fec->rbd_base[fec->rbd_index];
	uint32_t ievent;
	int len = 0;
	uint16_t bd_status;

	/*
	 * Check if any critical events have happened
	 */
	ievent = readl(fec->regs + FEC_IEVENT);
	ievent &= ~FEC_IEVENT_MII;
	writel(ievent, fec->regs + FEC_IEVENT);

	if (ievent & FEC_IEVENT_BABT) {
		/* BABT, Rx/Tx FIFO errors */
		fec_halt(dev);
		fec_init(dev);
		dev_err(&dev->dev, "some error: 0x%08x\n", ievent);
		return;
	}
	if (!fec_is_imx28(fec)) {
		if (ievent & FEC_IEVENT_HBERR) {
			/* Heartbeat error */
			writel(readl(fec->regs + FEC_X_CNTRL) | 0x1,
					fec->regs + FEC_X_CNTRL);
		}
	}
	if (ievent & FEC_IEVENT_GRA) {
		/* Graceful stop complete */
		if (readl(fec->regs + FEC_X_CNTRL) & 0x00000001) {
			fec_halt(dev);
			writel(readl(fec->regs + FEC_X_CNTRL) & ~0x00000001,
					fec->regs + FEC_X_CNTRL);
			fec_init(dev);
		}
	}

	/*
	 * ensure reading the right buffer status
	 */
	bd_status = readw(&rbd->status);

	if (bd_status & FEC_RBD_EMPTY)
		return;

	if (bd_status & FEC_RBD_ERR) {
		dev_warn(&dev->dev, "error frame: 0x%p 0x%08x\n",
			 rbd, bd_status);
	} else if (bd_status & FEC_RBD_LAST) {
		const uint16_t data_length = readw(&rbd->data_length);

		if (data_length - 4 > 14) {
			void *frame = phys_to_virt(readl(&rbd->data_pointer));
			/*
			 * Sync the data for CPU so that endianness
			 * fixup and net_receive below would get
			 * proper data
			 */
			dma_sync_single_for_cpu(fec->dev, (unsigned long)frame,
						data_length,
						DMA_FROM_DEVICE);
			if (fec_is_imx28(fec))
				imx28_fix_endianess_rd(frame,
						       (data_length + 3) >> 2);

			/*
			 * Get buffer address and size
			 */
			len = data_length - 4;
			net_receive(dev, frame, len);
			dma_sync_single_for_device(fec->dev, (unsigned long)frame,
						   data_length,
						   DMA_FROM_DEVICE);
		}
	}
	/*
	 * free the current buffer, restart the engine
	 * and move forward to the next buffer
	 */
	fec_rbd_clean(fec->rbd_index == (FEC_RBD_NUM - 1) ? 1 : 0, rbd);
	fec_rx_task_enable(fec);
	fec->rbd_index = (fec->rbd_index + 1) % FEC_RBD_NUM;
}

static int fec_alloc_receive_packets(struct fec_priv *fec, int count, int size)
{
	void *p;
	int i;


	p = dma_alloc(size * count);
	if (!p)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		dma_addr_t dma;
		/*
		 * Make sure there are no outstanding writes to the
		 * region of memory we are going to use as receive
		 * buffers as well as check that DMA mapping is valid
		 */
		dma = dma_map_single(fec->dev, p, size, DMA_FROM_DEVICE);
		if (dma_mapping_error(fec->dev, dma))
			return -EFAULT;

		writel(dma, &fec->rbd_base[i].data_pointer);
		p += size;
	}

	return 0;
}

static void fec_free_receive_packets(struct fec_priv *fec, int count, int size)
{
	void *p = phys_to_virt(fec->rbd_base[0].data_pointer);
	dma_free(p);
}

#ifdef CONFIG_OFDEVICE
static int fec_probe_dt(struct device *dev, struct fec_priv *fec)
{
	struct device_node *mdiobus;
	int ret;

	ret = of_get_phy_mode(dev->of_node);
	if (ret < 0)
		fec->interface = PHY_INTERFACE_MODE_MII;
	else
		fec->interface = ret;

	mdiobus = of_get_child_by_name(dev->of_node, "mdio");
	if (mdiobus)
		fec->miibus.dev.of_node = mdiobus;

	return 0;
}
#else
static int fec_probe_dt(struct device *dev, struct fec_priv *fec)
{
	return -ENODEV;
}
#endif

static int fec_clk_enable(struct fec_priv *fec)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fec->clk); i++) {
		const int err = clk_enable(fec->clk[i]);
		if (err < 0)
			return err;
	}

	for (i = 0; i < ARRAY_SIZE(fec->opt_clk); i++) {
		if (!IS_ERR_OR_NULL(fec->opt_clk[i])) {
			const int err = clk_enable(fec->opt_clk[i]);
			if (err < 0)
				return err;
		}
	}

	return 0;
}

static void fec_clk_disable(struct fec_priv *fec)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fec->clk); i++) {
		if (!IS_ERR_OR_NULL(fec->clk[i]))
			clk_disable(fec->clk[i]);
	}

	for (i = 0; i < ARRAY_SIZE(fec->opt_clk); i++) {
		if (!IS_ERR_OR_NULL(fec->opt_clk[i])) {
			clk_disable(fec->opt_clk[i]);
		}
	}
}

static void fec_clk_put(struct fec_priv *fec)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fec->clk); i++) {
		if (!IS_ERR_OR_NULL(fec->clk[i]))
			clk_put(fec->clk[i]);
	}

	for (i = 0; i < ARRAY_SIZE(fec->opt_clk); i++) {
		if (!IS_ERR_OR_NULL(fec->opt_clk[i]))
			clk_put(fec->opt_clk[i]);
	}
}

static int fec_clk_get(struct fec_priv *fec)
{
	int i, err = 0;
	static const char *clk_names[ARRAY_SIZE(fec->clk)] = {
		"ipg", "ahb",
	};
	static const char *opt_clk_names[ARRAY_SIZE(fec->opt_clk)] = {
		"enet_clk_ref", "enet_out", "ptp"
	};

	for (i = 0; i < ARRAY_SIZE(fec->clk); i++) {
		fec->clk[i] = clk_get(fec->edev.parent, clk_names[i]);
		if (IS_ERR(fec->clk[i])) {
			err = PTR_ERR(fec->clk[i]);
			fec_clk_put(fec);
			return err;
		}
	}

	for (i = 0; i < ARRAY_SIZE(fec->opt_clk); i++) {
		fec->opt_clk[i] = clk_get(fec->edev.parent, opt_clk_names[i]);
		if (IS_ERR(fec->opt_clk[i])) {
			fec->opt_clk[i] = NULL;
		}
	}

	return err;
}

static int fec_probe(struct device *dev)
{
	struct resource *iores;
	struct fec_platform_data *pdata = (struct fec_platform_data *)dev->platform_data;
	struct eth_device *edev;
	struct fec_priv *fec;
	void *base;
	int ret;
	enum fec_type type;
	void const *type_v;
	int phy_reset;
	u32 msec = 1, phy_post_delay = 0;
	u32 reg;

	ret = dev_get_drvdata(dev, &type_v);
	if (ret)
		return ret;

	type = (uintptr_t)(type_v);

	fec = xzalloc(sizeof(*fec));
	fec->type = type;
	fec->dev = dev;
	edev = &fec->edev;
	dev->priv = fec;
	edev->priv = fec;
	edev->open = fec_open;
	edev->send = fec_send;
	edev->recv = fec_recv;
	edev->halt = fec_halt;
	edev->get_ethaddr = fec_get_hwaddr;
	edev->set_ethaddr = fec_set_hwaddr;
	edev->set_promisc = fec_set_promisc;
	edev->parent = dev;

	dma_set_mask(dev, DMA_BIT_MASK(32));

	ret = fec_clk_get(fec);
	if (ret < 0)
		goto err_free;

	ret = fec_clk_enable(fec);
	if (ret < 0)
		goto put_clk;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto disable_clk;
	}
	fec->regs = IOMEM(iores->start);

	fec->reg_phy = regulator_get(dev, "phy");
	if (IS_ERR(fec->reg_phy)) {
		if (PTR_ERR(fec->reg_phy) == -EPROBE_DEFER) {
			ret = -EPROBE_DEFER;
			fec->reg_phy = NULL;
			goto release_res;
		}
		fec->reg_phy = NULL;
	}

	ret = regulator_enable(fec->reg_phy);
	if (ret) {
		dev_err(dev, "Failed to enable phy regulator: %d\n", ret);
		goto release_res;
	}

	phy_reset = of_get_named_gpio(dev->of_node, "phy-reset-gpios", 0);
	if (gpio_is_valid(phy_reset)) {
		of_property_read_u32(dev->of_node, "phy-reset-duration",
				     &msec);
		of_property_read_u32(dev->of_node, "phy-reset-post-delay",
				     &phy_post_delay);
		/* valid reset duration should be less than 1s */
		if (phy_post_delay > 1000)
			goto release_res;

		ret = gpio_request(phy_reset, "phy-reset");
		if (ret)
			goto release_res;

		ret = gpio_direction_output(phy_reset, 0);
		if (ret)
			goto free_gpio;

		mdelay(msec);
		gpio_set_value(phy_reset, 1);

		if (phy_post_delay)
			mdelay(phy_post_delay);
	}

	/* Reset chip. */
	writel(FEC_ECNTRL_RESET, fec->regs + FEC_ECNTRL);
	ret = readl_poll_timeout(fec->regs + FEC_ECNTRL, reg,
				 !(reg & FEC_ECNTRL_RESET), USEC_PER_SEC);
	if (ret)
		goto free_gpio;

	fec_set_promisc(edev, false);

	/*
	 * reserve memory for both buffer descriptor chains at once
	 * Datasheet forces the startaddress of each chain is 16 byte aligned
	 */
#define FEC_XBD_SIZE ((2 + FEC_RBD_NUM) * sizeof(struct buffer_descriptor))

	base = dma_alloc_coherent(FEC_XBD_SIZE, DMA_ADDRESS_BROKEN);
	fec->rbd_base = base;
	base += FEC_RBD_NUM * sizeof(struct buffer_descriptor);
	fec->tbd_base = base;

	ret = fec_alloc_receive_packets(fec, FEC_RBD_NUM, FEC_MAX_PKT_SIZE);
	if (ret < 0)
		goto free_xbd;

	if (dev->of_node) {
		ret = fec_probe_dt(dev, fec);
		fec->phy_addr = -1;
	} else if (pdata) {
		fec->interface = pdata->xcv_type;
		fec->phy_init = pdata->phy_init;
		fec->phy_addr = pdata->phy_addr;
	} else {
		fec->interface = PHY_INTERFACE_MODE_MII;
		fec->phy_addr = -1;
	}

	if (ret)
		goto free_receive_packets;

	fec->miibus.read = fec_miibus_read;
	fec->miibus.write = fec_miibus_write;

	fec->miibus.priv = fec;
	fec->miibus.parent = dev;

	ret = eth_register(edev);
	if (ret)
		goto free_receive_packets;

	ret = mdiobus_register(&fec->miibus);
	if (ret)
		goto unregister_eth;

	return 0;

unregister_eth:
	eth_unregister(edev);
free_receive_packets:
	fec_free_receive_packets(fec, FEC_RBD_NUM, FEC_MAX_PKT_SIZE);
free_xbd:
	dma_free_coherent(fec->rbd_base, 0, FEC_XBD_SIZE);
free_gpio:
	if (gpio_is_valid(phy_reset))
		gpio_free(phy_reset);
release_res:
	if (fec->reg_phy)
		regulator_disable(fec->reg_phy);

	release_region(iores);
disable_clk:
	fec_clk_disable(fec);
put_clk:
	fec_clk_put(fec);
err_free:
	free(fec);
	return ret;
}

static void fec_remove(struct device *dev)
{
	struct fec_priv *fec = dev->priv;

	fec_halt(&fec->edev);
}

static __maybe_unused struct of_device_id imx_fec_dt_ids[] = {
	{
		.compatible = "fsl,imx25-fec",
		.data = (void *)FEC_TYPE_IMX27,
	}, {
		.compatible = "fsl,imx27-fec",
		.data = (void *)FEC_TYPE_IMX27,
	}, {
		.compatible = "fsl,imx28-fec",
		.data = (void *)FEC_TYPE_IMX28,
	}, {
		.compatible = "fsl,imx6q-fec",
		.data = (void *)FEC_TYPE_IMX6,
	}, {
		.compatible = "fsl,imx6sx-fec",
		.data = (void *)FEC_TYPE_IMX6,
	}, {
		.compatible = "fsl,imx8mp-fec",
		.data = (void *)FEC_TYPE_IMX6,
	}, {
		.compatible = "fsl,mvf600-fec",
		.data = (void *)FEC_TYPE_IMX6,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, imx_fec_dt_ids);

static struct platform_device_id imx_fec_ids[] = {
	{
		.name = "imx27-fec",
		.driver_data = (unsigned long)FEC_TYPE_IMX27,
	}, {
		.name = "imx28-fec",
		.driver_data = (unsigned long)FEC_TYPE_IMX28,
	}, {
		.name = "imx6-fec",
		.driver_data = (unsigned long)FEC_TYPE_IMX6,
	}, {
		/* sentinel */
	},
};

/**
 * Driver description for registering
 */
static struct driver fec_driver = {
	.name   = "fec_imx",
	.probe  = fec_probe,
	.remove = fec_remove,
	.of_compatible = DRV_OF_COMPAT(imx_fec_dt_ids),
	.id_table = imx_fec_ids,
};
device_platform_driver(fec_driver);

/**
 * @file
 * @brief Network driver for FreeScale's FEC implementation.
 * This type of hardware can be found on i.MX27 CPUs
 */
