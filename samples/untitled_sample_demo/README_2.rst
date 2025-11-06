.. _untitled_sample_project:

Untitled sample project
#######################

.. contents::
   :local:
   :depth: 2

This sample template demonstrates how to structure an application for development in |NCS|.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

This sample hosts a MCUmgr SMP server over Bluetooth which allows devices to connect, query it and upload new firmware which the included MCUboot bootloader will swap to (and allow reverting if there is a problem with the image).
An optional Kconfig fragment file is provided which will enable security manager support in Bluetooth for pairing/bonding support, and require an authenticated Bluetooth link to use MCUmgr functionality on the device.

Configuration
*************

|config|

See :ref:`configuring_kconfig` for information about the different ways you can set Kconfig options in the |NCS|.

Configuration options
=====================

.. _CONFIG_APP_SHOW_VERSION:

CONFIG_APP_SHOW_VERSION
    When enabled, this will show the version of the application at boot time, from the :file:`VERSION` file located in the application directory.

.. _CONFIG_APP_SHOW_BUILD_DATE:

CONFIG_APP_SHOW_BUILD_DATE
    When enabled, this will show the build date of the application at boot time.

Building and running
********************

.. |sample path| replace:: :file:`samples/untitled_sample_demo`

.. include:: /includes/build_and_run.txt

To upload MCUboot and the bundle of images to the device, program the sample by using the :ref:`flash command without erase <programming_params_no_erase>`.

Testing over Bluetooth LE
=========================

The testing scenario uses the FOTA over Bluetooth LE method to load the firmware update to the device.
For the testing scenario to work, make sure you meet the following requirements:

* Your update image differs from the base image so that the update can be applied.
* You changed the firmware version by setting :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` to ``"2.0.0"``.
* You are familiar with the FOTA over Bluetooth LE method (see :ref:`the guide for the nRF54L15 DK <ug_nrf54l_developing_ble_fota_steps>`).
* You have the `nRF Connect Device Manager`_ mobile app installed on your smartphone to update your device with the new firmware over Bluetooth LE.

Meeting these requirements requires rebuilding the sample.

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Start the `nRF Connect Device Manager`_ mobile app.
#. Follow the testing steps for the FOTA over Bluetooth LE.
   See the following sections of the documentation:

   * :ref:`Testing steps for FOTA over Bluetooth LE with nRF54L15 <ug_nrf54l_developing_ble_fota_steps_testing>`

#. Once the firmware update has been loaded, check the UART output.
   See the following Sample output section.
#. Reboot the device.
   The firmware update will be applied to the device and it will boot into it.

Dependencies
************

The sample uses the following Zephyr library:

* :ref:`zephyr:mcu_mgr`
