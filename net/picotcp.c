#include <init.h>
#include <poller.h>
#include <pico_stack.h>

struct pico_stack *picostack;

static struct poller_struct picotcp_poller;

static void picotcp_poller_cb(struct poller_struct *poller)
{
	pico_stack_tick(picostack);
}

static int picotcp_net_init(void)
{
	if (pico_stack_init(&picostack) < 0) {
		pr_err("PicoTCP: cannot initialize stack\n");
		picostack = NULL;

		return -EINVAL;
	}

	picotcp_poller.func = picotcp_poller_cb;

	return poller_register(&picotcp_poller, "picotcp");
}
postcore_initcall(picotcp_net_init);
