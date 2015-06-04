#include <common.h>
#include <command.h>
#include <complete.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>

#define fail_if(a,msg) \
	if (a) { \
		printf("%s\n", msg); \
		return; \
	}

#define fail_unless(a,msg) fail_if(!(a), msg)

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
