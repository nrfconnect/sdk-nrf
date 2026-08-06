.. _single_slot_sample:
.. _firmware_loader_entrance:

.. ncs-sample::
   :title: Single-slot DFU with MCUboot

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

This sample employs one of alternatives:
* The :ref:`fw_loader_ble_mcumgr` firmware loader image, which uses the Simple Management Protocol (SMP) over Bluetooth LE.
* The :ref:`fw_loader_usb_mcumgr` firmware loader image, which uses the USB CDC ACM serial.

This sample can employ the buttonless DFU feature when the application can enter firmware loader mode without the need to hold a button during reset.
This is achieved by enabling the SMP MCUmgr group reset command with the boot mode parameter, which must be set to ``1`` to enter firmware loader mode.

Entering the firmware loader
============================

The sample demonstrates the following methods of entering the firmware loader image:

.. list-table:: Firmware loader entrance methods
   :header-rows: 1

   * - Method
     - Build variant
     - Description
   * - GPIO
     - Default
     - MCUboot reads the state of **Button 0** at boot time, as enabled by the :kconfig:option:`CONFIG_BOOT_FIRMWARE_LOADER_ENTRANCE_GPIO` Kconfig option.
   * - Buttonless over Bluetooth LE
     - ``ble_enter``
     - The main application exposes an SMP server over Bluetooth LE and requests the firmware loader through the MCUmgr reset command with the boot mode parameter.
   * - Buttonless over USB
     - ``usb_enter``
     - The main application exposes an SMP server over USB CDC ACM serial and requests the firmware loader through the MCUmgr reset command with the boot mode parameter.

The buttonless variants build with the :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER_BOOT_MODE_ENTRANCE` sysbuild option, which makes MCUboot read the requested boot mode from the retention subsystem instead of a GPIO.

.. note::
   Pressing **Button 0** without resetting the device does not enter the firmware loader mode.
   The button must be held during the reset for MCUboot to detect the entrance request.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/single_slot`

By default, the sample builds with the :ref:`fw_loader_ble_mcumgr` firmware loader image.
To build the sample with the :ref:`fw_loader_ble_mcumgr` firmware loader image and Bluetooth LE buttonless DFU support, append ``FILE_SUFFIX=ble_enter`` to the build command.
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

      * When built with ``FILE_SUFFIX=ble_enter``, the main application advertises itself as *single_slot* and accepts the MCUmgr reset command with the boot-mode parameter, which reboots the device into the firmware loader.
      * The firmware loader advertises itself as *FW loader* and accepts MCUmgr image upload.
      * Send the generated update package for the second version of the sample.
        See :ref:`ug_nrf54l_developing_ble_fota_steps_testing` for details on how to use the mobile app to perform the DFU.

   b. USB CDC ACM serial firmware loader:

      Use `nrfutil mcu-manager serial`_ command to perform DFU over serial port.

      * Send the generated update package for the second version of the sample.

#. Verify that the printed build time corresponds to the new version once the update is complete and the device reboots into the main application.

Dependencies
************

This sample uses the following |NCS| components:

* :ref:`MCUboot <mcuboot_index_ncs>`
* :ref:`fw_loader_ble_mcumgr` (as the firmware loader image built by sysbuild)
* :ref:`fw_loader_usb_mcumgr` (as the firmware loader image built by sysbuild)
