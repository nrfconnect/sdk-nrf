.. _child_parent_to_sysbuild_migration:

Migrating from multi-image builds to sysbuild
#############################################

:ref:`sysbuild` is a build system used in zephyr to configure, build, and flash multiple images as part of a single project.
It replaces the :ref:`child/parent system for multi-image builds <ug_multi_image>` in |NCS|.
As the previous system has been deprecated, you must update your existing multi-image build projects to support being built using sysbuild.

The following are the differences in how project configuration is performed in sysbuild compared to child/parent image configuration:

* Sysbuild controls which images are added to a build, instead of the main application.
* Sysbuild specifies the project configuration files for all images, which was previously done in the main application or child applications.
* Sysbuild controls the packaging of firmware components, instead of the main application.
* Sysbuild manages some software functionality of image builds, such as the configuration mode of an nRF70-series radio.
* Sysbuild runs partition manager code (if enabled) and distributes the output information to images.

This results in the CMake configuration step running more than once, as this information is only available after all images have been processed.

* Sysbuild output files have different names and locations (they are namespaced).
* Sysbuild introduces support for file suffixes, replacing the deprecated build type used by child/parent images.

The changes needed to convert a child/parent image project to a sysbuild project depend on the features used.
Review how :ref:`sysbuild` works to understand the basic usage and configuration methods, and how these differ from a child image build, before proceeding with project migration according to the guidelines listed in the following sections.

.. _child_parent_to_sysbuild_migration_sysbuild_configuration_file:

Sysbuild configuration files
============================

You can set sysbuild configuration for projects in the ``sysbuild.conf`` file in the project folder.
You can also add custom Kconfig values in the ``Kconfig.sysbuild`` file in the project folder, or use this file to set Kconfig defaults that depend on a board or other parameters when building a project.
Sysbuild Kconfig values in Kconfig fragment files have the ``SB_CONFIG_`` prefix.
The following example demonstrates how to use these files to set project configuration:

.. tabs::

    .. group-tab:: sysbuild.conf

        This example enables MCUboot, sets the key type to RSA, and specifies the overwrite-only mode for all boards:

        .. code-block:: cfg

            SB_CONFIG_BOOTLOADER_MCUBOOT=y
            SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY=y
            SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA=y

    .. group-tab:: Kconfig.sysbuild

        This example enables MCUboot, sets the key type to RSA, and specifies the overwrite-only mode for the application core of the nRF5340 DK:

        .. code-block:: kconfig

            if BOARD_NRF5340DK_NRF5340_CPUAPP

            choice BOOTLOADER
                    default BOOTLOADER_MCUBOOT
            endchoice

            if BOOTLOADER_MCUBOOT

            choice MCUBOOT_MODE
                    default MCUBOOT_MODE_OVERWRITE_ONLY
            endchoice

            choice BOOT_SIGNATURE_TYPE
                    default BOOT_SIGNATURE_TYPE_RSA
            endchoice

            endif # BOOTLOADER_MCUBOOT

           endif # BOARD_NRF5340DK_NRF5340_CPUAPP

Both approaches are used in |NCS| applications and samples depending on the required configuration.

.. note::

    Sysbuild has :ref:`file suffix support <sysbuild_file_suffixes>`, which means different files can be created and used depending on whether a file suffix is used for a build.

.. _child_parent_to_sysbuild_migration_network_core:

Network core
============

Sysbuild handles the selection of the network core image.
The following Kconfig options are available to include the desired image in the build or to set network core options:

+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Kconfig option                                          | Description                                                                                               |
+=========================================================+===========================================================================================================+
|               ``SB_CONFIG_NETCORE_EMPTY``               | Empty network core image: :ref:`nrf5340_empty_net_core`                                                   |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_HCI_IPC``             | Zephyr hci_ipc Bluetooth image: :zephyr:code-sample:`bluetooth_hci_ipc`                                   |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_RPC_HOST``            | |NCS| rpc_host Bluetooth image: :ref:`ble_rpc_host`                                                       |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_802154_RPMSG``        | Zephyr 802.15.4 image: :zephyr:code-sample:`nrf_ieee802154_rpmsg`                                         |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_MULTIPROTOCOL_RPMSG`` | |NCS| multiprotocol_rpmsg Bluetooth and 802.15.4 image: :ref:`multiprotocol-rpmsg-sample`                 |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_IPC_RADIO``           | |NCS| ipc_radio image: :ref:`ipc_radio`                                                                   |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_NONE``                | No network core image                                                                                     |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_NETCORE_APP_UPDATE``          | Will enable network core image update support in MCUboot (PCD)                                            |
+---------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+

