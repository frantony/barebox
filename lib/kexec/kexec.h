#ifndef KEXEC_H
#define KEXEC_H

#include "kexec-elf.h"

struct kexec_segment {
	const void *buf;
	size_t bufsz;
	const void *mem;
	size_t memsz;
};

struct kexec_info {
	struct kexec_segment *segment;
	int nr_segments;
	void *entry;
	unsigned long kexec_flags;
};

typedef int (probe_t)(const char *kernel_buf, off_t kernel_size);
typedef int (load_t)(const char *kernel_buf, off_t kernel_size,
	struct kexec_info *info);
struct kexec_file_type {
	const char *name;
	probe_t *probe;
	load_t  *load;
};

extern struct kexec_file_type kexec_file_type[];
extern int kexec_file_types;

extern void add_segment(struct kexec_info *info,
	const void *buf, size_t bufsz, unsigned long base, size_t memsz);
extern void add_segment_phys_virt(struct kexec_info *info,
	const void *buf, size_t bufsz, unsigned long base, size_t memsz,
	int phys);

extern long kexec_load(void *entry, unsigned long nr_segments,
                        struct kexec_segment *segments, unsigned long flags);
extern int kexec_load_file(char *kernel, unsigned long kexec_flags);

/* Limits of integral types. */
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

/* These values match the ELF architecture values.
 * Unless there is a good reason that should continue to be the case.
 */
#define KEXEC_ARCH_DEFAULT ( 0 << 16)
#define KEXEC_ARCH_386     ( 3 << 16)
#define KEXEC_ARCH_X86_64  (62 << 16)
#define KEXEC_ARCH_PPC     (20 << 16)
#define KEXEC_ARCH_PPC64   (21 << 16)
#define KEXEC_ARCH_IA_64   (50 << 16)
#define KEXEC_ARCH_ARM     (40 << 16)
#define KEXEC_ARCH_S390    (22 << 16)
#define KEXEC_ARCH_SH      (42 << 16)
#define KEXEC_ARCH_MIPS_LE (10 << 16)
#define KEXEC_ARCH_MIPS    ( 8 << 16)
#define KEXEC_ARCH_CRIS    (76 << 16)

#define KEXEC_MAX_SEGMENTS 16

#endif /* KEXEC_H */
