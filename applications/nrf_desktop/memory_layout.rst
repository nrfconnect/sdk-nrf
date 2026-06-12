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
You can use use the :kconfig:option:`SB_CONFIG_PARTITION_MANAGER` sysbuild Kconfig option to enable Partition Manager in the current build.

.. note::
   Starting from NCS v3.4.0, all of the nRF Desktop application configurations were migrated from Partition Manager to DTS memory layout because of Partition Manager deprecation.
   The Partition Manager should not be used for new products.

Memory layout and bootloaders
*****************************

The used memory layout depends on selected bootloader and DFU method:

* For the :ref:`background firmware upgrade <nrf_desktop_bootloader_background_dfu>`, you must define the secondary image partition.
  This is because the update image is stored on the secondary image partition while the device is running firmware from the primary partition.
  For this reason, the feature may be problematic for devices with smaller non-volatile memory size, because the size of the required internal non-volatile memory is essentially doubled.
  The devices with smaller non-volatile memory size can use either USB serial recovery or the MCUboot bootloader in swap mode with the secondary image partition located on an external non-volatile memory.
  In this mode, the MCUboot moves the image data from the secondary image partition on the external non-volatile memory to the primary image partition before booting the new firmware.
* When you use :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`, you do not need the secondary image partition.
  The firmware image is overwritten by the bootloader.

For more information about available bootloaders and their modes, see the :ref:`nrf_desktop_bootloader` documentation.

Memory layout in DTS
********************

If you rely on a non-volatile memory layout described in DTS files, define the ``partitions`` child node under the DTS node that represents the non-volatile memory.
For example, the nRF52 Series devices use internal non-volatile flash memory represented by the ``&flash0`` DTS node and application core of nRF54L Series devices uses internal non-volatile RRAM memory represented by the ``&cpuapp_rram`` DTS node.
Make sure to also update the DTS chosen nodes, which represent the code partition (``zephyr,code-partition``) and flash (``zephyr,flash``), if needed.

If you wish to change the default memory layout for the board without editing the board-specific files, use the DTS overlay file.
nRF Desktop application configurations define the non-volatile memory layout in :file:`memory_map.dtsi` files (for example, :file:`memory_map.dtsi` or :file:`memory_map_release.dtsi`).
These files are placed in the project's board configuration directory and are included by the DTS overlay file of the given build type (for example, :file:`app.overlay` or :file:`app_release.overlay`).
Keep in mind that DTS configuration is defined separately for every sysbuild image.
Because of that, the new memory map needs to be included in all of the built sysbuild images that use the memory map (for example bootloader).
The set of partitions that needs to be specified depends on the selected bootloader (and its mode) too.
See the nRF Desktop board configuration directories for examples of the DTS-based memory layout definitions.
For more details regarding application configurations, see the :ref:`nrf_desktop_board_configuration` documentation.

.. important::
   By default, Zephyr does not use the code partition defined in the DTS files.
   It is only used if the :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option is enabled.
   If this option is disabled, the code is loaded at the offset defined by the :kconfig:option:`CONFIG_FLASH_LOAD_OFFSET` Kconfig option.
   In that case, the code spawns for :kconfig:option:`CONFIG_FLASH_LOAD_SIZE` (or for the whole remaining chosen ``zephyr,flash`` memory if the load size is set to ``0``).
   The settings memory partition definition is still used by the firmware even if the :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option is disabled.
   nRF Desktop application explicitly enables the :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` Kconfig option to use memory layout defined in DTS.

For more information about how to configure the non-volatile memory layout in the DTS files, see :ref:`zephyr:flash_map_api`.

.. _nrf_desktop_external_flash:

External flash configuration
============================

Devices with smaller non-volatile memory size can use MCUboot bootloader in swap mode with secondary image partition located on an external non-volatile memory.
For an example of the nRF Desktop application configuration that uses an external flash, see the ``mcuboot_qspi`` configuration of the nRF52840 DK.
This configuration uses the ``MX25R64`` external flash that is part of the development kit.

The memory map is defined in DTS (see :file:`memory_map_mcuboot_qspi.dtsi`), with the ``slot1_partition`` placed under the ``mx25r64`` node.

Memory layout in Partition Manager
**********************************

.. include:: /includes/pm_deprecation.txt

When the :kconfig:option:`SB_CONFIG_PARTITION_MANAGER` sysbuild Kconfig option is enabled, the nRF Desktop application uses the Partition Manager for the memory layout configuration.
The nRF Desktop configurations use static configurations of partitions to ensure that the partition layout does not change between builds.

Add the :file:`pm_static_${FILE_SUFFIX}.yml` file to the project's board configuration directory to define the static Partition Manager configuration for given board and build type.
For example, to define the static partition layout for the ``nrf52840dk/nrf52840`` board and ``release`` build type, you would need to add the :file:`pm_static_release.yml` file into the :file:`applicatons/nrf_desktop/configuration/nrf52840dk_nrf52840` directory.

For an example of configuration, see the static partition maps defined for the existing configuration that uses a given DFU method.
For more information about how to configure the non-volatile memory layout using the Partition Manager, see :ref:`partition_manager`.

External flash configuration
============================

Devices with smaller non-volatile memory size can use MCUboot bootloader in swap mode with secondary image partition located on an external non-volatile memory.
For an example of the nRF Desktop application configuration that uses an external flash, see the ``mcuboot_qspi`` configuration of the nRF52840 DK.
This configuration uses the ``MX25R64`` external flash that is part of the development kit.

Up to the |NCS| v3.3.0 release, Partition Manager was used to control the memory partition layout for this application configuration.
The :kconfig:option:`SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY` sysbuild Kconfig option was used next to the proper static Partition Manager configuration to place the MCUboot secondary image slot in external flash.
For detailed information, see the :ref:`partition_manager` documentation.
