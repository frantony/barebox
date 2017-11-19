extra-$(CONFIG_GENERIC_LINKER_SCRIPT) += barebox.lds

obj-y += riscv_timer.o

obj-$(CONFIG_32BIT) += lshrdi3.o
obj-$(CONFIG_32BIT) += ashldi3.o
obj-$(CONFIG_32BIT) += ashrdi3.o

obj-y += muldi3.o mulsi3.o div.o

obj-$(CONFIG_BUILTIN_DTB) += dtb.o
