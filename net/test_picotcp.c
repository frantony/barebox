#include <command.h>
#include <common.h>
#include <complete.h>
#include <driver.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>
#include <pico_dhcp_client.h>

#define fail_if(a,msg) \
	if (a) { \
		printf("%s\n", msg); \
		return; \
	}

#define fail_unless(a,msg) fail_if(!a, msg)

static void do_test_ipv4(void)
{
    #define IP_TST_SIZ 256
    int i;

    struct pico_device *dev[IP_TST_SIZ];
    char devname[8];
    struct pico_ip4 a[IP_TST_SIZ], d[IP_TST_SIZ], *source[IP_TST_SIZ], nm16, nm32, gw[IP_TST_SIZ], r[IP_TST_SIZ], ret;
    struct pico_ipv4_link *l[IP_TST_SIZ];

    char ipstr[] = "192.168.1.1";
    struct pico_ip4 ipaddr;

    struct pico_frame *f_NULL = NULL;
    struct pico_ip4 *dst_NULL = NULL;

    nm16.addr = long_be(0xFFFF0000);
    nm32.addr = long_be(0xFFFFFFFF);

    /*link_add*/
    for (i = 0; i < IP_TST_SIZ; i++) {
        snprintf(devname, 8, "nul%d", i);
        dev[i] = pico_null_create(devname);
        a[i].addr = long_be(0x0a000001 + (i << 16));
        d[i].addr = long_be(0x0a000002 + (i << 16));
        fail_if(pico_ipv4_link_add(dev[i], a[i], nm16) != 0, "Error adding link");
    }
    /*link_find + link_get + route_add*/
    for (i = 0; i < IP_TST_SIZ; i++) {
        gw[i].addr = long_be(0x0a0000f0 + (i << 16));
        r[i].addr = long_be(0x0c00001 + (i << 16));
        fail_unless(pico_ipv4_link_find(&a[i]) == dev[i], "Error finding link");
        l[i] = pico_ipv4_link_get(&a[i]);
        fail_if(l[i] == NULL, "Error getting link");
        fail_if(pico_ipv4_route_add(r[i], nm32, gw[i], 1, l[i]) != 0, "Error adding route");
        fail_if(pico_ipv4_route_add(d[i], nm32, gw[i], 1, l[i]) != 0, "Error adding route");
    }
    /*get_gateway + source_find*/
    for (i = 0; i < IP_TST_SIZ; i++) {
        ret = pico_ipv4_route_get_gateway(&r[i]);
        fail_if(ret.addr != gw[i].addr, "Error get gateway: returned wrong route");
        source[i] = pico_ipv4_source_find(&d[i]);
        fail_if(source[i]->addr != a[i].addr, "Error find source: returned wrong route");
    }
    /*route_del + link_del*/
    for (i = 0; i < IP_TST_SIZ; i++) {
        fail_if(pico_ipv4_route_del(r[i], nm32, 1) != 0, "Error deleting route");
        fail_if(pico_ipv4_link_del(dev[i], a[i]) != 0, "Error deleting link");
    }
    /*string_to_ipv4 + ipv4_to_string*/
    pico_string_to_ipv4(ipstr, &(ipaddr.addr));
    fail_if(ipaddr.addr != long_be(0xc0a80101), "Error string to ipv4");
    memset(ipstr, 0, 12);
    pico_ipv4_to_string(ipstr, ipaddr.addr);
    fail_if(strncmp(ipstr, "192.168.1.1", 11) != 0, "Error ipv4 to string");

    /*valid_netmask*/
    fail_if(pico_ipv4_valid_netmask(long_be(nm32.addr)) != 32, "Error checking netmask");

    /*is_unicast*/
    fail_if((pico_ipv4_is_unicast(long_be(0xc0a80101))) != 1, "Error checking unicast");
    fail_if((pico_ipv4_is_unicast(long_be(0xe0000001))) != 0, "Error checking unicast");

    /*rebound*/
    fail_if(pico_ipv4_rebound(f_NULL) != -1, "Error rebound frame");

    /*frame_push*/
    fail_if(pico_ipv4_frame_push(f_NULL, dst_NULL, PICO_PROTO_TCP) != -1, "Error push frame");
}

static int do_test_picotcp(int argc, char *argv[])
{
	do_test_ipv4();

	return 0;
}

BAREBOX_CMD_START(test_picotcp)
	.cmd		= do_test_picotcp,
BAREBOX_CMD_END

#include <pico_icmp4.h>
#include <poller.h>

#define NUM_PING 10

#define PING_STATE_WORK		0
#define PING_STATE_DONE		1

static int ping_state;

/* callback function for receiving ping reply */
void cb_ping(struct pico_icmp4_stats *s)
{
	char host[30];
	int time_sec = 0;
	int time_msec = 0;

	if (ping_state == PING_STATE_DONE) {
		return;
	}

	/* convert ip address from icmp4_stats structure to string */
	pico_ipv4_to_string(host, s->dst.addr);

	/* get time information from icmp4_stats structure */
	time_sec = s->time / 1000;
	time_msec = s->time % 1000;

	if (s->err == PICO_PING_ERR_REPLIED) {
		/* print info if no error reported in icmp4_stats structure */
		printf("%lu bytes from %s: icmp_req=%lu ttl=%lu time=%llu ms\n", \
			s->size, host, s->seq, s->ttl, s->time);
	} else {
		/* else, print error info */
		printf("PING %lu to %s: Error %d\n", s->seq, host, s->err);
	}
}

static int do_picoping(int argc, char *argv[])
{
	if (argc < 1) {
		perror("picoping");
		return 1;
	}

	ping_state = PING_STATE_WORK;

	pico_icmp4_ping(argv[1], NUM_PING, 1000, 5000, 48, cb_ping);

	while (ping_state != PING_STATE_DONE) {
		if (ctrlc()) {
			ping_state = PING_STATE_DONE;
			break;
		}
		get_time_ns();
		poller_call();
	}

	return 0;
}

BAREBOX_CMD_START(picoping)
	.cmd		= do_picoping,
BAREBOX_CMD_END

#include <pico_tree.h>

extern struct pico_tree Device_tree;

static int do_ifconfig(int argc, char *argv[])
{
	struct pico_device *picodev;
        struct pico_ip4 ipaddr, nm;
	int ret;

	if (argc < 3) {
		perror("ifconfig");
		return 1;
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

static uint32_t dhcp_xid;

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
}

static int do_dhclient(int argc, char *argv[])
{
	struct pico_device *picodev;

	if (argc != 2) {
		perror("dhclient");
		return 1;
	}

	picodev = pico_get_device(argv[1]);
	if (pico_dhcp_initiate_negotiation(picodev, &callback_dhcpclient, &dhcp_xid) < 0) {
		printf("Failed to send DHCP request.\n");
		return 1;
	}

	return 0;
}

BAREBOX_CMD_START(dhclient)
	.cmd		= do_dhclient,
BAREBOX_CMD_END
