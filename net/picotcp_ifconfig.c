#include <common.h>
#include <command.h>
#include <complete.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>
#include <pico_icmp4.h>
#include <pico_tree.h>

extern struct pico_tree Device_tree;
extern struct pico_tree Tree_dev_link;

static void printf_iface(struct pico_device *dev)
{
	struct pico_ipv4_link *link;
	struct pico_tree_node *index;

	printf("%-10s", dev->name);

	if (dev->eth) {
		struct pico_ethdev *eth = dev->eth;

		printf("Link encap:Ethernet  HWaddr ");
		printf("%02x:%02x:%02x:%02x:%02x:%02x",
			eth->mac.addr[0], eth->mac.addr[1],
			eth->mac.addr[2], eth->mac.addr[3],
			eth->mac.addr[4], eth->mac.addr[5]);
	}
	printf("\n");

	pico_tree_foreach(index, &Tree_dev_link) {
		link = index->keyValue;
		if (dev == link->dev) {
			char ipstr[16];
			pico_ipv4_to_string(ipstr, link->address.addr);
			printf("          inet addr:%s", ipstr);
			pico_ipv4_to_string(ipstr, link->netmask.addr);
			printf(" Mask:%s\n", ipstr);
		}
	}
	printf("\n");
}

static int do_ifconfig(int argc, char *argv[])
{
	struct pico_device *picodev;
	struct pico_ip4 ipaddr, nm;
	int ret;

	if (argc == 1) {
		struct pico_tree_node *index;

		pico_tree_foreach(index, &Device_tree) {
			printf_iface(index->keyValue);
		}

		return 0;
	}

	picodev = pico_get_device(argv[1]);
	if (!picodev) {
		struct pico_device *dev;
		struct pico_tree_node *index;

		perror("wrong device name");

		printf("available interfaces:\n");

		pico_tree_foreach(index, &Device_tree) {
			dev = index->keyValue;
			printf("%s\n", dev->name);
		}

		return 1;
	}

	if (argc == 2) {
		struct pico_tree_node *index;

		pico_tree_foreach(index, &Device_tree) {
			struct pico_device *dev = index->keyValue;

			if (!strcmp(dev->name, argv[1])) {
				printf_iface(dev);
			}
		}

		return 0;
	}

	if (argc < 4) {
		perror("ifconfig");
		return 1;
	}

	ret = pico_string_to_ipv4(argv[2], &(ipaddr.addr));
	if (ret) {
		perror("wrong ipaddr");
		return 1;
	}

	ret = pico_string_to_ipv4(argv[3], &(nm.addr));
	if (ret) {
		perror("wrong netmask");
		return 1;
	}

	pico_ipv4_link_add(picodev, ipaddr, nm);

	return 0;
}

BAREBOX_CMD_START(ifconfig)
	.cmd		= do_ifconfig,
BAREBOX_CMD_END
