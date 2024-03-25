.. _nrf_desktop_bootloader:

nRF Desktop: Bootloader and Device Firmware Update
##################################################

.. contents::
   :local:
   :depth: 2

The nRF Desktop application uses the :ref:`bootloaders <app_bootloaders>` firmware image that allows you to upgrade the used application firmware image using the :ref:`app_dfu` procedure.

You can use the following Device Firmware Update (DFU) procedures for upgrading the application firmware version:

* :ref:`nrf_desktop_bootloader_background_dfu`
* :ref:`nrf_desktop_bootloader_serial_dfu`

Bootloader
**********

The nRF Desktop application can use one of the following bootloaders:

**Secure Bootloader**
  In this documentation, the Secure Bootloader is referred as *B0*.
  B0 is a small, simple, and secure bootloader that allows the application to boot directly from one of the application slots, thus increasing the speed of the direct firmware upgrade (DFU) process.
  B0 is supported on the SoCs from the following series:

  * nRF52 Series
  * nRF53 Series (supports only application core DFU)

  This bootloader can be used only for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>` through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
  For more information about the B0, see the :ref:`bootloader` page.

**MCUboot**
  MCUboot is supported on the SoCs from the following series:

  * nRF52 Series
  * nRF53 Series

  The MCUboot bootloader can be used in the following scenarios:

  * :ref:`Background DFU <nrf_desktop_bootloader_background_dfu>`.
    In this scenario, the MCUboot either swaps the application images located on the secondary and primary slots before booting a new image (``swap mode``) or boots a new application image directly from the secondary image slot (``direct-xip mode``).
    The swap operation significantly increases boot time after a successful image transfer, but external non-volatile memory can be used as the secondary image slot.

    You can use the MCUboot for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
    The MCUboot can also be used for the background DFU over the Simple Management Protocol (SMP).
    The SMP can be used to transfer the new firmware image in the background, for example, from an Android device.
    In that case, either the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` is used to handle the image transfer.
    The :ref:`nrf_desktop_ble_smp` relies on a generic module implemented in the Common Application Framework (CAF).
    The :ref:`nrf_desktop_dfu_mcumgr` uses an application-specific implementation that allows to synchronize a secondary image slot access with the :ref:`nrf_desktop_dfu` using the :ref:`nrf_desktop_dfu_lock`.

  * :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>`.
    In this scenario, the MCUboot bootloader supports the USB serial recovery.
    The USB serial recovery can be used for memory-limited devices that support the USB connection.
    In this mode, unlike in the background DFU mode, the DFU image transfer is handled by the bootloader.
    The application is not running and it can be overwritten.
    Because of that, only one application slot may be used.

  For more information about the MCUboot, see the :ref:`MCUboot <mcuboot:mcuboot_wrapper>` documentation.

.. note::
    The nRF Desktop application can use either B0 or MCUboot.
    The MCUboot is not used as the second stage bootloader.

.. important::
    Make sure that you use your own private key for the release version of the devices.
    Do not use the debug key for production.

If your configuration enables the bootloader, make sure to define a static non-volatile memory layout in the Partition Manager.
See :ref:`nrf_desktop_memory_layout` for details.

Configuring the B0 bootloader
=============================

To enable the B0 bootloader, select the :kconfig:option:`CONFIG_SECURE_BOOT` Kconfig option in the application configuration.

The B0 bootloader additionally requires enabling the following options in the application configuration:

* :kconfig:option:`CONFIG_SB_SIGNING_KEY_FILE` - Required for providing the signature used for image signing and verification.
* :kconfig:option:`CONFIG_FW_INFO` - Required for the application versioning information.
* :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` - Enable this option to set the version of the application after you enabled :kconfig:option:`CONFIG_FW_INFO`.
  The nRF Desktop application with the B0 bootloader configuration builds two application images: one for the S0 slot and the other for the S1 slot.
  To generate the DFU package, you need to update this configuration only in the main application image as the ``s1_image`` child image mirrors it.
  You can do that by rebuilding the application from scratch or by changing the configuration of the main image through menuconfig.
* :kconfig:option:`CONFIG_BUILD_S1_VARIANT` - Required for the build system to be able to construct the application binaries for both application's slots in non-volatile memory.

.. _nrf_desktop_configuring_mcuboot_bootloader:

Configuring the MCUboot bootloader
==================================

To enable the MCUboot bootloader, select the :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option in the application configuration.

The MCUboot private key path (:kconfig:option:`CONFIG_BOOT_SIGNATURE_KEY_FILE`) must be set only in the MCUboot bootloader configuration file.
The key is used both by the build system (to sign the application update images) and by the bootloader (to verify the application signature using public key derived from the selected private key).

The MCUboot bootloader configuration depends on the selected way of performing image upgrade.
For detailed information about the available MCUboot bootloader modes, see the following sections.

Swap mode
---------

