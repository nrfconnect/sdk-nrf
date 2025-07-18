.. _app_bootloaders:

Bootloaders and DFU
###################

.. contents::
   :local:
   :depth: 2

Depending on the device, you need to use different bootloader and DFU solutions :ref:`MCUboot and nRF Secure Immutable Bootloader (NSIB) <ug_bootloader_mcuboot_nsib>`.

See the following table for further comparison.


   MCUboot and |NSIB| architecture comparison

+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Characteristic           | MCUboot                                                                 | nRF Secure Immutable Bootloader                                                           |
+==========================+=========================================================================+===========================================================================================+
| Primary function         | Bootloader and DFU                                                      | Secure Immutable Bootloader                                                               |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Customization            | Built by users; partitions customized with Kconfig options and DTS.     | Limited; focused on initial boot security.                                                |
|                          | Kconfig and DTS configurable with multi-image builds or sysbuild.       |                                                                                           |
|                          | Becomes static post-compilation.                                        |                                                                                           |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Slot management          | Symmetrical primary and secondary slot style.                           | Not applicable for firmware updates.                                                      |
|                          | Primary slot is where the system is executed from (by default).         |                                                                                           |
|                          | Secondary slot is the destination for the DFU (by default).             |                                                                                           |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Slot characteristics     | Equal primary and secondary slot sizes lead to high memory overhead.    | Highly efficient due to immutable design.                                                 |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Slot definition          | Static definition; challenging to change post-deployment.               | Fixed and immutable; no post-deployment changes.                                          |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Invocation process       | Limited customization through metadata.                                 | Authenticated firmware execution; no update mechanism post-boot.                          |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+
| Flash memory layout      | Specific allocations for primary and secondary slots.                   | OTP regions for provisioned data; specific layout for boot and application partitions.    |
+--------------------------+-------------------------------------------------------------------------+-------------------------------------------------------------------------------------------+

To learn more, refer to the following documentation pages:

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   mcuboot_nsib/bootloader_mcuboot_nsib
   qspi_xip_split_image
   dfu_tools_mcumgr_cli
   mcuboot_serial_recovery
   mcuboot_image_compression
   sysbuild_image_ids
