Ainol Novo7 Paladin
===================

The tablet has

  * Ingenic JZ4770 SoC;
  * 512 MiB DDR2 SDRAM;
  * 8 GiB eMMC Flash Memory;
  * UART;
  * touchscreen (800x480);
  * 1xUSB interface;
  * buttons.

The tablet uses X-Boot (Ingenic's not interactive U-Boot) as bootloader.

Running barebox
---------------

  1. Connect to the tablet's UART (see. http://a320.emulate.su/2012/05/14/mini-obzor-planshet-ainol-novo7-paladin-i-processor-jz4770/);

  2. Download and compile ingenic-boot tool for GCW0

.. code-block:: none

    $ git clone https://github.com/gcwnow/ingenic-boot
    $ cd ingenic-boot
    ingenic-boot$ make
..

  3. Push & hold VOL- (GPIO 114 aka D18) while power on or reset (this selects USB boot mode by activating BOOTSEL1 signal);

  3. Upload and start barebox:

.. code-block:: none

    ingenic-boot$ sudo ./tool/down2ram.sh --config=gcw0_v11_ddr2_512mb --bin=${PATH_TO_BAREBOX_DIR}/barebox-flash-image --downto=0xa0800000 --runat=0xa0800000
..

Links
-----

  * http://www.ainol-novo.com/ainol-novo-7-paladin-first-android-4-0-tablet-pc.html
  * ftp://ftp.ingenic.cn/SOC/JZ4770/JZ4770_DS.PDF
  * https://github.com/IngenicSemiconductor/X-BOOT_NPM801
  * https://github.com/gcwnow/UBIBoot
