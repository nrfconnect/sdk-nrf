.. _ug_bootloader_external_flash:

Using external flash memory partitions
######################################

.. contents::
   :local:
   :depth: 2


When using MCUboot, you can store the storage partition for the secondary slot in the external flash memory, using a driver for the external flash memory that supports the following features:

* Single-byte read and write.
* Writing data from the internal flash memory to the external flash memory.

.. note::

   The Partition Manager will only support run-time access to flash partitions defined in regions placed on external flash devices that have drivers compiled in.
   Partition Manager cannot determine which partitions will be used at runtime, but only those that have drivers enabled, and those are included into the partition map.
   Lack of partition access will cause MCUboot to fail at runtime.
   For more details on configuring and enabling access to external flash devices, see :ref:`pm_external_flash`.

The QSPI NOR flash memory driver supports these features, and it can access the QSPI external flash memory of the nRF52840 DK and nRF5340 DK.

See the test in :file:`tests/modules/mcuboot/external_flash` for reference.
This test passes both devicetree overlay files and Kconfig fragments to the MCUboot child image through its :file:`child_image` folder.
See also :ref:`ug_multi_image_variables` for more details on how to pass configuration files to a child image.

Troubleshooting
***************

MCUboot could fail, reporting the following error:

.. code-block:: console

   *** Booting Zephyr OS build v3.1.99-ncs1-... ***
   I: Starting bootloader
   W: Failed reading sectors; BOOT_MAX_IMG_SECTORS=512 - too small?
   W: Cannot upgrade: not a compatible amount of sectors
   I: Bootloader chainload address offset: 0x10000
   I: Jumping to the first image slot

This error could be caused by the following issues:

  * The external flash driver for the application image partitions used by MCUboot is not enabled or an incorrect Kconfig option has been given to the ``DEFAULT_DRIVER_KCONFIG`` of the partition manager external region definition.
    See :ref:`pm_external_flash` for details.

  * An out-of-tree external flash driver is not selecting :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_HAS_DRIVER`, resulting in partitions for images located in the external flash memory being not accessible.

The compilation could fail, reporting a linker error similar to following:

.. code-block:: console

   undefined reference to '__device_dts_ord_<digits>

This error could be caused by the following issues:

  * :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK` has been used to override the driver check for the external flash driver, but no driver is actually compiled for the region.
    Disabling the option removes partitions without device drivers from the flash map, which may cause runtime failures.
    See :ref:`pm_external_flash` for details.

  * ``DEFAULT_DRIVER_KCONFIG`` is given a Kconfig that neither controls nor indicates whether a flash device driver is compiled in.
    See :ref:`pm_external_flash` for details.
