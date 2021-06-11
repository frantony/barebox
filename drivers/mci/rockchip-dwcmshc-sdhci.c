// SPDX-License-Identifier: GPL-2.0-or-later

#include <clock.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/clk.h>
#include <mci.h>
#include <dma.h>
#include <linux/iopoll.h>

#include "sdhci.h"

/* DWCMSHC specific Mode Select value */
#define DWCMSHC_CTRL_HS400		0x7

#define DWCMSHC_VER_ID			0x500
#define DWCMSHC_VER_TYPE		0x504
#define DWCMSHC_HOST_CTRL3		0x508
#define DWCMSHC_EMMC_CONTROL		0x52c
#define DWCMSHC_EMMC_ATCTRL		0x540

/* Rockchip specific Registers */
#define DWCMSHC_EMMC_DLL_CTRL		0x800
#define DWCMSHC_EMMC_DLL_RXCLK		0x804
#define DWCMSHC_EMMC_DLL_TXCLK		0x808
#define DWCMSHC_EMMC_DLL_STRBIN		0x80c
#define DWCMSHC_EMMC_DLL_STATUS0	0x840
#define DWCMSHC_EMMC_DLL_START		BIT(0)
#define DWCMSHC_EMMC_DLL_RXCLK_SRCSEL	29
#define DWCMSHC_EMMC_DLL_START_POINT	16
#define DWCMSHC_EMMC_DLL_INC		8
#define DWCMSHC_EMMC_DLL_DLYENA		BIT(27)
#define DLL_TXCLK_TAPNUM_DEFAULT	0x8
#define DLL_STRBIN_TAPNUM_DEFAULT	0x8
#define DLL_TXCLK_TAPNUM_FROM_SW	BIT(24)
#define DLL_STRBIN_TAPNUM_FROM_SW	BIT(24)
#define DWCMSHC_EMMC_DLL_LOCKED		BIT(8)
#define DWCMSHC_EMMC_DLL_TIMEOUT	BIT(9)
#define DLL_RXCLK_NO_INVERTER		1
#define DLL_RXCLK_INVERTER		0
#define DWCMSHC_ENHANCED_STROBE		BIT(8)
#define DLL_LOCK_WO_TMOUT(x) \
	((((x) & DWCMSHC_EMMC_DLL_LOCKED) == DWCMSHC_EMMC_DLL_LOCKED) && \
	(((x) & DWCMSHC_EMMC_DLL_TIMEOUT) == 0))

#define SDHCI_DWCMSHC_INT_DATA_MASK		SDHCI_INT_XFER_COMPLETE | \
						SDHCI_INT_DMA | \
						SDHCI_INT_SPACE_AVAIL | \
						SDHCI_INT_DATA_AVAIL | \
						SDHCI_INT_DATA_TIMEOUT | \
						SDHCI_INT_DATA_CRC | \
						SDHCI_INT_DATA_END_BIT

#define SDHCI_DWCMSHC_INT_CMD_MASK		SDHCI_INT_CMD_COMPLETE | \
						SDHCI_INT_TIMEOUT | \
						SDHCI_INT_CRC | \
						SDHCI_INT_END_BIT | \
						SDHCI_INT_INDEX

enum {
	CLK_CORE,
	CLK_BUS,
	CLK_AXI,
	CLK_BLOCK,
	CLK_TIMER,
	CLK_MAX,
};

struct rk_sdhci_host {
	struct mci_host		mci;
	struct sdhci		sdhci;
	struct clk_bulk_data	clks[CLK_MAX];
};


static inline
struct rk_sdhci_host *to_rk_sdhci_host(struct mci_host *mci)
{
	return container_of(mci, struct rk_sdhci_host, mci);
}

static int rk_sdhci_card_present(struct mci_host *mci)
{
	struct rk_sdhci_host *host = to_rk_sdhci_host(mci);

	return !!(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) & SDHCI_CARD_DETECT);
}

