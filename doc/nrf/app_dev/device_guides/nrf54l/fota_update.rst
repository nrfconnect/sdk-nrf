.. _ug_nrf54l_developing_ble_fota:

FOTA updates on nRF54L Series devices
#####################################

.. contents::
   :local:
   :depth: 2

.. fota_upgrades_intro_start

|fota_upgrades_def|
You can also use FOTA updates to replace the application.
See the :ref:`app_dfu` page for general Device Firmware Update (DFU) information, such as supported methods for sending and receiving updates on the device.
Currently, FOTA updates are supported only for the application core.

.. note::
   For more information about introducing immutable MCUboot bootloader, refer to :ref:`ug_bootloader_adding_sysbuild_immutable_mcuboot`.

.. fota_upgrades_intro_end

.. _ug_nrf54l_developing_ble_fota_steps:

FOTA over Bluetooth Low Energy
******************************

.. fota_upgrades_over_ble_intro_start

FOTA updates are supported using MCUmgr's Simple Management Protocol (SMP) over Bluetooth®.
The application acts as a GATT server and allows the connected Bluetooth Central device to perform a firmware update.
To use FOTA over Bluetooth LE, samples must support Bluetooth peripheral role (:kconfig:option:`CONFIG_BT_PERIPHERAL`).

By default, the application supports SMP handlers related to:

* Image management
* Operating System (OS) management, which is used to reboot the device after completing firmware uploads
* Erasing the settings partition to prevent booting a new application with incompatible content that was written by the previous application

To enable support for FOTA updates, do the following:

* Enable the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option, which implies configuration of the following:

   * All of the SMP command handlers mentioned in the :ref:`ug_nrf54l_developing_ble_fota_steps` section
   * SMP BT reassembly feature
   * The :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` Kconfig option, which automatically extends the Bluetooth buffers to speed up the FOTA transfer over Bluetooth while also increasing RAM usage

    .. note::
       The :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option enables the device to use MCUmgr for performing firmware over-the-air (FOTA) updates using Bluetooth LE.
       It can be used along with other samples, and is meant as a demonstration of the default DFU configuration over Bluetooth.

    .. note::
       To prevent an unauthenticated access to the device over SMP, it is strongly recommended to enable the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN` option.
       This will enforce a remote device to initiate a pairing request before accessing SMP characteristics.

.. fota_upgrades_over_ble_intro_end

.. fota_upgrades_over_ble_mandatory_mcuboot_start

* Enable the ``SB_CONFIG_BOOTLOADER_MCUBOOT`` option to use MCUboot as a bootloader.
  You can do this by, for example, setting the option in the :file:`sysbuild.conf` file.
  For more information, go to the :ref:`ug_bootloader_adding_sysbuild_immutable_mcuboot` page.

.. fota_upgrades_over_ble_mandatory_mcuboot_end

.. fota_upgrades_over_ble_additional_information_start

If necessary, you can modify any of the implied options or defaulted values introduced by the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

You can either add these Kconfig options to the configuration files of your application or have them inline in a project build command.
Here is an example of how you can build for the :ref:`peripheral_lbs` sample:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y

When you connect to the device after the build has completed and the firmware has been programmed to it, the SMP Service is enabled with the ``UUID 8D53DC1D-1DB7-4CD3-868B-8A527460AA84``.
If you want to add SMP Service to advertising data, refer to the :zephyr:code-sample:`smp-svr`.

.. fota_upgrades_over_ble_additional_information_end

.. _ug_nrf54l_developing_ble_fota_steps_testing:

Testing steps
=============

.. fota_upgrades_outro_start

To perform a FOTA update, complete the following steps:

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start

1. Locate the :file:`dfu_application.zip` archive in the build directory.
   The archive is automatically generated after adding the DFU configuration and building your project.

   .. note::
      For each image included in the DFU-generated package, use a higher version number than your currently active firmware.
      You can do this by modifying the VERSION file in the application directory or by making changes to the application code.
      For the semantic versioning, modify the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.
      For the monotonic counter (HW), modify the ``SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE`` Kconfig option.
      Otherwise, the DFU target may reject the FOTA process due to a downgrade prevention mechanism.

#. Download the :file:`dfu_application.zip` archive to your mobile phone.
   See :ref:`app_build_output_files` for more information about the contents of update archive.

   .. note::
      nRF Connect for Desktop does not currently support the FOTA process.

#. Use the `nRF Connect Device Manager`_ mobile app to update your device with the new firmware.

   a. Ensure that you can access the :file:`dfu_application.zip` archive from your phone or tablet.
   #. In the mobile app, scan and select the device to update.
   #. Switch to the :guilabel:`Image` tab.
   #. Tap the :guilabel:`SELECT FILE` button and select the :file:`dfu_application.zip` archive.
   #. Tap the :guilabel:`START` button.
   #. Initiate the DFU process of transferring the image to the device:

      * If you are using an Android phone or tablet, select a mode in the dialog window, and tap the :guilabel:`START` button.
      * If you are using an iOS device, tap the selected mode in the pop-up window.

      .. note::
         For samples using random HCI identities, the Test and Confirm mode should not be used.

   #. Wait for the DFU to finish and then verify that the new application works properly by observing the new device name visible in the Device Manager app.

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

