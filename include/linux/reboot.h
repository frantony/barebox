#ifndef _LINUX_REBOOT_H
#define _LINUX_REBOOT_H

/*
 * Commands accepted by the _reboot() system call.
 *
 * KEXEC       Restart system using a previously loaded Linux kernel
 */

#define	LINUX_REBOOT_CMD_KEXEC		0x45584543

extern int reboot(int cmd);

#endif /* _LINUX_REBOOT_H */