static int rk_sdhci_reset(struct rk_sdhci_host *host, u8 mask)
{
	sdhci_write8(&host->sdhci, SDHCI_SOFTWARE_RESET, mask);

	/* wait for reset completion */
	if (wait_on_timeout(100 * MSECOND,
			!(sdhci_read8(&host->sdhci, SDHCI_SOFTWARE_RESET) & mask))){
		dev_err(host->mci.hw_dev, "SDHCI reset timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int rk_sdhci_init(struct mci_host *mci, struct device_d *dev)
{
	struct rk_sdhci_host *host = to_rk_sdhci_host(mci);
	int ret;

	ret = rk_sdhci_reset(host, SDHCI_RESET_ALL);
	if (ret)
		return ret;

	sdhci_write8(&host->sdhci, SDHCI_POWER_CONTROL,
			    SDHCI_BUS_VOLTAGE_330 | SDHCI_BUS_POWER_EN);
	udelay(400);

	sdhci_write32(&host->sdhci, SDHCI_INT_ENABLE,
			    SDHCI_DWCMSHC_INT_DATA_MASK |
			    SDHCI_DWCMSHC_INT_CMD_MASK);
	sdhci_write32(&host->sdhci, SDHCI_SIGNAL_ENABLE, 0x00);

	/* Disable cmd conflict check */
	sdhci_write32(&host->sdhci, DWCMSHC_HOST_CTRL3, 0x0);
	/* Reset previous settings */
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_TXCLK, 0);
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_STRBIN, 0);

	return 0;
}

static void rk_sdhci_set_clock(struct rk_sdhci_host *host, unsigned int clock)
{
	u32 txclk_tapnum = DLL_TXCLK_TAPNUM_DEFAULT, extra;
	int err;

	host->mci.clock = 0;

	/* DO NOT TOUCH THIS SETTING */
	extra = DWCMSHC_EMMC_DLL_DLYENA |
		DLL_RXCLK_NO_INVERTER << DWCMSHC_EMMC_DLL_RXCLK_SRCSEL;
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_RXCLK, extra);

	if (clock == 0)
		return;

	/* Rockchip platform only support 375KHz for identify mode */
	if (clock <= 400000)
		clock = 375000;

	clk_set_rate(host->clks[CLK_CORE].clk, clock);

	sdhci_set_clock(&host->sdhci, clock, clk_get_rate(host->clks[CLK_CORE].clk));

	if (clock <= 400000)
		return;

	/* Reset DLL */
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_CTRL, BIT(1));
	udelay(1);
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_CTRL, 0x0);

	/* Init DLL settings */
	extra = 0x5 << DWCMSHC_EMMC_DLL_START_POINT |
		0x2 << DWCMSHC_EMMC_DLL_INC |
		DWCMSHC_EMMC_DLL_START;
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_CTRL, extra);
	err = readl_poll_timeout(host->sdhci.base + DWCMSHC_EMMC_DLL_STATUS0,
				 extra, DLL_LOCK_WO_TMOUT(extra),
				 500 * USEC_PER_MSEC);
	if (err) {
		dev_err(host->mci.hw_dev, "DLL lock timeout!\n");
		return;
	}

	/* Disable cmd conflict check */
	extra = sdhci_read32(&host->sdhci, DWCMSHC_HOST_CTRL3);
	extra &= ~BIT(0);
	sdhci_write32(&host->sdhci, DWCMSHC_HOST_CTRL3, extra);

	extra = 0x1 << 16 | /* tune clock stop en */
		0x2 << 17 | /* pre-change delay */
		0x3 << 19;  /* post-change delay */
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_ATCTRL, extra);

	extra = DWCMSHC_EMMC_DLL_DLYENA |
		DLL_TXCLK_TAPNUM_FROM_SW |
		txclk_tapnum;
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_TXCLK, extra);

	extra = DWCMSHC_EMMC_DLL_DLYENA |
		DLL_STRBIN_TAPNUM_DEFAULT |
		DLL_STRBIN_TAPNUM_FROM_SW;
	sdhci_write32(&host->sdhci, DWCMSHC_EMMC_DLL_STRBIN, extra);
}

static void rk_sdhci_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct rk_sdhci_host *host = to_rk_sdhci_host(mci);
	u16 val;

	/* stop clock */
	sdhci_write16(&host->sdhci, SDHCI_CLOCK_CONTROL, 0);

	if (ios->clock)
		rk_sdhci_set_clock(host, ios->clock);

	sdhci_set_bus_width(&host->sdhci, ios->bus_width);

	val = sdhci_read8(&host->sdhci, SDHCI_HOST_CONTROL);

	if (ios->clock > 26000000)
		val |= SDHCI_CTRL_HISPD;
	else
		val &= ~SDHCI_CTRL_HISPD;

	sdhci_write8(&host->sdhci, SDHCI_HOST_CONTROL, val);
}

static int rk_sdhci_wait_for_done(struct rk_sdhci_host *host, u32 mask)
{
	u64 start = get_time_ns();
	u16 stat;

	do {
		stat = sdhci_read16(&host->sdhci, SDHCI_INT_NORMAL_STATUS);
		if (stat & SDHCI_INT_ERROR) {
			dev_err(host->mci.hw_dev, "SDHCI_INT_ERROR: 0x%08x\n",
				sdhci_read16(&host->sdhci, SDHCI_INT_ERROR_STATUS));
			return -EPERM;
		}

		if (is_timeout(start, 1000 * MSECOND)) {
			dev_err(host->mci.hw_dev,
					"SDHCI timeout while waiting for done\n");
			return -ETIMEDOUT;
		}
	} while ((stat & mask) != mask);

	return 0;
}