.. fota_upgrades_outro_end

FOTA update sample
******************

.. fota_upgrades_update_start

The :zephyr:code-sample:`smp-svr` demonstrates how to set up your project to support FOTA updates.

When working in the |NCS| environment, ignore the part of the sample documentation that describes the building and programming steps.
In |NCS|, you can build and program the :zephyr:code-sample:`smp-svr` as any other sample using the following commands:

.. tabs::

    .. group-tab:: nRF54L15 SoCs

        .. parsed-literal::
           :class: highlight

            west build -b *board_name*/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE=overlay-bt.conf
            west flash

    .. group-tab:: nRF54L15 SoCs with HW cryptography support

        .. parsed-literal::
           :class: highlight

            west build -b *board_name*/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE=overlay-bt.conf -DSB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519=y -DSB_CONFIG_BOOT_SIGNATURE_TYPE_PURE=y -Dmcuboot_CONFIG_PM_PARTITION_SIZE_MCUBOOT=0x10000 -DSB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU=y
            west flash


    .. group-tab:: nRF54L15 DK with SPI Flash as update image bank

        .. parsed-literal::
           :class: highlight

            west build -b nrf54l15dk/nrf54l15/cpuapp -T sample.mcumgr.smp_svr.bt.nrf54l15dk.ext_flash
            west flash

Make sure to indicate the :file:`overlay-bt.conf` overlay configuration for the Bluetooth transport like in the command example.
This configuration was carefully selected to achieve the maximum possible throughput of the FOTA update transport over Bluetooth with the help of the following features:

* Bluetooth MTU - To increase the packet size of a single Bluetooth packet transmitted over the air (:kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` and others).
* Bluetooth connection parameters - To adaptively change the connection interval and latency on the detection of the SMP service activity (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL`).
* MCUmgr packet reassembly - To allow exchange of large SMP packets (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY`, :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE` and others).

Consider using these features in your project to speed up the FOTA update process.

.. fota_upgrades_update_end

.. _ug_nrf54l_developing_ble_fota_mcuboot_kmu:

Provisioning of keys for Hardware KMU
*************************************

In case of FOTA implementations using the MCUboot bootloader, which includes hardware cryptography and KMU, you must complete key provisioning before booting any application.
Refer to :ref:`ug_nrf54l_developing_provision_kmu` for detailed description.

.. _ug_nrf54l_developing_ble_fota_mcuboot_direct_xip_mode:

Build configuration additions for MCUboot in the direct-xip mode
****************************************************************

.. fota_upgrades_over_ble_mcuboot_direct_xip_information_start

FOTA updates are also supported when MCUboot is in the direct-xip mode.
In this mode, the MCUboot bootloader boots an image directly from a given slot, so the swap operation is not needed.
It can be used either with or without the revert mechanism support.
For more information about the direct-xip mode and the revert mechanism support, go to the Equal slots (direct-xip) section on the :doc:`mcuboot:design` page.

.. note::
   direct-xip mode can not be combined with the image encryption.

.. note::
   building a project with direct-xip for nRF54l15 SoC target mode requires static partition manager file for partitioning, see known issues.

To use MCUboot in the direct-xip mode together with FOTA updates, do the following:

* Enable the ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`` Kconfig option in sysbuild.

See how to build the :ref:`peripheral_lbs` sample with MCUboot in the direct-xip mode when the revert mechanism support is disabled:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_MCUBOOT_MODE_DIRECT_XIP=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y

Optionally, if you want to enable the revert mechanism support, complete the following:

* Enable the ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`` Kconfig option in sysbuild instead of ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP``.

See how to build the :ref:`peripheral_lbs` sample with MCUboot in direct-xip mode when the revert mechanism support is enabled:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DSB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y

.. note::
   When building the application with MCUboot in direct-xip mode with revert mechanism support, the signed image intended for flashing is automatically marked as confirmed (Pre-confirmation).
   Without this configuration, the application will fail to boot.
   Confirmation mark should not, however, be added when building update images.

Both the ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`` and ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`` Kconfig options automatically build application update images for both slots.
To read about the files that are built when the option is enabled, refer to the :ref:`app_build_mcuboot_output` page.

.. fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start

.. note::
   Support for FOTA updates with MCUboot in the direct-xip mode is available since the following versions of the `nRF Connect Device Manager`_ mobile app:

   * Version ``1.8.0`` on Android.
   * Version ``1.4.0`` on iOS.

.. fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

.. fota_upgrades_over_ble_mcuboot_direct_xip_information_end
