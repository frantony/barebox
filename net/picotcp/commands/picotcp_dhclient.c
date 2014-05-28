#include <common.h>
#include <command.h>
#include <poller.h>
#include <net.h>
#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>
#include <pico_dhcp_client.h>

struct pico_device *pico_dhcp_get_pico_device(void *dhcpc);

extern struct pico_stack *picostack;

static uint32_t dhcp_xid;
static int dhcp_done;
static int dhcp_code;

/* FIXME */
struct pico_device_barebox_eth {
	struct pico_device dev;
	struct eth_device *edev;
};

static void callback_dhcpclient(void *dhcpc, int code)
{
	if (code == PICO_DHCP_SUCCESS) {
		struct pico_device *dev = pico_dhcp_get_pico_device(dhcpc);
		struct pico_device_barebox_eth *t = (struct pico_device_barebox_eth *)dev;
		struct eth_device *edev = t->edev;
		void *identifier;
		struct pico_ip4 address = {0};
		struct pico_ip4 netmask = {0};
		struct pico_ip4 gateway = {0};
		struct pico_ip4 nameserver = {0};
		char *hostname;
		char *domain;

		identifier = pico_dhcp_get_identifier(picostack, dhcp_xid);
		if (!identifier) {
			printf("DHCP client: incorrect transaction ID %u\n", dhcp_xid);
			return;
		}

		address = pico_dhcp_get_address(identifier);
		netmask = pico_dhcp_get_netmask(identifier);
		gateway = pico_dhcp_get_gateway(identifier);
		nameserver = pico_dhcp_get_nameserver(identifier, 0);
		hostname = pico_dhcp_get_hostname(picostack);
		domain = pico_dhcp_get_domain(picostack);

		net_set_ip(edev, (IPaddr_t)address.addr);
		net_set_netmask(edev, (IPaddr_t)netmask.addr);
		net_set_gateway(edev, (IPaddr_t)gateway.addr);
		net_set_nameserver((IPaddr_t)nameserver.addr);

		barebox_set_hostname(hostname);
		net_set_domainname(domain);

		printf("DHCP result:\n"
			"  ip: %pI4\n"
			"  netmask: %pI4\n"
			"  gateway: %pI4\n"
			"  nameserver: %pI4\n"
			"  hostname: %s\n"
			"  domainname: %s\n",
			&address.addr,
			&netmask.addr,
			&gateway.addr,
			&nameserver.addr,
			hostname ? hostname : "",
			domain ? domain : "");
	} else {
		printf("DHCP transaction failed.\n");
	}

	dhcp_done = 1;
	dhcp_code = code;
}

static int do_dhclient(int argc, char *argv[])
{
	struct pico_device *picodev;

	if (argc != 2) {
		perror("dhclient");
		return 1;
	}

	dhcp_done = 0;
	dhcp_code = PICO_DHCP_RESET;

	picodev = pico_get_device(picostack, argv[1]);
	if (pico_dhcp_initiate_negotiation(picodev, &callback_dhcpclient, &dhcp_xid) < 0) {
		printf("Failed to send DHCP request.\n");
		return 1;
	}

	while (!dhcp_done) {
		if (ctrlc()) {
			pico_dhcp_client_abort(picostack, dhcp_xid);
			break;
		}
		get_time_ns();
		poller_call();
	}

	if (dhcp_code != PICO_DHCP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

BAREBOX_CMD_START(dhclient)
	.cmd		= do_dhclient,
BAREBOX_CMD_END