static void print_error(struct rk_sdhci_host *host, int cmdidx)
{
	dev_err(host->mci.hw_dev,
		"error while transfering data for command %d\n", cmdidx);
	dev_err(host->mci.hw_dev, "state = 0x%08x , interrupt = 0x%08x\n",
		sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE),
		sdhci_read32(&host->sdhci, SDHCI_INT_NORMAL_STATUS));
}

static int rk_sdhci_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				 struct mci_data *data)
{
	struct rk_sdhci_host *host = to_rk_sdhci_host(mci);
	u32 mask, command, xfer;
	int ret;
	dma_addr_t dma;

	/* Wait for idle before next command */
	mask = SDHCI_CMD_INHIBIT_CMD;
	if (cmd->cmdidx != MMC_CMD_STOP_TRANSMISSION)
		mask |= SDHCI_CMD_INHIBIT_DATA;

	ret = wait_on_timeout(10 * MSECOND,
			!(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) & mask));

	if (ret) {
		dev_err(host->mci.hw_dev,
				"SDHCI timeout while waiting for idle\n");
		return ret;
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);

	sdhci_write8(&host->sdhci, SDHCI_TIMEOUT_CONTROL, 0xe);

	sdhci_setup_data_dma(&host->sdhci, data, &dma);

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data,
				dma == SDHCI_NO_DMA ? false : true,
				&command, &xfer);

	sdhci_write16(&host->sdhci, SDHCI_TRANSFER_MODE, xfer);

	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(&host->sdhci, SDHCI_COMMAND, command);

	ret = rk_sdhci_wait_for_done(host, SDHCI_INT_CMD_COMPLETE);
	if (ret == -EPERM)
		goto error;
	else if (ret)
		return ret;

	sdhci_read_response(&host->sdhci, cmd);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, SDHCI_INT_CMD_COMPLETE);

	ret = sdhci_transfer_data_dma(&host->sdhci, data, dma);

error:
	if (ret) {
		print_error(host, cmd->cmdidx);
		rk_sdhci_reset(host, BIT(1)); /* SDHCI_RESET_CMD */
		rk_sdhci_reset(host, BIT(2)); /* SDHCI_RESET_DATA */
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, ~0);
	return ret;
}

static int rk_sdhci_probe(struct device_d *dev)
{
	struct rk_sdhci_host *host;
	struct resource *iores;
	struct mci_host *mci;
	int ret;

	host = xzalloc(sizeof(*host));

	mci = &host->mci;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	host->sdhci.base = IOMEM(iores->start);
	host->sdhci.mci = mci;
	mci->send_cmd = rk_sdhci_send_cmd;
	mci->set_ios = rk_sdhci_set_ios;
	mci->init = rk_sdhci_init;
	mci->card_present = rk_sdhci_card_present;
	mci->hw_dev = dev;

	host->clks[CLK_CORE].id = "core";
	host->clks[CLK_BUS].id = "bus";
	host->clks[CLK_AXI].id = "axi";
	host->clks[CLK_BLOCK].id = "block";
	host->clks[CLK_TIMER].id = "timer";

	ret = clk_bulk_get(host->mci.hw_dev, CLK_MAX, host->clks);
	if (ret) {
		dev_err(host->mci.hw_dev, "failed to get clocks: %s\n",
			strerror(-ret));
		return ret;
	}

	ret = clk_bulk_enable(CLK_MAX, host->clks);
	if (ret) {
		dev_err(host->mci.hw_dev, "failed to enable clocks: %s\n",
			strerror(-ret));
		return ret;
	}

	host->sdhci.max_clk = clk_get_rate(host->clks[CLK_CORE].clk);

	mci_of_parse(&host->mci);

	sdhci_setup_host(&host->sdhci);

	dev->priv = host;

	return mci_register(&host->mci);
}

static __maybe_unused struct of_device_id rk_sdhci_compatible[] = {
	{
		.compatible = "rockchip,rk3568-dwcmshc"
	}, {
		/* sentinel */
	}
};

static struct driver_d rk_sdhci_driver = {
	.name = "rk3568-dwcmshc-sdhci",
	.probe = rk_sdhci_probe,
	.of_compatible = DRV_OF_COMPAT(rk_sdhci_compatible),
};
device_platform_driver(rk_sdhci_driver);
