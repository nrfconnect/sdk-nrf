.. _ug_bootloader_external_flash:

Using external flash memory partitions
######################################

When using MCUboot, you can store the storage partition for the secondary slot in the external flash memory, using a driver for the external flash memory that supports the following features:

* Single-byte read and write.
* Writing data from the internal flash memory to the external flash memory.

The QSPI NOR flash memory driver supports these features, and it can access the QSPI external flash memory of the nRF52840 DK and nRF5340 DK.

See the test in :file:`tests/modules/mcuboot/external_flash` for reference.
This test passes both devicetree overlay files and Kconfig fragments to the MCUboot child image through its :file:`child_image` folder.
See also :ref:`ug_multi_image_variables` for more details on how to pass configuration files to a child image.
