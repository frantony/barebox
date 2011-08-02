#
# Makefile for the PCI bus specific drivers.
#
obj-y		+= pci.o bus.o
obj-$(CONFIG_8139)	+= rtl8139.o

ccflags-$(CONFIG_PCI_DEBUG) := -DDEBUG

CPPFLAGS += $(ccflags-y)
