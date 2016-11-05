BUILTIN_DTB := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
obj-$(CONFIG_BUILTIN_DTB) += $(BUILTIN_DTB).dtb.o

# just to build a built-in.o. Otherwise compilation fails when no devicetree is
# created.
obj- += dummy.o

always := $(dtb-y)
clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts
