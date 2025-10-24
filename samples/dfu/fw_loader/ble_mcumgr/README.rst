.. _fw_loader_ble_mcumgr:

Minimal Bluetooth LE SMP firmware loader
########################################

.. contents::
   :local:
   :depth: 2

This sample provides the minimal and recommended configuration of a Firmware Loader application using on :ref:`nRF54L15 DK <ug_nrf54l>`.
It uses code from Zephyr's :zephyr:code-sample:`smp-svr` sample for the Bluetooth LE SMP server functionality.

This sample is not intended to be used as a standalone sample, but rather as a starting point for a custom Firmware Loader application to be used with a MCUboot bootloader.

.. _fw_loader_minimal_overview:

Overview
********

The application uses the SMP (Simple Management Protocol) over Bluetooth® Low Energy to perform firmware updates and device management operations.
It is optimized for minimal memory usage and only contains the basic necessary functionalities for firmware loading.

The firmware loader advertises itself as "FW loader" over Bluetooth LE and accepts SMP commands for:

* Image upload and management
* Device information queries
* Storage operations
* Bootloader information

.. _fw_loader_minimal_reqs:

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. _fw_loader_minimal_build_run:

Building and running
********************

This sample is not intended for standalone building.
It automatically is included and built by projects with the following Kconfig options enabled:

* :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT`
* :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`
* :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_BLE_MCUMGR`

Configuration
=============

The sample does not require any additional configuration.

Testing
=======

This sample is not intended for standalone testing.
