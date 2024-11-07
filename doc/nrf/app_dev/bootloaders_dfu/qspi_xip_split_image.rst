.. _qspi_xip_split_image:

Using QSPI XIP split image
##########################

.. contents::
   :local:
   :depth: 2

The QSPI XIP split image feature lets you gain more flash storage space for applications by splitting application code into two parts:

* Code that runs on the internal flash memory.
* Code that runs on supported external flash memory over the Quad Serial Peripheral Interface (QSPI) using Execute in Place (XIP).

This feature is supported on nRF52840 and nRF5340.

.. warning::
    On the nRF52840, do not relocate interrupts to the QSPI XIP flash.
    Doing so can lock up or brick the device by making the debug access port inaccessible.

The QSPI XIP split images are supported in :doc:`MCUboot <mcuboot:index-ncs>`, which allows for updating them over-the-air.

For the nRF5340 DK and Nordic Thingy:53, you can also check out the :ref:`SMP Server with external XIP <smp_svr_ext_xip>` sample, which demonstrates this feature.

Requirements
************

To use this feature, meet the following requirements:

* Board based on nRF52840 or nRF5340, with file structure compatible with :ref:`Hardware model v2 (HWMv2) <hwmv1_to_v2_migration>`
* External QSPI flash chip with supported commands connected to QSPI pins
* QSPI flash chip in always-on mode (meaning, no DPM or low-power modes)
* :doc:`MCUboot configuration <mcuboot:design>` in the Swap using move mode (``MCUBOOT_SWAP_USING_MOVE``), the Upgrade only mode (``MCUBOOT_OVERWRITE_ONLY``), or in the direct-XIP mode
* :ref:`Static partition manager file <ug_pm_static>`
* Linker file with QSPI XIP flash offset and size
* :ref:`configuration_system_overview_sysbuild`

.. caution::
    Currently, MCUboot images cannot be linked together.
    This means that both parts of the firmware update must be loaded properly before an update is initiated.
    If one part fails to update, the image on the device will be unbootable and that will likely brick the device.

Network core image updates
**************************

The QSPI XIP split image feature supports network core image updates on nRF5340 devices if MCUboot is configured for using either the swap-using-move or the upgrade-only mode.

.. _qspi_xip_split_image_pm_static_files:

.. rst-class:: numbered-step

Create the Partition Manager static files
*****************************************

Create the following files:

* A static :ref:`partition_manager` file to set up the partitions that will be used on the device.
  The following three files from the :ref:`SMP Server with external XIP <smp_svr_ext_xip>` sample provide an example of the Partition Manager layouts for nRF5340:

  .. tabs::

      .. group-tab:: Swap using move with network core support
          .. literalinclude:: ../../../../samples/nrf5340/extxip_smp_svr/pm_static.yml
              :language: yaml

      .. group-tab:: Swap using move without network core support

          .. literalinclude:: ../../../../samples/nrf5340/extxip_smp_svr/pm_static_no_network_core.yml
              :language: yaml

      .. group-tab:: Direct-XIP without network core support

          .. literalinclude:: ../../../../samples/nrf5340/extxip_smp_svr/pm_static_no_network_core_directxip.yml
              :language: yaml

* A board overlay file to set the chosen Partition Manager external flash device:

  .. literalinclude:: ../../../../samples/nrf5340/extxip_smp_svr/boards/nrf5340dk_nrf5340_cpuapp.overlay
     :language: dts

  You can either place this file as a board overlay in the :file:`boards` folder of the application or name it :file:`app.overlay` and place it in the application folder so that it applies to all board targets.

.. rst-class:: numbered-step

Create a linker file
********************

Create a linker file that has the same information as the static Partition Manager file for the external QSPI flash device.
The following code example from the :ref:`SMP Server with external XIP <smp_svr_ext_xip>` sample shows how to set up a linker file for direct-XIP mode, with the direct-XIP secondary image configuration in the ``CONFIG_NCS_IS_VARIANT_IMAGE`` section.
You can omit this section if you are using other MCUboot operating modes.

