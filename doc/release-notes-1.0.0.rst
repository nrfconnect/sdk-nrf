.. _ncs_release_notes_100:

|NCS| v1.0.0 Release Notes
##########################

nRF Connect SDK delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products. It includes the MCUboot and the Zephyr RTOS open source projects which are continuously integrated and re-distributed with the SDK.

nRF Connect SDK v1.0.0 supports product development with the nRF9160 Cellular IoT device.
It contains reference applications, sample source code, and libraries for Bluetooth Low Energy devices in the nRF52 Series, though product development on these devices is not currently supported with the nRF Connect SDK.

Highlights
**********

* |NCS| v1.0.0 is aligned with the production release of nRF9160.
* Added a new AWS FOTA sample demonstrating application firmware update through the use of AWS IoT Jobs with MQTT and HTTP.
* Added secure services in Secure Partition Manager (SPM) for TrustZone devices (nRF9160).
* Updated the Download Client library with support for HTTPS and IPv6.
* Included a new version of the nRF BLE Controller.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.0.0**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`Getting the nRF Connect SDK code <cloning_the_repositories_win>` for more information.

Supported modem firmware
************************
* mfw_nrf9160_1.0.0

Starting from |NCS| v1.0.0, use `nRF Connect for Desktop`_ for modem firmware updates.

Supported boards
****************

* PCA10090 (nRF9160 DK)
* PCA10056 (nRF52840 Development Kit)
* PCA10059 (nRF52840 Dongle)
* PCA10040 (nRF52 Development Kit)
* PCA10028 (nRF51 Development Kit)
* PCA63519 (Smart Remote 3 DK add-on)


Required tools
**************

In addition to the tools mentioned in :ref:`gs_installing`, the following tool versions are required to work with the |NCS|:

