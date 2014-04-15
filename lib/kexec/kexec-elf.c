#include <common.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <asm/io.h>
#include "elf.h"
#include "kexec.h"
#include "kexec-elf.h"

uint16_t elf16_to_cpu(const struct mem_ehdr *ehdr, uint16_t value)
{
	if (ehdr->ei_data == ELFDATA2LSB) {
		value = le16_to_cpu(value);
	} else if (ehdr->ei_data == ELFDATA2MSB) {
		value = be16_to_cpu(value);
	}

	return value;
}

uint32_t elf32_to_cpu(const struct mem_ehdr *ehdr, uint32_t value)
{
	if (ehdr->ei_data == ELFDATA2LSB) {
		value = le32_to_cpu(value);
	} else if (ehdr->ei_data == ELFDATA2MSB) {
		value = be32_to_cpu(value);
	}

	return value;
}

static int build_mem_elf32_ehdr(const char *buf, off_t len, struct mem_ehdr *ehdr)
{
	Elf32_Ehdr lehdr;

	if ((size_t)len < sizeof(lehdr)) {
		printf("Buffer is too small to hold ELF header\n");
		return -1;
	}

	memcpy(&lehdr, buf, sizeof(lehdr));
	if (elf16_to_cpu(ehdr, lehdr.e_ehsize) != sizeof(Elf32_Ehdr)) {
		printf("Bad ELF header size\n");
		return -1;
	}

	if (elf32_to_cpu(ehdr, lehdr.e_entry) > UINT32_MAX) {
		printf("ELF e_entry is too large\n");
		return -1;
	}

	if (elf32_to_cpu(ehdr, lehdr.e_phoff) > UINT32_MAX) {
		printf("ELF e_phoff is too large\n");
		return -1;
	}

	if (elf32_to_cpu(ehdr, lehdr.e_shoff) > UINT32_MAX) {
		printf("ELF e_shoff is too large\n");
		return -1;
	}

	ehdr->e_type      = elf16_to_cpu(ehdr, lehdr.e_type);
	ehdr->e_machine   = elf16_to_cpu(ehdr, lehdr.e_machine);
	ehdr->e_version   = elf32_to_cpu(ehdr, lehdr.e_version);
	ehdr->e_entry     = elf32_to_cpu(ehdr, lehdr.e_entry);
	ehdr->e_phoff     = elf32_to_cpu(ehdr, lehdr.e_phoff);
	ehdr->e_shoff     = elf32_to_cpu(ehdr, lehdr.e_shoff);
	ehdr->e_flags     = elf32_to_cpu(ehdr, lehdr.e_flags);
	ehdr->e_phnum     = elf16_to_cpu(ehdr, lehdr.e_phnum);
	ehdr->e_shnum     = elf16_to_cpu(ehdr, lehdr.e_shnum);
	ehdr->e_shstrndx  = elf16_to_cpu(ehdr, lehdr.e_shstrndx);

	if ((ehdr->e_phnum > 0) &&
		(elf16_to_cpu(ehdr, lehdr.e_phentsize) != sizeof(Elf32_Phdr)))
	{
		printf("ELF bad program header size\n");
		return -1;
	}

	if ((ehdr->e_shnum > 0) &&
		(elf16_to_cpu(ehdr, lehdr.e_shentsize) != sizeof(Elf32_Shdr)))
	{
		printf("ELF bad section header size\n");
		return -1;
	}

	return 0;
}

static int build_mem_ehdr(const char *buf, off_t len, struct mem_ehdr *ehdr)
{
	unsigned char e_ident[EI_NIDENT];
	int result;

	memset(ehdr, 0, sizeof(*ehdr));

	if ((size_t)len < sizeof(e_ident)) {
		printf("Buffer is too small to hold ELF e_ident\n");

		return -1;
	}

	memcpy(e_ident, buf, sizeof(e_ident));

	ehdr->ei_class   = e_ident[EI_CLASS];
	ehdr->ei_data    = e_ident[EI_DATA];
	if (	(ehdr->ei_class != ELFCLASS32) &&
		(ehdr->ei_class != ELFCLASS64))
	{
		printf("Not a supported ELF class\n");
		return -1;
	}

	if (	(ehdr->ei_data != ELFDATA2LSB) &&
		(ehdr->ei_data != ELFDATA2MSB))
	{
		printf("Not a supported ELF data format\n");
		return -1;
	}

	result = -1;
	if (ehdr->ei_class == ELFCLASS32) {
		result = build_mem_elf32_ehdr(buf, len, ehdr);
	}

	if (result < 0) {
		return result;
	}

	if ((e_ident[EI_VERSION] != EV_CURRENT) ||
		(ehdr->e_version != EV_CURRENT))
	{
		printf("Unknown ELF version\n");
		return -1;
	}

	return 0;
}

