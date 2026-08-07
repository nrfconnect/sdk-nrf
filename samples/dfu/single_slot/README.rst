.. _single_slot_sample:

Single-slot DFU with MCUboot
############################

.. contents::
   :local:
   :depth: 2

The Single-slot DFU with MCUboot sample demonstrates how to maximize the available space for the application using MCUboot.
You can do this by using the firmware loader mode (single-slot layout) in MCUboot.
Both MCUboot and the firmware loader images are configured to achieve minimal size, leaving more space available for the application.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample contains a simple main application with no firmware update capabilities.
The firmware loader image is used to perform the DFU over Bluetooth® Low Energy or USB CDC ACM serial.
See :ref:`ug_bootloader_using_firmware_loader_mode` and :ref:`fw_loader_usb_mcumgr` for more details.

This sample employs one of the following firmware loader images, built as sysbuild child images:

* The :ref:`fw_loader_ble_mcumgr` firmware loader image, which uses the Simple Management Protocol (SMP) over Bluetooth LE.
* The :ref:`fw_loader_usb_mcumgr` firmware loader image, which uses the USB CDC ACM serial.

This sample can employ the buttonless DFU feature when the application can enter firmware loader mode without the need to hold a button during reset.
This is achieved by enabling the SMP MCUmgr group reset command with the boot mode parameter, which must be set to ``1`` to enter firmware loader mode.

.. _fw_loader_ble_mcumgr:

BLE MCUmgr firmware loader image
================================

The BLE MCUmgr firmware loader image provides the minimal and recommended configuration for the firmware loader application.
It uses code from Zephyr's :zephyr:code-sample:`smp-svr` sample to enable the Bluetooth LE SMP server functionality.

The firmware loader advertises itself as *FW loader* over Bluetooth LE and accepts SMP commands for:

* Image upload and management
* Device information queries
* Bootloader information

The image source and configuration files are located in :file:`samples/dfu/single_slot/sysbuild/ble_mcumgr/`.
If your application uses a custom memory layout, provide a board-specific overlay in that directory.
For reference, see :file:`samples/dfu/single_slot/sysbuild/ble_mcumgr/boards/nrf54l15dk_nrf54l15_cpuapp.overlay`.

This image is selected by default when the following sysbuild options are set in :file:`sysbuild.conf`:

* :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`
* :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`
* :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_BLE_MCUMGR`

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/single_slot`

By default, the sample builds with the :ref:`fw_loader_ble_mcumgr` firmware loader image.
To build with the :ref:`fw_loader_usb_mcumgr` firmware loader image, append ``FILE_SUFFIX=usb`` to the build command.
To build the sample for the :zephyr:board:`nrf54lm20dk` with the :ref:`fw_loader_usb_mcumgr` firmware loader image and USB buttonless DFU support, append ``FILE_SUFFIX=usb_enter`` to the build command.
To build the sample for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` target with the :ref:`fw_loader_usb_mcumgr` firmware loader image and USB buttonless DFU support, append ``FILE_SUFFIX=usb_enter_dongle`` to the build command.

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, perform the following steps:

1. |connect_terminal_specific|
#. Reset the development kit and observe the output on the terminal::

      Starting single_slot sample
      build time: <BUILD TIME>

#. Build a second version of the sample.
#. Enter the firmware loader mode by holding the **Button 0** on your development kit while you reset the device, or by sending the reset command with the boot-mode parameter set to '1' through MCUmgr.

   a. Bluetooth firmware loader:

      Open the `nRF Connect Device Manager`_ mobile app to perform DFU over Bluetooth® LE.

      * The firmware loader advertises itself as *FW loader* and accepts MCUmgr image upload.
      * Send the generated update package for the second version of the sample.
        See :ref:`ug_nrf54l_developing_ble_fota_steps_testing` for details on how to use the mobile app to perform the DFU.

   b. USB CDC ACM serial firmware loader:

      Use `nRF Util mcu-manager serial` to perform DFU over serial port.

      * Send the generated update package for the second version of the sample.
        See `nRF Util mcu-manager serial`_ for details on how to use the command to perform the DFU.

#. Verify that the printed build time corresponds to the new version once the update is complete and the device reboots into the main application.

Dependencies
************

This sample uses the following |NCS| components:

* :ref:`MCUboot <mcuboot_index_ncs>`
* :ref:`fw_loader_ble_mcumgr` (as the firmware loader image built by sysbuild)
* :ref:`fw_loader_usb_mcumgr` (as the firmware loader image built by sysbuild)
