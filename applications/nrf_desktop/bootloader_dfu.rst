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
  * nRF54 Series

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

To enable the B0 bootloader, select the :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE` Kconfig option in the sysbuild configuration.
This setting automatically enables the :kconfig:option:`SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE` Kconfig option, which generates application binaries for both slots in non-volatile memory.

The B0 bootloader additionally requires enabling the following options:

* In the sysbuild configuration:

  * :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE` - Required for providing the signature key file used by the build system (to sign the application update images) and by the bootloader (to verify the application signature).
    If this Kconfig option does not specify the signature key file, debug signature key files will be used by default.

* In the application configuration:

  * :kconfig:option:`CONFIG_FW_INFO` - Required for providing information about the application versioning.
  * :kconfig:option:`CONFIG_FW_INFO_FIRMWARE_VERSION` - Required for updating the application version.
    The nRF Desktop application with the B0 bootloader configuration builds two application images: one for the S0 slot and the other for the S1 slot.
    To generate the DFU package, update this configuration only in the main application image.
    The ``s1_image`` image will mirror it automatically.

.. note::
    To ensure that update image will boot after a successful DFU image transfer, the update image's version number must be higher than the version number of the application image running on device.
    Otherwise, the update image can be rejected by the bootloader.

.. _nrf_desktop_configuring_mcuboot_bootloader:

Configuring the MCUboot bootloader
==================================

To enable the MCUboot bootloader, select the :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` Kconfig option in the sysbuild configuration.

You must also set the MCUboot private key path (:kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`) in the sysbuild configuration.
The key is used both by the build system (to sign the application update images) and by the bootloader (to verify the application signature using public key derived from the selected private key).
If this Kconfig option is not overwritten in the sysbuild configuration, debug signature key files located in the MCUboot bootloader repository will be used by default.

To select a specific version of the application, change the :file:`VERSION` file in the nRF Desktop main directory.
By default, this change propagates to the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option in the application configuration.
If the nRF Desktop application is configured with the MCUboot in the direct-xip mode, the build system builds two application images: one for the primary slot and the other for the secondary slot, named ``mcuboot_secondary_app``.
You need to update this configuration only in the main application image, as the ``mcuboot_secondary_app`` image mirrors it.

MCUboot bootloader mode
-----------------------

The MCUboot bootloader configuration depends on the selected way of performing image upgrade.
For detailed information about the available MCUboot bootloader modes, see the following sections.

Swap mode
~~~~~~~~~

In the swap mode, the MCUboot bootloader moves the image to the primary slot before booting it.
The swap mode is the image upgrade mode used by default for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.

If the swap mode is used, the application must request a firmware upgrade and confirm the running image.
For this purpose, make sure to enable the :kconfig:option:`CONFIG_IMG_MANAGER` and :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER` Kconfig options in the application configuration.
These options allow the :ref:`nrf_desktop_dfu`, :ref:`nrf_desktop_ble_smp`, and :ref:`nrf_desktop_dfu_mcumgr` to manage the DFU image.

.. note::
   When the MCUboot bootloader is in the swap mode, it can use a secondary image slot located on the external non-volatile memory.
   For details on using external non-volatile memory for the secondary image slot, see the :ref:`nrf_desktop_pm_external_flash` section.

Direct-xip mode
~~~~~~~~~~~~~~~

The direct-xip mode is used for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.
In this mode, the MCUboot bootloader boots an image directly from a given slot, so the swap operation is not needed.
To set the MCUboot mode of operations to the direct-xip mode, enable the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP` Kconfig option in the sysbuild configuration.
This option automatically enables the :kconfig:option:`SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option, which builds the application update images for both slots.
The nRF Desktop application configurations do not use the direct-xip mode with the revert mechanism (:kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`).

The :kconfig:option:`CONFIG_BOOT_DIRECT_XIP` Kconfig option enables MCUboot to run the image directly from both image slots, and it is automatically applied to the bootloader configuration based on the sysbuild configuration.
Similarly, the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option, that informs the application about the MCUboot bootloader's mode, is also applied automatically based on the sysbuild configuration.
When the direct-xip mode is enabled, the application modules that control the DFU transport do not request firmware upgrades or confirm the running image.
In that scenario, the MCUboot bootloader simply boots the image with the higher image version.

