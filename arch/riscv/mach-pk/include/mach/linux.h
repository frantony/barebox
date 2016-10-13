#ifndef __ASM_ARCH_LINUX_H
#define __ASM_ARCH_LINUX_H

struct device_d;

int sandbox_add_device(struct device_d *dev);

int linux_register_device(const char *name, void *start, void *end);
int linux_read(int fd, void *buf, size_t count);
ssize_t linux_write(int fd, const void *buf, size_t count);
int linux_tstc(int fd);

int barebox_register_console(char *name_template, int stdinfd, int stdoutfd);

int barebox_register_dtb(const void *dtb);

struct linux_console_data {
	int stdinfd;
	int stdoutfd;
};

#endif /* __ASM_ARCH_LINUX_H */
