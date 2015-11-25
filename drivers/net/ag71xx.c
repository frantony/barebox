/*
 *  Atheros AR71xx built-in ethernet mac driver
 *
 *  Copyright (C) 2008-2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  Based on Atheros' AG7100 driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <common.h>
#include <net.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>

#include "ag71xx.h"

static inline u32 ag71xx_gmac_rr(struct ag71xx *dev, int reg)
{
	return __raw_readl(dev->regs_gmac + reg);
}

static inline void ag71xx_gmac_wr(struct ag71xx *dev, int reg, u32 val)
{
	__raw_writel(val, dev->regs_gmac + reg);
}

static inline u32 ag71xx_rr(struct ag71xx *dev, int reg)
{
	return __raw_readl(dev->regs + reg);
}

static inline void ag71xx_wr(struct ag71xx *dev, int reg, u32 val)
{
	__raw_writel(val, dev->regs + reg);
}

static int ag71xx_ether_mii_read(struct mii_bus *miidev, int addr, int reg)
{
	struct ag71xx *priv = miidev->priv;
	struct device_d *dev = priv->dev;

	dev_err(dev, "ag71xx_ether_mii_read\n");

	return 0;
}

static int ag71xx_ether_mii_write(struct mii_bus *miidev, int addr, int reg, u16 val)
{
	struct ag71xx *priv = miidev->priv;
	struct device_d *dev = priv->dev;

	dev_err(dev, "ag71xx_ether_mii_write\n");

	return 0;
}

static int ag71xx_ether_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	dev_err(dev, "ag71xx_ether_set_ethaddr\n");
	return 0;
}

static int ag71xx_ether_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	dev_err(dev, "ag71xx_ether_get_ethaddr\n");
	/* We have no eeprom */
	return -1;
}

static void ag71xx_ether_halt (struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;

	dev_err(dev, "ag71xx_ether_halt\n");

	ag71xx_wr(priv, AG71XX_REG_RX_CTRL, 0);
	while (ag71xx_rr(priv, AG71XX_REG_RX_CTRL))
		;
}

static int ag71xx_ether_rx(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	ag7240_desc_t *f;
	unsigned int work_done;

	//dev_err(dev, "ag71xx_ether_rx\n");

	for (work_done = 0; work_done < NO_OF_RX_FIFOS; work_done++) {
		unsigned int *next_rx = &priv->next_rx;
		unsigned int pktlen;
		f = &priv->fifo_rx[*next_rx];
		pktlen = f->pkt_size;

		//dev_err(dev, "ag71xx_ether_rx: pktlen %d\n", pktlen);
		//dev_err(dev, "ag71xx_ether_rx: owned %s\n", f->is_empty ? "yes": "no");
		if (f->is_empty)
			break;

		*next_rx = (*next_rx + 1) % NO_OF_RX_FIFOS;


		dma_sync_single_for_cpu((unsigned long)f->pkt_start_addr, pktlen,
					DMA_FROM_DEVICE);

		net_receive(edev, (unsigned char *)f->pkt_start_addr, pktlen - 4);

		dma_sync_single_for_device((unsigned long)f->pkt_start_addr, pktlen,
					   DMA_FROM_DEVICE);

		f->is_empty = 1;
		//dev_err(dev, "ag71xx_ether_rx %d\n", work_done);
	}


	if (!(ag71xx_rr(priv, AG71XX_REG_RX_CTRL))) {
		ag71xx_wr(priv, AG71XX_REG_RX_DESC, virt_to_phys(f));
		ag71xx_wr(priv, AG71XX_REG_RX_CTRL, 1);
	}

	return work_done;

	//dev_err(dev, "ag71xx_ether_rx\n");
	return 0;
}

