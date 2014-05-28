#include <command.h>
#include <common.h>
#include <complete.h>
#include <driver.h>
#include <poller.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>
#include <pico_dhcp_client.h>

static uint32_t dhcp_xid;
static int dhcp_done;
static int dhcp_code;

static void callback_dhcpclient(void __attribute__((unused)) *cli, int code)
{
	struct pico_ip4 address = {0}, gateway = {0};
	char s_address[16] = { }, s_gateway[16] = { };
	void *identifier = NULL;

	if (code == PICO_DHCP_SUCCESS) {
		identifier = pico_dhcp_get_identifier(dhcp_xid);
		if (!identifier) {
			printf("DHCP client: incorrect transaction ID %u\n", dhcp_xid);
			return;
		}

		address = pico_dhcp_get_address(identifier);
		gateway = pico_dhcp_get_gateway(identifier);
		pico_ipv4_to_string(s_address, address.addr);
		pico_ipv4_to_string(s_gateway, gateway.addr);
		printf("DHCP client: IP assigned by the server: %s\n", s_address);
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

	picodev = pico_get_device(argv[1]);
	if (pico_dhcp_initiate_negotiation(picodev, &callback_dhcpclient, &dhcp_xid) < 0) {
		printf("Failed to send DHCP request.\n");
		return 1;
	}

	while (!dhcp_done) {
		if (ctrlc()) {
			pico_dhcp_client_abort(dhcp_xid);
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
