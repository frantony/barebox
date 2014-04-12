/**
 * This function is called by shutdown_barebox to get a clean
 * memory/cache state.
 */
#include <common.h>
#include <init.h>
#include <asm/cache.h>

static void arch_shutdown(void)
{
	pr_err("%s\n", __func__);
	flush_cache_all();
}
archshutdown_exitcall(arch_shutdown);