.. list-table::
   :header-rows: 1

   * - Tool
     - Version
     - Download link
   * - SEGGER J-Link
     - V6.41
     - `J-Link Software and Documentation Pack`_
   * - nRF5x Command Line Tools
     - v9.8.1
     - `nRF5x Command Line Tools`_
   * - nRF Connect for Desktop
     - v3.0.0 or later
     - `nRF Connect for Desktop`_
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools`
   * - GCC
     - See :ref:`gs_installing_toolchain`
     - `GNU Arm Embedded Toolchain`_


As IDE, we recommend to use |SES| (Nordic Edition), version 4.18 or later.
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

  * :ref:`aws_fota_sample` - shows how to perform over-the-air firmware updates of an nRF9160 through the use of AWS IoT Jobs with MQTT and HTTP.
  * :ref:`secure_services` - demonstrates using the reboot and random number services.

* Added the following libraries:

  * :ref:`lib_fota_download` - handles Firmware Over The Air (FOTA) downloads.
  * :ref:`at_cmd_readme` - facilitates handling of AT Commands by multiple modules.
  * :ref:`lib_aws_jobs` - facilitates communication with the AWS IoT Jobs service.
  * :ref:`lib_aws_fota` - combines the :ref:`lib_aws_jobs` and :ref:`lib_fota_download` libraries to create a user-friendly library that can perform firmware-over-the-air (FOTA) update using HTTP and MQTT TLS.

* Asset Tracker sample:

  * The orientation detector now supports interrupt handling.

* nRF Connect SDK now uses upstream CoAP implementation. The :ref:`mqtt_simple_sample` sample was rewritten to use the upstream library, and the downstream CoAP was removed.
* The :ref:`http_application_update_sample` sample has been updated to use the :ref:`lib_fota_download` library.

BSD library
-----------

* Updated bsdlib to version 0.3.3.
* Introduced a new header :file:`bsdlib.h` to be used by the application to initialize and shut down the library.
* Library initialization during system initialization (``SYS_INIT``) is now optional, and controlled via ``Kconfig``. The default behavior is unchanged.

Secure Partition Manager (SPM) library
--------------------------------------

* Added random number secure service, providing access to the RNG hardware from the non-secure firmware.
* Non-Secure callable support for TrustZone:

  * A secure_services module is now available over secure entry functions. This means:

    * :file:`secure_services.c` resides in secure firmware (SPM).
    * :file:`secure_services.h` declares functions that can be called from non-secure firmware.

  * :ref:`lib_spm` now exposes secure entry functions by default.
  * Added reboot as a secure service. The reboot secure service is called when the non-secure firmware calls ``sys_reboot()``.

* PWM0-3 added as non-secure.


Common libraries
================

* Added the following library:

  * :ref:`ppi_trace` - enables tracing of hardware peripheral events on pins.

Enhanced Shockburst
-------------------

* Added support for nRF52811.

Download Client
---------------

* Added IPv6 support, with fallback to IPv4.
* Added HTTPS support. The application must provision the TLS security credentials.
* Several improvements to buffer handling and network code.
* Library now runs in a separate thread.


Crypto
======

* Added :ref:`nrfxlib:nrf_cc310_mbedcrypto_readme` library v0.8.1 (experimental release) to perform hardware-accelerated cryptography using Arm CryptoCell CC310 on devices with the CC310 peripheral.

nRF BLE Controller
==================

* Added support for the nRF BLE controller 0.2.0-4.prealpha. Includes drivers to access HCI, flash, clock control, and entropy hardware.
  For details, see :ref:`nrfxlib:ble_controller_changelog`.

Subsystems
==========

Bluetooth Low Energy
--------------------

* Added the following samples:

  * :ref:`central_bas` - demonstrates how do use the :ref:`bas_c_readme` to receive battery level information from a compatible device.
  * :ref:`shell_bt_nus` - demonstrates how to use the :ref:`shell_bt_nus_readme` to receive shell commands from a remote device.

* Added the following libraries:

  * :ref:`bas_c_readme` - used to retrieve information about the battery level from a device.
  * :ref:`shell_bt_nus_readme` - allows for sending shell commands from a host to the application.

* Added :ref:`ble_console_readme` - a desktop application that can be used to communicate with an nRF device over *Bluetooth* Low Energy using the :ref:`shell_bt_nus_readme`.
* Added Manufacturer Data filter to the :ref:`nrf_bt_scan_readme`.
* Added application callbacks for the Output Report related operations in the HID service.


Partition Manager
-----------------

* Partition Manager now handles all HEX file merging.
* :ref:`ug_pm_static` of upgradable images is now supported.


nRF Desktop
===========

* The nrf_desktop reference implementation is moved from the ``samples/`` folder to ``applications/``.
* The nrf_desktop configuration channel now allows data to be exchanged between the device and host in both directions.


Documentation
=============

* Added or updated documentation for the following samples:

  * nRF9160:

    * :ref:`secure_services`
    * :ref:`secure_partition_manager`
    * :ref:`aws_fota_sample`
    * :ref:`lte_sensor_gateway`

  * Bluetooth Low Energy:

    * :ref:`central_bas`
    * :ref:`bluetooth_central_hids`
    * :ref:`peripheral_lbs`
    * :ref:`shell_bt_nus`

  * Other:

    * :ref:`ppi_trace_sample`

* Added or updated documentation for the following libraries:

  * nRF9160:

    * :ref:`lib_spm`
    * :ref:`at_cmd_readme`
    * :ref:`lib_download_client`
    * :ref:`lib_aws_fota`
    * :ref:`lib_fota_download`
    * :ref:`lib_secure_services`

  * Bluetooth Low Energy:

    * :ref:`bas_c_readme`

  * Other:

    * :ref:`ppi_trace`
    * :ref:`ble_console_readme`

* Added or updated the following documentation:

  * nRF BLE Controller (experimental)
  * :ref:`ug_multi_image`
  * :ref:`partition_manager`
  * :ref:`nrf_desktop`
  * :ref:`shell_bt_nus_readme`

* API documentation of all libraries now also mentions the location of header files and source files.

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
* nRF91 fails to receive large packets (over 4000 bytes) over NB-IoT. LTE-M is not affected.
* nrf_connect fails if called immediately after initialization of the device. A delay of 1000 ms is required for this to work as intended.

Crypto
======

* The :ref:`nrfxlib:nrf_security` glue layer is broken because symbol renaming is not handled correctly.
  Therefore, the behavior is undefined when selecting multiple back-ends for the same algorithm (for example, AES).


Subsystems
==========

Bluetooth Low Energy
--------------------

* :ref:`peripheral_lbs` does not report the Button 1 state correctly.
* The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.
* On some operating systems, the nrf_desktop application is unable to reconnect to a host.
* central_uart: A too long 212-byte string cannot be handled when entered to the console to send to peripheral_uart.
* central_hids: After flashing a HEX file to the nrf52_pca10040 board, UART connectivity is lost when using the BLE Controller. The board must be reset to get UART output.
* On nRF51 devices, BLE samples that use GPIO might crash when buttons are pressed frequently. In such case, the GPIO ISR introduces latency that violates real-time requirements of the Radio ISR. nRF51 is more sensitive to this issue than nRF52 (faster core).

Bootloader
----------

* Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.
* The immutable bootlader can only be used with the following boards:

  * nrf52840_pca10056
  * nrf9160_pca10090

DFU
---

* Firmware upgrade using mcumgr or USB DFU is broken for non-secure applications, because the metadata used by MCUboot is stored in a secure section of flash and is not readable by the non-secure application.
  Therefore, it is not possible to upload the image.
  To work around this issue, modify mcumgr to hard code the addresses instead of reading them from the metadata.

Zephyr
======

* The :ref:`zephyr:alarm_sample` does not work. A fix can be found in `Pull Request #16736 <https://github.com/zephyrproject-rtos/zephyr/pull/16736>`_.
* :ref:`zephyr:usb_mass` does not compile.

nrfxlib
=======

* In the BSD library, the GNSS sockets implementation is experimental.

 * Forcing a cold start and writing AGPS data is not yet supported.

nrfx 1.7.1
==========

* nrfx_saadc driver:
  Samples might be swapped when buffer is set after starting the sample process, when more than one channel is sampled.
  This can happen when the sample task is connected using PPI and setting buffers and sampling are not synchronized.
* The nrfx_uarte driver does not disable RX and TX in uninit, which can cause higher power consumption.
* The nrfx_uart driver might incorrectly set the internal tx_buffer_length variable when compiled with high optimization level.

In addition to the known issues above, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