By default, the MCUboot bootloader ignores the build number while comparing image versions.
Enable the :kconfig:option:`CONFIG_BOOT_VERSION_CMP_USE_BUILD_NUMBER` Kconfig option in the bootloader configuration to use the build number while comparing image versions.
To apply the same option for the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr`, enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_VERSION_CMP_USE_BUILD_NUMBER` Kconfig option in the application configuration.

It is recommended to also enable the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option in the application configuration to make sure that MCUmgr rejects application image updates with invalid start address.
This prevents uploading an update image build for improper slot through the MCUmgr's Simple Management Protocol (SMP).

.. note::
    When the MCUboot bootloader is in the direct-xip mode, the update image must have a higher version number than the application currently running on the device.
    This ensures that the update image will be booted after a successful DFU image transfer.
    Otherwise, the update image can be rejected by the bootloader.

Serial recovery mode
~~~~~~~~~~~~~~~~~~~~

In the :ref:`USB serial recovery <nrf_desktop_bootloader_serial_dfu>` mode, the MCUboot bootloader uses a built-in foreground DFU transport over serial interface through USB.
The application is not involved in the foreground DFU transport, therefore it can be directly overwritten by the bootloader.
Because of that, the configuration with the serial recovery mode requires only a single application slot.
To set the MCUboot mode of operations to single application slot, enable the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_SINGLE_APP` Kconfig option in the sysbuild configuration.

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

For an example of a bootloader Kconfig configuration file defined by the application, see the MCUboot bootloader ``debug`` configuration defined for nRF52833 dongle (:file:`applications/nrf_desktop/configuration/nrf52833dongle_nrf52833/images/mcuboot/prj.conf`).

.. note::
  The nRF Desktop devices use either the serial recovery DFU with a single application slot or the background DFU.
  Both mentioned firmware upgrade methods are not used simultaneously by any of the configurations.
  For example, the ``nrf52840dk/nrf52840`` board in ``mcuboot_smp`` file suffix uses only the background DFU and does not enable the serial recovery feature.

.. _nrf_desktop_configuring_mcuboot_bootloader_ram_load:

RAM load mode
~~~~~~~~~~~~~

The RAM load mode is used for the :ref:`background DFU <nrf_desktop_bootloader_background_dfu>`.
In this mode, the MCUboot bootloader uses the same NVM partitioning as the direct-xip mode (the dual-bank DFU solution).
Similarly to the direct-xip mode, the RAM load mode also relies on the image version to select the application image slot to be booted.
However, instead of booting the image from the NVM slot, the bootloader in the RAM load mode copies the image from the non-volatile memory (NVM) to the RAM and boots it from there.
The application image is always built for the RAM address space in only one variant.

.. caution::
   The RAM load mode of the MCUboot bootloader is not officially supported in |NCS|.
   However, the mode is available in the |NCS| as the support for this feature has been developed as part of the Zephyr RTOS project.
   This feature is only used in a limited context for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target configuration to improve performance.

You can use the RAM load mode of the MCUboot bootloader to speed up the code execution for the application image, as code execution from the RAM is generally faster than from the NVM.
This can improve the device performance during the activities that require high CPU usage.
As an example, the nRF Desktop application uses the RAM load mode for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to improve HID report rate over USB.

To set the MCUboot mode of operations to the RAM load mode, enable the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_RAM_LOAD` Kconfig option in the sysbuild configuration.

To support the RAM load mode, you must use DTS as the partitioning method, as the Partition Manager (PM) is not supported in this mode.
To satisfy this requirement, disable explicitly the :kconfig:option:`SB_CONFIG_PARTITION_MANAGER` Kconfig option in your sysbuild configuration.
Additionally, you must define the custom memory layout for the RAM in your target board configuration.
Your RAM layout must define the following DTS child nodes as part of the ``cpuapp_sram`` DTS node in the address order listed below:

* ``cpuapp_sram_app_rxm_region`` - This DTS node defines the hard limits for the executable ROM section (with the application image) and must be aligned with the :kconfig:option:`CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_START` and the :kconfig:option:`CONFIG_BOOT_IMAGE_EXECUTABLE_RAM_SIZE` Kconfig options that are set in the MCUboot image configuration.
  This DTS node describes the region in which the build system places the executable ROM section (code) and RAM section (data) of the application image.
  The RAM section is located right after the ROM section - the RAM section may overflow the ``cpuapp_sram_app_rxm_region`` region and spill into the subsequent ``cpuapp_sram_mcuboot_ram_region`` region or even be entirely contained in this subsequent region.
  The ``cpuapp_sram_mcuboot_ram_region`` region can be filled with the RAM section of the application image, as the application and bootloader code cannot run simultaneously.
