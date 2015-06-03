#include <common.h>
#include <command.h>
#include <complete.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_icmp4.h>

static int do_route(int argc, char *argv[])
{
	struct pico_ipv4_route *r;
	struct pico_tree_node *index;

	printf("picotcp IPv4 routing table\n");
	printf("Destination     Gateway         Genmask         Metric Iface\n");

	pico_tree_foreach(index, &Routes) {
		char ipstr[32];

		r = index->keyValue;

		pico_ipv4_to_string(ipstr, r->dest.addr);
		printf("%-16s", ipstr);
		pico_ipv4_to_string(ipstr, r->gateway.addr);
		printf("%-16s", ipstr);
		pico_ipv4_to_string(ipstr, r->netmask.addr);
		printf("%-16s", ipstr);
		printf("%-7d%s\n", r->metric, r->link->dev->name);
	}

	return 0;
}

BAREBOX_CMD_START(route)
	.cmd		= do_route,
BAREBOX_CMD_END
