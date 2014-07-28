Canon DIGIC
===========

Canon PowerShot A1100 IS
------------------------

Running barebox on QEMU
^^^^^^^^^^^^^^^^^^^^^^^

QEMU supports Canon A1100 camera emulation since version 2.0.

Usage::

  $ qemu-system-arm -M canon-a1100 \
      -nographic -monitor null -serial stdio \
      -bios barebox.canon-a1100.bin


Running barebox on real camera
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Install CHDK firmware on SD-card (see http://chdk.wikia.com/wiki/CHDK_For_Newbies_-_How_To_Install and http://chdk.wikia.com/wiki/A1100).

Make your SD-card bootable (see http://chdk.wikia.com/wiki/Prepare_your_SD_card).

Build barebox: you will get the ``DISKBOOT-A1100.BIN`` file.

Overwrite CHDK boot file on your SD-card (``DISKBOOT.BIN``)
with the barebox ``DISKBOOT-A1100.BIN`` file.

Lock your SD-card (use small switch on the card).
**It is obligatory!**

Insert the SD-card into your camera.
Close your camera's SD-card slot.


Turn your camera on (press the 'Playback mode' button).
Barebox will run; it will use the serial port for console
(see http://chdk.wikia.com/wiki/UART for details on A1100
UART connection).


LED script
~~~~~~~~~~

If you have no oppotunity to use camera's serial port then
you can use leds for checking barebox.

Enable ``CONFIG_DEFAULT_ENVIRONMENT`` in the ``.config``
and set ``CONFIG_DEFAULT_ENVIRONMENT_PATH="arch/arm/boards/canon-a1100/env"``.

Use this ``arch/arm/boards/canon-600d/env/bin/init``::

  #!/bin/sh
  
  # use "direct print" led (blue)
  LED=0
  
  DELAY=300
  
  for i in 1 2 3; do
         led $LED 1
         msleep $DELAY
         led $LED 0
         msleep $DELAY
  done

After barebox start the camera's blue led will blink.
