#include <bootm.h>
#include <init.h>
#include <binfmt.h>
#include <errno.h>
#include <linux/reboot.h>
#include <environment.h>

#include "kexec.h"

static int do_bootm_elf(struct image_data *data)
{
	kexec_load_bootm_data(data);

	reboot(LINUX_REBOOT_CMD_KEXEC, data);

	return -ERESTARTSYS;
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
