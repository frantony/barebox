/**
 * This function is called by shutdown_barebox to get a clean
 * memory/cache state.
 */
#include <common.h>
#include <asm/cache.h>

void arch_shutdown(void)
{
	flush_cache_all();
}
EXPORT_SYMBOL(arch_shutdown);
