.. _ug_nrf5340_multi_image_dfu:

Simultaneous multi-image DFU with nRF5340 DK
############################################

The simultaneous update of multiple images is available for testing since |NCS| v1.7.0.
It allows the updating of both the application core and the network core in one go.

To enable the simultaneous update of multiple images in the MCUboot, set the following options:

* :kconfig:option:`CONFIG_BOOT_UPGRADE_ONLY` - The simultaneous update of multiple images does not support network core image reversion, so you need to disable application image reversion.
* :kconfig:option:`CONFIG_PCD_APP` - Enable commands exchange with the network core.
* :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER` - Enable support for multiple update partitions by setting this option to ``2``.

As described in :ref:`multi-image build guide <ug_multi_image_variables>`, make sure to add the child image prefix to the name of Kconfig options that are used when building MCUboot as a child image (for example, ``child_image_CONFIG_PCD_APP``).
If not changed, then the default child image prefix for MCUboot is ``mcuboot_`` (for example, ``mcuboot_CONFIG_PCD_APP``).

.. note::

   The application core can be reverted, but doing so breaks the network core upon reversal, as the reversion process fills the network core with the content currently in the RAM that PCD uses.
   To enable this, define the :kconfig:option:`CONFIG_USE_NRF53_MULTI_IMAGE_WITHOUT_UPGRADE_ONLY` Kconfig option in the project-level Kconfig file.
   When this option is defined, you can enable it by setting :kconfig:option:`CONFIG_USE_NRF53_MULTI_IMAGE_WITHOUT_UPGRADE_ONLY`.

The :kconfig:option:`CONFIG_NRF53_MULTI_IMAGE_UPDATE` option selects this feature by default if these options and all its other dependencies are asserted.

To enable the simultaneous update of multiple images in the application, in addition to enabling the MCUboot support, set the following options:

* :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER` - Enable support for multiple update partitions by setting this option to ``2``.

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
