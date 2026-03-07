.. _fw_loader_usb_mcumgr:

Minimal USB virtual serial port SMP firmware loader
###################################################

.. contents::
   :local:
   :depth: 2

This sample provides the minimal and recommended configuration for the Firmware Loader application to run on the :zephyr:board:`nrf54lm20dk`.
It uses code from Zephyr's :zephyr:code-sample:`smp-svr` sample to enable the USB serial Simple Management Protocol (SMP) server functionality.

This sample is not intended to function as a standalone sample.
Instead, it serves as a starting point for developing a custom Firmware Loader application that works with the MCUboot bootloader.

.. _fw_loader_usb_minimal_overview:

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. _fw_loader_usb_minimal_build_run:

Overview
********

The application uses the SMP over Bluetooth LE to perform firmware updates and manage device operations.
It is optimized for minimal memory usage and contains only the essential functionalities required for firmware loading.

The firmware loader over USB CDC ACM virtual serial port accepts SMP commands for:

* Image upload and management
* Device information queries
* Bootloader information

.. _fw_loader_usb_minimal_reqs:

Building and running
********************

This sample is not intended for standalone building.
It is automatically included and built when the following Kconfig options are enabled in a project configuration:

* :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`
* :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`
* :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_USB_MCUMGR`


Testing
=======

This sample is not intended for standalone testing.
