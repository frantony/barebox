#ifndef KEXEC_ELF_H
#define KEXEC_ELF_H

struct kexec_info;

struct mem_ehdr {
	unsigned ei_class;
	unsigned ei_data;
	unsigned e_type;
	unsigned e_machine;
	unsigned e_version;
	unsigned e_flags;
	unsigned e_phnum;
	unsigned e_shnum;
	unsigned e_shstrndx;
	unsigned long long e_entry;
	unsigned long long e_phoff;
	unsigned long long e_shoff;
	unsigned e_notenum;
	struct mem_phdr *e_phdr;
	struct mem_shdr *e_shdr;
	struct mem_note *e_note;
	unsigned long rel_addr, rel_size;
};

struct mem_phdr {
	unsigned long long p_paddr;
	unsigned long long p_vaddr;
	unsigned long long p_filesz;
	unsigned long long p_memsz;
	unsigned long long p_offset;
	const char *p_data;
	unsigned p_type;
	unsigned p_flags;
	unsigned long long p_align;
};

struct mem_shdr {
	unsigned sh_name;
	unsigned sh_type;
	unsigned long long sh_flags;
	unsigned long long sh_addr;
	unsigned long long sh_offset;
	unsigned long long sh_size;
	unsigned sh_link;
	unsigned sh_info;
	unsigned long long sh_addralign;
	unsigned long long sh_entsize;
	const unsigned char *sh_data;
};

struct mem_note {
	unsigned n_type;
	unsigned n_descsz;
	const char *n_name;
	const void *n_desc;
};

/* The definition of an ELF note does not vary depending
 * on ELFCLASS.
 */
typedef struct
{
	uint32_t n_namesz;		/* Length of the note's name.  */
	uint32_t n_descsz;		/* Length of the note's descriptor.  */
	uint32_t n_type;		/* Type of the note.  */
} ElfNN_Nhdr;

extern void free_elf_info(struct mem_ehdr *ehdr);
extern int build_elf_info(const char *buf, off_t len, struct mem_ehdr *ehdr,
				uint32_t flags);
extern int build_elf_exec_info(const char *buf, off_t len,
				struct mem_ehdr *ehdr, uint32_t flags);

extern int elf_exec_load(struct mem_ehdr *ehdr, struct kexec_info *info);

uint16_t elf16_to_cpu(const struct mem_ehdr *ehdr, uint16_t value);
uint32_t elf32_to_cpu(const struct mem_ehdr *ehdr, uint32_t value);
uint64_t elf64_to_cpu(const struct mem_ehdr *ehdr, uint64_t value);

unsigned long elf_max_addr(const struct mem_ehdr *ehdr);
int check_room_for_elf(struct list_head *elf_segments);
resource_size_t dcheck_res(struct list_head *elf_segments);
void list_add_used_region(struct list_head *new, struct list_head *head);

#endif /* KEXEC_ELF_H */
