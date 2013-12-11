#!/bin/bash

BBORIG=arch/arm/pbl/zbarebox.bin
BBOUT=canon-a1100-rom1.bin

dd if=/dev/zero of=$BBOUT bs=64K count=64
dd if=$BBORIG of=$BBOUT bs=64K count=64 conv=notrunc

#
# Put branch instraction into reset high vector:
#
# ffc00000 <_start>:
#         ...
# ffff0000:       eaf03ffe        b       ffc00000 <_start>
#
echo -n -e "\xfe\x3f\xf0\xea" | dd of=$BBOUT bs=64K seek=63 conv=notrunc
