.. _ug_nrf52:
.. _ug_nrf52_developing:

Developing with nRF52 Series
############################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for developing on all nRF52 Series devices and contains board definitions for all development kits and reference design hardware.

See one of the following guides for detailed information about the corresponding nRF52 Series development kit (DK) hardware:

* `nRF52840 DK <nRF52840 DK User Guide_>`_
* `nRF52833 DK <nRF52833 DK User Guide_>`_
* `nRF52 DK <nRF52 DK User Guide_>`_

To get started with your nRF52 Series DK, follow the steps in the :ref:`ug_nrf52_gs` section.
If you are not familiar with the |NCS| and its development environment, see :ref:`installation` and :ref:`configuration_and_build` documentation.

.. _ug_nrf52_developing_ble_fota:
.. fota_upgrades_intro_start

FOTA updates
************

|fota_upgrades_def|
You can also use FOTA updates to replace the application.

.. note::
   For the possibility of introducing an upgradable bootloader, refer to :ref:`ug_bootloader_adding`.

.. fota_upgrades_intro_end

.. fota_upgrades_over_ble_intro_start

FOTA over Bluetooth Low Energy
==============================

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

.. fota_upgrades_over_ble_intro_end

.. fota_upgrades_over_ble_mandatory_mcuboot_start

* Use MCUboot as the upgradable bootloader (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` must be enabled).
  For more information, go to the :doc:`mcuboot:index-ncs` page.

.. fota_upgrades_over_ble_mandatory_mcuboot_end

.. fota_upgrades_over_ble_additional_information_start

If necessary, you can modify any of the implied options or defaulted values introduced by the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

You can either add these Kconfig options to the configuration files of your application or have them inline in a project build command.
Here is an example of how you can build for the :ref:`peripheral_lbs` sample:

.. parsed-literal::
   :class: highlight

    west build -b *build_target* -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y

When you connect to the device after the build has completed and the firmware has been programmed to it, the SMP Service is enabled with the ``UUID 8D53DC1D-1DB7-4CD3-868B-8A527460AA84``.
If you want to add SMP Service to advertising data, refer to the :zephyr:code-sample:`smp-svr`.

.. fota_upgrades_over_ble_additional_information_end

.. _ug_nrf52_developing_ble_fota_mcuboot_direct_xip_mode:

Build configuration additions for MCUboot in the direct-xip mode
----------------------------------------------------------------

.. fota_upgrades_over_ble_mcuboot_direct_xip_information_start

FOTA updates are also supported when MCUboot is in the direct-xip mode.
In this mode, the MCUboot bootloader boots an image directly from a given slot, so the swap operation is not needed.
It can be used either with or without the revert mechanism support.
For more information about the direct-xip mode and the revert mechanism support, go to the Equal slots (direct-xip) section on the :doc:`mcuboot:design` page.

To use MCUboot in the direct-xip mode together with FOTA updates, do the following:

* Enable the ``CONFIG_BOOT_DIRECT_XIP`` Kconfig option in the MCUboot bootloader configuration.
* Enable the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option to inform the application about the chosen MCUboot mode of operation.

See how to build the :ref:`peripheral_lbs` sample with MCUboot in the direct-xip mode when the revert mechanism support is disabled:

.. parsed-literal::
   :class: highlight

    west build -b *build_target* -- -Dmcuboot_CONFIG_BOOT_DIRECT_XIP=y -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y -DCONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP=y

Optionally, if you want to enable the revert mechanism support, complete the following:

* Enable the ``CONFIG_BOOT_DIRECT_XIP_REVERT`` Kconfig option in the MCUboot bootloader configuration.
* Enable the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT` Kconfig option in place of :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` to inform the application about the chosen MCUboot mode of operation.

See how to build the :ref:`peripheral_lbs` sample with MCUboot in direct-xip mode when the revert mechanism support is enabled:

.. parsed-literal::
   :class: highlight

    west build -b *build_target* -- -Dmcuboot_CONFIG_BOOT_DIRECT_XIP=y -Dmcuboot_CONFIG_BOOT_DIRECT_XIP_REVERT=y -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y -DCONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT=y

.. note::
   When building the application for the first time with MCUboot in direct-xip mode and the revert mechanism support, use an additional option ``-DCONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS=\"--confirm\"``.
   This option will mark the application as ``confirmed`` during the image signing process.
   If the application is built without this option, it will fail to boot.
   It must, however, be disabled when building update images.

Both the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` and :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT` Kconfig options automatically enable :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT`, which allows to build application update images for both slots.
To read about the files that are built when the option is enabled, refer to the :ref:`app_build_mcuboot_output` page.

.. fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_start

.. note::
   Support for FOTA updates with MCUboot in the direct-xip mode is available since the following versions of the `nRF Connect Device Manager`_ mobile app:

   * Version ``1.8.0`` on Android.
   * Version ``1.4.0`` on iOS.

.. fota_upgrades_over_ble_mcuboot_direct_xip_nrfcdm_note_end

.. fota_upgrades_over_ble_mcuboot_direct_xip_information_end

.. fota_upgrades_outro_start

Testing steps
-------------

To perform a FOTA update, complete the following steps:

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start

1. Generate the DFU package by building your application with the FOTA support over Bluetooth Low Energy.
   You can find the generated :file:`dfu_application.zip` archive in the following directory :file:`<build_dir>/zephyr`.

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
   #. Initiate the DFU process of transferring the image to the device:

      * If you are using an Android device, select a mode in the dialog window, and tap the :guilabel:`START` button.
      * If you are using an iOS device, tap the selected mode in the pop-up window.

   #. Wait for the DFU to finish and then verify that the application works properly.

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

FOTA update sample
==================

The :zephyr:code-sample:`smp-svr` demonstrates how to set up your project to support FOTA updates.

The sample documentation is from the Zephyr project and is incompatible with the :ref:`ug_multi_image`.
When working in the |NCS| environment, ignore the part of the sample documentation that describes the building and programming steps.
In |NCS|, you can build and program the :zephyr:code-sample:`smp-svr` as any other sample using the following commands:

.. parsed-literal::
   :class: highlight

    west build -b *build_target* -- -DOVERLAY_CONFIG=overlay-bt.conf
    west flash

Make sure to indicate the :file:`overlay-bt.conf` overlay configuration for the Bluetooth transport like in the command example.
This configuration was carefully selected to achieve the maximum possible throughput of the FOTA update transport over Bluetooth with the help of the following features:

* Bluetooth MTU - To increase the packet size of a single Bluetooth packet transmitted over the air (:kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` and others).
* Bluetooth connection parameters - To adaptively change the connection interval and latency on the detection of the SMP service activity (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL`).
* MCUmgr packet reassembly - To allow exchange of large SMP packets (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY`, :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE` and others).

Consider using these features in your project to speed up the FOTA update process.

.. fota_upgrades_outro_end

.. _ug_nrf52_developing_fota_in_mesh:
.. fota_upgrades_bt_mesh_start

FOTA in Bluetooth Mesh
======================

When performing a FOTA update when working with the Bluetooth Mesh protocol, use one of the following DFU methods:

* DFU over Bluetooth Mesh using the Zephyr Bluetooth Mesh DFU subsystem.
* Point-to-point DFU over Bluetooth Low Energy as described in `FOTA over Bluetooth Low Energy`_ above.

For more information about both methods, see :ref:`ug_bt_mesh_fota`.

.. fota_upgrades_bt_mesh_end

.. fota_upgrades_matter_start

FOTA in Matter
==============

To perform a FOTA upgrade when working with the Matter protocol, use one of the following methods:

* DFU over Bluetooth LE using either smartphone or PC command-line tool.
  Both options are similar to `FOTA over Bluetooth Low Energy`_.

  .. note::
     This protocol is not part of the Matter specification.

* DFU over Matter using Matter-compliant BDX protocol and Matter OTA Provider device.
  This option requires an OpenThread Border Router (OTBR) set up either in Docker or on a Raspberry Pi.

For more information about both methods, read the :doc:`matter:nrfconnect_examples_software_update` page in the Matter documentation.

.. fota_upgrades_matter_end

.. fota_upgrades_thread_start

FOTA over Thread
================

:ref:`ug_thread` does not offer a proprietary FOTA method.

.. fota_upgrades_thread_end

.. fota_upgrades_zigbee_start

FOTA over Zigbee
================

You can enable support for FOTA over the Zigbee network using the :ref:`lib_zigbee_fota` library.
For detailed information about how to configure the Zigbee FOTA library for your application, see :ref:`ug_zigbee_configuring_components_ota`.

.. fota_upgrades_zigbee_end

.. _nrf52_computer_testing:

Testing the application with a computer
***************************************

If you have an nRF52 Series DK with the :ref:`peripheral_uart` sample and either a dongle or second Nordic Semiconductor development kit that supports Bluetooth Low Energy, you can test the sample on your computer.
Use the Bluetooth Low Energy app in `nRF Connect for Desktop`_ for testing.

To perform the test, :ref:`connect to the nRF52 Series DK <nrf52_gs_connecting>` and complete the following steps:

1. Connect the dongle or second development kit to a USB port of your computer.
#. Open the Bluetooth Low Energy app.
#. Select the serial port that corresponds to the dongle or second development kit.
   Do not select the kit you want to test.

   .. note::
      If the dongle or second development kit has not been used with the Bluetooth Low Energy app before, you may be asked to update the J-Link firmware and connectivity firmware on the nRF SoC to continue.
      When the nRF SoC has been updated with the correct firmware, the nRF Connect Bluetooth Low Energy app finishes connecting to your device over USB.
      When the connection is established, the device appears in the main view.

#. Click :guilabel:`Start scan`.
#. Find the DK you want to test and click the corresponding :guilabel:`Connect` button.

   The default name for the Peripheral UART sample is *Nordic_UART_Service*.

#. Select the **Universal Asynchronous Receiver/Transmitter (UART)** RX characteristic value.
#. Write ``30 31 32 33 34 35 36 37 38 39`` (the hexadecimal value for the string "0123456789") and click :guilabel:`Write`.

   The data is transmitted over Bluetooth LE from the app to the DK that runs the Peripheral UART sample.
   The terminal emulator connected to the DK then displays ``"0123456789"``.

#. In the terminal emulator, enter any text, for example ``Hello``.

   The data is transmitted to the DK that runs the Peripheral UART sample.
   The **UART TX** characteristic displayed in the Bluetooth Low Energy app changes to the corresponding ASCII value.
   For example, the value for ``Hello`` is ``48 65 6C 6C 6F``.

.. _build_pgm_nrf52:

Building and programming
************************

You can program applications and samples on the nRF52 Series DK after obtaining the corresponding firmware images.

To program applications using the Programmer app from `nRF Connect for Desktop`_, follow the instructions in :ref:`nrf52_gs_installing_application`.
In Step 5, choose the :file:`.hex` file for the application you are programming.

.. _build_pgm_nrf52_vsc:

Building and programming using |VSC|
====================================

Complete the following steps to build and program using the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`nrf/applications/nrf_desktop`

.. |vsc_sample_board_target_line| replace:: see the :ref:`list of supported boards <nrf52_supported_boards>` for the build target corresponding to the nRF52 Series DK you are using

.. include:: ../../../includes/vsc_build_and_run.txt

3. Connect the nRF52 Series DK to the computer with a micro-USB cable, and then turn on the DK.

   **LED1** starts blinking.

#. In |nRFVSC|, click the :guilabel:`Flash` option in the :guilabel:`Actions View`.

   If you have multiple boards connected, you are prompted to pick a device at the top of the screen.

   A small notification banner appears in the bottom right corner of |VSC| to display the progress and confirm when the flash is complete.

.. _build_pgm_nrf52_cmdline:

Building and programming on the command line
============================================

Complete the :ref:`command-line build setup <build_environment_cli>` before you start building |NCS| projects on the command line.

To build and program the source code from the command line, complete the following steps:

1. Open a terminal window.
#. Go to the specific sample or application directory.

   For example, the folder path is ``ncs/nrf/applications/nrf_desktop`` when building the source code for the :ref:`nrf_desktop` application.

#. Make sure that you have the required version of the |NCS| repository by pulling the |NCS| repository `sdk-nrf`_ on GitHub.

   Follow the procedures described in :ref:`dm-wf-get-ncs` and :ref:`dm-wf-update-ncs`.

#. To get the rest of the dependencies, run the ``west update`` command as follows:

   .. code-block:: console

      west update

#. To build the sample or application code, run the ``west build`` command as follows:

   .. parsed-literal::
      :class: highlight

      west build -b *build_target* -d *destination_directory_name*

   For the *build_target* parameter, see the :ref:`list of supported boards <nrf52_supported_boards>` for the build target corresponding to the nRF52 Series DK you are using.

   .. note::

	    You can use the optional *destination_directory_name* parameter to specify the destination directory in the west command.
	    By default, the build files are generated in ``build/zephyr/`` if you have not specified a *destination_directory_name*.

#. Connect the nRF52 Series DK to the computer with a micro-USB cable, and turn on the DK.

   **LED1** starts blinking.

#. Program the sample or application to the device using the following command:

   .. code-block:: console

      west flash

   The device resets and runs the programmed sample or application.

nRF Desktop
===========

The nRF Desktop application is a complete project that integrates Bluetooth LE, see the :ref:`nrf_desktop` application.
You can build it for the nRF Desktop reference hardware or an nRF52840 DK.

The nRF Desktop is a reference design of a HID device that is connected to a host through Bluetooth LE or USB, or both.
This application supports configurations for simple mouse, gaming mouse, keyboard, and USB dongle.
