#include <boot.h>
#include <common.h>
#include <init.h>
#include <errno.h>
#include <binfmt.h>
#include <elf.h>

#include <asm/byteorder.h>

static int do_bootm_elf(struct image_data *data)
{
	void (*barebox)(int a0, int a1, int a2, int a3);
	extern int my_load(char *kernel, unsigned long kexec_flags);

	printf("\ndo_bootm_elf()\n\n");

	my_load(data->os_file, 0);

	barebox = (void *)0x8041acc0;

	printf("\njumping to %p\n\n", barebox);
	barebox(2,              /* number of arguments? */
                0x80002000,
                0x80002008,
                0x10000000      /* no matter */
                );

	/* unreachable(); */
	return -1;
}

static struct image_handler elf_handler = {
	.name = "ELF",
	.bootm = do_bootm_elf,
	.filetype = filetype_elf,
};

static struct binfmt_hook binfmt_elf_hook = {
	.type = filetype_elf,
	.exec = "bootm",
};

static int elf_register_image_handler(void)
{
	register_image_handler(&elf_handler);
	binfmt_register(&binfmt_elf_hook);

	return 0;
}
late_initcall(elf_register_image_handler);
