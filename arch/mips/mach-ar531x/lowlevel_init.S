#include <mach/ar531x_regs.h>
//#include <common.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm-generic/memory_layout.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>


.globl lowlevel_init
lowlevel_init:
	li	t5, MEM_CFG0_X	# try x16
	li	t6, 0x23000	# use 8-word burst

        /* Configure for a x16 SDRAM with 8-word burst */
        li      t5, MEM_CFG0_X           # try x16
        li      t6, 0x23000              # use 8-word burst
99:
        li      a0, KSEG1 | AR531X_MEM_CFG0
        li      a1, KSEG1 | AR531X_MEM_CFG1
        li      a2, KSEG1 | AR531X_MEM_REF

        /* 1) wait 100us - handled by latency to boot up to this point. */

        /* 2) Set the I and M bits to issue an SDRAM nop */
        li      t0, MEM_CFG1_E0 | (MEM_CFG1_AC_128 << MEM_CFG1_AC0_S) | \
                    MEM_CFG1_E1 | (MEM_CFG1_AC_128 << MEM_CFG1_AC1_S) | \
                    MEM_CFG1_M | MEM_CFG1_I
        sw      t0, 0(a1)               # MEM_CFG1

        /* 3) Wait 200us (roughly) */
//        MEMDELAY(50, t2)

        /* 4) Reset the M bit to issue an SDRAM PRE-ALL */
        li      t0, MEM_CFG1_E0 | (MEM_CFG1_AC_128 << MEM_CFG1_AC0_S) | \
                    MEM_CFG1_E1 | (MEM_CFG1_AC_128 << MEM_CFG1_AC1_S) | \
                    MEM_CFG1_I
        sw      t0, 0(a1)               # MEM_CFG1
        sync

        /* 5) Generate a refresh every 16 clocks (spec says 10) */
        li      t0, 16                  # very fast refresh for now
        sw      t0, 0(a2)               # MEM_REF

        /* 6) Wait for 128 clocks (8 refresh cycles) */
//        MEMDELAY(5, t2)

        /*
         * 7) Set operational value into the reset timer.
         *    [out of order from ARM SDRAM ctrl spec]
         */

        /* 8) Set command write mode, and read each SDRAM */
        li      t0, MEM_CFG1_E0 | (MEM_CFG1_AC_128 << MEM_CFG1_AC0_S) | \
                    MEM_CFG1_E1 | (MEM_CFG1_AC_128 << MEM_CFG1_AC1_S) | \
                    MEM_CFG1_M
        sw      t0, 0(a1)               # MEM_CFG1
        sync
        li      t0, KSEG1 | AR531X_SDRAM0 
        or      t0, t6 # burst
        lw      zero, 0(t0)
        li      t0, KSEG1 | AR531X_SDRAM1
        or      t0, t6 # burst
        lw      zero, 0(t0)

        /* 9) Program configuration register 0 */
        li      t0, MEM_CFG0_C | MEM_CFG0_C2 | MEM_CFG0_R1 | \
                    MEM_CFG0_B0 | MEM_CFG0_B1
        or      t0, t5 # x16 or x32
        sw      t0, 0(a0)               # MEM_CFG0
        sync

        li      t0, AR531X_SDRAM_MEMORY_REFRESH_VALUE
        sw      t0, 0(a2)               # MEM_REF
        sync

        /* 10) Clear I and M and set cfg1 to the normal operational value */
        li      t0, MEM_CFG1_E0 | (MEM_CFG1_AC_128 << MEM_CFG1_AC0_S) | \
                    MEM_CFG1_E1 | (MEM_CFG1_AC_128 << MEM_CFG1_AC1_S)
        sw      t0, 0(a1)               # MEM_CFG1
        sync