static int build_mem_elf32_phdr(const char *buf, struct mem_ehdr *ehdr, int idx)
{
	struct mem_phdr *phdr;
	const char *pbuf;
	Elf32_Phdr lphdr;

	pbuf = buf + ehdr->e_phoff + (idx * sizeof(lphdr));
	phdr = &ehdr->e_phdr[idx];
	memcpy(&lphdr, pbuf, sizeof(lphdr));

	if (	(elf32_to_cpu(ehdr, lphdr.p_filesz) > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lphdr.p_memsz)  > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lphdr.p_offset) > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lphdr.p_paddr)  > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lphdr.p_vaddr)  > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lphdr.p_align)  > UINT32_MAX))
	{
		printf("Program segment size out of range\n");
		return -1;
	}

	phdr->p_type   = elf32_to_cpu(ehdr, lphdr.p_type);
	phdr->p_paddr  = elf32_to_cpu(ehdr, lphdr.p_paddr);
	phdr->p_vaddr  = elf32_to_cpu(ehdr, lphdr.p_vaddr);
	phdr->p_filesz = elf32_to_cpu(ehdr, lphdr.p_filesz);
	phdr->p_memsz  = elf32_to_cpu(ehdr, lphdr.p_memsz);
	phdr->p_offset = elf32_to_cpu(ehdr, lphdr.p_offset);
	phdr->p_flags  = elf32_to_cpu(ehdr, lphdr.p_flags);
	phdr->p_align  = elf32_to_cpu(ehdr, lphdr.p_align);

	return 0;
}

static int build_mem_phdrs(const char *buf, off_t len, struct mem_ehdr *ehdr,
				uint32_t flags)
{
	size_t phdr_size, mem_phdr_size, i;

	/* e_phnum is at most 65535 so calculating
	 * the size of the program header cannot overflow.
	 */
	/* Is the program header in the file buffer? */
	phdr_size = 0;
	if (ehdr->ei_class == ELFCLASS32) {
		phdr_size = sizeof(Elf32_Phdr);
	} else if (ehdr->ei_class == ELFCLASS64) {
		phdr_size = sizeof(Elf64_Phdr);
	} else {
		printf("Invalid ei_class?\n");
		return -1;
	}
	phdr_size *= ehdr->e_phnum;

	/* Allocate the e_phdr array */
	mem_phdr_size = sizeof(ehdr->e_phdr[0]) * ehdr->e_phnum;
	ehdr->e_phdr = xmalloc(mem_phdr_size);

	for (i = 0; i < ehdr->e_phnum; i++) {
		struct mem_phdr *phdr;
		int result;

		result = -1;
		if (ehdr->ei_class == ELFCLASS32) {
			result = build_mem_elf32_phdr(buf, ehdr, i);

		}

		if (result < 0) {
			return result;
		}

		/* Check the program headers to be certain
		 * they are safe to use.
		 */
		phdr = &ehdr->e_phdr[i];
		if ((phdr->p_paddr + phdr->p_memsz) < phdr->p_paddr) {
			/* The memory address wraps */
			printf("ELF address wrap around\n");
			return -1;
		}

		/* Remember where the segment lives in the buffer */
		phdr->p_data = buf + phdr->p_offset;
	}

	return 0;
}

