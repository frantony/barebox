RISC-V
======

At the moment only qemu emulator is supported (see https://github.com/riscv/riscv-isa-sim
for details).

Running RISC-V barebox
----------------------

Obtain RISC-V GCC/Newlib Toolchain,
see https://github.com/riscv/riscv-tools/blob/master/README.md
for details. The ``build.sh`` script from ``riscv-tools`` should
create toolchain.

Next compile qemu emulator::

  $ git clone https://github.com/riscv/riscv-qemu
  $ cd riscv-qemu
  $ cap="no" ./configure \
    --extra-cflags="-Wno-maybe-uninitialized" \
    --audio-drv-list="" \
    --disable-attr \
    --disable-blobs \
    --disable-bluez \
    --disable-brlapi \
    --disable-curl \
    --disable-curses \
    --disable-docs \
    --disable-kvm \
    --disable-spice \
    --disable-sdl \
    --disable-vde \
    --disable-vnc-sasl \
    --disable-werror \
    --enable-trace-backend=simple \
    --disable-stack-protector \
    --target-list=riscv32-softmmu,riscv64-softmmu
  $ make

Next compile barebox::

  $ make qemu-sifive_defconfig ARCH=riscv
  ...
  $ make ARCH=riscv CROSS_COMPILE=<path to your riscv toolchain>/riscv64-unknown-elf-

Run barebox::

  $ <path to riscv-qemu source>/riscv64-softmmu/qemu-system-riscv64 \
      -nographic -M sifive -kernel <path to barebox sources >/barebox \
      -serial stdio -monitor none -trace file=/dev/null

  Switch to console [cs0]
  
  
  barebox 2016.11.0-00006-g3c29f61 #1 Mon Nov 13 22:21:03 MSK 2016
  
  
  Board: QEMU SiFive
  Warning: Using dummy clocksource
  malloc space: 0x00800000 -> 0x00bfffff (size 4 MiB)
  running /env/bin/init...
  /env/bin/init not found
  barebox:/
