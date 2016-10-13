
/*
 * common.c - common wrapper functions between barebox and the host
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * These are host includes. Never include any barebox header
 * files here...
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

/*
 * ...except the ones needed to connect with barebox
 */
#include <mach/linux.h>

static void cookmode(void)
{
	fflush(stdout);
}

int linux_tstc(int fd)
{
	(void)fd;

	return 0;
}

int ctrlc(void)
{
	return 0;
}

void __attribute__((noreturn)) linux_exit(void)
{
	cookmode();
	exit(0);
}

int linux_read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	if (count == 0)
		return 0;

	do {
		ret = read(fd, buf, count);

		if (ret == 0) {
			printf("read on fd %d returned 0, device gone? - exiting\n", fd);
			linux_exit();
		} else if (ret == -1) {
			if (errno == EAGAIN)
				return -errno;
			else if (errno == EINTR)
				continue;
			else {
				printf("read on fd %d returned -1, errno %d - exiting\n", fd, errno);
				linux_exit();
			}
		}
	} while (ret <= 0);

	return (int)ret;
}

ssize_t linux_write(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}

extern void start_barebox(void);
extern void mem_malloc_init(void *start, void *end);

int main(int argc, char *argv[])
{
	void *ram;
	int malloc_size = CONFIG_MALLOC_SIZE;

	(void)argc;
	(void)argv;

	ram = malloc(malloc_size);
	if (!ram) {
		printf("unable to get malloc space\n");
		exit(1);
	}
	mem_malloc_init(ram, ram + malloc_size - 1);

	barebox_register_console("console", fileno(stdin), fileno(stdout));

	start_barebox();

	/* never reached */
	return 0;
}