static int build_mem_elf32_shdr(const char *buf, struct mem_ehdr *ehdr, int idx)
{
	struct mem_shdr *shdr;
	const char *sbuf;
	int size_ok;
	Elf32_Shdr lshdr;

	sbuf = buf + ehdr->e_shoff + (idx * sizeof(lshdr));
	shdr = &ehdr->e_shdr[idx];
	memcpy(&lshdr, sbuf, sizeof(lshdr));

	if (	(elf32_to_cpu(ehdr, lshdr.sh_flags)     > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lshdr.sh_addr)      > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lshdr.sh_offset)    > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lshdr.sh_size)      > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lshdr.sh_addralign) > UINT32_MAX) ||
		(elf32_to_cpu(ehdr, lshdr.sh_entsize)   > UINT32_MAX))
	{
		printf("Program section size out of range\n");
		return -1;
	}

	shdr->sh_name      = elf32_to_cpu(ehdr, lshdr.sh_name);
	shdr->sh_type      = elf32_to_cpu(ehdr, lshdr.sh_type);
	shdr->sh_flags     = elf32_to_cpu(ehdr, lshdr.sh_flags);
	shdr->sh_addr      = elf32_to_cpu(ehdr, lshdr.sh_addr);
	shdr->sh_offset    = elf32_to_cpu(ehdr, lshdr.sh_offset);
	shdr->sh_size      = elf32_to_cpu(ehdr, lshdr.sh_size);
	shdr->sh_link      = elf32_to_cpu(ehdr, lshdr.sh_link);
	shdr->sh_info      = elf32_to_cpu(ehdr, lshdr.sh_info);
	shdr->sh_addralign = elf32_to_cpu(ehdr, lshdr.sh_addralign);
	shdr->sh_entsize   = elf32_to_cpu(ehdr, lshdr.sh_entsize);

	/* Now verify sh_entsize */
	size_ok = 0;
	switch(shdr->sh_type) {
	case SHT_SYMTAB:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Sym);
		break;
	case SHT_RELA:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Rela);
		break;
	case SHT_DYNAMIC:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Dyn);
		break;
	case SHT_REL:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Rel);
		break;
	case SHT_NOTE:
	case SHT_NULL:
	case SHT_PROGBITS:
	case SHT_HASH:
	case SHT_NOBITS:
	default:
		/* This is a section whose entsize requirements
		 * I don't care about.  If I don't know about
		 * the section I can't care about it's entsize
		 * requirements.
		 */
		size_ok = 1;
		break;
	}

	if (!size_ok) {
		printf("Bad section header(%x) entsize: %lld\n",
			shdr->sh_type, shdr->sh_entsize);
		return -1;
	}

	return 0;
}

static int build_mem_shdrs(const char *buf, off_t len, struct mem_ehdr *ehdr,
				uint32_t flags)
{
	size_t shdr_size, mem_shdr_size, i;

	/* e_shnum is at most 65536 so calculating
	 * the size of the section header cannot overflow.
	 */
	/* Is the program header in the file buffer? */
	shdr_size = 0;
	if (ehdr->ei_class == ELFCLASS32) {
		shdr_size = sizeof(Elf32_Shdr);
	} else if (ehdr->ei_class == ELFCLASS64) {
		shdr_size = sizeof(Elf64_Shdr);
	} else {
		printf("Invalid ei_class?\n");
		return -1;
	}
	shdr_size *= ehdr->e_shnum;

	/* Allocate the e_shdr array */
	mem_shdr_size = sizeof(ehdr->e_shdr[0]) * ehdr->e_shnum;
	ehdr->e_shdr = xmalloc(mem_shdr_size);

	for (i = 0; i < ehdr->e_shnum; i++) {
		struct mem_shdr *shdr;
		int result;

		result = -1;
		if (ehdr->ei_class == ELFCLASS32) {
			result = build_mem_elf32_shdr(buf, ehdr, i);
		}

		if (result < 0) {
			return result;
		}

		/* Check the section headers to be certain
		 * they are safe to use.
		 */
		shdr = &ehdr->e_shdr[i];
		if ((shdr->sh_addr + shdr->sh_size) < shdr->sh_addr) {
			printf("ELF address wrap around\n");
			return -1;
		}

		/* Remember where the section lives in the buffer */
		shdr->sh_data = (unsigned char *)(buf + shdr->sh_offset);
	}

	return 0;
}

static void read_nhdr(const struct mem_ehdr *ehdr,
	ElfNN_Nhdr *hdr, const unsigned char *note)
{
	memcpy(hdr, note, sizeof(*hdr));
	hdr->n_namesz = elf32_to_cpu(ehdr, hdr->n_namesz);
	hdr->n_descsz = elf32_to_cpu(ehdr, hdr->n_descsz);
	hdr->n_type   = elf32_to_cpu(ehdr, hdr->n_type);
}

