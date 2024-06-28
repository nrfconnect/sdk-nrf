.. _ug_nrf52_developing_ble_fota:

FOTA updates on nRF52 Series devices
####################################

.. contents::
   :local:
   :depth: 2

.. fota_upgrades_intro_start

|fota_upgrades_def|
You can also use FOTA updates to replace the application.
See the :ref:`app_dfu` page for general Device Firmware Update (DFU) information, such as supported methods for sending and receiving updates on the device.

.. note::
   For the possibility of introducing an upgradable bootloader, refer to :ref:`ug_bootloader_adding`.

.. fota_upgrades_intro_end

FOTA over Bluetooth Low Energy
******************************

.. fota_upgrades_over_ble_intro_start

FOTA updates are supported using MCUmgr's Simple Management Protocol (SMP) over Bluetooth.
The application acts as a GATT server and allows the connected Bluetooth Central device to perform a firmware update.
To use FOTA over Bluetooth LE, samples must support Bluetooth peripheral role (:kconfig:option:`CONFIG_BT_PERIPHERAL`).

The application supports SMP handlers related to:

* Image management.
* Operating System (OS) management used to reboot the device after the firmware upload is complete.
* Erasing settings partition used to ensure that a new application is not booted with incompatible content in the settings partition written by the previous application.

To enable support for FOTA updates, do the following:

* Enable the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option, which implies configuration of the following:

   * All of the SMP command handlers mentioned in the previous paragraph.
   * SMP BT reassembly feature.
   * The :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` Kconfig option automatically extends the Bluetooth buffers, which allows to speed up the FOTA transfer over Bluetooth, but also increases RAM usage.

.. note::
   The :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option can be used devices to enable MCUmgr to perform firmware over-the-air (FOTA) updates using Bluetooth LE.
   It can be used along with other samples, and is meant as a demonstration of the default DFU configuration over Bluetooth.

.. fota_upgrades_over_ble_intro_end

.. fota_upgrades_over_ble_mandatory_mcuboot_start

* Use MCUboot as the upgradable bootloader (``SB_CONFIG_BOOTLOADER_MCUBOOT`` must be enabled).
  For more information, go to the :ref:`ug_bootloader_adding` page.

.. fota_upgrades_over_ble_mandatory_mcuboot_end

.. fota_upgrades_over_ble_additional_information_start

If necessary, you can modify any of the implied options or defaulted values introduced by the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

You can either add these Kconfig options to the configuration files of your application or have them inline in a project build command.
Here is an example of how you can build for the :ref:`peripheral_lbs` sample:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y

When you connect to the device after the build has completed and the firmware has been programmed to it, the SMP Service is enabled with the ``UUID 8D53DC1D-1DB7-4CD3-868B-8A527460AA84``.
If you want to add SMP Service to advertising data, refer to the :zephyr:code-sample:`smp-svr`.

.. fota_upgrades_over_ble_additional_information_end

.. _ug_nrf52_developing_ble_fota_mcuboot_direct_xip_mode:

Build configuration additions for MCUboot in the direct-xip mode
================================================================

.. fota_upgrades_over_ble_mcuboot_direct_xip_information_start

FOTA updates are also supported when MCUboot is in the direct-xip mode.
In this mode, the MCUboot bootloader boots an image directly from a given slot, so the swap operation is not needed.
It can be used either with or without the revert mechanism support.
For more information about the direct-xip mode and the revert mechanism support, go to the Equal slots (direct-xip) section on the :doc:`mcuboot:design` page.

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
   When building the application for the first time with MCUboot in direct-xip mode and the revert mechanism support, use an additional option ``-DCONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS=\"--confirm\"``.
   This option will mark the application as ``confirmed`` during the image signing process.
   If the application is built without this option, it will fail to boot.
   It must, however, be disabled when building update images.

Both the ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP`` and ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT`` Kconfig options automatically build application update images for both slots.
To read about the files that are built when the option is enabled, refer to the :ref:`app_build_mcuboot_output` page.

.. fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start

.. note::
   Support for FOTA updates with MCUboot in the direct-xip mode is available since the following versions of the `nRF Connect Device Manager`_ mobile app:

   * Version ``1.8.0`` on Android.
   * Version ``1.4.0`` on iOS.

.. fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

.. fota_upgrades_over_ble_mcuboot_direct_xip_information_end

Testing steps
=============

.. fota_upgrades_outro_start

To perform a FOTA update, complete the following steps:

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start

1. Generate the DFU package by building your application with the FOTA support over Bluetooth Low Energy.
   You can find the generated :file:`dfu_application.zip` archive in the build directory.

   .. note::
      For each image included in the DFU-generated package, use a higher version number than your currently active firmware.
      Otherwise, the DFU target may reject the FOTA process due to a downgrade prevention mechanism.

#. Download the :file:`dfu_application.zip` archive to your device.
   See :ref:`app_build_output_files` for more information about the contents of update archive.

   .. note::
      nRF Connect for Desktop does not currently support the FOTA process.

#. Use the `nRF Connect Device Manager`_ mobile app to update your device with the new firmware.

   a. Ensure that you can access the :file:`dfu_application.zip` archive from your phone or tablet.
   #. In the mobile app, scan and select the device to update.
   #. Switch to the :guilabel:`Image` tab.
   #. Tap the :guilabel:`SELECT FILE` button and select the :file:`dfu_application.zip` archive.
   #. Tap the :guilabel:`START` button.

      .. note::
         When performing a FOTA update with the iOS app for samples using random HCI identities, ensure that the :guilabel:`Erase application settings` option is deselected before starting the procedure.
         Otherwise, the new image will boot with random IDs, causing communication issues between the app and the device.

   #. Initiate the DFU process of transferring the image to the device:

      * If you are using an Android device, select a mode in the dialog window, and tap the :guilabel:`START` button.
      * If you are using an iOS device, tap the selected mode in the pop-up window.

      .. note::
         For samples using random HCI identities, the Test and Confirm mode should not be used.

   #. Wait for the DFU to finish and then verify that the application works properly.

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

.. fota_upgrades_outro_end

FOTA update sample
******************

.. fota_upgrades_update_start

The :zephyr:code-sample:`smp-svr` demonstrates how to set up your project to support FOTA updates.

The sample documentation is from the Zephyr project and is incompatible with the :ref:`ug_multi_image`.
When working in the |NCS| environment, ignore the part of the sample documentation that describes the building and programming steps.
In |NCS|, you can build and program the :zephyr:code-sample:`smp-svr` as any other sample using the following commands:

.. tabs::

    .. group-tab:: nRF5340 SoCs

        .. parsed-literal::
           :class: highlight

            west build -b *board_target* -- -DEXTRA_CONF_FILE=overlay-bt.conf -DSB_CONFG_NETCORE_HCI_IPC=y
            west flash

    .. group-tab:: nRF52 SoCs

        .. parsed-literal::
           :class: highlight

            west build -b *board_target* -- -DEXTRA_CONF_FILE=overlay-bt.conf
            west flash

Make sure to indicate the :file:`overlay-bt.conf` overlay configuration for the Bluetooth transport like in the command example.
This configuration was carefully selected to achieve the maximum possible throughput of the FOTA update transport over Bluetooth with the help of the following features:

* Bluetooth MTU - To increase the packet size of a single Bluetooth packet transmitted over the air (:kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` and others).
* Bluetooth connection parameters - To adaptively change the connection interval and latency on the detection of the SMP service activity (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL`).
* MCUmgr packet reassembly - To allow exchange of large SMP packets (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY`, :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE` and others).

Consider using these features in your project to speed up the FOTA update process.

.. fota_upgrades_update_end

.. _ug_nrf52_developing_fota_in_mesh:

FOTA in Bluetooth Mesh
**********************

.. fota_upgrades_bt_mesh_start

When performing a FOTA update when working with the Bluetooth Mesh protocol, use one of the following DFU methods:

* DFU over Bluetooth Mesh using the Zephyr Bluetooth Mesh DFU subsystem.
* Point-to-point DFU over Bluetooth Low Energy as described in `FOTA over Bluetooth Low Energy`_ above.
  The Bluetooth Mesh samples use random HCI identities.
  See the related notes in the `Testing steps`_ section.

For more information about both methods, see :ref:`ug_bt_mesh_fota`.

.. fota_upgrades_bt_mesh_end

FOTA in Matter
**************

.. fota_upgrades_matter_start

To perform a FOTA upgrade when working with the Matter protocol, use one of the following methods:

* DFU over Bluetooth LE using either smartphone or PC command-line tool.
  Both options are similar to `FOTA over Bluetooth Low Energy`_.

  .. note::
     This protocol is not part of the Matter specification.

* DFU over Matter using Matter-compliant BDX protocol and Matter OTA Provider device.
  This option requires an OpenThread Border Router (OTBR) set up either in Docker or on a Raspberry Pi.

For more information about both methods, read the :doc:`matter:nrfconnect_examples_software_update` page in the Matter documentation.

.. fota_upgrades_matter_end

FOTA over Thread
****************

.. fota_upgrades_thread_start

:ref:`ug_thread` does not offer a proprietary FOTA method.

.. fota_upgrades_thread_end

FOTA over Zigbee
****************

.. fota_upgrades_zigbee_start

You can enable support for FOTA over the Zigbee network using the :ref:`lib_zigbee_fota` library.
For detailed information about how to configure the Zigbee FOTA library for your application, see :ref:`ug_zigbee_configuring_components_ota`.

.. fota_upgrades_zigbee_end