static int ag71xx_ether_send(struct eth_device *edev, void *packet, int length)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	ag7240_desc_t *f = &priv->fifo_tx[priv->next_tx];
	int i;

	//dev_err(dev, "ag71xx_ether_send\n");

	dma_sync_single_for_device((unsigned long)packet, length, DMA_TO_DEVICE);

	f->pkt_start_addr = virt_to_phys(packet);
	f->res1 = 0;
	f->pkt_size = length;
	f->is_empty = 0;
	ag71xx_wr(priv, AG71XX_REG_TX_DESC, virt_to_phys(f));
	ag71xx_wr(priv, AG71XX_REG_TX_CTRL, TX_CTRL_TXE);

	dma_sync_single_for_cpu((unsigned long)packet, length, DMA_TO_DEVICE);

        for (i = 0; i < MAX_WAIT; i++) {
                mdelay(10);
                if (f->is_empty) {
                        break;
                }
        }

        if (i == MAX_WAIT) {
                dev_err(dev, "Tx Timed out\n");
        }

        f->pkt_start_addr = 0;
        f->pkt_size = 0;

	priv->next_tx++;
	priv->next_tx %= NO_OF_TX_FIFOS;

	return 0;
}

static int ag71xx_ether_open(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	dev_err(dev, "ag71xx_ether_open\n");
	return 0;
}

static int ag71xx_ether_init(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	int i;
	void *rxbuf = priv->rx_buffer;

	dev_err(dev, "ag71xx_ether_init\n");

        priv->next_rx = 0;

        for (i = 0; i < NO_OF_RX_FIFOS; i++) {
		u32  *next_rx = &priv->next_rx;
                ag7240_desc_t *fr = &priv->fifo_rx[*next_rx];
		fr->pkt_start_addr = virt_to_phys(rxbuf);
		fr->pkt_size = MAX_RBUFF_SZ;
		fr->is_empty = 1;

		dma_sync_single_for_device((unsigned long)rxbuf, MAX_RBUFF_SZ,
					   DMA_FROM_DEVICE);
		*next_rx = (*next_rx + 1) % NO_OF_RX_FIFOS;
		rxbuf += MAX_RBUFF_SZ;
        }

	/* Clean Tx BD's */
	memset(priv->fifo_tx, 0, TX_RING_SZ);

	dev_err(dev, "ag71xx_ether_init: enable RX\n");

	ag71xx_wr(priv, AG71XX_REG_RX_DESC, virt_to_phys(priv->fifo_rx));
	ag71xx_wr(priv, AG71XX_REG_RX_CTRL, RX_CTRL_RXE);

	dev_err(dev, "ag71xx_ether_init: done\n");

	return 1;
}

static int ag71xx_mii_setup(struct ag71xx *priv)
{
	struct device_d *dev = priv->dev;
	u32 rd;

	dev_err(dev, "ag71xx_mii_setup\n");

	rd = ag71xx_gmac_rr(priv, 0);
	rd |= AG71XX_ETH_CFG_MII_GE0_SLAVE;
	ag71xx_gmac_wr(priv, 0, rd);

	return 0;
}

