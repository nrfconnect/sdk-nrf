.. _nrf_desktop_memory_layout:

nRF Desktop: Memory layout
##########################

.. contents::
   :local:
   :depth: 2

You must define the memory layout as a part of the application configuration for a given board.
The set of required partitions differs depending on the configuration:

* There must be at least one partition where the code is stored.
* There must be one partition for storing :ref:`zephyr:settings_api`.
* If the bootloader is enabled, it adds more partitions to the set.
* When using an SoC with multiple cores, the firmware for additional cores adds more partitions to the set.
  For example, the network core of the nRF53 SoC uses the :ref:`ipc_radio` image, which allows to use the core for Bluetooth LE communication.

.. important::
   Before updating the firmware, make sure that the data stored in the settings partition is compatible with the new firmware.
   If it is incompatible, erase the settings area before using the new firmware.

The memory layout is defined through one of the following methods:

* `Memory layout in DTS`_
* `Memory layout in Partition Manager`_

By default, a Zephyr-based application defines the memory layout in the DTS.
If enabled, the :ref:`partition_manager` defines a new memory layout that is used instead of the memory layout defined in the DTS.
You can use the :kconfig:option:`CONFIG_PARTITION_MANAGER_ENABLED` Kconfig option value to check whether the Partition Manager is enabled in the current build.
The option is automatically selected as part of the :ref:`ug_multi_image` feature to build the application with more than one image.
Enabling the :ref:`nrf_desktop_bluetooth_guide_fast_pair` also results in using the Partition Manager.
To store the Fast Pair Provisioning data, the Fast Pair integration in the |NCS| uses partition defined by the Partition Manager.

Memory layout in DTS
********************

If you rely on a non-volatile memory layout described in DTS files, define the ``partitions`` child node under the DTS node that represents the non-volatile memory.
For example, the nRF52 Series devices use non-volatile flash memory represented by the flash device node (``&flash0``).
Make sure to also update the DTS chosen nodes, which represent the code partition (``zephyr,code-partition``) and flash (``zephyr,flash``), if needed.

If you wish to change the default memory layout of the board without editing the board-specific files, edit the DTS overlay file.
The nRF Desktop application automatically adds the :file:`dts.overlay` file if it is present in the project's board configuration directory.
For more details, see the :ref:`nrf_desktop_board_configuration` section.

.. important::
   By default, Zephyr does not use the code partition defined in the DTS files.
   It is only used if the :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option is enabled.
   If this option is disabled, the code is loaded at the offset defined by the :kconfig:option:`CONFIG_FLASH_LOAD_OFFSET` Kconfig option.
   In that case, the code spawns for :kconfig:option:`CONFIG_FLASH_LOAD_SIZE` (or for the whole remaining chosen ``zephyr,flash`` memory if the load size is set to ``0``).
   The settings memory partition definition is still used by the firmware even if the :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option is disabled.

For more information about how to configure the non-volatile memory layout in the DTS files, see :ref:`zephyr:flash_map_api`.

Memory layout in Partition Manager
**********************************

When the :kconfig:option:`CONFIG_PARTITION_MANAGER_ENABLED` Kconfig option is enabled, the nRF Desktop application uses the Partition Manager for the memory layout configuration.
The nRF Desktop configurations use static configurations of partitions to ensure that the partition layout does not change between builds.

Add the :file:`pm_static_${FILE_SUFFIX}.yml` file to the project's board configuration directory to define the static Partition Manager configuration for given board and build type.
For example, to define the static partition layout for the ``nrf52840dk/nrf52840`` board and ``release`` build type, you would need to add the :file:`pm_static_release.yml` file into the :file:`applicatons/nrf_desktop/configuration/nrf52840dk_nrf52840` directory.

Take into account the following points:

* For the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`, you must define the secondary image partition.
  This is because the update image is stored on the secondary image partition while the device is running firmware from the primary partition.
  For this reason, the feature is not available for devices with smaller non-volatile memory size, because the size of the required non-volatile memory is essentially doubled.
  The devices with smaller non-volatile memory size can use either USB serial recovery or the MCUboot bootloader with the secondary image partition located on an external non-volatile memory.
* When you use :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`, you do not need the secondary image partition.
  The firmware image is overwritten by the bootloader.

For an example of configuration, see the static partition maps defined for the existing configuration that uses a given DFU method.
For more information about how to configure the non-volatile memory layout using the Partition Manager, see :ref:`partition_manager`.

.. _nrf_desktop_pm_external_flash:

External flash configuration
============================

The Partition Manager supports partitions in external flash.

Enabling external flash can be useful especially for memory-limited devices.
For example, the MCUboot can use it as a secondary image partition for the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`.
The MCUboot moves the image data from the secondary image partition to the primary image partition before booting the new firmware.
To use external flash for the secondary image partition, in addition to defining the proper static Partition Manager configuration, you must enable the ``SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY`` Kconfig option in the sysbuild configuration.

For an example of the nRF Desktop application configuration that uses an external flash, see the ``mcuboot_qspi`` configuration of the nRF52840 Development Kit (DK).
This configuration uses the ``MX25R64`` external flash that is part of the development kit.

For detailed information, see the :ref:`partition_manager` documentation.
