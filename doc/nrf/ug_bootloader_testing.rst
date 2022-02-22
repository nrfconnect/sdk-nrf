.. _ug_bootloader_testing:

Testing the bootloader chain
############################

To test either of the bootloaders and the secure bootloader chain, build and program them with any sample as described in :ref:`Adding a bootloader chain  <ug_bootloader_adding>`.

By default, both |NSIB| and MCUboot print information to their serial output on boot.
This output includes information about the validation of images in its slots, as well as firmware-specific information if using :kconfig:option:`CONFIG_FW_INFO` with the |NSIB|.
To see this output:

1. |connect_terminal|
#. Reset the development kit.
#. Observe that each bootloader in the chain prints its information upon boot (some values may vary by build):

.. tabs::

   .. tab:: |NSIB|

      .. code-block::

         Attempting to boot slot 0.
         Attempting to boot from address 0x9000.
         Verifying signature against key 0.
         Hash: 0xc0...71
         Firmware signature verified.
         Firmware version 1
         Setting monotonic counter (version: 1, slot: 0)

   .. tab:: MCUboot

      .. code-block::

         [00:00:00.359,039] <inf> mcuboot: Starting bootloader
         [00:00:00.365,295] <inf> mcuboot: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
         [00:00:00.375,671] <inf> mcuboot: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
         [00:00:00.386,169] <inf> mcuboot: Boot source: none
         [00:00:00.391,815] <inf> mcuboot: Swap type: none
         [00:00:00.420,166] <inf> mcuboot: Bootloader chainload address offset: 0xc000
         [00:00:00.428,039] <inf> mcuboot: Jumping to the first image slot

When compiled with minimal configurations that disable logging output, such as ``prj_minimal.conf``, you can disable the bootloader information output altogether or per-bootloader.
Refer to the source code directories of each bootloader to see what minimal configuration options are already available, or add them through a custom Kconfig fragment if desired.
