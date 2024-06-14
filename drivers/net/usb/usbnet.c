// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <linux/usb/usb.h>
#include <linux/usb/usbnet.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <malloc.h>
#include <linux/phy.h>
#include <dma.h>

/* handles CDC Ethernet and many other network "bulk data" interfaces */
int usbnet_get_endpoints(struct usbnet *dev)
{
	struct usb_device	*udev = dev->udev;
	int				tmp;
	struct usb_interface		*alt = NULL;
	struct usb_endpoint_descriptor	*in = NULL, *out = NULL;
	struct usb_endpoint_descriptor	*status = NULL;

	for (tmp = 0; tmp < udev->config.no_of_if; tmp++) {
		unsigned	ep;

		in = out = status = NULL;
		alt = &udev->config.interface[tmp];

		/* take the first altsetting with in-bulk + out-bulk;
		 * remember any status endpoint, just in case;
		 * ignore other endpoints and altsetttings.
		 */
		for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
			struct usb_endpoint_descriptor	*e;
			int				intr = 0;

			e = &alt->ep_desc[ep];
			switch (e->bmAttributes) {
			case USB_ENDPOINT_XFER_INT:
				if (!usb_endpoint_dir_in(e))
					continue;
				intr = 1;
				/* FALLTHROUGH */
			case USB_ENDPOINT_XFER_BULK:
				break;
			default:
				continue;
			}
			if (usb_endpoint_dir_in(e)) {
				if (!intr && !in)
					in = e;
				else if (intr && !status)
					status = e;
			} else {
				if (!out)
					out = e;
			}
		}
		if (in && out)
			break;
	}

	if (!alt || !in || !out)
		return -EINVAL;

	if (alt->desc.bAlternateSetting != 0
			|| !(dev->driver_info->flags & FLAG_NO_SETINT)) {
		tmp = usb_set_interface(dev->udev, alt->desc.bInterfaceNumber,
				alt->desc.bAlternateSetting);
		if (tmp < 0)
			return tmp;
	}

	dev->in = usb_rcvbulkpipe (dev->udev,
			in->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	dev->out = usb_sndbulkpipe (dev->udev,
			out->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	dev_dbg(&dev->edev.dev, "found endpoints: IN=%d OUT=%d\n",
			in->bEndpointAddress, out->bEndpointAddress);

	return 0;
}
EXPORT_SYMBOL(usbnet_get_endpoints);

static int usbnet_send(struct eth_device *edev, void *eth_data, int data_length)
{
	struct usbnet		*dev = edev->priv;
	struct driver_info	*info = dev->driver_info;
	int			len, alen, ret;

	dev_dbg(&edev->dev, "%s\n",__func__);

	/* some devices want funky USB-level framing, for
	 * win32 driver (usually) and/or hardware quirks
	 */
        if(info->tx_fixup) {
                if(info->tx_fixup(dev, eth_data, data_length, dev->tx_buf, &len)) {
			dev_dbg(&edev->dev, "can't tx_fixup packet");
                        return 0;
                }
        } else {
                len = data_length;
                memmove(dev->tx_buf, (void*) eth_data, len);
        }

	/* don't assume the hardware handles USB_ZERO_PACKET
	 * NOTE:  strictly conforming cdc-ether devices should expect
	 * the ZLP here, but ignore the one-byte packet.
	 */
	if ((len % dev->maxpacket) == 0) {
		*(unsigned char *)(dev->tx_buf + len) = 0;
		len++;
	}

	ret = usb_bulk_msg(dev->udev, dev->out, dev->tx_buf, len, &alen, 1000);
	dev_dbg(&edev->dev, "%s: ret: %d len: %d alen: %d\n", __func__, ret, len, alen);

	return ret;
}

static void usbnet_recv(struct eth_device *edev)
{
	struct usbnet		*dev = (struct usbnet*) edev->priv;
	struct driver_info	*info = dev->driver_info;
	int len, ret, alen = 0;

	dev_dbg(&edev->dev, "%s\n",__func__);

	len = dev->rx_urb_size;

	ret = usb_bulk_msg(dev->udev, dev->in, dev->rx_buf, len, &alen, 2);
	if (ret)
		return;

	if (alen) {
		if (info->rx_fixup)
			info->rx_fixup(dev, dev->rx_buf, alen);
		else
			net_receive(edev, dev->rx_buf, alen);
	}
}

static int usbnet_init(struct eth_device *edev)
{
	struct usbnet		*dev = (struct usbnet*) edev->priv;
	struct driver_info      *info = dev->driver_info;
	int ret = 0;

	dev_dbg(&edev->dev, "%s\n",__func__);

	/* put into "known safe" state */
	if (info->reset)
		ret = info->reset(dev);

	if (ret) {
                dev_info(&edev->dev, "open reset fail (%d)", ret);
                return ret;
        }

	return 0;
}

static void usbnet_adjust_link(struct eth_device *edev)
{
	struct usbnet		*dev = (struct usbnet*)edev->priv;
	struct driver_info      *info = dev->driver_info;

	if (info->link_reset)
		info->link_reset(dev);
}

static int usbnet_open(struct eth_device *edev)
{
	struct usbnet		*dev = (struct usbnet*)edev->priv;

	dev_dbg(&edev->dev, "%s\n",__func__);

	return phy_device_connect(edev, &dev->miibus, dev->phy_addr, usbnet_adjust_link,
				0, PHY_INTERFACE_MODE_NA);
}

static void usbnet_halt(struct eth_device *edev)
{
	dev_dbg(&edev->dev, "%s\n",__func__);
}

int usbnet_probe(struct usb_device *usbdev, const struct usb_device_id *prod)
{
	struct usbnet *undev;
	struct eth_device *edev;
	struct driver_info *info;
	int status;

	dev_dbg(&usbdev->dev, "%s\n", __func__);

	undev = xzalloc(sizeof (*undev));

	usbdev->drv_data = undev;

	edev = &undev->edev;
	undev->udev = usbdev;

	edev->open = usbnet_open;
	edev->init = usbnet_init;
	edev->send = usbnet_send;
	edev->recv = usbnet_recv;
	edev->halt = usbnet_halt;
	edev->priv = undev;
	edev->parent = &usbdev->dev;

	info = (struct driver_info *)prod->driver_info;
	undev->driver_info = info;

	if (info->bind) {
		status = info->bind (undev);
		if (status < 0)
			goto out1;
	}

	if (!undev->rx_urb_size)
		undev->rx_urb_size = 1514; /* FIXME: What to put here? */
	undev->maxpacket = usb_maxpacket(undev->udev, undev->out);

	undev->rx_buf = dma_alloc(undev->rx_urb_size);
	if (!undev->rx_buf) {
		status = -ENOMEM;
		goto out1;
	}

	undev->tx_buf = dma_alloc(4096);
	if (!undev->tx_buf) {
		status = -ENOMEM;
		goto out1;
	}

	eth_register(edev);

	slice_depends_on(eth_device_slice(edev), usb_device_slice(usbdev));
	slice_depends_on(mdiobus_slice(&undev->miibus), usb_device_slice(usbdev));

	return 0;
out1:
	dev_dbg(&edev->dev, "err: %d\n", status);
	return status;
}

void usbnet_disconnect(struct usb_device *usbdev)
{
	struct usbnet *undev = usbdev->drv_data;
	struct eth_device *edev = &undev->edev;
	struct driver_info *info;

	info = undev->driver_info;
	if (info->unbind)
		info->unbind(undev);

	eth_unregister(edev);

	free(undev->rx_buf);
	free(undev->tx_buf);
	free(undev);
}
