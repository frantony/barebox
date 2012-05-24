#include <boot.h>
#include <common.h>
#include <init.h>
#include <fs.h>
#include <errno.h>
#include <binfmt.h>

#include <asm/byteorder.h>

static int do_bootm_barebox(struct image_data *data)
{
	void (*barebox)(int a0, int a1, int a2, int a3);

	barebox = read_file(data->os_file, NULL);
	if (!barebox)
		return -EINVAL;

	shutdown_barebox();

	if (data->os_address != UIMAGE_INVALID_ADDRESS) {
		char *dest_addr;

		dest_addr = (char *)data->os_address;
		/* FIXME: very dirty HACK */
		memcpy(dest_addr, barebox, 8 * 1024 * 1024);
		barebox = (void *)dest_addr;
	}

	barebox(2,		/* number of arguments? */
		0x80002000,
		0x80002008,
		0x10000000	/* no matter */
		);

	reset_cpu(0);
}

static struct image_handler barebox_handler = {
	.name = "MIPS barebox",
	.bootm = do_bootm_barebox,
	.filetype = filetype_mips_barebox,
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_mips_barebox,
	.exec = "bootm",
};

static int mips_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	binfmt_register(&binfmt_barebox_hook);

	return 0;
}
late_initcall(mips_register_image_handler);
