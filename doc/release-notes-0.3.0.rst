.. _ncs_release_notes_030:

|NCS| v0.3.0 Release Notes
##########################

This project is hosted by Nordic Semiconductor to demonstrate the integration of Nordic SoC support in open source projects, like MCUBoot and the Zephyr RTOS, with libraries and source code for low-power wireless applications.

nRF Connect SDK v0.3.0 supports development with nRF9160 Cellular IoT devices.
It contains references and code for Bluetooth Low Energy devices in the nRF52 Series, though development on these devices is not currently supported with the nRF Connect SDK.

Highlights
**********

* Added support for the new nRF9160 SiP:

  * New target nRF9160_PCA10090
  * Driver support
  * Samples and libraries demonstrating LTE functionality
  * Cortex-M33 and TrustZone (limited) including secure partition manager

* Added the following samples for nRF9160:

  * :ref:`asset_tracker`
  * :ref:`lte_sensor_gateway`
  * AT Client

* Added a first stage bootloader B0

Repositories
************

.. list-table::
   :header-rows: 1

   * - Component
     - Tag
   * - `fw-nrfconnect-nrf <https://github.com/NordicPlayground/fw-nrfconnect-nrf>`_
     - v0.3.0
   * - `nrfxlib <https://github.com/NordicPlayground/nrfxlib>`_
     - v0.3.0
   * - `fw-nrfconnect-zephyr <https://github.com/NordicPlayground/fw-nrfconnect-zephyr>`_
     - v1.13.99-ncs2
   * - `fw-nrfconnect-mcuboot <https://github.com/NordicPlayground/fw-nrfconnect-mcuboot>`_
     - v1.2.99-ncs2

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
     - * `nRF5x Command Line Tools Linux 32`_
       * `nRF5x Command Line Tools Linux 64`_
       * `nRF5x Command Line Tools Windows 32`_
       * `nRF5x Command Line Tools Windows 64`_
       * `nRF5x Command Line Tools OSX`_
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools`


Change log
**********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added support for the new nRF9160 SiP (see `Highlights`_).
* Added target nRF52840_PCA10090 (used when compiling for the nRF9160 DK Board Controller).
* Added the following samples:

  * :ref:`secure_partition_manager`:
    This sample provides a reference implementation of a first-stage boot firmware.
    The sample configures resources for the secure domain and boots an application from the non-secure domain.
  * **at_client**:
    This sample uses the **at_host** library to provide a UART interface for AT commands.
  * :ref:`asset_tracker`:
    This sample uses the **nrf_cloud** library to transmit GPS and device orientation data to the nRF Cloud via LTE.
  * :ref:`lte_sensor_gateway`:
    This sample uses the **nrf_cloud** library to transmit sensor data collected via Bluetooth LE to the nRF Cloud via LTE.

* Added the following libraries:

  * **at_host**:
    This library helps creating an AT command socket and forwards requests and responses from and to the modem.
  * :ref:`lib_nrf_cloud`:
    This library implements features to connect and send data to nRF Cloud services.
  * **bsdlib**:
    This library is a porting library for the BSD socket library that is located in the nrfxlib repository.
  * **mqtt_socket**:
    This library uses the MQTT protocol over BSD sockets.
    It will be replaced by the upstream Zephyr library in the future.
  * **lte_link_control**
    This library can be used to send AT commands to the modem to control the link and the modem state (for example, on/off/power saving).

Common libraries
================

* Added the following libraries:

  * **gps_sim**:
    This library simulates a simple GPS device providing NMEA strings with generated data that can be accessed through the GPS API.
  * **sensor_sim**:
    This library simulates a sensor device that can be accessed through the sensor API, currently supporting the acceleration channels in the API.
  * **dk_buttons_and_leds**:
    This library selectively initializes LEDs or buttons.

Crypto
======

* Added an initial release of **nrf_oberon** and **nrf_cc310_bl** for the |NCS|, with support for Cortex-M0, Cortex-M4, and Cortex-M33 devices.

.. note::
   * These libraries are delivered in an experimental state.
   * Only the no-interrupt version of the **nrf_cc310_bl** library is supported in the |NCS|.

nRF Desktop
===========

* Added support for PCA10059 (nRF52840 Dongle).
* Added USB HID support.
* Added support for battery level measurement.

Subsystems
==========

Bluetooth Low Energy
--------------------

* Added the following samples:

  * :ref:`bluetooth_central_hids`:
    This sample connects to HID devices and uses the :ref:`gatt_dm_readme` library to perform HID service discovery.
  * :ref:`central_uart`:
    This sample connects to NUS Servers and uses the :ref:`nus_c_readme` library to interact with them.
    The sample can be tested with the :ref:`peripheral_uart` sample.

* Added the following libraries:

  * :ref:`nrf_bt_scan_readme`:
    This library handles BLE scanning for your application.
  * :ref:`gatt_dm_readme`:
    This library handles service discovery on BLE GATT servers.
  * :ref:`nus_c_readme`:
    This library can be used to act as a NUS Client.

Bootloader
----------

* Added an initial release of a first stage immutable bootloader.
  See :ref:`ug_bootloader`.

NFC
---

* Added the following samples:

  * **record_text**:
    This sample uses the NFC Type 2 Tag to expose a Text record to NFC polling devices.
    It requires the binary libraries in the nrfxlib repository.
  * **writable_ndef_msg**:
    This sample uses the NFC Type 4 Tag to expose an NDEF message, which can be overwritten by NFC polling devices.
    It requires the binary libraries in the nrfxlib repository.

* Added the following libraries:

  * **NDEF**:
    These libraries handle NDEF records and message generation.
    For now, only Text and URI records are supported.

Profiler
--------

* Several fixes and improvements.

Documentation
=============

* Added :ref:`getting_started` information.
* Added :ref:`user_guides` for working with nRF9160 samples, Enhanced ShockBurst (ESB), and the secure bootloader chain.
* Added documentation for various :ref:`samples` and :ref:`libraries`.
* Added :doc:`MCUboot <mcuboot:index>` and :doc:`nrfxlib <nrfxlib:README>` documentation.

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