Place this file in the application directory with a name similar to :file:`linker_arm_extxip.ld`.

.. literalinclude:: ../../../../samples/nrf5340/extxip_smp_svr/linker_arm_extxip.ld

.. rst-class:: numbered-step

Set additional Kconfig options
******************************

Set additional Kconfig option in the application's :file:`prj.conf` to allow it to execute code from XIP:

.. code-block:: cfg

    # Update the filename here if you have given it a different name
    CONFIG_CUSTOM_LINKER_SCRIPT="linker_arm_extxip.ld"

.. rst-class:: numbered-step

Configure sysbuild
******************

Configure :ref:`configuration_system_overview_sysbuild` to enable the required Kconfig options for this functionality:

1. Create a :file:`sysbuild.conf` file in the application directory withs the following options, depending on the required functionality:

   .. tabs::

      .. group-tab:: Swap using move with network core support

         .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y
            SB_CONFIG_NETCORE_APP_UPDATE=y
            SB_CONFIG_SECURE_BOOT_NETCORE=y
            SB_CONFIG_QSPI_XIP_SPLIT_IMAGE=y

            # This will enable the hci_ipc image for the network core, change to the desired image
            SB_CONFIG_NETCORE_HCI_IPC=y

      .. group-tab:: Swap using move without network core support

         .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y
            SB_CONFIG_QSPI_XIP_SPLIT_IMAGE=y

      .. group-tab:: Direct-XIP without network core support

         .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP=y
            SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y
            SB_CONFIG_QSPI_XIP_SPLIT_IMAGE=y

#. Create a :file:`sysbuild.cmake` file.
   This file applies the devicetree overlay file for enabling the external flash device for the Partition Manager (created in :ref:`qspi_xip_split_image_pm_static_files`) to MCUboot.
   The following examples assume you created the :file:`app.overlay` file valid for all board targets; adjust as needed if you created as a board overlay instead.

   .. tabs::

      .. group-tab:: With network core support

         .. code-block:: cmake

            list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.overlay")

      .. group-tab:: Without networe core support

         .. code-block:: cmake

            list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.overlay")
            set(mcuboot_CONFIG_BOOT_IMAGE_ACCESS_HOOKS n)

.. rst-class:: numbered-step

Relocate code to QSPI flash
***************************

Relocate the code to run from the QSPI flash inside the applications :file:`CMakeLists.txt` file using the ``zephyr_code_relocate`` function.
This can be used to relocate files and libraries.

