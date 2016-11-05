CPPFLAGS += -fno-strict-aliasing

cflags-y += -fno-pic -pipe
cflags-y += -Wall -Wmissing-prototypes -Wstrict-prototypes \
	-Wno-uninitialized -Wno-format -Wno-main

LDFLAGS += $(ldflags-y)
LDFLAGS_barebox += -nostdlib

machine-$(CONFIG_MACH_SIFIVE)	:= sifive
board-$(CONFIG_BOARD_QEMU_SIFIVE)	:= qemu-sifive

TEXT_BASE = $(CONFIG_TEXT_BASE)
CPPFLAGS += -DTEXT_BASE=$(CONFIG_TEXT_BASE)

machdirs := $(patsubst %,arch/riscv/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

archprepare: maketools

PHONY += maketools

ifneq ($(machine-y),)
MACH  := arch/riscv/mach-$(machine-y)/
else
MACH  :=
endif

ifneq ($(board-y),)
BOARD := arch/riscv/boards/$(board-y)/
else
BOARD :=
endif

common-y += $(BOARD) $(MACH)
common-y += arch/riscv/lib/
common-y += arch/riscv/boot/

common-$(CONFIG_OFTREE) += arch/riscv/dts/

CPPFLAGS += $(cflags-y)
CFLAGS += $(cflags-y)

lds-y	:= arch/riscv/lib/barebox.lds

CLEAN_FILES += arch/riscv/lib/barebox.lds
