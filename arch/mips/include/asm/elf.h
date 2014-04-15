/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Much of this is taken from binutils and GNU libc ...
 */

/**
 * @file
 * @brief mips specific elf information
 *
 */

#ifndef _ASM_MIPS_ELF_H
#define _ASM_MIPS_ELF_H

#ifndef ELF_ARCH

/* Legal values for e_machine (architecture).  */

#define EM_MIPS		 8		/* MIPS R3000 big-endian */
#define EM_MIPS_RS4_BE	10		/* MIPS R4000 big-endian */

#ifdef CONFIG_32BIT

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x)->e_machine == EM_MIPS)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS32

#endif /* CONFIG_32BIT */

#ifdef CONFIG_64BIT
/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS	ELFCLASS64

#endif /* CONFIG_64BIT */

/*
 * These are used to set parameters in the core dumps.
 */
#ifdef __MIPSEB__
#define ELF_DATA	ELFDATA2MSB
#elif defined(__MIPSEL__)
#define ELF_DATA	ELFDATA2LSB
#endif
#define ELF_ARCH	EM_MIPS

#endif /* !defined(ELF_ARCH) */
#endif /* _ASM_MIPS_ELF_H */
