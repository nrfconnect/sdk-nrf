.. _qspi_xip_split_image:

QSPI XIP split image
####################

.. contents::
   :local:
   :depth: 2

QSPI XIP split image is a way of gaining more flash storage space for applications by splitting applications up into code that runs on the internal flash and code that runs on a supported external flash chip over QSPI using eXecute-In-Place (XIP). This is supported on the nRF52840 and nRF5340 SoCs.

.. warning::
    On the nRF52840, interrupts must not be relocated to QSPI XIP flash, doing so may lock up or brick the device by making the debug access port inaccessible

These split images are supported in MCUboot which allows for updating them over-the-air. A sample application is provided for the nrf5340dk and thingy53 which demonstrates this features: :ref:`smp_svr_ext_xip`.

Requirements
------------

Thw following is required to make use of this feature:

* nRF52840 or nRF5340-based design in HWMv2 (hardware model version 2)
* External QSPI flash chip with supported commands connected to QSPI pins
* QSPI flash chip in always-on mode (i.e. no DPM or low power modes)
* MCUboot in swap using move mode, upgrade only mode or directXIP mode
* Static partition manager file
* Sysbuild

.. caution::
    At present, MCUboot images cannot be linked together, this means that both parts of the firmware update must be loaded properly before an update is initiated, if one part fails to update then the image on the device will be unbootable and will likely fault

Implementation
--------------

Network core image updates
**************************

This feature can be used and will retain support for network core image updates on nRF5340 devices if MCUboot is configured for swap using move mode or upgrade only mode.

Partition manager
*****************

A static partition manager file is required to set up the partitions that will be used on the device, the following 3 files provide sample partition manager layouts for an nRF5340:

.. tabs::

    .. group-tab:: Swap using move with network core support

        .. literalinclude:: ../../../samples/nrf5340/extxip_smp_svr/pm_static.yml
            :language: yaml

    .. group-tab:: Swap using move without network core support

        .. literalinclude:: ../../../samples/nrf5340/extxip_smp_svr/pm_static.yml
            :language: yaml

        TODO: ../../../samples/nrf5340/extxip_smp_svr/pm_static_no_network_core.yml

    .. group-tab:: DirectXIP without network core support

        .. literalinclude:: ../../../samples/nrf5340/extxip_smp_svr/pm_static.yml
            :language: yaml

        TODO: ../../../samples/nrf5340/extxip_smp_svr/pm_static_no_network_core_directxip.yml

A board overlay file must be created to set the chosen partition manager external flash device:

        .. literalinclude:: ../../../samples/nrf5340/extxip_smp_svr/boards/nrf5340dk_nrf5340_cpuapp.overlay
            :language: dts

This file can be a board overlay in the ``boards`` folder of the application, or named ``app.overlay`` in the application folder to apply to all board targets.

Sysbuild configuration
**********************

Sysbuild must be configured to enable the required Kconfig options for this functionality, a ``sysbuild.conf`` file should be created in the application directory which has the following options, depending upon required functionality:

.. tabs::

    .. group-tab:: Swap using move with network core support

        .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y
            SB_CONFIG_NETCORE_APP_UPDATE=y
            SB_CONFIG_SECURE_BOOT_NETCORE=y
            SB_CONFIG_QSPI_XIP_SUPPORT=y

            # This will enable the hci_ipc image for the network core, change to the desired image
            SB_CONFIG_NETCORE_HCI_IPC=y

    .. group-tab:: Swap using move without network core support

        .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y
            SB_CONFIG_QSPI_XIP_SUPPORT=y

    .. group-tab:: DirectXIP without network core support

        .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP=y
            SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY=y
            SB_CONFIG_QSPI_XIP_SUPPORT=y

In addition, the devicetree overlay file for enabling the external flash device for partition manager must also be applied to MCUboot, to do this, create a ``sysbuild.cmake`` file with the following (this assumes you created the overlay file as ``app.overlay``, adjust as needed if it was created as a board overlay instead):

