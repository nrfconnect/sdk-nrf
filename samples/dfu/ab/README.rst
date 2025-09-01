.. _ab_sample:

A/B with MCUboot
################

.. contents::
   :local:
   :depth: 2

The A/B with MCUboot sample demonstrates how to configure the application for updates using the A/B method using MCUboot.
It also includes an example to perform a device health check before confirming the image after the update.

You can update the sample using the Simple Management Protocol (SMP) with UART or Bluetooth® Low Energy.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need the nRF Device Manager app for update over Bluetooth Low Energy:

* `nRF Device Manager mobile app for Android`_
* `nRF Device Manager mobile app for iOS`_


Overview
********

This sample demonstrates firmware update using the A/B method.
This method allows two copies of the application in the NVM memory.
It is possible to switch between these copies without performing a swap, which significantly reduces time of device's unavailability during the update.
The switch between images can be triggered by the application or, for example, by a hardware button.

This sample implements an SMP server.
SMP is a basic transfer encoding used with the MCUmgr management protocol.
For more information about MCUmgr and SMP, see :ref:`device_mgmt`.

The sample supports the following MCUmgr transports by default:

* Bluetooth
* Serial (UART)

User interface
**************

LED 0:
    This LED indicates that the application is running from slot A.
    It is controlled as active low, meaning it will turn on once the application is booted and blinks (turns off) in short intervals.
    The number of short blinks is configurable using the :kconfig:option:`CONFIG_N_BLINKS` Kconfig option.
    It will remain off if the application is running from slot B.

LED 1:
    This LED indicates that the application is running from slot B.
    It is controlled as active low, meaning it will turn on once the application is booted and blinks (turns off) in short intervals.
    The number of short blinks is configurable using the :kconfig:option:`CONFIG_N_BLINKS` Kconfig option.
    It will remain off if the application is running from slot A.

Button 0:
    By pressing this button, the non-active slot will be selected as the preferred slot on the next reboot.
    This preference applies only to the next boot and is cleared after the subsequent reset.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. _CONFIG_N_BLINKS:

CONFIG_N_BLINKS - The number of blinks.
   This configuration option sets the number of times the LED corresponding to the currently active slot blinks (LED0 for slot A, LED1 for slot B).
   The default value of the option is set to ``1``, causing a single blink to indicate *Version 1*.
   You can increment this value to represent an update, such as set it to ``2`` to indicate *Version 2*.

.. _CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE:

CONFIG_EMULATE_APP_HEALTH_CHECK_FAILURE - Enables emulation of a broken application that fails the self-test.
   This configuration option emulates a broken application that does not pass the self-test.

Additional configuration
========================

Check and configure the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` library Kconfig option specific to the MCUboot library.
This configuration option sets the version to pass to imgtool when signing.
To ensure the updated build is preferred after a DFU, set this option to a higher version than the version currently running on the device.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/ab`

.. include:: /includes/build_and_run.txt

Testing
=======

To perform DFU using the `nRF Connect Device Manager`_ mobile app, complete the following steps:

.. include:: /app_dev/device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start
   :end-before: fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`MCUboot <mcuboot_index_ncs>`