* ``cpuapp_sram_mcuboot_ram_region`` - This DTS node defines the RAM region for the MCUboot image and must be assigned to the MCUboot image as its chosen SRAM DTS node.

For an example of the custom RAM layout that satisfies these requirements, see the :file:`nrf/applications/nrf_desktop/configuration/nrf54lm20dk_nrf54lm20a_cpuapp/memory_map_ram_load.dtsi` file.
For an example of the RAM layout usage in the MCUboot bootloader image, see the :file:`nrf/applications/nrf_desktop/configuration/nrf54lm20dk_nrf54lm20a_cpuapp/images/mcuboot/app_ram_load.overlay` file.

.. note::
   The application image and the MCUboot image configuration must use the same memory layout.

The RAM load mode requires using the Zephyr retention subsystem with the bootloader information sharing system.
This subsystem is used by the MCUboot bootloader to provide image metadata to the application image.
To enable the Zephyr retention subsystem, enable the following Kconfig options in your application image configuration and the MCUboot image configuration:

* :kconfig:option:`CONFIG_RETENTION`
* :kconfig:option:`CONFIG_RETAINED_MEM`
* :kconfig:option:`CONFIG_RETAINED_MEM_ZEPHYR_RAM`

In the application image configuration, enable the following Kconfig options:

* :kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO`
* :kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO_TYPE_MCUBOOT`
* :kconfig:option:`CONFIG_RETENTION_BOOTLOADER_INFO_OUTPUT_FUNCTION`

In the MCUboot image configuration, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BOOT_SHARE_DATA`
* :kconfig:option:`CONFIG_BOOT_SHARE_DATA_BOOTINFO`
* :kconfig:option:`CONFIG_BOOT_SHARE_BACKEND_RETENTION`

The Zephyr retention subsystem requires the retention partition to be defined in the devicetree.
For an example of the retention partition definition, see the :file:`nrf/applications/nrf_desktop/configuration/nrf54lm20dk_nrf54lm20a_cpuapp/memory_map_ram_load.dtsi` file.
You must also assign the retention partition to the chosen DTS node ``zephyr,bootloader-info`` in both the application image configuration and the MCUboot image configuration.

.. note::
   If your board target uses the Key Management Unit (KMU) feature (:kconfig:option:`CONFIG_CRACEN_LIB_KMU`), you must additionally define the ``nrf_kmu_reserved_push_area`` DTS node in your custom memory layout.
   Place this RAM section at the very beginning of the physical RAM due to the dependency on the ``nrfutil device`` tool and its KMU provisioning functionality.
   For an example of the ``nrf_kmu_reserved_push_area`` DTS node definition, see the :file:`nrf/applications/nrf_desktop/configuration/nrf54lm20dk_nrf54lm20a_cpuapp/memory_map_ram_load.dtsi` file.

   The KMU feature (:kconfig:option:`CONFIG_CRACEN_LIB_KMU`) is enabled by default for the nRF54L series.

.. note::
   The RAM load mode of the MCUboot bootloader is not yet integrated in the :ref:`nrf_desktop_dfu_mcumgr`.

MCUboot bootloader on nRF54
---------------------------

The nRF54 SoC Series enhances security and reduces boot times by using hardware cryptography in the MCUboot immutable bootloader.
The |NCS| allows using hardware cryptography for ED25519 signature (:kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519`) on the nRF54 SoC Series.

You can enhance security further by enabling the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE` sysbuild Kconfig option.
This option enables using a pure signature of the image, verifying the signature directly on the image, rather than on its hash.
However, you cannot use this option if the secondary image slot uses external memory.

Bootloader features that are specific to the nRF54L and the nRF54H series are listed in their respective subsections below.

MCUboot bootloader features specific to nRF54L series
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The nRF54L devices support the use of the Key Management Unit (KMU) to store keys for signature verification instead of compiling key data into the MCUboot bootloader image.
To use KMU in the MCUboot bootloader, enable the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU` sysbuild Kconfig option.
You must also make sure to provision the public key to your target device before running the firmware.
See the :ref:`ug_nrf54l_developing_provision_kmu` documentation for details.

