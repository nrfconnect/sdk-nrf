.. _lib_flash_patch:

Flash patch
###########

.. contents::
   :local:
   :depth: 2

The Flash patch library is used to disable the Flash Patch and Breakpoint (FPB) feature to increase the security of the device.
Flash Patch and Breakpoint (FPB) is a debug feature in the Cortex-M4 core.
For more information, see the `ARM Flash Patch and Breakpoint Unit`_ documentation.

The FPB allows code executing on the device to configure flash patches.
The CPU executes these flash patches on the next system reset instead of the regular code stored in the flash.
A rogue application can use the FPB to patch away parts of the secure boot code on next system reset.
Hence, you can disable the Flash Patch and Breakpoint (FPB) feature using this library and increase the security of the device.

The library is intended to be used only on the nRF52840 and nRF52833 SoCs.
Though all the Cortex-M4 SoCs have the FPB feature, only the nRF52840 and nRF52833 SOCs have the option to disable it.

Implementation
==============

The library runs on each boot and checks (in UICR) whether the Flash Patch and Breakpoint (FPB) feature is enabled and disables it if enabled.
Disabling is a one-time operation and after this, the library does not perform any further action for the rest of the lifetime of the device.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_DISABLE_FLASH_PATCH` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

API documentation
*****************

The library does not expose any API of its own.

| Source files: :file:`lib/flash_patch/flash_patch.c`
