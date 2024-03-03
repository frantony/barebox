/* based on picotcp.git/test/examples/dnsclient.c */

#include <common.h>
#include <command.h>
#include <net.h>
#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dns_client.h>

static void cb_udpdnsclient_getaddr(char *ip, void *arg)
{
	uint8_t *id = (uint8_t *) arg;

	if (!ip) {
		pr_err("%s: ERROR occured! (id: %u)\n", __FUNCTION__, *id);
		return;
	}

	printf("%s: ip %s (id: %u)\n", __FUNCTION__, ip, *id);

	if (arg)
		PICO_FREE(arg);
}

static void app_udpdnsclient(char *dname)
{
	IPaddr_t bbnameserver;
	struct pico_ip4 nameserver;
	uint8_t *getaddr_id;

	bbnameserver = net_get_nameserver();
	if (!bbnameserver) {
		pr_err("no nameserver specified in $global.net.nameserver\n");
		return;
	}

	nameserver.addr = bbnameserver;

	pico_dns_client_nameserver(&nameserver, PICO_DNS_NS_ADD);

	getaddr_id = calloc(1, sizeof(uint8_t));
	*getaddr_id = 1;
	printf(">>>>> DNS GET ADDR OF %s\n", dname);
	pico_dns_client_getaddr(dname, &cb_udpdnsclient_getaddr, getaddr_id);

	return;
}

static int do_picohost(int argc, char *argv[])
{
	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	app_udpdnsclient(argv[1]);

	return 0;
}

BAREBOX_CMD_START(picohost)
	.cmd		= do_picohost,
	BAREBOX_CMD_DESC("resolve a hostname")
	BAREBOX_CMD_OPTS("<HOSTNAME>")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
