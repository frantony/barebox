#include <init.h>
#include <poller.h>
#include <pico_stack.h>

static struct poller_struct picotcp_poller;

static void picotcp_poller_cb(struct poller_struct *poller)
{
	pico_stack_tick();
}

static int picotcp_net_init(void)
{
	pico_stack_init();

	picotcp_poller.func = picotcp_poller_cb;

	return poller_register(&picotcp_poller);
}
postcore_initcall(picotcp_net_init);
