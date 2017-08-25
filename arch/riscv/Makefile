CPPFLAGS += -fno-strict-aliasing

ifeq ($(CONFIG_ARCH_RV32I),y)
	cflags-y += -march=rv32im
endif

cflags-y += -fno-pic -pipe
cflags-y += -Wall -Wmissing-prototypes -Wstrict-prototypes \
	-Wno-uninitialized -Wno-format -Wno-main -mcmodel=medany

LDFLAGS += $(ldflags-y)
LDFLAGS_barebox += -nostdlib

machine-$(CONFIG_MACH_ERIZO)	:= erizo
board-$(CONFIG_BOARD_ERIZO_GENERIC)	:= erizo-generic

TEXT_BASE = $(CONFIG_TEXT_BASE)
CPPFLAGS += -DTEXT_BASE=$(CONFIG_TEXT_BASE)

ifndef CONFIG_MODULES
# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_barebox += -static --gc-sections
endif

KBUILD_BINARY := barebox.bin

machdirs := $(patsubst %,arch/riscv/mach-%/,$(machine-y))

ifneq ($(board-y),)
BOARD := arch/riscv/boards/$(board-y)/
else
BOARD :=
endif

ifeq ($(KBUILD_SRC),)
CPPFLAGS += -I$(BOARD)/include
else
CPPFLAGS += -I$(srctree)/$(BOARD)/include
endif

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

ifeq ($(CONFIG_MACH_ERIZO),y)
KBUILD_IMAGE := barebox.erizo.nmon
endif

quiet_cmd_erizo_nmon_image = MKIMAGE $@
      cmd_erizo_nmon_image = $(srctree)/scripts/erizo-nmon-image $< $@ || \
	echo "WARNING: Couldn't create erizo nmon image due to previous errors."

barebox.erizo.nmon: $(KBUILD_BINARY) FORCE
	$(call if_changed,erizo_nmon_image)
