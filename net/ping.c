#include <common.h>
#include <command.h>
#include <clock.h>
#include <net.h>
#include <errno.h>
#include <linux/err.h>
#include <poller.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>
#include <pico_icmp4.h>

static uint16_t ping_sequence_number;

static IPaddr_t	net_ping_ip;		/* the ip address to ping 		*/

#define PING_STATE_INIT		0
#define PING_STATE_SUCCESS	1

static int ping_state;

static struct net_connection *ping_con;

static int ping_send(void)
{
	unsigned char *payload;
	struct icmphdr *icmp;
	uint64_t ts;

	icmp = ping_con->icmp;

	icmp->type = ICMP_ECHO_REQUEST;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->un.echo.id = 0;
	icmp->un.echo.sequence = htons(ping_sequence_number);

	ping_sequence_number++;

	payload = (char *)(icmp + 1);
	ts = get_time_ns();
	memcpy(payload, &ts, sizeof(ts));
	payload[8] = 0xab;
	payload[9] = 0xcd;
	return net_icmp_send(ping_con, 9);
}

static void ping_handler(void *ctx, char *pkt, unsigned len)
{
	IPaddr_t tmp;
	struct iphdr *ip = net_eth_to_iphdr(pkt);

	tmp = net_read_ip((void *)&ip->saddr);
	if (tmp != net_ping_ip)
		return;

	ping_state = PING_STATE_SUCCESS;
}

static int do_ping_legacy(char *argv[])
{
	int ret;
	uint64_t ping_start;
	unsigned retries = 0;

	ret = resolv(argv[1], &net_ping_ip);
	if (ret) {
		printf("Cannot resolve \"%s\": %s\n", argv[1], strerror(-ret));
		return 1;
	}

	ping_state = PING_STATE_INIT;
	ping_sequence_number = 0;

	ping_con = net_icmp_new(net_ping_ip, ping_handler, NULL);
	if (IS_ERR(ping_con)) {
		ret = PTR_ERR(ping_con);
		goto out;
	}

	printf("PING %s (%pI4)\n", argv[1], &net_ping_ip);

	ping_start = get_time_ns();
	ret = ping_send();
	if (ret)
		goto out_unreg;

	while (ping_state == PING_STATE_INIT) {
		if (ctrlc()) {
			ret = -EINTR;
			break;
		}

		net_poll();

		if (is_timeout(ping_start, SECOND)) {
			/* No answer, send another packet */
			ping_start = get_time_ns();
			ret = ping_send();
			if (ret)
				goto out_unreg;
			retries++;
		}

		if (retries > PKT_NUM_RETRIES) {
			ret = -ETIMEDOUT;
			goto out_unreg;
		}
	}

	if (!ret)
		printf("host %s is alive\n", argv[1]);

out_unreg:
	net_unregister(ping_con);
out:
	if (ret)
		printf("ping failed: %s\n", strerror(-ret));
	return ping_state == PING_STATE_SUCCESS ? 0 : 1;
}

/* picotcp icmp support */

#define NUM_PING 3

static int ping_done;
static int ping_code;

/* callback function for receiving ping reply */
static void cb_ping(struct pico_icmp4_stats *s)
{
	char host[30];
	int time_sec = 0;
	int time_msec = 0;

	/* convert ip address from icmp4_stats structure to string */
	pico_ipv4_to_string(host, s->dst.addr);

	/* get time information from icmp4_stats structure */
	time_sec = s->time / 1000;
	time_msec = s->time % 1000;

	if (s->err == PICO_PING_ERR_REPLIED) {
		/* print info if no error reported in icmp4_stats structure */
		printf("%lu bytes from %s: icmp_req=%lu ttl=%lu time=%llu ms\n", \
			s->size, host, s->seq, s->ttl, s->time);
		if (s->seq == NUM_PING) {
			ping_done = 1;
		}
	} else {
		/* else, print error info */
		printf("PING %lu to %s: Error %d\n", s->seq, host, s->err);
		ping_done = 1;
	}

	ping_code = s->err;
}

static int do_picoping(char *argv[])
{
	int id;

	id = pico_icmp4_ping(argv[1], NUM_PING, 1000, 5000, 48, cb_ping);

	if (id == -1) {
		return -EIO;
	}

	ping_done = 0;
	ping_code = PICO_PING_ERR_PENDING;

	while (!ping_done) {
		if (ctrlc()) {
			break;
		}
		get_time_ns();
		poller_call();
	}

	pico_icmp4_ping_abort(id);

	if (ping_code != PICO_PING_ERR_REPLIED) {
		return -EIO;
	}

	return 0;
}

static int do_ping(int argc, char *argv[])
{
	int ret;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	if (IS_ENABLED(CONFIG_NET_PICOTCP))
		ret = do_picoping(argv);
	else
		ret = do_ping_legacy(argv);

	if (!ret)
		printf("host %s is alive\n", argv[1]);
	else
		printf("ping failed: %s\n", strerror(-ret));

	return ret ? 1 : 0;
}

BAREBOX_CMD_START(ping)
	.cmd		= do_ping,
	BAREBOX_CMD_DESC("send ICMP echo requests")
	BAREBOX_CMD_OPTS("DESTINATION")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
