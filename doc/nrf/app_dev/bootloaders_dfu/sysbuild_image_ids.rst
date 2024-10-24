.. _sysbuild_assigned_images_ids:

Sysbuild-assigned image IDs
###########################

.. contents::
   :local:
   :depth: 2

When using sysbuild in |NCS| with MCUboot, the build system automatically assigns image IDs based on your project configuration.

Sysbuild has the following configuration options that display the image IDs assigned to each image.
A value of ``-1`` indicates that the image is not present:

.. note::
    These options are also shared with MCUboot and the main application through Kconfig values, except for the ``SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER`` option, which is exclusive to MCUboot.

+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| Kconfig option (sysbuild)                       | Kconfig option (application/MCUboot)                       | Description                                  | Dependencies                                                                                                 |
+=================================================+============================================================+==============================================+==============================================================================================================+
| ``SB_CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER``  | :kconfig:option:`CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER`  | Image number for application update          | --                                                                                                           |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER`` | :kconfig:option:`CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER` | Image number for network core update         | nRF5340 device and ``SB_CONFIG_NETCORE_APP_UPDATE``                                                          |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER`` | :kconfig:option:`CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER` | Image number for Wi-Fi-patch update          | nRF7x device used and ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP`` or ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE`` |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER``     | :kconfig:option:`CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER`     | Image number for QSPI XIP split image update | nRF52840 or nRF5340 device and ``SB_CONFIG_QSPI_XIP_SPLIT_IMAGE``                                            |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER``      | :kconfig:option:`CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER`      | Image number for MCUboot update              | ``SB_CONFIG_SECURE_BOOT_APPCORE``                                                                            |
|                                                 | (only set for MCUboot image)                               |                                              |                                                                                                              |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+

The following configuration options specify the number of images and how these image numbers are configured:

+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| Kconfig option (sysbuild)                              | Description                                                                                                                                          |
+========================================================+======================================================================================================================================================+
| ``SB_CONFIG_MCUBOOT_MIN_UPDATEABLE_IMAGES``            | Minimum number of MCUboot images                                                                                                                     |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_MIN_ADDITIONAL_UPDATEABLE_IMAGES`` | Minimum number of additional MCUboot images for MCUboot only (see :ref:`sysbuild_assigned_images_ids_mcuboot_only`)                                  |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_MAX_UPDATEABLE_IMAGES``            | Maximum number of MCUboot images                                                                                                                     |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES``                | Number of MCUboot images to set in images                                                                                                            |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES``     | Number of additional MCUboot images to include in MCUboot (see :ref:`sysbuild_assigned_images_ids_mcuboot_only`)                                     |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES``       | If enabled, this option sets the number of MCUboot images for both MCUboot and the main application.                                                 |
|                                                        | If disabled, it sets the number only for the MCUboot image.                                                                                          |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+

See :ref:`configuring_kconfig` for information about how to configure these options.

.. _sysbuild_assigned_images_ids_mcuboot_only:

MCUboot-only images
*******************

Some images are visible only to MCUboot, allowing it to manage them independently of the main application.
This setup supports features like shared secondary slots.
Currently, MCUboot uses this capability to update itself, which can be enabled with secure boot.
MCUboot then shares the main application's secondary slot for firmware updates.
However, it directly transfers the update to the designated MCUboot image slot, which is then booted by :ref:`bootloader`.

The main application does not need to be aware of this process; it only needs to handle an MCUboot update as it would any normal application update and reboot to apply changes.
This functionality is possible because there are additional MCUboot-only images that are not accessible to the main application and are configured only within MCUboot.

The total number of images configured in MCUboot is the sum of ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES`` and ``SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES``.
If ``SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES`` is enabled, the main application sets only ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES``.
Otherwise, it is not set.

Getting image IDs in an application
***********************************

When building with sysbuild, the image IDs are provided to the application through Kconfig values and can be used in the code as needed.
These image IDs are also used in |NCS| hooks for various tasks (such as :ref:`nRF5340 network core updates <ug_nrf5340_multi_image_dfu>`), so no additional configuration is required.

Image numbers
*************

Image numbers are assigned in ascending order based on the following priority:

+----------------------+--------------------------------+-------------------------------------------------+------------------------------------------------------------+
| Image                | Value (if all enabled)         | Kconfig option (sysbuild)                       | Kconfig option (application/MCUboot)                       |
+======================+================================+=================================================+============================================================+
| Application          | 0                              | ``SB_CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER``  | :kconfig:option:`CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER`  |
+----------------------+--------------------------------+-------------------------------------------------+------------------------------------------------------------+
| Network core         | 1                              | ``SB_CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER`` | :kconfig:option:`CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER` |
+----------------------+--------------------------------+-------------------------------------------------+------------------------------------------------------------+
| nRF7x Wi-Fi patch    | 2                              | ``SB_CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER`` | :kconfig:option:`CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER` |
+----------------------+--------------------------------+-------------------------------------------------+------------------------------------------------------------+
| QSPI XIP split image | 3                              | ``SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER``     | :kconfig:option:`CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER`     |
+----------------------+--------------------------------+-------------------------------------------------+------------------------------------------------------------+
| MCUboot              | 4 (only set for MCUboot image) | ``SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER``      | :kconfig:option:`CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER`      |
+----------------------+--------------------------------+-------------------------------------------------+------------------------------------------------------------+

MCUboot update package version
******************************

When MCUboot updates are enabled, the firmware embeds the |NSIB| version using the :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` Kconfig option.
However, during an update, MCUboot does not check this version; it only checks the MCUboot package version.

You can set the MCUboot update package version in sysbuild with ``SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION``.
You must increase this version number when deploying an MCUboot update.
If the version in the update is lower than the current version, MCUboot will reject the update and not transfer it to the opposing secure boot firmware slot.

Additionally, make sure to load the correct update image onto the device.
If MCUboot is currently running from the ``s0`` slot, then you must use the ``s1`` update, and if it is running from the ``s1`` slot, then you must use the ``s0`` update.
If you upload to the wrong slot image, MCUboot will reject the update.
