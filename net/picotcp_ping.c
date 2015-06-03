#include <command.h>
#include <common.h>
#include <complete.h>
#include <driver.h>
#include <poller.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_dev_null.h>
#include <pico_dhcp_client.h>
#include <pico_icmp4.h>

#define NUM_PING 3

static int ping_done;
static int ping_code;

/* callback function for receiving ping reply */
void cb_ping(struct pico_icmp4_stats *s)
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

	if (argc < 1) {
		perror("picoping");
		return 1;
	}

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

BAREBOX_CMD_START(picoping)
	.cmd		= do_picoping,
BAREBOX_CMD_END
