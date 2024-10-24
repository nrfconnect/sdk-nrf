.. _sysbuild_assigned_images_ids:

Sysbuild-assigned image IDs
###########################

.. contents::
   :local:
   :depth: 2

When using sysbuild in |NCS| with MCUboot, IDs for images will be automatically assigned by the build system depending upon configuration of a project, this allows for features to be enabled and disabled and have the image IDs update automatically without needing manual changes to applications or the bootloader (code and/or configuration)

The following configuration options are available in sysbuild which show the image IDs assigned to a particular image (a value of ``-1`` indicates the image is not present):

.. note::
    These are also provided to MCUboot and the main application via Kconfig values too (with exception of ``SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER`` which is provided to MCUboot only)

+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| Kconfig option (sysbuild)                       | Kconfig option (application/MCUboot)                       | Description                                  | Dependencies                                                                                                 |
+=================================================+============================================================+==============================================+==============================================================================================================+
| ``SB_CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER``  | :kconfig:option:`CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER`  | Image number for application update          | --                                                                                                           |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER`` | :kconfig:option:`CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER` | Image number for network core update         | nRF5340 device and ``SB_CONFIG_NETCORE_APP_UPDATE``                                                          |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER`` | :kconfig:option:`CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER` | Image number for wifi-patch update           | nRF7x device used and ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP`` or ``SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE`` |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER``     | :kconfig:option:`CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER`     | Image number for QSPI XIP split image update | nRF52840 or nRF5340 device and ``SB_CONFIG_QSPI_XIP_SPLIT_IMAGE``                                            |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER``      | :kconfig:option:`CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER`      | Image number for MCUboot update              | ``SB_CONFIG_SECURE_BOOT_APPCORE``                                                                            |
|                                                 | (only set for MCUboot image)                               |                                              |                                                                                                              |
+-------------------------------------------------+------------------------------------------------------------+----------------------------------------------+--------------------------------------------------------------------------------------------------------------+

The following configuration options relate the to the number of images and how those image numbers are setup in images:

+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------|
| Kconfig option (sysbuild)                              | Description                                                                                                                                          |
+========================================================+======================================================================================================================================================|
| ``SB_CONFIG_MCUBOOT_MIN_UPDATEABLE_IMAGES``            | Minimum number of MCUboot images                                                                                                                     |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_MIN_ADDITIONAL_UPDATEABLE_IMAGES`` | Minimum number of additional MCUboot images for MCUboot only (described below)                                                                       |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_MAX_UPDATEABLE_IMAGES``            | Maximum number of MCUboot images                                                                                                                     |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES``                | Number of MCUboot images to set in images                                                                                                            |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES``     | Number of additional MCUboot images to include in MCUboot (described below)                                                                          |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES``       | If enabled then will set both MCUboot and the main application's number of MCUboot images, if not set then will only set this is the MCUboot image   |
+--------------------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------+

MCUboot-only images
*******************

There are images that are only visible to MCUboot, this allows for MCUboot to deal with those images without interference/knowledge from the application, or to allow for features like shared secondary slots.
At present, this is used by MCUboot to update MCUboot itself, which can enabled with secure boot, this will share the main application's secondary slot for storing firmware updates but will move the update directly to the opposing MCUboot image slot which the xNSIB?x can then boot (the application does not need to be aware of how this works, just that it can place an MCUboot update as it would a normal application update and reboot to apply).
To allow for this, there are additional MCUboot-only images which are not presented to the main application and are only set in MCUboot.
The total number of images set in MCUboot is ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES + SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES``, whilst in the main application it is just ``SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES`` (if ``SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES`` is enabled, otherwise will not be set)

Getting image IDs in an application
***********************************

When building with sysbuild, the image IDs are provided to the application via Kconfig values, they can be used in the code as needed.
These image IDs are also used in |NCS| hooks for things like nRF5340 network core updates, so no extra configuration is needed to configure these.

Image numbers
*************

Image numbers will be assigned in ascending order with the following priority:

+----------------------+--------------------------------+-------------------------------------------------+----------------------------------------------+
| Image                | Value (if all enabled)         | Kconfig option (sysbuild)                       | Kconfig option (application/MCUboot)         |
+======================+================================+=================================================+==============================================+
| Application          | 0                              | ``SB_CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER``  | ``CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER``  |
+----------------------+--------------------------------+-------------------------------------------------+----------------------------------------------+
| Network core         | 1                              | ``SB_CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER`` | ``CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER`` |
+----------------------+--------------------------------+-------------------------------------------------+----------------------------------------------+
| nRF7x wifi patch     | 2                              | ``SB_CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER`` | ``CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER`` |
+----------------------+--------------------------------+-------------------------------------------------+----------------------------------------------+
| QSPI XIP split image | 3                              | ``SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER``     | ``CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER``     |
+----------------------+--------------------------------+-------------------------------------------------+----------------------------------------------+
| MCUboot              | 4 (only set for MCUboot image) | ``SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER``      | ``CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER``      |
+----------------------+--------------------------------+-------------------------------------------------+----------------------------------------------+

MCUboot update package version
******************************

When MCUboot updates are supported, the MCUboot update will have the xNSIBx?x version embedded into the firmware using :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION`, however this version is not checked by MCUboot when doing an update, only the MCUboot package version is checked.
The MCUboot update package version is set in sysbuild with ``SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION``, it is important to up-issue this version number if deploying an MCUboot update as if the version of MCUboot in the update is lower, the update will be rejected by MCUboot and not placed into the opposing secure boot firmware slot.
It is also important to load the correct update image to the device, if the device is currently running MCUboot from the s0 slot then the s1 update needs to be used, likewise if MCUboot is currently running from the s1 slot then the s0 update needs to be loaded, if the wrong slot image is uploaded, the update will be rejected by MCUboot.
