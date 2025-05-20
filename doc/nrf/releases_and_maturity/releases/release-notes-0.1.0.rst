.. _ncs_release_notes_010:

|NCS| v0.1.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

This project is hosted by Nordic Semiconductor to demonstrate the integration of Nordic SoC support in open source projects, like MCUBoot and the Zephyr RTOS, with libraries and source code for low-power wireless applications.
It is not intended or supported by Nordic Semiconductor for product development.

Highlights
**********

* Added support for PCA20041 and PCA63519.
* Added Bluetooth Low Energy samples.
* Added Nordic Proprietary samples.

.. note::
   See the changelog and readme files in the component repositories for a detailed description of changes.

Repositories
************
.. list-table::
   :header-rows: 1

   * - Component
     - Tag
   * - `sdk-nrf <https://github.com/nrfconnect/sdk-nrf>`_
     - v0.1.0
   * - `nrfxlib <https://github.com/nrfconnect/nrfxlib>`_
     - v0.1.0
   * - `sdk-zephyr <https://github.com/nrfconnect/sdk-zephyr>`_
     - v1.13.99-ncs1
   * - `sdk-mcuboot <https://github.com/nrfconnect/sdk-mcuboot>`_
     - v1.2.99-ncs1


Supported boards
****************

* PCA10028 (nRF51 Development Kit)
* PCA10040 (nRF52 Development Kit)
* PCA10056 (nRF52840 Development Kit)
* PCA10059 (nRF52840 Dongle)
* PCA63519 (Smart Remote 3 DK add-on)
* PCA20041 (TBD)


Changelog
*********

The following sections provide detailed lists of changes by component.

Bluetooth Low Energy
====================

The following samples that work with the Zephyr BLE Controller have been added:

* :ref:`peripheral_lbs` - sample server implementation using a proprietary vendor-specific UUID.
* :ref:`peripheral_uart` - sample server implementation using a proprietary vendor-specific UUID.
* :ref:`peripheral_hids_mouse` - sample server implementation using the :ref:`hids_readme`.
* :ref:`ble_throughput` - sample server and client implementation showing the usage of Data Length Extension and PHY update procedures.
* :ref:`nrf_desktop` - reference design running on PCA10056 (nRF52840 Development Kit).


Proprietary RF protocols
========================

* Added Nordic proprietary RF protocol samples (enhanced_shockburst) for Nordic devices.

Subsystems
==========

* Added Event Manager.
* Added Profiler.


Documentation
=============

* Created first draft including documentation for selected samples and libraries.
