.. _ug_nrf52:

Working with nRF52 Series
#########################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for developing on all nRF52 Series devices and contains board definitions for all development kits and reference design hardware.

See `Getting started with nRF Connect SDK (nRF52 Series)`_ for step-by-step instructions on how to test and develop with an nRF52 Series development kit.

Introduction
************

The nRF52 Series of System-on-Chip (SoC) devices embed a powerful yet low-power Arm Cortex-M4 processor with our industry leading 2.4 GHz RF transceivers.
All of the nRF52 Series SoCs have support for Bluetooth 5 features, in addition to multiprotocol capabilities.

See `nRF52 Series`_ for the technical documentation on the nRF52 Series chips and associated kits.

Supported boards
================

Devices in the nRF52 Series are supported by these boards in the Zephyr open source project and in |NCS|.


.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Build target
     - Documentation
   * - :ref:`zephyr:nrf52840dk_nrf52840`
     - PCA10056
     - ``nrf52840dk_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | `User Guide <nRF52840 DK User Guide_>`_
   * - :ref:`zephyr:nrf52840dk_nrf52811`
     - PCA10056
     - ``nrf52840dk_nrf52811``
     - `Product Specification <nRF52811 Product Specification_>`_
   * - :ref:`zephyr:nrf52833dk_nrf52833`
     - PCA10100
     - ``nrf52833dk_nrf52833``
     - | `Product Specification <nRF52833 Product Specification_>`_
       | `User Guide <nRF52833 DK User Guide_>`_
   * - :ref:`zephyr:nrf52833dk_nrf52820`
     - PCA10100
     - ``nrf52833dk_nrf52820``
     - `Product Specification <nRF52820 Product Specification_>`_
   * - :ref:`zephyr:nrf52dk_nrf52832`
     - PCA10040
     - ``nrf52dk_nrf52832``
     - | `Product Specification <nRF52832 Product Specification_>`_
       | `User Guide <nRF52 DK User Guide_>`_
   * - :ref:`zephyr:nrf52dk_nrf52810`
     - PCA10040
     - ``nrf52dk_nrf52810``
     - `Product Specification <nRF52810 Product Specification_>`_
   * - :ref:`zephyr:nrf52840dongle_nrf52840`
     - PCA10059
     - ``nrf52840dongle_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | `User Guide <nRF52840 Dongle User Guide_>`_


nRF Desktop
===========

The nRF Desktop application is a complete project that integrates Bluetooth Low Energy, see the :ref:`nrf_desktop` application.
It can be built for the nRF Desktop reference hardware or an nRF52840 DK.

The nRF Desktop is a reference design of a HID device that is connected to a host through Bluetooth Low Energy or USB, or both.
This application supports configurations for simple mouse, gaming mouse, keyboard, and USB dongle.

Secure bootloader chain
=======================

nRF52 Series devices support a secure bootloader solution based on the chain of trust concept.

See :ref:`ug_bootloader` for more information and instructions on how to enable one or more bootloaders in your application.


Supported protocols
*******************

The nRF52 Series multiprotocol radio supports Bluetooth Low Energy, proprietary (including Enhanced Shock Burst), ANT, Thread, Zigbee, and 802.15.4.
Standard interface protocols like NFC and USB are supported on a range of the devices in the series and with supporting software.

.. note::
   |NCS| currently has no support for ANT.

The following sections give pointers on where to start when working with these protocols.

To test the general capabilities of the 2.4 GHz radio transceiver, use the :ref:`radio_test` sample.

Bluetooth Low Energy
====================

When you develop a Bluetooth Low Energy (LE) application, you must use the Bluetooth software stack.
This stack is split into two core components: the Bluetooth Host and the Bluetooth LE Controller.

The |NCS| Bluetooth stack is fully supported by Nordic Semiconductor for nRF52 Series devices.
The :ref:`ug_ble_controller` user guide contains more information about the two available Bluetooth LE Controllers and instructions for switching between them.

See :ref:`zephyr:bluetooth` for documentation on the Bluetooth Host and open source LE Controller.
For documentation about the SoftDevice Controller and information on what variants of the controller support which chips, see :ref:`nrfxlib:softdevice_controller`.

The |NCS| contains a variety of :ref:`ble_samples` that target nRF52 Series devices.
In addition, you can run the :ref:`zephyr:bluetooth-samples` that are included from Zephyr.

For available libraries, see :ref:`lib_bluetooth_services` (|NCS|) and :ref:`zephyr:bluetooth_api` (Zephyr).

Project Connected Home over IP (Project CHIP)
=============================================

.. include:: ug_chip.rst
   :start-after: chip_intro_start
   :end-before: chip_intro_end

