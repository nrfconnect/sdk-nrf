.. _ncs_release_notes_040:

|NCS| v0.4.0 Release Notes
##########################

This project is hosted by Nordic Semiconductor to demonstrate the integration of Nordic SoC support in open source projects, like MCUBoot and the Zephyr RTOS, with libraries and source code for low-power wireless applications.

nRF Connect SDK v0.4.0 supports development with nRF9160 Cellular IoT devices.
It contains references and code for Bluetooth Low Energy devices in the nRF52 Series, though development on these devices is not currently supported with the nRF Connect SDK.

Highlights
**********

TBD

Repositories
************

.. list-table::
   :header-rows: 1

   * - Component
     - Tag
   * - `fw-nrfconnect-nrf <https://github.com/NordicPlayground/fw-nrfconnect-nrf>`_
     - v0.4.0
   * - `nrfxlib <https://github.com/NordicPlayground/nrfxlib>`_
     - v0.4.0
   * - `fw-nrfconnect-zephyr <https://github.com/NordicPlayground/fw-nrfconnect-zephyr>`_
     - v1.14.0-ncs1
   * - `fw-nrfconnect-mcuboot <https://github.com/NordicPlayground/fw-nrfconnect-mcuboot>`_
     - v1.3.0-ncs1

Supported boards
****************

* PCA10090 (nRF9160 DK)
* PCA10056 (nRF52840 Development Kit)
* PCA10059 (nRF52840 Dongle)
* PCA10040 (nRF52 Development Kit)
* PCA10028 (nRF51 Development Kit)
* PCA63519 (Smart Remote 3 DK add-on)
* PCA20041 (TBD)

Required tools
**************

In addition to the tools mentioned in :ref:`gs_installing`, the following tool versions are required to work with the |NCS|:

.. list-table::
   :header-rows: 1

   * - Tool
     - Version
     - Download link
   * - SEGGER J-Link
     - V6.40
     - `J-Link Software and Documentation Pack`_
   * - nRF5x Command Line Tools
     - v9.8.1
     - `nRF5x Command Line Tools`_
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools_linux`



As IDE, we recommend to use |SES| (Nordic Edition), version 4.16 or later.
It is available from the following links:

* `SEGGER Embedded Studio (Nordic Edition) - Windows x86`_
* `SEGGER Embedded Studio (Nordic Edition) - Windows x64`_
* `SEGGER Embedded Studio (Nordic Edition) - Mac OS x64`_
* `SEGGER Embedded Studio (Nordic Edition) - Linux x86`_
* `SEGGER Embedded Studio (Nordic Edition) - Linux x64`_


Change log
**********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added the following samples:

* Added the following libraries:



Common libraries
================

* Added the following libraries:



Crypto
======



nRF Desktop
===========



Subsystems
==========

Bluetooth Low Energy
--------------------

* Added the following samples:

* Added the following libraries:



Bootloader
----------


NFC
---

* Added the following samples:

* Added the following libraries:

Profiler
--------


Documentation
=============



Known issues
************

nRF9160
=======

* The :ref:`asset_tracker` sample does not wait for connection to nRF Cloud before trying to send data.
  This causes the sample to crash if the user toggles one of the switches before the board is connected to the cloud.
* The :ref:`asset_tracker` sample might show up to 2.5 mA current consumption in idle mode with ``CONFIG_POWER_OPTIMIZATION_ENABLE=y``.
* If a debugger (for example, J-Link) is connected via SWD to the nRF9160, the modem firmware will reset.
  Therefore, the LTE modem cannot be operational during debug sessions.
* The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.
  As a workaround, set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the ``builddir/zephyr/zephyr.map`` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.

Subsystems
==========

Bluetooth Low Energy
--------------------

* :ref:`peripheral_lbs` does not report the Button 1 state correctly.
  This issue will be fixed with `pull request #312 <https://github.com/NordicPlayground/fw-nrfconnect-nrf/pull/312>`_.
* :ref:`peripheral_uart` cannot handle the corner case that a user attempts to send a string of more than 211 bytes.
  This issue will be fixed with `pull request #313 <https://github.com/NordicPlayground/fw-nrfconnect-nrf/pull/313>`_.
* The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.
* The peripheral samples (:ref:`peripheral_uart`, :ref:`peripheral_lbs`, :ref:`peripheral_hids_mouse`) have reconnection issues after performing bonding (LE Secure Connection pairing enable) with nRF Connect for Desktop.
  These issues result in disconnection.

Bootloader
----------

* Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.
* The immutable bootlader can only be used with the following boards:

  * nrf52840_pca10056
  * nrf9160_pca10090

In addition to the known issues above, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