In the swap mode, the MCUboot bootloader moves the image to the primary slot before booting it.
The swap mode is the image upgrade mode used by default for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.

If the swap mode is used, the application must request a firmware upgrade and confirm the running image.
For this purpose, make sure to enable the :kconfig:option:`CONFIG_IMG_MANAGER` and :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER` Kconfig options in the application configuration.
These options allow the :ref:`nrf_desktop_dfu`, :ref:`nrf_desktop_ble_smp`, and :ref:`nrf_desktop_dfu_mcumgr` to manage the DFU image.

.. note::
   When the MCUboot bootloader is in the swap mode, it can use a secondary image slot located on the external non-volatile memory.
   For details on using external non-volatile memory for the secondary image slot, see the :ref:`nrf_desktop_pm_external_flash` section.

Direct-xip mode
---------------

The direct-xip mode is used for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.
In this mode, the MCUboot bootloader boots an image directly from a given slot, so the swap operation is not needed.
To set the MCUboot mode of operations to the direct-xip mode, make sure to enable the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option in the application configuration.
This option automatically enables :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` to build application update images for both slots.

Enable the ``CONFIG_BOOT_DIRECT_XIP`` Kconfig option in the bootloader configuration to make the MCUboot run the image directly from both image slots.
The nRF Desktop's bootloader configurations do not enable the revert mechanism (``CONFIG_BOOT_DIRECT_XIP_REVERT``).
When the direct-xip mode is enabled (the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option is set in the application configuration), the application modules that control the DFU transport do not request firmware upgrades and do not confirm the running image.
In that scenario, the MCUboot bootloader simply boots the image with the higher image version.

By default, the MCUboot bootloader ignores the build number while comparing image versions.
Enable the ``CONFIG_BOOT_VERSION_CMP_USE_BUILD_NUMBER`` Kconfig option in the bootloader configuration to use the build number while comparing image versions.
To apply the same option for the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr`, enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_VERSION_CMP_USE_BUILD_NUMBER` Kconfig option in the application configuration.

It is recommended to also enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option in the application configuration to make sure that MCUmgr rejects application image updates with invalid start address.
This prevents uploading an update image build for improper slot through the MCUmgr's Simple Management Protocol (SMP).

Serial recovery mode
--------------------

In the :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>` mode, the MCUboot bootloader uses a built-in foreground DFU transport over serial interface through USB.
The application is not involved in the foreground DFU transport, therefore it can be directly overwritten by the bootloader.
Because of that, the configuration with the serial recovery mode requires only a single application slot.

Enable the USB serial recovery DFU using the following configuration options:

* ``CONFIG_MCUBOOT_SERIAL`` - This option enables the serial recovery DFU.
* ``CONFIG_BOOT_SERIAL_CDC_ACM`` - This option enables the serial interface through USB.

  .. note::
    Make sure to enable and properly configure the USB subsystem in the bootloader configuration.
    See :ref:`usb_api` for more information.

If you press the predefined button during the boot, the MCUboot bootloader enters the serial recovery mode instead of booting the application.
The GPIO pin used to trigger the serial recovery mode is configured using Devicetree Specification (DTS).
The pin is configured with the ``mcuboot-button0`` alias.
The ``mcuboot-led0`` alias can be used to define the LED activated in the serial recovery mode.
You must select the ``CONFIG_MCUBOOT_INDICATION_LED`` Kconfig option to enable the LED.
By default, both the GPIO pin and the LED are defined in the board's DTS file.
See :file:`boards/nordic/nrf52833dongle/nrf52833dongle_nrf52833.dts` for an example of board's DTS file used by the nRF Desktop application.

For an example of bootloader Kconfig configuration file defined by the application, see the MCUboot bootloader ``debug`` configuration defined for nRF52833 dongle (:file:`applications/nrf_desktop/configuration/nrf52833dongle_nrf52833/child_image/mcuboot/prj.conf`).

.. note::
  The nRF Desktop devices use either the serial recovery DFU with a single application slot or the background DFU.
  Both mentioned firmware upgrade methods are not used simultaneously by any of the configurations.
  For example, the ``nrf52840dk/nrf52840`` board in ``prj_mcuboot_smp.conf`` uses only the background DFU and does not enable the serial recovery feature.

.. _nrf_desktop_bootloader_background_dfu:

Background Device Firmware Upgrade
**********************************

The nRF Desktop application uses the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu` for the background DFU process.
From the application perspective, the update image transfer during the background DFU process is very similar for all supported configurations:

* MCUboot
* Secure Bootloader (B0)

The firmware update process has the following three stages:

* Update image generation
* Update image transfer
* Update image verification and application image update

These stages are described in the following sections.

At the end of these three stages, the nRF Desktop application will be rebooted with the new firmware package installed.

