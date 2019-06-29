// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <clock.h>
#include <poller.h>
#include <pico_ipv4.h>
#include <pico_icmp4.h>

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

static int do_picoping(int argc, char *argv[])
{
	int id;
	int ret = 0;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	id = pico_icmp4_ping(argv[1], NUM_PING, 1000, 5000, 48, cb_ping);
	if (id == -1) {
		ret = -EIO;
		goto out;
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
		ret = -EIO;
	}

out:
	if (!ret)
		printf("host %s is alive\n", argv[1]);
	else
		printf("ping failed: %s\n", strerror(-ret));

	return ret ? COMMAND_ERROR : COMMAND_SUCCESS;
}

BAREBOX_CMD_START(picoping)
	.cmd		= do_picoping,
	BAREBOX_CMD_DESC("send ICMP echo requests")
	BAREBOX_CMD_OPTS("DESTINATION")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
