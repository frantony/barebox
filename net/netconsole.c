// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

/* netconsole.c - network console support */

#define pr_fmt(fmt) "netconsole: " fmt

#include <common.h>
#include <command.h>
#include <fs.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <stringlist.h>
#include <net.h>
#include <kfifo.h>
#include <init.h>
#include <linux/err.h>

struct nc_priv {
	struct console_device cdev;
	struct kfifo *fifo;
	int busy;
	struct net_connection *con;

	unsigned int port;
	IPaddr_t ip;
};

static struct nc_priv *g_priv;

static void nc_handler(void *ctx, char *pkt, unsigned len)
{
	struct nc_priv *priv = g_priv;
	unsigned char *packet = net_eth_to_udp_payload(pkt);

	kfifo_put(priv->fifo, packet, net_eth_to_udplen(pkt));
}

static int nc_getc(struct console_device *cdev)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);
	unsigned char c;

	if (!priv->con)
		return 0;

	while (!kfifo_len(priv->fifo))
		net_poll();

	kfifo_getc(priv->fifo, &c);

	return c;
}

static int nc_tstc(struct console_device *cdev)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);

	if (!priv->con)
		return 0;

	if (priv->busy)
		return kfifo_len(priv->fifo) ? 1 : 0;

	net_poll();

	return kfifo_len(priv->fifo) ? 1 : 0;
}

static void nc_putc(struct console_device *cdev, char c)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);
	unsigned char *packet;

	if (!priv->con)
		return;

	if (priv->busy)
		return;

	packet = net_udp_get_payload(priv->con);
	*packet = c;

	priv->busy = 1;
	net_udp_send(priv->con, 1);
	priv->busy = 0;
}

static int nc_open(struct console_device *cdev)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);

	if (!priv->port) {
		pr_err("port not set\n");
		return -EINVAL;
	}

	if (!priv->ip) {
		pr_err("ip not set\n");
		return -EINVAL;
	}

	priv->con = net_udp_new(priv->ip, priv->port, nc_handler, NULL);
	if (IS_ERR(priv->con)) {
		int ret = PTR_ERR(priv->con);
		priv->con = NULL;
		return ret;
	}

	net_udp_bind(priv->con, priv->port);

	pr_info("netconsole initialized with %pI4:%d\n", &priv->ip, priv->port);

	return 0;
}

static int nc_close(struct console_device *cdev)
{
	struct nc_priv *priv = container_of(cdev,
					struct nc_priv, cdev);

	if (priv->con) {
		net_unregister(priv->con);
		priv->con = NULL;
		return 0;
	}

	return -EINVAL;
}

static int netconsole_init(void)
{
	struct nc_priv *priv;
	struct console_device *cdev;
	int ret;

	priv = xzalloc(sizeof(*priv));
	cdev = &priv->cdev;
	cdev->tstc = nc_tstc;
	cdev->putc = nc_putc;
	cdev->getc = nc_getc;
	cdev->devname = "netconsole";
	cdev->devid = DEVICE_ID_SINGLE;
	cdev->open = nc_open;
	cdev->close = nc_close;

	g_priv = priv;

	priv->fifo = kfifo_alloc(1024);

	ret = console_register(cdev);
	if (ret) {
		pr_err("registering failed with %s\n", strerror(-ret));
		kfree(priv);
		return ret;
	}

	priv->port = 6666;

	dev_add_param_ip(&cdev->class_dev, "ip", NULL, NULL, &priv->ip, NULL);
	dev_add_param_int(&cdev->class_dev, "port", NULL, NULL, &priv->port, "%u", NULL);

	pr_info("registered as %s%d\n", cdev->class_dev.name, cdev->class_dev.id);

	return 0;
}

device_initcall(netconsole_init);