.. note::
  The background firmware upgrade can also be performed over the Simple Management Protocol (SMP).
  For more details about the DFU procedure over SMP, read the documentation of the following modules:

  * :ref:`nrf_desktop_ble_smp` (supported only with MCUboot bootloader)
  * :ref:`nrf_desktop_dfu_mcumgr` (supported only with MCUboot bootloader)
    The module uses the :ref:`nrf_desktop_dfu_lock` to synchronize non-volatile memory access with other DFU methods.
    Therefore, this module should be used for configurations that enable multiple DFU transports (for example, if a configuration also enables :ref:`nrf_desktop_dfu`).

Update image generation
=======================

The update image is generated in the build directory when building the firmware regardless of the chosen DFU configuration.

MCUboot and B0 bootloaders
--------------------------

The :file:`zephyr/dfu_application.zip` file is used by both B0 and MCUboot bootloader for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
The package contains firmware images along with additional metadata.
If the used bootloader boots the application directly from either slot 0 or slot 1, the host script transfers the update image that can be run from the unused slot.
Otherwise, the image is always uploaded to the slot 1 and then moved to slot 0 by the bootloader before boot.

.. note::
   Make sure to properly configure the bootloader to ensure that the build system generates the :file:`zephyr/dfu_application.zip` archive containing all of the required update images.

Update image transfer
=====================

The update image is transmitted in the background through the :ref:`nrf_desktop_config_channel`.
The configuration channel data is transmitted either through USB or over Bluetooth, using HID feature reports.
This allows the device to be used normally during the whole process (that is, the device does not need to enter any special state in which it becomes non-responsive to the user).

Depending on the side on which the process is handled:

* On the application side, the process is handled by :ref:`nrf_desktop_dfu`.
  See the module documentation for how to enable and configure it.
* On the host side, the process is handled by the :ref:`nrf_desktop_config_channel_script`.
  See the tool documentation for more information about how to execute the background DFU process on the host.

.. _nrf_desktop_image_transfer_over_smp:

Image transfer over SMP
-----------------------

The update image can also be transferred in the background through one of the following modules:

* :ref:`nrf_desktop_ble_smp` (supported only with MCUboot bootloader)
* :ref:`nrf_desktop_dfu_mcumgr` (supported only with MCUboot bootloader)
  The module uses the :ref:`nrf_desktop_dfu_lock` to synchronize non-volatile memory access with other DFU methods.
  Therefore, this module should be used for configurations that enable multiple DFU transports (for example, if a configuration also enables :ref:`nrf_desktop_dfu`).

The `nRF Connect Device Manager`_ application transfers the image update files over the Simple Management Protocol (SMP).

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. include:: /device_guides/working_with_nrf/nrf52/developing.rst
   :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
   :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

.. include:: /device_guides/working_with_nrf/nrf52/developing.rst
   :start-after: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start
   :end-before: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

.. note::
   If the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option is enabled in the application configuration, the device rejects update image upload for the invalid slot.
   It is recommended to enable the option if the application uses MCUboot in the direct-xip mode.

Update image verification and application image update
======================================================

Once the update image transfer is completed, the background DFU process will continue after the device reboot.
If :ref:`nrf_desktop_config_channel_script` is used, the reboot is triggered by the script right after the image transfer completes.

The implementation details of the reboot mechanism for the chosen DFU configuration are described in the following subsections.

MCUboot and B0 bootloaders
--------------------------

For these configuration variants, the :c:func:`sys_reboot` function is called to reboot the device.
After the reboot, the bootloader locates the update image on the update partition of the device.
The image verification process ensures the integrity of the image and checks if its signature is valid.
If verification is successful, the bootloader boots the new version of the application.
Otherwise, the old version is used.

.. _nrf_desktop_bootloader_serial_dfu:

Serial recovery DFU
********************

The serial recovery DFU is a feature of MCUboot and you need to enable it in the bootloader's configuration.
For the configuration details, see the :ref:`nrf_desktop_configuring_mcuboot_bootloader` section.

To start the serial recovery DFU, the device should boot into recovery mode, in which the bootloader is waiting for a new image upload to start.
In the serial recovery DFU mode, the new image is transferred through an USB CDC ACM class instance.
The bootloader overwrites the existing application located on the primary slot with the new application image.
If the transfer is interrupted, the device cannot boot the incomplete application, and the image upload must be performed again.

Once the device enters the serial recovery mode, you can use the :ref:`mcumgr <zephyr:device_mgmt>` to:

* Query information about the present image.
* Upload the new image.
  The :ref:`mcumgr <zephyr:device_mgmt>` uses the :file:`zephyr/app_update.bin` update image file.
  It is generated by the build system when building the firmware.

For example, the following line starts the upload of the new image to the device:

.. code-block:: console

  mcumgr -t 60 --conntype serial --connstring=/dev/ttyACM0 image upload build-nrf52833dongle/zephyr/app_update.bin

The command assumes that ``/dev/ttyACM0`` serial device is used by the MCUboot bootloader for the serial recovery.