If a project uses network-core functionality (for example, Bluetooth) in the main application but does not enable a network-core image in sysbuild, then no network-core image will be built, resulting in a non-working application.
Projects must be updated to select the correct network core image.

.. _child_parent_to_sysbuild_migration_mcuboot:

MCUboot
=======

Sysbuild handles MCUboot mode selection and key file configuration.
The following Kconfig options are available:

+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
| Kconfig option                                                            | Description                                                                                                              |
+===========================================================================+==========================================================================================================================+
|               ``SB_CONFIG_BOOTLOADER_MCUBOOT``                            | Build MCUboot image                                                                                                      |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE``                      | Set MCUboot signature type to none (SHA256 hash check only)                                                              |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA``                       | Set MCUboot signature type to RSA                                                                                        |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256``                | Set MCUboot signature type to ECDSA-P256                                                                                 |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519``                   | Set MCUboot signature type to ED25519                                                                                    |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE``                       | Absolute path to MCUboot private signing key file                                                                        |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_ENCRYPTION``                               | Enable MCUboot image encryption                                                                                          |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE``                      | Absolute path to MCUboot private encryption key file                                                                     |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_MODE_SINGLE_APP``                       | Build MCUboot in single slot mode (application can only be updated by MCUboot's serial recovery mode)                    |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_MODE_SWAP_WITHOUT_SCRATCH``             | Build MCUboot and application in swap using move mode (default)                                                          |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH``                     | Build MCUboot and application in swap using scratch mode                                                                 |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY``                   | Build MCUboot and application in overwrite only mode                                                                     |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP``                       | Build MCUboot and application in direct-XIP mode                                                                         |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT``           | Build MCUboot and application in direct-XIP mode, with revert support                                                    |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT``              | Build secondary image for direct-XIP mode for the alternative execution slot                                             |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION``         | Enable hardware downgrade protection in MCUboot and application                                                          |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS`` | Number of available hardware counter slots for downgrade prevention                                                      |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE`` | Security counter value of the image for downgrade prevention                                                             |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES``                     | Number of updateable images for MCUboot to support                                                                       |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES``            | Will set the main application number of updateable images as well as MCUboot if enabled, otherwise will only set MCUboot |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION``                   | MCUboot version string to use when creating MCUboot update package for application secure boot mode                      |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+
|               ``SB_CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM``                 | Use all available RAM when building TF-M for nRF5340 (see Kconfig text for security implication details)                 |
+---------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------------------------+

Support for unsigned images and image encryption has been added.
These options generate the respective output files for the main application build.
Any MCUboot configuration that was previously done in the main application or MCUboot needs to be updated to apply at the sysbuild level.
If this is not done, the settings of these builds will be forcefully replaced with the default generated by sysbuild, making firmware updates incompatible with firmware images built in previous versions of the |NCS|.

.. _child_parent_to_sysbuild_migration_secure_boot:

Secure boot
===========

Sysbuild handles the mode selection of secure boot and the configuration of the key file.
The following Kconfig options are available:

+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
| Kconfig option                                             | Description                                                                             |
+============================================================+=========================================================================================+
|               ``SB_CONFIG_SECURE_BOOT_APPCORE``            | Enable secure boot for application core (or main core if device only has a single core) |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_NETCORE``            | Enable secure boot for network core                                                     |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_SIGNING_PYTHON``     | Sign b0 images using python (default)                                                   |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_SIGNING_OPENSSL``    | Sign b0 images using OpenSSL                                                            |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM``     | Sign b0 images with a custom command                                                    |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE``   | Absolute path to signing private key file                                               |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND``    | Command called for custom signing, will have file to sign provided as an argument       |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_SIGNING_PUBLIC_KEY`` | Absolute path to signing key public file                                                |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+
|               ``SB_CONFIG_SECURE_BOOT_PUBLIC_KEY_FILES``   | Comma-separated value list of absolute paths to signing public key files                |
+------------------------------------------------------------+-----------------------------------------------------------------------------------------+

Secure boot can now be enabled centrally from sysbuild for both the application and network cores for nRF53-based boards.
Configuration that was previously done in the images themselves must now be applied at the sysbuild level.
If not, the secure boot images are not built, or the settings of these builds are forcefully replaced with the default generated by sysbuild, making firmware updates incompatible with firmware images built in previous versions of the |NCS|.

.. _child_parent_to_sysbuild_migration_bluetooth_fast_pair:

Google Fast Pair
================

Sysbuild now handles the HEX generation with Google Fast Pair provisioning data.
See the :ref:`ug_bt_fast_pair_provisioning_register` section in the Fast Pair integration guide for more details regarding the provisioning process.
The following Kconfig options are available:

+------------------------------------------+----------------------------------------+
| Kconfig option                           | Description                            |
+==========================================+========================================+
|               ``SB_CONFIG_BT_FAST_PAIR`` | Enables Google Fast Pair functionality |
+------------------------------------------+----------------------------------------+

To generate the Google Fast Pair provisioning data, you must set this Kconfig option at the sysbuild level.
The method of supplying the Fast Pair Model ID and Anti-Spoofing Private Key via the command line arguments remains unchanged from previous |NCS| versions.

.. note::
    When building with sysbuild, the value of the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option is overwritten by ``SB_CONFIG_BT_FAST_PAIR``.
    For more details about enabling Fast Pair for your application, see the :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section in the Fast Pair integration guide.

.. _child_parent_to_sysbuild_migration_matter:

Matter
======

Sysbuild now directly controls Matter configuration for generating factory data and over-the-air firmware update images.
The following Kconfig options are available:

+---------------------------------------------------------------------+---------------------------------------------------+
| Kconfig option                                                      | Description                                       |
+=====================================================================+===================================================+
|               ``SB_CONFIG_MATTER``                                  | Enable matter support                             |
+---------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_MATTER_FACTORY_DATA_GENERATE``            | Generate factory data                             |
+---------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_MATTER_FACTORY_DATA_MERGE_WITH_FIRMWARE`` | Merge factory data with main application firmware |
+---------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_MATTER_OTA``                              | Generate over-the-air firmware update image       |
+---------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME``              | Filename for over-the-air firmware update image   |
+---------------------------------------------------------------------+---------------------------------------------------+

Applications must enable these options if they generate factory data or need an over-the-air firmware update.

.. note::

    The configuration data for the factory data file is still configured from the main application.

.. _child_parent_to_sysbuild_migration_nrf700x:

nRF70 Series
============

Support for the nRF70 Series operating mode and firmware storage has moved to sysbuild.
The following Kconfig options are available:

+----------------------------------------------------------------+-----------------------------------------------------------------------------+
| Kconfig option                                                 | Description                                                                 |
+================================================================+=============================================================================+
|               ``SB_CONFIG_WIFI_NRF70``                         | Enable Wifi support for the nRF70 Series devices                            |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_NRF70_SYSTEM_MODE``             | Use system mode firmware patches and set application to this mode           |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_NRF70_SCAN_ONLY``               | Use Scan-only mode firmware patches and set application to this mode        |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_NRF70_RADIO_TEST``              | Use Radio Test mode firmware patches and set application to this mode       |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_NRF70_SYSTEM_WITH_RAW_MODES``   | Use system with Raw modes firmware patches and set application to this mode |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_DISABLED``    | Load firmware patches directly from ram (default)                           |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP``         | Load firmware patches from external flash using XIP                         |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+
|               ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE``       | Load firmware patches from external flash into RAM and load to radio        |
+----------------------------------------------------------------+-----------------------------------------------------------------------------+

You must update your applications to select the required Kconfig options at the sysbuild level for applications to work.
These sysbuild Kconfig options are no longer defaulted or gated depending on the features that the main application uses, so you must set these manually.
If these options are not set, nRF700x functionality will not work.

.. _child_parent_to_sysbuild_migration_dfu_multi_image_build:

Multi-image builds for DFU
==========================

Support for creating multi-image build files for Device Firmware Update (DFU) was moved to sysbuild.
The following Kconfig options are available:

+-------------------------------------------------------------------+---------------------------------------------------+
| Kconfig option                                                    | Description                                       |
+===================================================================+===================================================+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD``         | Enables building a DFU multi-image package        |
+-------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP``           | Include application update in package             |
+-------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET``           | Include network core image update in package      |
+-------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT``       | Include MCUboot update in package                 |
+-------------------------------------------------------------------+---------------------------------------------------+
|               ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH`` | Include nRF70 firmware patch update in package    |
+-------------------------------------------------------------------+---------------------------------------------------+

You must update your application to select the required Kconfig options at the sysbuild level to have this file generated.

.. _child_parent_to_sysbuild_migration_dfu_zip:

DFU Zip file generation
=======================

Support for generating a firmware update zip has moved to sysbuild.
The following Kconfig options are available:

+-------------------------------------------------------------+----------------------------------------------------------------------------+
| Kconfig option                                              | Description                                                                |
+=============================================================+============================================================================+
|               ``SB_CONFIG_DFU_ZIP``                         | Will generate a dfu_application.zip archive with manifest file and updates |
+-------------------------------------------------------------+----------------------------------------------------------------------------+
|               ``SB_CONFIG_DFU_ZIP_APP``                     | Include application update in zip archive                                  |
+-------------------------------------------------------------+----------------------------------------------------------------------------+
|               ``SB_CONFIG_DFU_ZIP_NET``                     | Include network-core image update in zip archive                           |
+-------------------------------------------------------------+----------------------------------------------------------------------------+
|               ``SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH``           | Include nRF700x firmware patch update in zip archive                       |
+-------------------------------------------------------------+----------------------------------------------------------------------------+
|               ``SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA`` | Include Bluetooth mesh metadata in zip archive                             |
+-------------------------------------------------------------+----------------------------------------------------------------------------+

You must update your application to select the required Kconfig options at the sysbuild level to have the correct firmware update images in the zip generated, the firmware zip is generated by default.

.. _child_parent_to_sysbuild_migration_partition_manager:

Partition Manager
=================

Support for using the Partition Manager for an image has been moved to sysbuild.
The following Kconfig options are available:

+---------------------------------------------------+-----------------------------------------------------------------+
|                  Kconfig option                   |                           Description                           |
+===================================================+=================================================================+
| ``SB_CONFIG_PARTITION_MANAGER``                   | Enables partition manager support                               |
+---------------------------------------------------+-----------------------------------------------------------------+
| ``SB_CONFIG_PM_MCUBOOT_PAD``                      | MCUboot image header padding                                    |
+---------------------------------------------------+-----------------------------------------------------------------+
| ``SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY`` | Places the secondary MCUboot update partition in external flash |
+---------------------------------------------------+-----------------------------------------------------------------+
| ``SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK``   | Will force override the external flash driver check             |
+---------------------------------------------------+-----------------------------------------------------------------+

You must update your applications to select the required Kconfig options at the sysbuild level for applications to work.
If these options are not set, firmware updates may not work or images may fail to boot.

.. _child_parent_to_sysbuild_migration_qspi_xip:

QSPI XIP flash split code
-------------------------

Support for using an application image based on the Quad Serial Peripheral Interface (QSPI) with the Execute in place (XIP) flash memory split has been moved to sysbuild.
The following Kconfig options are available:

+------------------------------------+------------------------------------------------------------------------------------------------------------+
|           Kconfig option           |                                                Description                                                 |
+====================================+============================================================================================================+
| ``SB_CONFIG_QSPI_XIP_SPLIT_IMAGE`` | Enables splitting application into internal flash and external QSPI XIP flash images with MCUboot signing. |
+------------------------------------+------------------------------------------------------------------------------------------------------------+

You must update your applications to select the required Kconfig options at the sysbuild level for applications to work.
If these options are not set, the QSPI XIP flash code sections will not be generated.
The MCUboot image number is now dependent upon what images are present in a build, and the Kconfig option ``SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER`` gives the image number of this section.

The format for the Partition Manager static partition file has also changed.
There must now be a ``pad`` section and an ``app`` section which form the primary section in a span.
Here's an example from the :ref:`SMP Server with external XIP <smp_svr_ext_xip>` sample:

.. code-block:: yaml

    mcuboot_primary_2:
      address: 0x120000
      device: MX25R64
      end_address: 0x160000
    +  orig_span: &id003
    +  - mcuboot_primary_2_pad
    +  - mcuboot_primary_2_app
      region: external_flash
      size: 0x40000
    +  span: *id003
    +mcuboot_primary_2_pad:
    +  address: 0x120000
    +  end_address: 0x120200
    +  region: external_flash
    +  size: 0x200
    +mcuboot_primary_2_app:
    +  address: 0x120200
    +  device: MX25R64
    +  end_address: 0x40000
    +  region: external_flash
    +  size: 0x3FE00

For more details about the QSPI XIP flash split image feature, see :ref:`qspi_xip_split_image`.

.. _child_parent_to_sysbuild_migration_filename_changes:

Filename changes
================

Some output file names have changed from child/parent image configurations or have changed the directory where they are created.
This is because sysbuild properly namespaces images in a project.
The changes to final output files (ignoring artifacts and intermediary files) are as follows:

+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
|                  Child/parent file                  |                                                                         Sysbuild file                                                                         |
+=====================================================+===============================================================================================================================================================+
| ``zephyr/app_update.bin``                           | ``<app_name>/zephyr/<kernel_name>.signed.bin`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value            |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/app_signed.hex``                           | ``<app_name>/zephyr/<kernel_name>.signed.hex`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value            |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/app_test_update.hex``                      | No equivalent                                                                                                                                                 |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/app_moved_test_update.hex``                | No equivalent                                                                                                                                                 |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/net_core_app_update.bin``                  | ``signed_by_mcuboot_and_b0_<net_core_app_name>.bin`` where ``<net_core_app_name>`` is the name of the network core application                                |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/net_core_app_signed.hex``                  | ``signed_by_b0_<net_core_app_name>.hex`` where ``<net_core_app_name>`` is the name of the network core application                                            |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/net_core_app_test_update.hex``             | No equivalent                                                                                                                                                 |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/net_core_app_moved_test_update.hex``       | No equivalent                                                                                                                                                 |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/mcuboot_secondary_app_update.bin``         | ``mcuboot_secondary_app/zephyr/<kernel_name>.signed.bin`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/mcuboot_secondary_app_signed.hex``         | ``mcuboot_secondary_app/zephyr/<kernel_name>.signed.hex`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/matter.ota``                               | ``<matter_ota_name>.ota`` where ``<matter_ota_name>`` is the value of Kconfig ``SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME``                                        |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/signed_by_b0_s0_image.hex``                | ``signed_by_b0_<app_name>.hex`` where ``<app_name>`` is the name of the application                                                                           |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/signed_by_b0_s1_image.hex``                | ``signed_by_b0_s1_image.hex``                                                                                                                                 |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/signed_by_b0_s0_image.bin``                | ``signed_by_b0_<app_name>.bin`` where ``<app_name>`` is the name of the application                                                                           |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/signed_by_b0_s1_image.bin``                | ``signed_by_b0_s1_image.bin``                                                                                                                                 |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``<net_core_app_name>/zephyr/signed_by_b0_app.hex`` | ``signed_by_b0_<net_core_app_name>.hex`` where ``<net_core_app_name>`` is the name of the network core application                                            |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``<net_core_app_name>/zephyr/signed_by_b0_app.bin`` | ``signed_by_b0_<net_core_app_name>.bin`` where ``<net_core_app_name>`` is the name of the network core application                                            |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/internal_flash.hex``                       | ``<app_name>/zephyr/<kernel_name>.internal.hex`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value          |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/internal_flash_signed.hex``                | ``<app_name>/zephyr/<kernel_name>.internal.signed.hex`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value   |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/internal_flash_update.bin``                | ``<app_name>/zephyr/<kernel_name>.internal.signed.bin`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value   |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/qspi_flash.hex``                           | ``<app_name>/zephyr/<kernel_name>.external.hex`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value          |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/qspi_flash_signed.hex``                    | ``<app_name>/zephyr/<kernel_name>.external.signed.hex`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value   |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/qspi_flash_update.bin``                    | ``<app_name>/zephyr/<kernel_name>.external.signed.bin`` where ``<kernel_name>`` is the application's Kconfig :kconfig:option:`CONFIG_KERNEL_BIN_NAME` value   |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/merged.hex``                               | ``merged.hex``                                                                                                                                                |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``<net_core_app_name>/zephyr/merged_CPUNET.hex``    | ``merged_CPUNET.hex``                                                                                                                                         |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/merged_domains.hex``                       | No equivalent, use ``merged.hex`` for application core and ``merged_CPUNET.hex`` for network core                                                             |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/dfu_multi_image.bin``                      | ``dfu_multi_image.bin``                                                                                                                                       |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/dfu_application.zip``                      | ``dfu_application.zip``                                                                                                                                       |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``zephyr/dfu_mcuboot.zip``                          | ``dfu_mcuboot.zip``                                                                                                                                           |
+-----------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+

Example output files
====================

To demonstrate the expected output files when using sysbuild for an application build, the following sections show and describe the output files for the ``matter_weather_station`` application when building using the ``thingy53/nrf5340/cpaupp`` board target:

Provision/container files
-------------------------

The expected output files are the following:

+-----------------------+-------------------------------------------------------+
| File                  | Description                                           |
+=======================+=======================================================+
| ``b0n_container.hex`` | Copy of ``b0n/zephyr/zephyr.hex``                     |
+-----------------------+-------------------------------------------------------+
| ``net_provision.hex`` | Provision data for the network core secure boot image |
+-----------------------+-------------------------------------------------------+

Image build files
-----------------

The expected output files are the following:

+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| File                                                | Description                                                                                           |
+=====================================================+=======================================================================================================+
| ``matter_weather_station/zephyr/zephyr.hex``        | Unsigned main application HEX file                                                                    |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``matter_weather_station/zephyr/zephyr.bin``        | Unsigned main application binary file                                                                 |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``matter_weather_station/zephyr/zephyr.signed.hex`` | Signed (with MCUboot signing key) main application HEX file                                           |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``mcuboot/zephyr/zephyr.hex``                       | MCUboot HEX file                                                                                      |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``ipc_radio/zephyr/zephyr.hex``                     | Network core IPC radio HEX file                                                                       |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``ipc_radio/zephyr/zephyr.bin``                     | Network core IPC radio binary file                                                                    |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``b0n/zephyr/zephyr.bin``                           | Network core secure bootloader binary file                                                            |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``b0n/zephyr/zephyr.hex``                           | Network core secure bootloader HEX file                                                               |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``signed_by_b0_ipc_radio.hex``                      | Signed (with b0 signing key) network core IPC radio HEX file                                          |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``signed_by_b0_ipc_radio.bin``                      | Signed (with b0 signing key) network core IPC radio binary file                                       |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+
| ``signed_by_mcuboot_and_b0_ipc_radio.hex``          | Signed (with b0 and MCUboot signing key) network core IPC radio update from application core HEX file |
+-----------------------------------------------------+-------------------------------------------------------------------------------------------------------+

Combined files
--------------

The expected output files are the following:

+-----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| File                  | Description                                                                                                                                                           |
+=======================+=======================================================================================================================================================================+
| ``merged.hex``        | Merged application core HEX file (contains merged contents of ``mcuboot/zephyr/zephyr.hex`` and ``matter_weather_station/zephyr/zephyr.signed.hex``)                  |
+-----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``merged_CPUNET.hex`` | Merged network core HEX file (contains merged contents of ``net_provision.hex``, ``b0n_container.hex``, ``b0n/zephyr/zephyr.hex`` and ``signed_by_b0_ipc_radio.hex``) |
+-----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------+

Update files
------------

The expected output files are the following:

+-----------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| File                                                | Description                                                                                              |
+=====================================================+==========================================================================================================+
| ``matter_weather_station/zephyr/zephyr.signed.bin`` | Signed (with MCUboot signing key) main application binary file which can be used directly with MCUmgr    |
+-----------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| ``signed_by_mcuboot_and_b0_ipc_radio.bin``          | Signed (with b0 and MCUboot signing key) network core IPC radio update from application core binary file |
+-----------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| ``dfu_multi_image.bin``                             | DFU multi image file containing firmware update files and manifest                                       |
+-----------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| ``matter.ota``                                      | Matter over-the-air firmware update file                                                                 |
+-----------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| ``dfu_application.zip``                             | Zip file containing firmware update files and manifest                                                   |
+-----------------------------------------------------+----------------------------------------------------------------------------------------------------------+

.. _child_parent_to_sysbuild_migration_image_overlay_changes:

Image overlay configuration
===========================

In child/parent image configurations, an application could include additional configuration files in the ``child_image`` folder that would be applied to these images (see :ref:`ug_multi_image_permanent_changes`).
This feature has been adapted in sysbuild; see :ref:`sysbuild_application_configuration` for an overview.
You must update child/parent image configuration to use it with sysbuild, as the way these files can be used differs:

* In child/parent image configurations, there can be Kconfig fragments and board overlays that are all merged into the final output files.

* In sysbuild, there can either be a Kconfig fragment overlay, or replacement for the whole application configuration directory.

In sysbuild, if an image application configuration directory is created then it must include all the required files for that image, as none of the original application configuration files will be used.
Sysbuild includes support for :ref:`application-file-suffixes` in applications, and it can also use :ref:`sysbuild_file_suffixes`.

Example for MCUboot
===================

The following table shows how to add custom MCUboot configuration for a project.
The ``sysbuild`` folder must be created in the application's folder:

+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+
| File                                                         | Description                                                                                                                               |
+==============================================================+===========================================================================================================================================+
| ``sysbuild/mcuboot/prj.conf``                                | Copy of ``boot/zephyr/prj.conf`` from the MCUboot repository, this may have additional changes for this specific application              |
+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+
| ``sysbuild/mcuboot/prj_release.conf``                        | Modification of prj.conf with changes for a release configuration (can be selected using ``-DFILE_SUFFIX=release``)                       |
+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+
| ``sysbuild/mcuboot/app.overlay``                             | Copy of ``boot/zephyr/app.overlay`` from the MCUboot repository                                                                           |
+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+
| ``sysbuild/mcuboot/boards/nrf52840dk_nrf52840.conf``         | Kconfig fragment for the ``nrf52840dk/nrf52840`` board target                                                                             |
+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+
| ``sysbuild/mcuboot/boards/nrf52840dk_nrf52840.overlay``      | DTS overlay for the ``nrf52840dk/nrf52840`` board target, note: used **instead** of app.overlay, not with as child/parent used to do      |
+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+
| ``sysbuild/mcuboot/boards/nrf9160dk_nrf9160_0_14_0.overlay`` | DTS overlay for the ``nrf9160dk@0.14.0/nrf9160`` board target, note: used **instead** of app.overlay, not with as child/parent used to do |
+--------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------+

.. _child_parent_to_sysbuild_migration_scope_changes:

Scope changes
=============

In child/parent images, the application controlled all images, so variables without a prefix would apply to the main application only.
In Sysbuild, elements like file suffixes, shields, and snippets without an image prefix will be applied **globally** to all images.
To apply them to a single image, they must be prefixed with the image name.
Without doing this, projects with multiple images (for example, those with MCUboot) might fail to build due to invalid configuration for other images.

+-------------------------------+----------------------------------+-------------------------+
| Configuration parameter       | Child/parent                     | Sysbuild                |
+===============================+==================================+=========================+
| ``-DFILE_SUFFIX=...``         | Applies only to main application | Applies to all images   |
+-------------------------------+----------------------------------+-------------------------+
| ``-D<image>_FILE_SUFFIX=...`` | Applies only to <image>          | Applies only to <image> |
+-------------------------------+----------------------------------+-------------------------+
| ``-DSNIPPET=...``             | Applies only to main application | Applies to all images   |
+-------------------------------+----------------------------------+-------------------------+
| ``-D<image>_SNIPPET=...``     | Applies only to <image>          | Applies only to <image> |
+-------------------------------+----------------------------------+-------------------------+
| ``-DSHIELD=...``              | Applies only to main application | Applies to all images   |
+-------------------------------+----------------------------------+-------------------------+
| ``-D<image>_SHIELD=...``      | Applies only to <image>          | Applies only to <image> |
+-------------------------------+----------------------------------+-------------------------+

Configuration values that specify Kconfig fragment or overlay files (for example, :makevar:`EXTRA_CONF_FILE` and :makevar:`EXTRA_DTC_OVERLAY_FILE`) cannot be applied globally using either child/parent image or sysbuild.
They function the same in both systems:

* Without a prefix, they will be applied to the main application only.

* With a prefix, they will apply to that specific image only.

.. _child_parent_to_sysbuild_migration_building:

Building with sysbuild
======================

Sysbuild needs to be enabled from the command-line when building with ``west build``.
You can pass the ``--sysbuild`` parameter to the build command or :ref:`configure west to use sysbuild whenever you build <sysbuild_enabled_ncs_configuring>`.

Similarly, you can pass the ``--no-sysbuild`` parameter to the build command to disable sysbuild.
With these two parameters, which always take precedence over the west configuration, the usage of sysbuild can always be selected from the command line.

.. note::
    The |NCS| v2.7.0 :ref:`modifies the default behavior <sysbuild_enabled_ncs>` of ``west build``, so that building with west uses sysbuild for :ref:`repository applications <create_application_types_repository>` in the :ref:`SDK repositories <dm_repo_types>`.

See the following command patterns for building with sysbuild for different use cases:

.. tabs::

    .. group-tab:: west (sysbuild)

        West can build a specific project using sysbuild with the following command:

        .. parsed-literal::
           :class: highlight

           west build -b *board_target* --sysbuild *app_path*

    .. group-tab:: west (child/parent image)

        West can build a specific project using child/parent image with the following command:

        .. parsed-literal::
           :class: highlight

           west build -b *board_target* --no-sysbuild *app_path*

        .. note::

            This is deprecated in |NCS| 2.7 and support will be removed in |NCS| 2.9

    .. group-tab:: CMake (sysbuild)

        CMake can be used to configure a specific project using sysbuild image with the following command:

        .. parsed-literal::
           :class: highlight

           cmake -GNinja -DBOARD=*board_target* -DAPP_DIR=*app_path* *path_to_zephyr*/share/sysbuild

    .. group-tab:: CMake (child/parent image)

        CMake can be used to configure a specific project using child/parent image with the following command:

        .. parsed-literal::
           :class: highlight

           cmake -GNinja -DBOARD=*board_target* *app_path*

        .. note::

            This is deprecated in |NCS| 2.7 and support will be removed in |NCS| 2.9

    .. group-tab:: twister (sysbuild)

        Twister test cases can build using sysbuild with the following:

        .. code-block:: yaml

            sysbuild: true

    .. group-tab:: twister (child/parent image)

        Twister test cases can build using child/parent image with the following:

        .. code-block:: yaml

            sysbuild: false

        .. note::

            This is deprecated in |NCS| 2.7 and support will be removed in |NCS| 2.9

.. _child_parent_to_sysbuild_forced_kconfig_options:

Forced Kconfig options
======================

As sysbuild deals with configuration of features for some features and propagating this information to other images, some Kconfig options in applications will be forcefully overwritten by sysbuild, for details on these options and how to set them from sysbuild, check the :ref:`sysbuild_forced_options` section.

.. _child_parent_to_sysbuild_migration_incompatibilities:

Incompatibities
===============

In the sysbuild release included in the |NCS| 2.7, the following features of the multi-image builds using child and parent images are not supported:

* Using pre-built HEX files for images (like MCUboot).
    All images in a project will be built from source

    As a workaround for this, you can first build a project, then use ``mergehex`` manually to merge the project output HEX file with a previously-generated HEX file in overwrite mode to replace that firmware in the output image.

* Moved and confirmed output files when MCUboot is enabled
    These files are not generated when using sysbuild so would need to be manually generated.