static int build_mem_notes(struct mem_ehdr *ehdr)
{
	const unsigned char *note_start, *note_end, *note;
	size_t note_size, i;

	/* First find the note segment or section */
	note_start = note_end = NULL;

	for (i = 0; !note_start && (i < ehdr->e_phnum); i++) {
		struct mem_phdr *phdr = &ehdr->e_phdr[i];
		/*
		 * binutils <= 2.17 has a bug where it can create the
		 * PT_NOTE segment with an offset of 0. Therefore
		 * check p_offset > 0.
		 *
		 * See: http://sourceware.org/bugzilla/show_bug.cgi?id=594
		 */
		if (phdr->p_type == PT_NOTE && phdr->p_offset) {
			note_start = (unsigned char *)phdr->p_data;
			note_end = note_start + phdr->p_filesz;
		}
	}

	for (i = 0; !note_start && (i < ehdr->e_shnum); i++) {
		struct mem_shdr *shdr = &ehdr->e_shdr[i];
		if (shdr->sh_type == SHT_NOTE) {
			note_start = shdr->sh_data;
			note_end = note_start + shdr->sh_size;
		}
	}

	if (!note_start) {
		return 0;
	}

	/* Walk through and count the notes */
	ehdr->e_notenum = 0;
	for (note = note_start; note < note_end; note += note_size) {
		ElfNN_Nhdr hdr;
		read_nhdr(ehdr, &hdr, note);
		note_size  = sizeof(hdr);
		note_size += (hdr.n_namesz + 3) & ~3;
		note_size += (hdr.n_descsz + 3) & ~3;
		ehdr->e_notenum += 1;
	}

	/* Now walk and normalize the notes */
	ehdr->e_note = xmalloc(sizeof(*ehdr->e_note) * ehdr->e_notenum);
	for (i = 0, note = note_start; note < note_end;
			note += note_size, i++) {
		const unsigned char *name, *desc;
		ElfNN_Nhdr hdr;
		read_nhdr(ehdr, &hdr, note);
		note_size  = sizeof(hdr);
		name       = note + note_size;
		note_size += (hdr.n_namesz + 3) & ~3;
		desc       = note + note_size;
		note_size += (hdr.n_descsz + 3) & ~3;

		if ((hdr.n_namesz != 0) && (name[hdr.n_namesz -1] != '\0')) {
			/* If note name string is not null terminated, just
			 * warn user about it and continue processing. This
			 * allows us to parse /proc/kcore on older kernels
			 * where /proc/kcore elf notes were not null
			 * terminated. It has been fixed in 2.6.19.
			 */
			printf("Warning: Elf Note name is not null "
					"terminated\n");
		}
		ehdr->e_note[i].n_type = hdr.n_type;
		ehdr->e_note[i].n_name = (char *)name;
		ehdr->e_note[i].n_desc = desc;
		ehdr->e_note[i].n_descsz = hdr.n_descsz;

	}

	return 0;
}

void free_elf_info(struct mem_ehdr *ehdr)
{
	free(ehdr->e_phdr);
	free(ehdr->e_shdr);
	memset(ehdr, 0, sizeof(*ehdr));
}

int build_elf_info(const char *buf, off_t len, struct mem_ehdr *ehdr,
			uint32_t flags)
{
	int result;

	result = build_mem_ehdr(buf, len, ehdr);
	if (result < 0) {
		return result;
	}

	if ((ehdr->e_phoff > 0) && (ehdr->e_phnum > 0)) {
		result = build_mem_phdrs(buf, len, ehdr, flags);
		if (result < 0) {
			free_elf_info(ehdr);
			return result;
		}
	}

	if ((ehdr->e_shoff > 0) && (ehdr->e_shnum > 0)) {
		result = build_mem_shdrs(buf, len, ehdr, flags);
		if (result < 0) {
			free_elf_info(ehdr);
			return result;
		}
	}

	result = build_mem_notes(ehdr);
	if (result < 0) {
		free_elf_info(ehdr);
		return result;
	}

	return 0;
}

