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

Memory partitions (devicetree)
==============================

Partition layout is defined in devicetree for each sysbuild image.
The application image and MCUboot sysbuild images must use the same slot addresses and devicetree node labels; network-core |NSIB| uses ``sysbuild/b0n/`` (or equivalent) overlays on ``&flash1``.
See :ref:`bootloader_partitioning` for general partitioning principles and :ref:`migration_partitions` if you are moving away from Partition Manager.

For simultaneous multi-image DFU on nRF5340, the application-core map must include at least:

* ``slot0_partition`` and ``slot1_partition`` — primary and secondary slots for the application core firmware.
* ``slot2_partition`` and ``slot3_partition`` — primary and secondary slots for the network core firmware.
  MCUboot uses these devicetree node labels for the second image when :kconfig:option:`SB_CONFIG_MCUBOOT_NRF53_MULTI_IMAGE_UPDATE` is enabled.

The network-core primary slot (``slot2_partition``) for the networking firmware is often provided through the RAM flash simulator devicetree overlay used for PCD (see :kconfig:option:`CONFIG_PCD_APP` and :file:`nrf/modules/mcuboot/flash_sim.overlay`).
The secondary network-core slot may be placed in internal or external flash depending on your board overlay; see :file:`nrf/samples/dfu/smp_svr/boards/nrf5340dk_nrf5340_cpuapp_nrf5340_bt.overlay` and :ref:`ug_bootloader_external_flash`.

.. note::

   The application core does not have direct access to the network core flash memory;
   the update image is passed indirectly using RAM.
   Due to the above, the network-core primary slot is backed by a RAM partition for the application core (flash simulator / ``slot2_partition``), not by network-core internal flash alone.
   To enable providing such a region on the device, see :kconfig:option:`CONFIG_FLASH_SIMULATOR`.

For QSPI XIP split images on nRF5340, see :ref:`qspi_xip_split_image` and :ref:`smp_svr_ext_xip`.

Samples and applications built for Thingy:53 enable simultaneous update of multiple images by default.
To learn more about Thingy:53, see :ref:`ug_thingy53`.
