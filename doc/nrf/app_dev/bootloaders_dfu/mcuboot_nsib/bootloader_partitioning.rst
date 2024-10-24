.. _bootloader_partitioning:

Partitioning device memory
##########################

.. contents::
   :local:
   :depth: 2

Partitioning device memory is a crucial aspect of managing how a device's storage is utilized, especially when dealing with firmware updates and bootloader configurations.
By default, the Partition Manager in the system dynamically generates a partition map, which is suitable for most applications that do not use Device Firmware Upgrades (DFU).
For scenarios involving DFU, read the following sections.

.. _bootloader_partitioning_partitions_file:

Partition map file
******************

After you enable the Partition Manager, it will :ref:`start generating <pm_build_system>` the :file:`partitions.yml` file in the build folder directory.
In this file, you will see detailed information about the memory layout used for the build.

The :file:`partitions.yml` file is present also if the Partition Manager generates the partition map dynamically.
You can use this file as a base for your static partition map.

.. _bootloader_partitioning_partitions_file_report:

Partition reports
=================

After building the application, you can print a report of how the partitioning has been handled for a bootloader, or combination of bootloaders, by using partition memory reports.

Depending on your development environment, you can use one of the following options:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      Use the extension's `Memory report`_ feature, which shows the size and percentage of memory that each symbol uses on your device for RAM, ROM, and partitions.
      Click the :guilabel:`Memory report` button in the :guilabel:`Actions View` to generate the report.
      The partition map is available in the :guilabel:`Partitions` tab.

      Alternatively, you can also use the `Memory Explorer <How to work with the Memory Explorer_>`_ feature of the extension's nRF Debug to check memory sections for the partitions.
      This feature requires `enabling debugging in the build configuration <How to debug_>`_ and providing the partition addresses manually.

   .. group-tab:: Command line

      Use the Partition Manager's :ref:`pm_partition_reports` feature.
      Run the following command:

      .. code-block:: console

         west build -t partition_manager_report

      This command generates an output in ASCII with the addresses and sizes of each partition.

.. _ug_bootloader_flash_static_requirement:

Static partition requirement for DFU
************************************

If you want to perform DFU, you must :ref:`define a static partition map <ug_pm_static>` to ensure compatibility across different builds.
The dynamically generated partitions can change between builds.
This is important also when you use a precompiled HEX file as a child image (sub-image) instead of building it.
In such cases, the newly generated application images may no longer use a partition map that is compatible with the partition map used by the bootloader.
As a result, the newly built application image may not be bootable by the bootloader.

The memory partitions that must be defined in the static partition map depend on the selected bootloader chain.
For details, see :ref:`ug_bootloader_flash`.

.. _ug_bootloader_flash:

Flash memory partitions
***********************

Each bootloader handles flash memory partitioning differently.

After building the application, you can print a report of how the flash partitioning has been handled for a bootloader, or combination of bootloaders, by using :ref:`bootloader_partitioning_partitions_file_report`.

.. _ug_bootloader_flash_b0:

|NSIB| partitions
=================

See :ref:`bootloader_flash_layout` for implementation-specific information about this bootloader.

.. _ug_bootloader_flash_mcuboot:

MCUboot partitions
==================

For most applications, MCUboot requires two image slots:

* The *primary slot*, containing the application that will be booted.
* The *secondary slot*, where a new application can be stored before it is activated.

It is possible to use only the *primary slot* for MCUboot by using the ``CONFIG_SINGLE_APPLICATION_SLOT`` option.
This is particularly useful in memory-constrained devices to avoid providing space for two images.

See the *Image Slots* section in the :doc:`MCUboot documentation <mcuboot:design>` for more information.

The |NCS| variant of MCUboot uses the :ref:`partition_manager` to configure the flash memory partitions for these image slots.
In the default configuration, defined in :file:`bootloader/mcuboot/boot/zephyr/pm.yml`, the Partition Manager dynamically sets up the partitions as required for MCUboot.
For example, the partition layout for :file:`zephyr/samples/hello_world` using MCUboot on the ``nrf52840dk`` board would look like the following:

