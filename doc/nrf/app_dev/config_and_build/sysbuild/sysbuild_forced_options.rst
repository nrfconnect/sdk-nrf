.. _sysbuild_forced_options:

Sysbuild forced options
#######################

Sysbuild controls some Kconfig options in images that are part of a project.
This means that these Kconfig options can only be changed from within sysbuild itself and cannot be changed directly in an image.
Trying to change them directly in an image will result in the new value being overwritten with the sysbuild value.

+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| Kconfig                                                                 | Sysbuild Kconfig                                                          | Images                  |
+=========================================================================+===========================================================================+=========================+
| :kconfig:option:`CONFIG_PARTITION_MANAGER_ENABLED`                      + :kconfig:option:`SB_CONFIG_PARTITION_MANAGER`                             | All                     |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BUILD_OUTPUT_BIN`                               + :kconfig:option:`SB_CONFIG_BUILD_OUTPUT_BIN`                              |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BUILD_OUTPUT_HEX`                               + :kconfig:option:`SB_CONFIG_BUILD_OUTPUT_HEX`                              |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| :kconfig:option:`CONFIG_WIFI_NRF70`                                     + :kconfig:option:`SB_CONFIG_WIFI_NRF70`                                    | Main application        |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_DISABLED`            + :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_DISABLED`               |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP`                 + :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_XIP`                    |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE`               + :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE`                  |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF70_SYSTEM_MODE`                              + :kconfig:option:`SB_CONFIG_WIFI_NRF70_SYSTEM_MODE`                        |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF70_SCAN_ONLY`                                + :kconfig:option:`SB_CONFIG_WIFI_NRF70_SCAN_ONLY`                          |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF70_RADIO_TEST`                               + :kconfig:option:`SB_CONFIG_WIFI_NRF70_RADIO_TEST`                         |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF70_SYSTEM_WITH_RAW_MODES`                    + :kconfig:option:`SB_CONFIG_WIFI_NRF70_SYSTEM_WITH_RAW_MODES`              |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU`                          + :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH`         |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP`             + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SINGLE_APP`                       |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_WITHOUT_SCRATCH`   + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_WITHOUT_SCRATCH`             |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH`           + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH`                     |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY`         + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY`                   |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP`             + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`                       |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT` + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`           |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER`       + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`                 |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`          + :kconfig:option:`SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`         |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS`  + :kconfig:option:`SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS` |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE`  + :kconfig:option:`SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE` |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER`                        + :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` if                  |                         |
|                                                                         + :kconfig:option:`SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES` is enabled |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_CHIP`                                           + :kconfig:option:`SB_CONFIG_MATTER`                                        |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_CHIP_OTA_REQUESTOR`                             + :kconfig:option:`SB_CONFIG_MATTER_OTA`                                    |                         |
+-------------------------------------------------------------------------+                                                                           |                         |
| :kconfig:option:`CONFIG_CHIP_OTA_IMAGE_BUILD`                           +                                                                           |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| :kconfig:option:`CONFIG_SINGLE_APPLICATION_SLOT`                        + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SINGLE_APP`                       | MCUboot                 |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BOOT_SWAP_USING_MOVE`                           + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_WITHOUT_SCRATCH`             |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BOOT_SWAP_USING_SCRATCH`                        + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH`                     |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BOOT_UPGRADE_ONLY`                              + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY`                   |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BOOT_DIRECT_XIP`                                + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`                       |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BOOT_DIRECT_XIP_REVERT`                         + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`           |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_BOOT_FIRMWARE_LOADER`                           + :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`                 |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_PCD_APP`                                        + :kconfig:option:`SB_CONFIG_NETCORE_APP_UPDATE`                            |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER`                        + :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES`                     |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION`                + :kconfig:option:`SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`         | Main application,       |
+-------------------------------------------------------------------------+                                                                           | MCUboot                 |
| :kconfig:option:`CONFIG_SECURE_BOOT_STORAGE`                            +                                                                           |                         |
+-------------------------------------------------------------------------+                                                                           |                         |
| :kconfig:option:`CONFIG_SECURE_BOOT_CRYPTO`                             +                                                                           |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY`            + :kconfig:option:`SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY`           |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK`              + :kconfig:option:`SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK`             |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+                         |
| :kconfig:option:`CONFIG_FW_INFO`                                        + :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE`                           |                         |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| :kconfig:option:`CONFIG_NCS_MCUBOOT_IN_BUILD`                           + :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`                            | :ref:`b0 <bootloader>`, |
|                                                                         +                                                                           | :ref:`b0n <bootloader>` |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| :kconfig:option:`CONFIG_SECURE_BOOT`                                    + :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE` or                        | Main application,       |
|                                                                         + :kconfig:option:`SB_CONFIG_SECURE_BOOT_NETCORE`                           | Network core main image,|
|                                                                         +                                                                           | MCUboot                 |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
| :kconfig:option:`CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION`       + :kconfig:option:`SB_CONFIG_SECURE_BOOT_MONOTONIC_COUNTER`                 | :ref:`b0 <bootloader>`  |
+-------------------------------------------------------------------------+---------------------------------------------------------------------------+-------------------------+
