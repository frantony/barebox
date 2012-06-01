#include <boot.h>
#include <common.h>
#include <init.h>
#include <errno.h>
#include <binfmt.h>
#include <elf.h>

#include <asm/byteorder.h>

static int do_bootm_elf(struct image_data *data)
{
	Elf32_Ehdr *ehdr;

	ehdr = (Elf32_Ehdr *)data;

	printf("\ndo_bootm_elf()\n\n");

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
