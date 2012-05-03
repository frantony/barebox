/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#ifndef _ASM_MIPS_MODULE_H_
#define _ASM_MIPS_MODULE_H_

struct mod_arch_specific
{
};

#ifdef CONFIG_32BIT
#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym		Elf32_Sym
#define Elf_Ehdr	Elf32_Ehdr
#define Elf_Addr	Elf32_Addr

#define Elf_Mips_Rel	Elf32_Rel
#define Elf_Mips_Rela	Elf32_Rela

#define ELF_MIPS_R_SYM(rel) ELF32_R_SYM(rel.r_info)
#define ELF_MIPS_R_TYPE(rel) ELF32_R_TYPE(rel.r_info)
#endif

#endif /* _ASM_MIPS_MODULE_H_ */
