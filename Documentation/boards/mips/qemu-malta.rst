QEMU Malta
==========

Big-endian mode
---------------

QEMU run string:

.. code-block:: sh

  qemu-system-mips -nodefaults -M malta -m 256 \
      -device VGA -serial stdio -monitor null \
      -bios ./images/barebox-qemu-malta.img


Little-endian mode
------------------

In little-endian mode the 32bit words in the boot flash image are swapped,
a neat trick which allows bi-endian firmware.

The barebox build generates a second ``./images/barebox-qemu-malta.img.swapped``
image that can be used in this case, e.g.:

.. code-block:: sh

  qemu-system-mipsel -nodefaults -M malta -m 256 \
      -device VGA -serial stdio -monitor null \
      -bios ./images/barebox-qemu-malta.img.swapped


Using GXemul
------------

GXemul supports MIPS Malta except PCI stuff.
You can use GXemul to run little-endian barebox (use gxemul-malta_defconfig).

N.B. There is no need to swap words in ``zbarebox.bin`` for little-endian GXemul!

GXemul run string:

.. code-block:: sh

  gxemul -Q -e malta -M 256 0xbfc00000:barebox-flash-image


Links
-----

  * http://www.linux-mips.org/wiki/Mips_Malta
  * http://www.qemu.org/
  * http://gxemul.sourceforge.net/
