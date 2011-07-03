#
# Makefile for the PCI bus specific drivers.
#
obj-y		+= pci.o bus.o

ccflags-$(CONFIG_PCI_DEBUG) := -DDEBUG

CPPFLAGS += $(ccflags-y)