int check_room_for_elf(struct list_head *elf_segments)
{
	struct memory_bank *bank;
	struct resource *res, *r;

	list_for_each_entry(r, elf_segments, sibling) {
		int got_bank;

		got_bank = 0;
		for_each_memory_bank(bank) {
			resource_size_t start, end;

			res = bank->res;

			start = virt_to_phys((void *)res->start);
			end = virt_to_phys((void *)res->end);

			if ((start <= r->start) && (end >= r->end)) {
				got_bank = 1;
				break;
			}
		}

		if (!got_bank)
			return -1;
	}

	return 0;
}

/* sort by size */
static int compare(struct list_head *a, struct list_head *b)
{
	struct resource *ra = (struct resource *)list_entry(a, struct resource, sibling);
	struct resource *rb = (struct resource *)list_entry(b, struct resource, sibling);
	resource_size_t sa, sb;

	sa = ra->end - ra->start;
	sb = rb->end - rb->start;

	if (sa > sb)
		return -1;
	if (sa < sb)
		return 1;
	return 0;
}

void list_add_used_region(struct list_head *new, struct list_head *head)
{
	struct list_head *pos, *insert = head;
	struct resource *rb =
		(struct resource *)list_entry(new, struct resource, sibling);
	struct list_head *n;

	/* rb --- new region */
	list_for_each_safe(pos, n, head) {
		struct resource *ra = (struct resource *)list_entry(pos, struct resource, sibling);

		if (((rb->end >= ra->start) && (rb->end <= ra->end))
			|| ((rb->start >= ra->start) && (rb->start <= ra->end))
			|| ((rb->start >= ra->start) && (rb->end <= ra->end))
			|| ((ra->start >= rb->start) && (ra->end <= rb->end))
			|| (ra->start == rb->end + 1)
			|| (rb->start == ra->end + 1)) {
			rb->start = min(ra->start, rb->start);
			rb->end = max(ra->end, rb->end);
			rb->name = "join";
			list_del(pos);
		}
	}

	list_for_each(pos, head) {
		struct resource *ra = (struct resource *)list_entry(pos, struct resource, sibling);

		if (ra->start < rb->start)
			continue;

		insert = pos;
		break;
	}

	list_add_tail(new, insert);
}

resource_size_t dcheck_res(struct list_head *elf_segments)
{
	struct memory_bank *bank;
	struct resource *res, *r, *t;

	LIST_HEAD(elf_relocate_banks);
	LIST_HEAD(elf_relocate_banks_size_sorted);
	LIST_HEAD(used_regions);

	for_each_memory_bank(bank) {
		res = bank->res;

		list_for_each_entry(r, &res->children, sibling) {
			t = create_resource("tmp",
				virt_to_phys((void *)r->start),
				virt_to_phys((void *)r->end));
			list_add_used_region(&t->sibling, &used_regions);
		}
	}

	list_for_each_entry(r, elf_segments, sibling) {
		t = create_resource(r->name, r->start, r->end);
		list_add_used_region(&t->sibling, &used_regions);
	}

	for_each_memory_bank(bank) {
		resource_size_t start;

		res = bank->res;
		res = create_resource("tmp",
				virt_to_phys((void *)res->start),
				virt_to_phys((void *)res->end));
		start = res->start;

		list_for_each_entry(r, &used_regions, sibling) {
			if (res->start > r->end)
				continue;

			if (res->end < r->start)
				continue;

			if (r->start - start) {
				struct resource *t;

				t = create_resource("ELF buffer", start, r->start - 1);
				list_add_used_region(&t->sibling, &elf_relocate_banks);
			}
			start = r->end + 1;
		}

		if (res->end - start) {
			struct resource *t;

			t = create_resource("ELF buffer", start, res->end);
			list_add_used_region(&t->sibling, &elf_relocate_banks);
		}
	}

	list_for_each_entry(r, &elf_relocate_banks, sibling) {
		struct resource *t;

		t = create_resource("ELF buffer", r->start, r->end);
		list_add_sort(&t->sibling,
			&elf_relocate_banks_size_sorted, compare);
	}

	r = list_first_entry(&elf_relocate_banks_size_sorted, struct resource, sibling);

	/* FIXME */
	return r->start;
}