.. tabs::

   .. group-tab:: Relocating files

      .. code-block:: cmake

         zephyr_code_relocate(FILES src/complex_sensor_calculcation_code.c LOCATION EXTFLASH_TEXT NOCOPY)
         zephyr_code_relocate(FILES src/complex_sensor_calculcation_code.c LOCATION RAM_DATA)

      .. note::
          This assumes that the application has a :file:`src/complex_sensor_calculcation_code.c`` file.

   .. group-tab:: Relocating libraries

      .. code-block:: cmake

         zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION EXTFLASH_TEXT NOCOPY)
         zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION RAM_DATA)

      .. note::
          The library name comes from the Zephyr code.

.. rst-class:: numbered-step

Set QSPI flash initialization priority
**************************************

Set the initialization priority of the QSPI flash so that it is configured before any code on the device runs, otherwise the application operation is going to be undefined.

There is no built-in check for this setting.
Therefore, you must manually set the priority based on the relocated code.
For example, if you relocate the shell subsystem, its initialization priority must be higher than the priority of QSPI, and the value of :kconfig:option:`CONFIG_SHELL_BACKEND_SERIAL_INIT_PRIORITY` must be greater than :kconfig:option:`CONFIG_NORDIC_QSPI_NOR_INIT_PRIORITY`.

.. note::
    Not all libraries or subsystems can be relocated.
    Any code that runs early in the boot process or uses ``SYS_INIT()`` with a non-configurable init priority cannot be relocated.
    Library and subsystem code needs to be manually checked before relocating it.

Programming with the QSPI XIP split image
*****************************************

Programming of the application is supported using the :ref:`standard procedure <programming>`.
The standard procedure will program the firmware using the default nrfjprog configuration which, for QSPI, is PP4IO mode.

.. note::
      |nrfjprog_deprecation_note|

Programming using a different SPI mode
======================================

If you are using a different SPI mode on the QSPI interface, such as DSPI, you must use a custom :file:`Qspi.ini` file.
The following is an example for the Thingy:53, which supports DSPI and PP:

.. literalinclude:: ../../../../samples/nrf5340/extxip_smp_svr/Qspi_thingy53.ini

To use this file when programming, add the following lines to the application's :file:`CMakeLists.txt` file before the ``find_package()`` line:

.. code-block:: cmake

    macro(app_set_runner_args)
      # Replace with the filename of your ini file
      board_runner_args(nrfjprog "--qspiini=${CMAKE_CURRENT_SOURCE_DIR}/Qspi_thingy53.ini")
    endmacro()

This will enable programming the target board successfully when using ``west flash``.

Firmware updates
****************

The build system will automatically generate the signed update files that can be loaded to a device to run the newer version of the application.
The :file:`dfu_application.zip` file also contains these files and you can use the `nRF Connect for Mobile`_ application (available for Android and iOS) to upload the firmware over Bluetooth® Low Energy.

File descriptions
*****************

The following table lists the files generated when building a QSPI XIP split-image application.

In some file names, ``<application>`` is the name of the application and ``<kernel_name>`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.

+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
|                         File name and location                         |                                                                    Description                                                                     |
+========================================================================+====================================================================================================================================================+
| :file:`<application>/zephyr/<kernel_name>.hex`                         | Initial HEX output file, unsigned, containing internal and external QSPI flash data.                                                               |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.internal.hex`                | Initial HEX output file, unsigned, containing internal flash data.                                                                                 |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.external.hex`                | Initial HEX output file, unsigned, containing external QSPI flash data.                                                                            |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.internal.signed.hex`         | Signed internal flash data HEX file.                                                                                                               |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.external.signed.hex`         | Signed external QSPI flash data HEX file.                                                                                                          |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.signed.hex`                  | Signed internal and external QSPI flash data HEX file.                                                                                             |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.internal.signed.bin`         | Signed internal flash data binary file (for firmware updates).                                                                                     |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`<application>/zephyr/<kernel_name>.external.signed.bin`         | Signed external QSPI flash data binary file (for firmware updates).                                                                                |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.hex`                 | Initial HEX output file, unsigned, containing internal and external QSPI flash data for direct-XIP mode secondary slot image.                      |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.internal.hex`        | Initial HEX output file, unsigned, containing internal flash data for direct-XIP mode secondary slot image.                                        |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.external.hex`        | Initial HEX output file, unsigned, containing external QSPI flash data for direct-XIP mode secondary slot image.                                   |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.internal.signed.hex` | Signed internal flash data HEX file for direct-XIP mode secondary slot image.                                                                      |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.external.signed.hex` | Signed external QSPI flash data HEX file for direct-XIP mode secondary slot image.                                                                 |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.signed.hex`          | Signed internal and external QSPI flash data HEX file for direct-XIP mode secondary slot image.                                                    |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.internal.signed.bin` | Signed internal flash data binary file (for firmware updates) for direct-XIP mode secondary slot image.                                            |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`mcuboot_secondary_app/zephyr/<kernel_name>.external.signed.bin` | Signed external QSPI flash data binary file (for firmware updates) for direct-XIP mode secondary slot image.                                       |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`merged.hex`                                                     | Merged HEX file containing all images, includes both internal and external QSPI flash data for all images.                                         |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| :file:`dfu_application.zip`                                            | Created if ``SB_CONFIG_DFU_ZIP`` is set, will contain the internal and external QSPI flash FOTA update images if ``SB_CONFIG_DFU_ZIP_APP`` is set. |
+------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
