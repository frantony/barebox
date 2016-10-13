KBUILD_DEFCONFIG := pk_defconfig

CPPFLAGS += -fno-strict-aliasing

machine-y := pk
board-y := pk

TEXT_BASE = $(CONFIG_TEXT_BASE)

machdirs := $(patsubst %,arch/riscv/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

archprepare: maketools

PHONY += maketools

cmd_barebox__ = $(CC) -o $@ -Wl,-T,$(barebox-lds) \
	-Wl,--start-group $(barebox-common) -Wl,--end-group

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

common-$(CONFIG_OFTREE) += arch/riscv/dts/

lds-y   := $(BOARD)/barebox.lds

CLEAN_FILES += $(BOARD)/barebox.lds

ifeq ($(machine-y),pk)
CFLAGS += \
	-Dmalloc=barebox_malloc \
	-Dcalloc=barebox_calloc \
	-Dfree=barebox_free \
	-Drealloc=barebox_realloc \
	-Dread=barebox_read \
	-Dwrite=barebox_write \
	-Dopen=barebox_open \
	-Dclose=barebox_close \
	-Dlseek=barebox_lseek \
	-Dperror=barebox_perror \
	-Derrno=barebox_errno \
	-Dgetc=barebox_getc \
	-Dputc=barebox_putc \
	-Dfgetc=barebox_fgetc \
	-Dfputc=barebox_fputc \
	-Dfgets=barebox_fgets \
	-Dfputs=barebox_fputs \
	-Dsetenv=barebox_setenv \
	-Dgetenv=barebox_getenv \
	-Dprintf=barebox_printf \
	-Dglob=barebox_glob \
	-Dglobfree=barebox_globfree \
	-Dioctl=barebox_ioctl \
	-Dfstat=barebox_fstat \
	-Dlstat=barebox_lstat \
	-Dstat=barebox_stat \
	-Dchdir=barebox_chdir \
	-Dunlink=barebox_unlink \
	-Dgetcwd=barebox_getcwd \
	-Dsbrk=barebox_sbrk \
	-Dgetopt=barebox_getopt \
	-Doptarg=barebox_optarg \
	-Doptind=barebox_optind \
	-Dopterr=barebox_opterr \
	-Doptopt=barebox_optopt \
	-Dgetopt_context_store=barebox_getopt_context_store \
	-Dgetopt_context_restore=barebox_getopt_context_restore
endif
