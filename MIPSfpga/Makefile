CROSS_COMPILE=mips-linux-gnu-
OBJDUMP=$(CROSS_COMPILE)objdump

.PHONY: clean

all: mipsfpga_nmon.mif mipsfpga_nmon.txt

mipsfpga_nmon.bin: zbarebox.bin
	dd if=$< of=$@ bs=1k count=2

mipsfpga_nmon.mif: mipsfpga_nmon.bin
	srec_cat $< -binary -offset 0 -o $@ -Memory_Initialization_File

mipsfpga_nmon.txt: mipsfpga_nmon.bin
	$(OBJDUMP) -EB -m mips:isa32 -b binary --adjust-vma=0xbfc00000 -z -D $< | grep "^[0-9a-f]\{8\}:" | awk '{ printf "%s // %s\n", $$2, $$0; }' | cut -f 2 --complement > $@

clean:
	rm -f mipsfpga_nmon.bin
	rm -f mipsfpga_nmon.mif
	rm -f mipsfpga_nmon.txt