See the :ref:`ug_chip` user guide for information about how to work with Project CHIP applications.
To start developing, check out :ref:`chip_samples`.

Enhanced ShockBurst
===================

.. include:: ug_esb.rst
   :start-after: esb_intro_start
   :end-before: esb_intro_end

See the :ref:`ug_esb` user guide for information about how to work with Enhanced ShockBurst.
To start developing, check out the :ref:`esb_prx_ptx` sample.

Near Field Communication
========================

Near Field Communication (NFC) is a technology for wireless transfer of small amounts of data between two devices that are in close proximity.
It makes transferring data fast and easy when devices are in close proximity.
The range of NFC is typically <10 cm.

|NCS| provides two protocol stacks for developing NFC applications: Type 2 Tag and Type 4 Tag.
These stacks are provided in binary format in the `sdk-nrfxlib`_ repository.
See :ref:`nrfxlib:nfc` for documentation about the NFC stacks, and :ref:`ug_nfc` for general information.

The NFC stack requires the NFCT driver for nRF52 devices, which is available as part of `nrfx`_.
The nrfx repository is included in the |NCS| as a module of the Zephyr repository.

See :ref:`nfc_samples` and :ref:`lib_nfc` for lists of samples and libraries that the |NCS| provides.

USB
===

The |NCS| contains a USB device stack for the USB 2.0 Full Speed peripheral that is available on a number of the nRF52 devices.
You can find the implementation in the Zephyr repository.
See :ref:`zephyr:usb_api` for documentation and :ref:`zephyr:usb-samples` for a list of available samples.

The USB stack requires the USBD driver for nRF52 devices, which is available as part of `nrfx`_.
The nrfx repository is included in the |NCS| as a module of the Zephyr repository.

Thread
======

.. include:: ug_thread.rst
   :start-after: thread_intro_start
   :end-before: thread_intro_end

See the :ref:`ug_thread` user guide for information about how to work with Thread.
To start developing, check out the :ref:`openthread_samples`.

Zigbee
======

.. include:: ug_zigbee.rst
   :start-after: zigbee_ug_intro_start
   :end-before: zigbee_ug_intro_end

See the :ref:`Zigbee user guide <ug_zigbee>` for information about how to work with the Zigbee protocol and stack.
To start developing, check out the :ref:`Zigbee light switch <zigbee_light_switch_sample>` sample.

Multiprotocol support
*********************

The nRF52 Series devices support running another protocol in parallel with the SoftDevice Controller.
See the :ref:`ug_multiprotocol_support` user guide for instructions on how to enable multiprotocol support for Thread or Zigbee in combination with Bluetooth.

The :ref:`nrfxlib:mpsl` library provides services for multiprotocol applications.


.. |note| replace:: There is currently no support for upgrading the bootloader (:doc:`mcuboot:index`) on nRF52 Series devices.

.. fota_upgrades_start

FOTA upgrades
*************

|fota_upgrades_def|
FOTA upgrades can be used to replace the application.

.. note::
   |note|

To perform a FOTA upgrade, complete the following steps:

1. Make sure that your application supports FOTA upgrades.
      To download and apply FOTA upgrades, the following requirements apply:

      * You must enable the mcumgr module, which handles the transport protocol over Bluetooth Low Energy.
        To enable this module in your application, complete the following steps:

        a. Enable :option:`CONFIG_MCUMGR_CMD_OS_MGMT`, :option:`CONFIG_MCUMGR_CMD_IMG_MGMT`, and :option:`CONFIG_MCUMGR_SMP_BT`.
        #. Call ``os_mgmt_register_group()`` and ``img_mgmt_register_group()`` in your application.
        #. Call ``smp_bt_register()`` in your application to initialize the mcumgr Bluetooth Low Energy transport.

        See the code of the :ref:`zephyr:smp_svr_sample` for an implementation example.
        After completing these steps, your application should advertise the SMP Service with UUID 8D53DC1D-1DB7-4CD3-868B-8A527460AA84.

      * |fota_upgrades_req_mcuboot|

#. Create a binary file that contains the new image.
      |fota_upgrades_building|
      The :file:`app_update.bin` file is the file that must be downloaded to the device.

#. Download the new image to a device.
      Use `nRF Connect for Mobile`_ or `nRF Toolbox`_ to upgrade your device with the new firmware.

      To do so, make sure that you can access the :file:`app_update.bin` file from your phone or tablet.
      Then connect to the device with the mobile app and initiate the DFU process to transfer :file:`app_update.bin` to the device.

      .. note::
         There is currently no support for the FOTA process in nRF Connect for Desktop.

.. fota_upgrades_end

Building and programming a sample
*********************************

To build your application, follow the instructions in :ref:`gs_programming`.
