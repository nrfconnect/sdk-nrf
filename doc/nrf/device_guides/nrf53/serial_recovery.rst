.. _ug_nrf5340_serial_recovery:

MCUboot's serial recovery of the networking core image
######################################################

.. contents::
   :local:
   :depth: 2


In addition to the recovery of the application core image, also the networking core image can be recovered.
When you build MCUboot for the nRF5340 DK or the Thingy:53, you can use this feature with one of the following options:

* :kconfig:option:`CONFIG_NRF53_MULTI_IMAGE_UPDATE` - Simultaneous multi-image DFU.
* :kconfig:option:`CONFIG_NRF53_RECOVERY_NETWORK_CORE` - Serial recovery for the network core in the one image pair mode, where :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER` is set to ``== 1``.

To upload the networking image, use the following command::

     ./mcumgr image upload <build_dir_path>/zephyr/net_core_app_update.bin -e -n 3 -c serial_conn

``serial_conn`` is the serial connection configuration.
For more information on MCUmgr image management, see :ref:`zephyr:image_mgmt`.

To enable the serial recovery of the network core while the multi-image update is not enabled in the MCUboot, set the following options:

* select :kconfig:option:`CONFIG_BOOT_IMAGE_ACCESS_HOOK`
* select :kconfig:option:`CONFIG_FLASH_SIMULATOR`
* select :kconfig:option:`CONFIG_FLASH_SIMULATOR_DOUBLE_WRITES`
* disable :kconfig:option:`CONFIG_FLASH_SIMULATOR_STATS`
* select :kconfig:option:`CONFIG_MCUBOOT_SERIAL_DIRECT_IMAGE_UPLOAD`
* select :kconfig:option:`CONFIG_NRF53_RECOVERY_NETWORK_CORE`

Additionally, define and include the following memory partitions:

* ``mcuboot_primary`` and ``mcuboot_secondary`` - Partitions for the application core image slots.
* ``mcuboot_primary_1`` - Partition for the network core image slot.
* ``pcd_sram`` - Partition used for command exchange between the application core and the network core (see :kconfig:option:`CONFIG_PCD_APP`).

.. note::
   When using MCUboot with the :kconfig:option:`CONFIG_NRF53_RECOVERY_NETWORK_CORE` option enabled, the application core does not have direct access to the network core flash memory.
   Due to this, ``mcuboot_primary_1`` must be used as the RAM partition mediator.

Container for firmware update binaries
**************************************

The build system will automatically place both the application core and the network core update binaries (:file:`app_update.bin` and :file:`net_core_app_update.bin`) into a container package named :file:`dfu_application.zip`.
This container package can be used by update tools to pass both images during the simultaneous update of multiple images.
