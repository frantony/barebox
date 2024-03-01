#include <init.h>
#include <poller.h>
#include <net.h>
#include <globalvar.h>
#include <magicvar.h>
#include <pico_stack.h>
#include <pico_device.h>

static int picotcp_receive_debug;
static int picotcp_send_debug;

BAREBOX_MAGICVAR(global.picotcp.receive.debug,
		"If true, print debug message on every received package");
BAREBOX_MAGICVAR(global.picotcp.send.debug,
		"If true, print debug message on every sent package");

void tcpdump(unsigned char *pkt, int len);

static struct poller_struct picotcp_poller;

static void picotcp_poller_cb(struct poller_struct *poller)
{
	pico_stack_tick();
}

static int picotcp_net_init(void)
{
	pico_stack_init();

	globalvar_add_simple_bool("picotcp.receive.debug", &picotcp_receive_debug);
	globalvar_add_simple_bool("picotcp.send.debug", &picotcp_send_debug);

	picotcp_poller.func = picotcp_poller_cb;

	return poller_register(&picotcp_poller, "picotcp");
}
postcore_initcall(picotcp_net_init);

struct pico_device_barebox_eth {
	struct pico_device dev;
	struct eth_device *edev;
};

void pico_adapter_set_ethaddr(struct eth_device *edev, const char *ethaddr)
{
	struct pico_device *picodev = edev->picodev;

	if (picodev)
		memcpy(picodev->eth->mac.addr, edev->ethaddr, ETH_ALEN);
}

void pico_adapter_receive(struct eth_device *edev, unsigned char *pkt, int len)
{
	if (picotcp_receive_debug) {
		pr_warning("%s: forwarding packet to picotcp (len=%d)\n",
				edev->devname, len);
	}

	tcpdump(pkt, len);

	pico_stack_recv(edev->picodev, pkt, len);
}

static int pico_adapter_send(struct pico_device *dev, void *buf, int len)
{
	struct pico_device_barebox_eth *t = (struct pico_device_barebox_eth *)dev;
	struct eth_device *edev = t->edev;
	int ret;

	if (!edev->active) {
		ret = eth_open(edev);
		if (ret)
			goto out;
	}

	if (picotcp_send_debug) {
		pr_warning("%s: sending packet (len=%d) via %s\n",
				__func__, len, edev->devname);
	}

	tcpdump(buf, len);

	ret = eth_send(edev, buf, len);
	if (!ret)
		ret = len;

out:
	return ret;
}

static int pico_adapter_poll(struct pico_device *dev, int loop_score)
{
	struct pico_device_barebox_eth *t = (struct pico_device_barebox_eth *)dev;
	struct eth_device *edev = t->edev;

	/* pico_stack_recv(dev, buf, len) will be called from net_receive() */
	edev->recv(edev);

	return loop_score;
}

static void pico_adapter_destroy(struct pico_device *dev)
{
	pr_err("%s %s\n", __func__, dev->name);
}

void pico_adapter_init(struct eth_device *edev)
{
	struct pico_device_barebox_eth *pif;
	struct pico_device *picodev;
	char *name;

	pif = PICO_ZALLOC(sizeof(struct pico_device_barebox_eth));
	picodev = &pif->dev;
	name = strdup(edev->devname);

	if (0 != pico_device_init(picodev, name, edev->ethaddr)) {
		pr_err("%s: pico_device_init failed.\n", name);
		PICO_FREE(pif);
		return;
	}

	picodev->send = pico_adapter_send;
	picodev->poll = pico_adapter_poll;
	picodev->destroy = pico_adapter_destroy;

	pif->edev = edev;
	edev->picodev = picodev;

	pr_info("pico_device %s created.\n", picodev->name);
}
