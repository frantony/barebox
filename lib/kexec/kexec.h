#ifndef KEXEC_H
#define KEXEC_H

#include <common.h>

#include <stdio.h>
#include <string.h>
#define USE_BSD
#include <asm/byteorder.h>
#define _GNU_SOURCE

#include "kexec-elf.h"
#include "unused.h"

/*
 * Document some of the reasons why crashdump may fail, so we can give
 * better error messages
 */
#define EFAILED		-1	/* default error code */

extern unsigned long long mem_min, mem_max;
extern int kexec_debug;

#define dbgprintf(...) \
do { \
	if (kexec_debug) \
		fprintf(stderr, __VA_ARGS__); \
} while(0)

struct kexec_segment {
	const void *buf;
	size_t bufsz;
	const void *mem;
	size_t memsz;
};

struct memory_range {
	unsigned long long start, end;
	unsigned type;
#define RANGE_RAM	0
#define RANGE_RESERVED	1
#define RANGE_ACPI	2
#define RANGE_ACPI_NVS	3
#define RANGE_UNCACHED	4
};

struct kexec_info {
	struct kexec_segment *segment;
	int nr_segments;
	struct memory_range *memory_range;
	int memory_ranges;
	void *entry;
	struct mem_ehdr rhdr;
	unsigned long backup_start;
	unsigned long kexec_flags;
	unsigned long backup_src_start;
	unsigned long backup_src_size;
};

struct arch_map_entry {
	const char *machine;
	unsigned long arch;
};

extern const struct arch_map_entry arches[];
long physical_arch(void);

#define KERNEL_VERSION(major, minor, patch) \
	(((major) << 16) | ((minor) << 8) | patch)
long kernel_version(void);

void usage(void);
int get_memory_ranges(struct memory_range **range, int *ranges,
						unsigned long kexec_flags);
int valid_memory_range(struct kexec_info *info,
		       unsigned long sstart, unsigned long send);
int sort_segments(struct kexec_info *info);
unsigned long locate_hole(struct kexec_info *info,
	unsigned long hole_size, unsigned long hole_align, 
	unsigned long hole_min, unsigned long hole_max,
	int hole_end);

typedef int (probe_t)(const char *kernel_buf, off_t kernel_size);
typedef int (load_t )(//int argc, char **argv,
	const char *kernel_buf, off_t kernel_size, 
	struct kexec_info *info);
typedef void (usage_t)(void);
struct kexec_file_type {
	const char *name;
	probe_t *probe;
	load_t  *load;
	usage_t *usage;
};

extern struct kexec_file_type kexec_file_type[];
extern int kexec_file_types;

extern void die(char *fmt, ...);
extern void *xrealloc(void *ptr, size_t size);
extern unsigned long virt_to_phys(unsigned long addr);
extern void add_segment(struct kexec_info *info,
	const void *buf, size_t bufsz, unsigned long base, size_t memsz);
extern void add_segment_phys_virt(struct kexec_info *info,
	const void *buf, size_t bufsz, unsigned long base, size_t memsz,
	int phys);
extern unsigned long add_buffer(struct kexec_info *info,
	const void *buf, unsigned long bufsz, unsigned long memsz,
	unsigned long buf_align, unsigned long buf_min, unsigned long buf_max,
	int buf_end);
extern unsigned long add_buffer_virt(struct kexec_info *info,
	const void *buf, unsigned long bufsz, unsigned long memsz,
	unsigned long buf_align, unsigned long buf_min, unsigned long buf_max,
	int buf_end);
extern unsigned long add_buffer_phys_virt(struct kexec_info *info,
	const void *buf, unsigned long bufsz, unsigned long memsz,
	unsigned long buf_align, unsigned long buf_min, unsigned long buf_max,
	int buf_end, int phys);

extern char purgatory[];
extern size_t purgatory_size;

int arch_compat_trampoline(struct kexec_info *info);
char *get_command_line(void);

int kexec_iomem_for_each_line(char *match,
			      int (*callback)(void *data,
					      int nr,
					      char *str,
					      unsigned long base,
					      unsigned long length),
			      void *data);
int parse_iomem_single(char *str, uint64_t *start, uint64_t *end);
const char * proc_iomem(void);

extern int add_backup_segments(struct kexec_info *info,
			       unsigned long backup_base,
			       unsigned long backup_size);

#define MAX_LINE	160

char *concat_cmdline(const char *base, const char *append);

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

#endif /* KEXEC_H */
