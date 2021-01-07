.. _ug_bootloader_external_flash:

Using external flash memory partitions
######################################

You can use the external flash memory as the storage partition for the secondary slot.
This requires a driver for the external flash memory that supports the following features:

* Single-byte read and write.
* Writing data from the internal flash memory to the external flash memory.

See :ref:`pm_external_flash` for general information about how to set up partitions in the external flash memory using the Partition Manager.

.. note::

   Currently, only MCUboot supports external flash memory partitions for image slots.

To configure a bootloader to use the external flash memory for the secondary slot, edit the configuration file of the bootloader child image to specify the external flash memory region.
For MCUboot, this configuration is :file:`ncs/bootloader/mcuboot/boot/zephyr/pm.yml`.

In this file, you must define the region under the section ``mcuboot_secondary``:

.. code-block:: yaml

   mcuboot_secondary:
       region: external_flash
       size: CONFIG_PM_EXTERNAL_FLASH_SIZE

The nRF52840 DK comes with an external flash memory that can be used for the secondary slot and be accessed using the QSPI NOR flash memory driver.
The following steps show how to configure an application for the nRF52840 DK:

1. Add the following configuration options to the ``prj.conf`` file associated with the MCUboot child image:

   .. code-block:: console

      CONFIG_NORDIC_QSPI_NOR=y
      CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE=4096
      CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE=4
      CONFIG_MULTITHREADING=y
      CONFIG_BOOT_MAX_IMG_SECTORS=256
      CONFIG_PM_EXTERNAL_FLASH=y
      CONFIG_PM_EXTERNAL_FLASH_DEV_NAME="MX25R64"
      CONFIG_PM_EXTERNAL_FLASH_SIZE=0xf4000
      CONFIG_PM_EXTERNAL_FLASH_BASE=0

   These options enable the QSPI NOR flash memory driver, multi-threading (required by the flash memory driver), and the external flash memory of the nRF52840 DK.

#. Update the :file:`ncs/bootloader/mcuboot/boot/zephyr/pm.yml` file as mentioned above:

   .. code-block:: yaml

      mcuboot_secondary:
          region: external_flash
          size: CONFIG_PM_EXTERNAL_FLASH_SIZE

#. Add the following configuration options to the :file:`prj.conf` file in your application directory:

   .. code-block:: console

      CONFIG_NORDIC_QSPI_NOR=y
      CONFIG_NORDIC_QSPI_NOR_FLASH_LAYOUT_PAGE_SIZE=4096
      CONFIG_NORDIC_QSPI_NOR_STACK_WRITE_BUFFER_SIZE=4
      CONFIG_PM_EXTERNAL_FLASH=y
      CONFIG_PM_EXTERNAL_FLASH_DEV_NAME="MX25R64"
      CONFIG_PM_EXTERNAL_FLASH_SIZE=0xf4000
      CONFIG_PM_EXTERNAL_FLASH_BASE=0

   These options enable the QSPI NOR flash memory driver and the external flash memory of the nRF52840 DK.
   Multi-threading is enabled by default, so you do not need to enable it again.
