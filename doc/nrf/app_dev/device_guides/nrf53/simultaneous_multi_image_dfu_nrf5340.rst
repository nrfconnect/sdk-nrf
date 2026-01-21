.. _ug_nrf5340_multi_image_dfu:

Simultaneous multi-image DFU with nRF5340 DK
############################################

The simultaneous update of multiple images is available for testing since |NCS| v1.7.0.
It allows the updating of both the application core and the network core in one go.

To enable the simultaneous update of multiple images by MCUboot, set the following options:

* :kconfig:option:`SB_CONFIG_SECURE_BOOT_NETCORE` - Enables |NSIB| for the network core.
* :kconfig:option:`SB_CONFIG_NETCORE_APP_UPDATE` - Enables firmware updates for the network core.
* :kconfig:option:`SB_CONFIG_MCUBOOT_NRF53_MULTI_IMAGE_UPDATE` - Performs network core updates in a single operation.

.. note::

   The application core can be reverted, but doing so breaks the network core upon reversal, as the reversion process fills the network core with the content currently in the RAM that PCD uses.

Additionally, the memory partitions must be defined and include:

* ``mcuboot_primary`` and ``mcuboot_secondary`` partitions for the application core image slots.
* ``mcuboot_primary_1`` and ``mcuboot_secondary_1`` partitions for the network core image slots.
* ``pcd_sram`` partition used for command exchange between the application core and the network core (see :kconfig:option:`CONFIG_PCD_APP`).

.. note::

   The application core does not have direct access to the network core flash memory.
   The update image is passed indirectly using RAM.
   Because of this, the ``mcuboot_primary_1`` must be stored in ``ram_flash`` region.
   To enable providing such region on the device, see :kconfig:option:`CONFIG_FLASH_SIMULATOR`.

Samples and applications built for Thingy:53 enable simultaneous update of multiple images by default.
To learn more about Thingy:53, see :ref:`ug_thingy53`.