.. code-block:: console

    (0x100000 - 1024.0kB):
   +-----------------------------------------+
   | 0x0: mcuboot (0xc000)                   |
   +---0xc000: mcuboot_primary (0x7a000)-----+
   | 0xc000: mcuboot_pad (0x200)             |
   +---0xc200: mcuboot_primary_app (0x79e00)-+
   | 0xc200: app (0x79e00)                   |
   | 0x86000: mcuboot_secondary (0x7a000)    |
   +-----------------------------------------+

You can also store secondary slot images in external flash memory when using MCUboot.
See :ref:`ug_bootloader_external_flash` for more information.

.. _ug_bootloader_external_flash:

Using external flash memory partitions
**************************************

When using MCUboot, you can store the storage partition for the secondary slot in the external flash memory, using a driver for the external flash memory that supports the following features:

* Single-byte read and write.
* Writing data from the internal flash memory to the external flash memory.

To enable external flash with MCUboot, complete the following steps:

1. Follow the instructions in :ref:`pm_external_flash`, which enables external flash use in the nRF5340 DK's DTS file.

#. Enable the :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY` Kconfig option.
   (Depending on the build configuration, this option will be set to ``y`` automatically.)

#. Update the ``CONFIG_BOOT_MAX_IMG_SECTORS`` `MCUboot Kconfig option`_ accordingly for child images.
   This option defines the maximum number of image sectors MCUboot can handle, as MCUboot typically increases slot sizes when external flash is enabled.
   Otherwise the ``CONFIG_BOOT_MAX_IMG_SECTORS`` Kconfig option defaults to the value used for internal flash, and the application may not boot if the value is set too low.

   (The image sector size is the same as the flash erase-block-size across all |NCS| integrated memory.)

.. note::

   The Partition Manager will only support run-time access to flash partitions defined in regions placed on external flash devices that have drivers compiled in.
   The Partition Manager cannot determine which partitions will be used at runtime, but only those that have drivers enabled, and those are included into the partition map.
   Lack of partition access will cause MCUboot to fail at runtime.
   For more details on configuring and enabling access to external flash devices, see :ref:`pm_external_flash`.

The Quad Serial Peripheral Interface (QSPI) NOR flash memory driver supports these features, and it can access the QSPI external flash memory of the nRF52840 DK and nRF5340 DK.

See the test in :file:`tests/modules/mcuboot/external_flash` for reference.
This test passes both devicetree overlay files and Kconfig fragments to the MCUboot child image through its :file:`child_image` folder.
See also :ref:`ug_multi_image_variables` for more details on how to pass configuration files to a child image.

Troubleshooting
***************

This section describes some of the issues you might come across when partitioning device memory.

MCUboot failure
===============

MCUboot could fail, reporting the following error:

.. code-block:: console

   *** Booting Zephyr OS build v3.1.99-ncs1-... ***
   I: Starting bootloader
   W: Failed reading sectors; BOOT_MAX_IMG_SECTORS=512 - too small?
   W: Cannot upgrade: not a compatible amount of sectors
   I: Bootloader chainload address offset: 0x10000
   I: Jumping to the first image slot

This error could be caused by the following issues:

  * The external flash driver for the application image partitions used by MCUboot is not enabled or an incorrect Kconfig option has been given to the ``DEFAULT_DRIVER_KCONFIG`` of the Partition Manager external region definition.
    See :ref:`pm_external_flash` for details.

  * An out-of-tree external flash driver is not selecting :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_HAS_DRIVER`, resulting in partitions for images located in the external flash memory being not accessible.
    See :ref:`pm_external_flash` for details.

  * Insufficient value set for the ``CONFIG_BOOT_MAX_IMG_SECTORS`` Kconfig option, as MCUboot typically increases slot sizes when external flash is enabled.
    See `MCUboot's Kconfig options used in Zephyr <https://github.com/nrfconnect/sdk-mcuboot/blob/main/boot/zephyr/Kconfig#L370>`_ for details.

Compilation failure
===================

The compilation could fail, reporting a linker error similar to following:

.. code-block:: console

   undefined reference to '__device_dts_ord_<digits>

This error could be caused by the following issues:

  * :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK` has been used to override the driver check for the external flash driver, but no driver is actually compiled for the region.
    Disabling the option removes partitions without device drivers from the flash map, which may cause runtime failures.
    See :ref:`pm_external_flash` for details.

  * ``DEFAULT_DRIVER_KCONFIG`` is given a Kconfig that neither controls nor indicates whether a flash device driver is compiled in.
    See :ref:`pm_external_flash` for details.