.. note::
    To use automatic provisioning, enable the :kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` sysbuild Kconfig option.
    This option enables generating a default :file:`keyfile.json` file during the build process based on the input file provided by the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` sysbuild Kconfig option.
    The automatic provisioning is only performed if the west flash command is executed with the ``--erase`` or ``--recover`` flag.

MCUboot bootloader features specific to nRF54H series
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On the nRF54H devices, the MCUboot direct-xip mode uses by default a merged image slot that combines both application and radio core images.
The merged binary size is the sum of the application image, the radio core image, and the padding between them.
The merged image slot configuration is indicated by the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY` sysbuild Kconfig option.

.. note::
   Because padding is added between the application and radio core images, the merged binary ends up including the unused NVM from the application image partition.
   To limit the impact of this unused NVM on the merged binary size, you can tailor the partition size of the application image to the image size.
   If more space is needed for the application image in the future, you can increase the partition size of the application image using DFU.

The MCUboot bootloader on the nRF54H devices requires additional configuration to properly support Suspend to RAM (S2RAM) feature.
For further details, see the :ref:`ug_nrf54h20_pm_optimizations_bootloader` documentation.

For more general information regarding the MCUboot bootloader on the nRF54H devices, see the :ref:`ug_nrf54h20_mcuboot_dfu` documentation.

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
  * :ref:`nrf_desktop_dfu_mcumgr`
    The module uses the :ref:`nrf_desktop_dfu_lock` to synchronize non-volatile memory access with other DFU methods.
    Therefore, this module should be used for configurations that enable multiple DFU transports (for example, if a configuration also enables :ref:`nrf_desktop_dfu`).

Update image generation
=======================

The update image is generated in the build directory when building the firmware regardless of the chosen DFU configuration.

MCUboot and B0 bootloaders
--------------------------

The :file:`<build_dir>/dfu_application.zip` file is used by both B0 and MCUboot bootloader for the background DFU through the :ref:`nrf_desktop_config_channel` and :ref:`nrf_desktop_dfu`.
The package contains firmware images along with additional metadata.
If the used bootloader boots the application directly from either slot 0 or slot 1, the host script transfers the update image that can be run from the unused slot.
Otherwise, the image is always uploaded to the slot 1 and then moved to slot 0 by the bootloader before boot.

.. note::
   Make sure to properly configure the sysbuild to ensure that the build system generates the :file:`<build_dir>/dfu_application.zip` archive containing all of the required update images.

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
* :ref:`nrf_desktop_dfu_mcumgr`
  The module uses the :ref:`nrf_desktop_dfu_lock` to synchronize non-volatile memory access with other DFU methods.
  Therefore, this module should be used for configurations that enable multiple DFU transports (for example, if a configuration also enables :ref:`nrf_desktop_dfu`).

The `nRF Connect Device Manager`_ application transfers the image update files over the Simple Management Protocol (SMP).

.. note::
   If your DFU target is not paired through Bluetooth with your Android device, the DFU procedure automatically triggers the pairing procedure.
   The nRF Desktop configurations with the DFU support over SMP require encryption for operations on the Bluetooth GATT SMP service (see the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_ENCRYPT` Kconfig option).

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. tabs::

   .. tab:: MCUboot

      .. include:: /app_dev/device_guides/nrf52/fota_update.rst
         :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
         :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

      .. include:: /app_dev/device_guides/nrf52/fota_update.rst
         :start-after: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start
         :end-before: fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

      .. note::
         When the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option is enabled in the application configuration, the device rejects the update image upload for the invalid slot.
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
In the serial recovery DFU mode, the new image is transferred through a USB CDC ACM class instance.
The bootloader overwrites the existing application located on the primary slot with the new application image.
If the transfer is interrupted, the device cannot boot the incomplete application, and the image upload must be performed again.

Once the device enters the serial recovery mode, you can use the :ref:`mcumgr <zephyr:device_mgmt>` to do the following:

* Query information about the present image.
* Upload the new image.
  The :ref:`mcumgr <zephyr:device_mgmt>` uses the :file:`<build_dir>/nrf_desktop/zephyr/zephyr.signed.bin` update image file.
  It is generated by the build system when building the firmware.

For example, the following line starts the upload of the new image to the device:

.. code-block:: console

  mcumgr -t 60 --conntype serial --connstring=/dev/ttyACM0 image upload build/nrf_desktop/zephyr/zephyr.signed.bin

The command assumes that ``/dev/ttyACM0`` serial device is used by the MCUboot bootloader for the serial recovery.
