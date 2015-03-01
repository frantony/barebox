/*
 * tftp.c
 *
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <driver.h>
#include <clock.h>
#include <fs.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <getopt.h>
#include <fs.h>
#include <init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <kfifo.h>
#include <linux/sizes.h>

#include <pico_socket.h>
#include <pico_ipv4.h>
#include <pico_tftp.h>
#include <poller.h>

#define TFTP_BLOCK_SIZE		512	/* default TFTP block size */
#define TFTP_FIFO_SIZE		4096

struct file_priv {
	struct pico_tftp_session *session;
	int synchro;
	int err;
	const char *filename;
	int filesize;
	struct kfifo *fifo;
	void *buf;
};

struct tftp_priv {
	union pico_address server;
};

static int tftp_create(struct device_d *dev, const char *pathname, mode_t mode)
{
	return 0;
}

static int tftp_unlink(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int tftp_mkdir(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int tftp_rmdir(struct device_d *dev, const char *pathname)
{
	return -ENOSYS;
}

static int tftp_truncate(struct device_d *dev, FILE *f, ulong size)
{
	return 0;
}

static int tftp_poll(struct file_priv *priv)
{
	#ifdef ARCH_HAS_CTRLC
	poller_call();
	#endif

#if 0
	if (ctrlc()) {
		priv->state = STATE_DONE;
		priv->err = -EINTR;
		return -EINTR;
	}

#endif

	return 0;
}

static struct file_priv *tftp_do_open(struct device_d *dev,
		int accmode, const char *filename)
{
	struct file_priv *priv;
	struct tftp_priv *tpriv = dev->priv;
	int ret;

	priv = xzalloc(sizeof(*priv));

	filename++;

	switch (accmode & O_ACCMODE) {
	case O_RDONLY:
		break;

	case O_RDWR:
		ret = -ENOSYS;
		goto out;
	}

	priv->err = -EINVAL;
	priv->filename = filename;

	priv->fifo = kfifo_alloc(2 * TFTP_FIFO_SIZE);
	if (!priv->fifo) {
		ret = -ENOMEM;
		goto out;
	}

	priv->session = pico_tftp_app_setup(&tpriv->server,
					short_be(PICO_TFTP_PORT),
					PICO_PROTO_IPV4,
					&priv->synchro);
	if (!priv->session) {
		printf("TFTP: Error in session setup\n");
		ret = -EIO;
		goto out2;
	}

	ret = pico_tftp_app_start_rx(priv->session, filename);
	if (ret) {
		printf("Error in pico_tftp_app_start_rx\n");
	}

	return priv;
out2:
	// FIXME
	kfifo_free(priv->fifo);
out:
	free(priv);

	return ERR_PTR(ret);
}

static int tftp_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct file_priv *priv;

	priv = tftp_do_open(dev, file->flags, filename);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	file->priv = priv;
	file->size = SZ_2G;

	return 0;
}

static int tftp_do_close(struct file_priv *priv)
{
	//tftp_poll(priv);

	kfifo_free(priv->fifo);
	free(priv->buf);
	free(priv);

	return 0;
}

static int tftp_close(struct device_d *dev, FILE *f)
{
	struct file_priv *priv = f->priv;

	return tftp_do_close(priv);
}

static int tftp_write(struct device_d *_dev, FILE *f, const void *inbuf,
		size_t insize)
{
	return -ENOSYS;
}

static int tftp_read(struct device_d *dev, FILE *f, void *buf, size_t insize)
{
	struct file_priv *priv = f->priv;
	int ret;
	int t;
	int timeout = 100000;

	//debug("%s %zu\n", __func__, insize);

	t = insize;
	while (t > 0 && timeout) {
		tftp_poll(priv);
		timeout--;
		if (priv->synchro) {
			int32_t llen;
			uint8_t tbuf[PICO_TFTP_PAYLOAD_SIZE];

			timeout = 1000;

			llen = pico_tftp_get(priv->session, tbuf, PICO_TFTP_PAYLOAD_SIZE);
			if (llen < 0) {
				printf("Failure in pico_tftp_get\n");
				return -EIO;
			}
			t -= llen;
			kfifo_put(priv->fifo, tbuf, llen);
			if (llen < PICO_TFTP_PAYLOAD_SIZE) {
				printf("EOFR\n");
				break;
			}
		}
	}

	insize = insize - t;
	ret = kfifo_get(priv->fifo, buf, insize);
	return ret;
}

static loff_t tftp_lseek(struct device_d *dev, FILE *f, loff_t pos)
{
	/* not implemented in tftp protocol */
	return -ENOSYS;
}

static DIR* tftp_opendir(struct device_d *dev, const char *pathname)
{
	/* not implemented in tftp protocol */
	return NULL;
}

static int tftp_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct tftp_priv *tpriv = dev->priv;
	int32_t file_size;
	struct pico_tftp_session *session;

	session = pico_tftp_app_setup(&tpriv->server,
					short_be(PICO_TFTP_PORT),
					PICO_PROTO_IPV4,
					NULL);

	s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

	if (!pico_tftp_get_file_size(session, &file_size))
		s->st_size = file_size;
	else
		s->st_size = FILESIZE_MAX;

	pico_tftp_abort(session, TFTP_ERR_EACC, "Error opening file");

	return 0;
}

static int tftp_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct tftp_priv *priv = xzalloc(sizeof(struct tftp_priv));

	dev->priv = priv;

	//priv->server = resolv(fsdev->backingstore);

	/* FIXME: not name resolving with picotcp */
	pico_string_to_ipv4(fsdev->backingstore, &priv->server.ip4.addr);

	return 0;
}

static void tftp_remove(struct device_d *dev)
{
	struct tftp_priv *priv = dev->priv;

	free(priv);
}

static struct fs_driver_d tftp_driver = {
	.open      = tftp_open,
	.close     = tftp_close,
	.read      = tftp_read,
	.lseek     = tftp_lseek,
	.opendir   = tftp_opendir,
	.stat      = tftp_stat,
	.create    = tftp_create,
	.unlink    = tftp_unlink,
	.mkdir     = tftp_mkdir,
	.rmdir     = tftp_rmdir,
	.write     = tftp_write,
	.truncate  = tftp_truncate,
	.flags     = 0,
	.drv = {
		.probe  = tftp_probe,
		.remove = tftp_remove,
		.name = "tftp",
	}
};

static int tftp_init(void)
{
	return register_fs_driver(&tftp_driver);
}
coredevice_initcall(tftp_init);