static int ag71xx_probe(struct device_d *dev)
{
	void __iomem *regs, *regs_gmac;
	struct mii_bus *miibus;
	struct eth_device *edev;
	struct ag71xx *priv;
	u32 mac_h, mac_l;
	u32 rd;

	dev_err(dev, "ag71xx_probe\n");

	regs_gmac = dev_request_mem_region_by_name(dev, "gmac");
	if (IS_ERR(regs_gmac))
		return PTR_ERR(regs_gmac);

	dev_err(dev, "ag71xx_probe: regs gmac = %p\n", regs_gmac);

	regs = dev_request_mem_region_by_name(dev, "ge0");
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	dev_err(dev, "ag71xx_probe: regs ge0 = %p\n", regs);


	priv = xzalloc(sizeof(struct ag71xx));
	edev = &priv->netdev;
	miibus = &priv->miibus;
	edev->priv = priv;

	edev->init = ag71xx_ether_init;
	edev->open = ag71xx_ether_open;
	edev->send = ag71xx_ether_send;
	edev->recv = ag71xx_ether_rx;
	edev->halt = ag71xx_ether_halt;
	edev->get_ethaddr = ag71xx_ether_get_ethaddr;
	edev->set_ethaddr = ag71xx_ether_set_ethaddr;

	priv->dev = dev;
	priv->regs = regs;
	priv->regs_gmac = regs_gmac;

	miibus->read = ag71xx_ether_mii_read;
	miibus->write = ag71xx_ether_mii_write;
	miibus->priv = priv;

#if 0
	/* enable switch core*/
	rd = __raw_readl(base + AR933X_ETHSW_CLOCK_CONTROL_REG) & ~(0x1f);
	rd |= 0x10;
	__raw_writel(base + AR933X_ETHSW_CLOCK_CONTROL_REG, rd);
#endif

        if(ath79_reset_rr(AR933X_RESET_REG_RESET_MODULE) != 0)
	    ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, 0);

	dev_err(dev, "ag71xx_probe: reset MAC and MDIO\n");

	/* reset GE0 MAC and MDIO */
	rd = ath79_reset_rr(AR933X_RESET_REG_RESET_MODULE);
	rd |= AR933X_RESET_GE0_MAC | AR933X_RESET_GE0_MDIO | AR933X_RESET_SWITCH;
	ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, rd);
	mdelay(100);

	rd = ath79_reset_rr(AR933X_RESET_REG_RESET_MODULE);
	rd &= ~(AR933X_RESET_GE0_MAC | AR933X_RESET_GE0_MDIO | AR933X_RESET_SWITCH);
	ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, rd);
	mdelay(100);

	dev_err(dev, "ag71xx_probe: config MAC\n");

	ag71xx_wr(priv, AG71XX_REG_MAC_CFG1, (MAC_CFG1_SR | MAC_CFG1_TX_RST | MAC_CFG1_RX_RST));
	ag71xx_wr(priv, AG71XX_REG_MAC_CFG1, (MAC_CFG1_RXE | MAC_CFG1_TXE));

	rd = ag71xx_rr(priv, AG71XX_REG_MAC_CFG2);
	rd |= (MAC_CFG2_PAD_CRC_EN | MAC_CFG2_LEN_CHECK | MAC_CFG2_IF_10_100);
	ag71xx_wr(priv, AG71XX_REG_MAC_CFG2, rd);

	dev_err(dev, "ag71xx_probe: config FIFOs\n");

	/* config FIFOs */
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG0, 0x1f00);

	ag71xx_mii_setup(priv);

	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG1, 0x10ffff);
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG2, 0xAAA0555);

	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG4, 0x3ffff);
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG5, 0x66b82);
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG3, 0x1f00140);


	dev_err(dev, "ag71xx_probe: allocating DMA\n");
	priv->rx_buffer = dma_alloc_coherent(NO_OF_TX_FIFOS * MAX_RBUFF_SZ, DMA_ADDRESS_BROKEN);
	priv->fifo_tx = dma_alloc_coherent(NO_OF_TX_FIFOS * sizeof(ag7240_desc_t), DMA_ADDRESS_BROKEN);
	priv->fifo_rx = dma_alloc_coherent(NO_OF_RX_FIFOS * sizeof(ag7240_desc_t), DMA_ADDRESS_BROKEN);

	mac_l = 0x3344;
	mac_h = 0x0004d980;

	ag71xx_wr(priv, AG71XX_REG_MAC_ADDR1, mac_l);
	ag71xx_wr(priv, AG71XX_REG_MAC_ADDR2, mac_h);

	dev_err(dev, "ag71xx_probe: register miibus and edev\n");

	mdiobus_register(miibus);
	eth_register(edev);

	return 0;
}

static __maybe_unused struct of_device_id ag71xx_dt_ids[] = {
	{
		.compatible = "qca,ar7100-gmac",
	}, {
		/* sentinel */
	}
};

static struct driver_d ag71xx_driver = {
	.name	= AG71XX_DRV_NAME,
	.probe		= ag71xx_probe,
	.of_compatible = DRV_OF_COMPAT(ag71xx_dt_ids),
};
device_platform_driver(ag71xx_driver);
