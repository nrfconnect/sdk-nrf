.. _ug_nrf52:

Working with nRF52
##################

The |NCS| provides support for developing on all nRF52 Series devices and contains board definitions for all development kits and reference design hardware.

Introduction
************

The nRF52 Series of System-on-Chip (SoC) devices embed a powerful yet low-power Arm Cortex-M4 processor with our industry leading 2.4 GHz RF transceivers.
All of the nRF52 Series SoCs have support for Bluetooth 5 features, in addition to multi-protocol capabilities.

See `nRF52 Series`_ for the technical documentation on the nRF52 Series chips and associated kits.

Supported boards
================

All supported boards have a PCA number, which is used to uniquely identify each board.
The build system uses a combination of device part code and PCA number to identify a build target in the format ``nrf52xxx_pcaxxxxx``.
For example, for the :ref:`nRF52840 Development Kit (PCA10056) <zephyr:nrf52840_pca10056>`, use ``nrf52840_pca10056`` as target when building.

Devices in the nRF52 Series are supported by these boards in the Zephyr open source project and in |NCS|.
Refer to the PCA number for building projects for these boards and find a link to their descriptions.

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Build target
   * - nRF52840 DK
     - PCA10056
     - :ref:`nrf52840_pca10056<zephyr:nrf52840_pca10056>`
   * - nRF52840 DK (emulating the nRF52811)
     - PCA10056
     - :ref:`nrf52811_pca10056<zephyr:nrf52811_pca10056>`
   * - nRF52833 DK
     - PCA10100
     - :ref:`nrf52833_pca10100<zephyr:nrf52833_pca10100>`
   * - nRF52 DK
     - PCA10040
     - :ref:`nrf52_pca10040<zephyr:nrf52_pca10040>`
   * - nRF52 DK (emulating the nRF52810)
     - PCA10040
     - :ref:`nrf52810_pca10040<zephyr:nrf52810_pca10040>`


Secure bootloader chain
=======================

nRF52 Series devices support a secure bootloader solution based on the chain of trust concept.

See :ref:`ug_bootloader` for more information and instructions on how to enable one or more bootloaders in your application.


Supported protocols
*******************

The nRF52 Series multi-protocol radio supports Bluetooth Low Energy, proprietary (including Enhanced Shock Burst), ANT, Thread, Zigbee, and 802.15.4.
Standard interface protocols like NFC and USB are supported on a range of the devices in the series and with supporting software.

.. note::
   |NCS| currently has limited support for Thread.
   It does not support ANT or Zigbee.

The following sections give pointers on where to start when working with these protocols.

To test the general capabilities of the 2.4 GHz radio transceiver, use the :ref:`radio_test` sample.

Bluetooth Low Energy
====================

When you develop a Bluetooth Low Energy (LE) application, you must use the Bluetooth software stack.
This stack is split into two core components: the Bluetooth Host and the Bluetooth LE Controller.

The |NCS| Bluetooth stack is fully supported by Nordic Semiconductor for nRF52 Series devices.
The :ref:`ug_ble_controller` user guide contains more information about the two available Bluetooth LE Controllers and instructions for switching between them.

See :ref:`zephyr:bluetooth` for documentation on the Bluetooth Host and open source LE Controller.
For documentation about the nRF Bluetooth LE Controller and information on what variants of the controller support which chips, see :ref:`nrfxlib:ble_controller`.

The |NCS| contains a variety of :ref:`ble_samples` that target nRF52 Series devices.
In addition, you can run the :ref:`zephyr:bluetooth-samples` that are included from the Zephyr project.

For available libraries, see :ref:`lib_bluetooth_services` (|NCS|) and :ref:`zephyr:bluetooth_api` (Zephyr).


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
These stacks are provided in binary format in the `nrfxlib`_ repository.
See :ref:`nrfxlib:nfc` for documentation about the NFC stacks.

The NFC stack requires the NFCT driver for nRF52 devices, which is available as part of `nrfx`_.
The nrfx repository is included in the |NCS| as a module of the Zephyr repository.

See :ref:`nfc_samples` and :ref:`lib_nfc` for lists of samples and libraries that the |NCS| provides.

USB
===

The |NCS| contains a USB device stack for the USB 2.0 Full Speed peripheral that is available on a number of the nRF52 devices.
You can find the implementation in the Zephyr repository.
See :ref:`zephyr:usb_device_stack` for documentation and :ref:`zephyr:usb-samples` for a list of available samples.

The USB stack requires the USBD driver for nRF52 devices, which is available as part of `nrfx`_.
The nrfx repository is included in the |NCS| as a module of the Zephyr repository.


Multi-protocol support
**********************

The nRF52 Series devices support running another protocol in parallel with the nRF Bluetooth LE Controller.

The :ref:`Multi-Protocol Service Layer (MPSL) <nrfxlib:mpsl>` library provides services for multi-protocol applications.



Building and programming a sample
*********************************

To build your application, follow the instructions in :ref:`gs_programming`.
