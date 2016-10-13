machdirs := arch/riscv/mach-pk/

ifeq ($(KBUILD_SRC),)
CPPFLAGS := $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS := $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

CFLAGS := -W -Wall -DCONFIG_MALLOC_SIZE=$(CONFIG_MALLOC_SIZE)
NOSTDINC_FLAGS :=

obj-y = common.o