.. tabs::

    .. group-tab:: With networe core support

        .. code-block:: cmake

            list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.overlay")

    .. group-tab:: Without networe core support

        .. code-block:: cmake

            list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.overlay")
            set(mcuboot_CONFIG_BOOT_IMAGE_ACCESS_HOOKS n)

Relocating code to QSPI flash
*****************************

Relocating code to run from the QSPI flash can be done inside the applications ``CMakeLists.txt`` file using the ``zephyr_code_relocate`` function, this can be used to relocate files and libraries

.. tabs::

    .. group-tab:: Relocating files

        .. code-block:: cmake

            zephyr_code_relocate(FILES src/complex_sensor_calculcation_code.c LOCATION EXTFLASH_TEXT NOCOPY)
            zephyr_code_relocate(FILES src/complex_sensor_calculcation_code.c LOCATION RAM_DATA)

        *This assumes that the application has a ``src/complex_sensor_calculcation_code.c`` file*

    .. group-tab:: Relocating libraries

        .. code-block:: cmake

            zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION EXTFLASH_TEXT NOCOPY)
            zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION RAM_DATA)

        *The library name comes from the zephyr code*

TODO

QSPI flash init priority
************************

The init priority of QSPI flash must be set so that it is configured before any code on the device runs, otherwise the application operation is undefined. There is no in-built check for this so the init priority must be set manually based upon the code that has been relocated. If for example, the shell subsystem was relocated then the init priority of any of that subsystem must be higher than the init priority of QSPI, :kconfig:option:`CONFIG_SHELL_BACKEND_SERIAL_INIT_PRIORITY` must be greater than :kconfig:option:`CONFIG_NORDIC_QSPI_NOR_INIT_PRIORITY`.

.. note::
    Not all libraries or subsystems can be relocated, any code that runs early in the boot process or uses ``SYS_INIT()`` with a non-configurable init priority cannot be relocated.
    Library and subsystem code needs to be manually checked before relocating it.

Flashing
********

Flashing of the application is supported using ``west flash`` as normal, however this will flash the firmware using the default nrfjprog configuration which, for QSPI, is PP4IO mode. If you are using an a different SPI mode on the QSPI interface such as DSPI then a custom ``Qspi.ini`` file must be used, an example for the thingy53 which only supports DSPI and PP is shown below:

.. literalinclude:: ../../../samples/nrf5340/extxip_smp_svr/Qspi_thingy53.ini

To use this file when flashing, the ``--qspiini`` argument can be used when flashing with the path to the file, for example:

.. code-block:: bash

    west flash --qspiini <my.ini>

TODO: requires PR https://github.com/zephyrproject-rtos/zephyr/pull/75731

Firmware updates
****************

The build system will automatically generate the signed update files which can be loaded to a device to run the newer version of the application. The ``dfu_application.zip`` file also contains these files which can be directly used with the nRF Connect mobile applications for Android and iOS to load the firmware updates over Bluetooth.

File descriptions
*****************

The following files are generated when building a QSPI XIP split image application:

+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
| File name and location                                    | Description                                                                                |
+===========================================================+============================================================================================+
|``<application>/zephyr/<kernel_name>.hex``                 | Initial hex output file, unsigned, containing internal and external QSPI flash data        |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>_internal.hex``        | Initial hex output file, unsigned, containing internal flash data                          |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>_external.hex``        | Initial hex output file, unsigned, containing external QSPI flash data                     |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>_internal.signed.hex`` | Signed internal flash data hex file                                                        |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>_external.signed.hex`` | Signed external QSPI flash data hex file                                                   |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>.signed.hex``          | Signed internal and external QSPI flash data hex file                                      |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>_internal.signed.bin`` | Signed internal flash data binary file (for firmware updates)                              |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``<application>/zephyr/<kernel_name>_external.signed.bin`` | Signed external QSPI flash data binary file (for firmware updates)                         |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+
|``merged.hex``                                             | Merged hex file containing all images, includes both internal and external QSPI flash data |
+-----------------------------------------------------------+--------------------------------------------------------------------------------------------+

TODO: directXIP

Where ``<application>`` is the name of the application and ``<kernel_name>`` is the value of :kconfig:option:`CONFIG_KERNEL_BIN_NAME`.
