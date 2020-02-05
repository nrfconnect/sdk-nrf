.. _ncs_release_notes_040:

|NCS| v0.4.0 Release Notes
##########################

This project is hosted by Nordic Semiconductor to demonstrate the integration of Nordic SoC support in open source projects, like MCUBoot and the Zephyr RTOS, with libraries and source code for low-power wireless applications.

nRF Connect SDK v0.4.0 supports development with nRF9160 Cellular IoT devices.
It contains references and code for Bluetooth Low Energy devices in the nRF52 Series, though development on these devices is not currently supported with the nRF Connect SDK.

Highlights
**********

* Added support for the new triple mode (LTE-M/NB-IoT/GPS) with ``mfw_nrf9160_0.7.0-29.alpha`` (see :ref:`nrf9160_ug_network_mode` for instructions for changing the network mode)
* Updated BSD library:

  * Added API supporting GPS
  * Fixed various issues (see the :ref:`nrfxlib:bsdlib_changelog`)

* Added sample showing how to use the new GPS API (tested on nRF9160 DK v0.8.5)
* Added sample showing how to download a new image via HTTP
* Added multi-image support (see :ref:`partition_manager`)

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v0.4.0**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`Getting the nRF Connect SDK code <cloning_the_repositories_win>` for more information.


Supported boards
****************

* PCA10090 (nRF9160 DK) - only for LTE samples
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
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools`


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

* Deprecated the ``nrf91`` branch in fw-nrfconnect-zephyr in favor of master, which now supports the nRF9160 IC
* Split the previous ``nrf9160_pca10090`` board into two boards, one for the secure (``nrf9160_pca10090``) and one for the non-secure (``nrf9160_pca10090ns``) image (both reside in the same board folder)
* Added the following samples:

  * :ref:`http_application_update_sample` - demonstrates a basic FOTA (firmware over-the-air) update using the :ref:`lib_download_client`
  * :ref:`mqtt_simple_sample` - connects to an MQTT broker and sends and receives data
  * :ref:`nrf_coap_client_sample` - receives data from a public CoAP server using the Nordic CoAP library
  * Simple GPS sample - gets the current position and logs it on UART

* Fixed an issue in the :ref:`lte_sensor_gateway` sample where the host and the controller would go out of sync after a reset
* Various fixes and updates to BSD Library, sockets offloading layer, and OS adaption:

  * Updated the :ref:`nrfxlib:bsdlib` (in nrfxlib) library to version 0.3.0 (see the :ref:`nrfxlib:bsdlib_changelog` for details)
  * Added support for GNSS supporting GPS as a socket (in nrfxlib)
  * Implemented :cpp:func:`bsd_os_timedwait` (in ``lib/bsdlib/bsd_os.c`` in fw-nrfconnect-nrf), allowing a proper poll operation and blocking sockets
  * Minor fixes to the nRF91 sockets offloading layer (in ``lib/bsdlib/nrf91_sockets.c`` in fw-nrfconnect-nrf)


Common libraries
================

* Added the following libraries:

  * :ref:`lib_download_client` - downloads files over HTTP and reports back the progress (as data fragments) to the application
  * Nordic CoAP library - ported to the |NCS|
  * :ref:`lib_spm` (replacing Secure Boot) - configures security attributions for the flash, SRAM, and peripherals


Crypto
======

* Added :ref:`nrfxlib:nrf_oberon_readme` v3.0.0 (see :ref:`nrfxlib:crypto_changelog_oberon` for details)
* Added :ref:`nrfxlib:nrf_cc310_mbedcrypto_readme` v0.7.0 (initial, experimental release) to do hardware-accelerated cryptography using Arm CryptoCell CC310 in select architectures
* Added :ref:`nrfxlib:nrf_security` v0.7.0 (initial, experimental release) to bridge software-implemented mbed TLS and hardware-accelerated alternatives (nrf_cc310_mbedcrypto library) through the use of a glue layer


Subsystems
==========

Bluetooth Low Energy
--------------------

* Added the following samples:

  * :ref:`peripheral_hids_keyboard` - simulates a HID input device using GATT HID Service
  * :ref:`bluetooth_central_dfu_smp` - connects to an SMP server and sends simple echo commands

* Added the following libraries:

  * :ref:`dfu_smp_c_readme` - GATT Client implementation compatible with the GATT SMP Service, which is used as DFU transport
  * :ref:`hids_c_readme` - GATT Client implementation compatible with the HID Service


NFC
---

* Added the following samples:

  * :ref:`nfc_tag_reader` - reads NFC tags using additional hardware (expansion board X-NUCLEO-NFC05A1)

* Added the following drivers:

  * :ref:`st25r3911b_nfc_readme` reads and writes NFC-A compatible tags using the NFC Device ST25R3911B

Event Manager
-------------

* Optimized and cleaned up the code
* Added events logging dynamic control over shell
* Updated to use logger instead of printk
* Added a sample (:ref:`event_manager_sample`)

Profiler
--------

* Added dynamic selection of profiled events over shell
* Added a sample (:ref:`profiler_sample`)

Build and configuration system
==============================

Bootloaders (such as SPM) and MCUboot no longer need to be built and flashed separately, but are instead automatically built and flashed (if enabled by the application).
See :ref:`partition_manager` for more information.

Documentation
=============

* Added or updated documentation for the following samples:

  * :ref:`asset_tracker`, :ref:`nrf_coap_client_sample`,  :ref:`mqtt_simple_sample`, and :ref:`http_application_update_sample`
  * :ref:`peripheral_hids_mouse`, :ref:`peripheral_hids_keyboard`,  :ref:`bluetooth_central_hids`, and :ref:`bluetooth_central_dfu_smp`
  * :ref:`record_text`, :ref:`writable_ndef_msg`, and :ref:`nfc_tag_reader`
  * :ref:`event_manager_sample` and :ref:`profiler_sample`

* Added or updated documentation for the following libraries:

  * :ref:`bt_conn_ctx_readme`
  * :ref:`dk_buttons_and_leds_readme`
  * :ref:`nfc`
  * :ref:`event_manager` and :ref:`profiler`
  * :ref:`at_cmd_parser_readme`, :ref:`at_params_readme`, and :ref:`modem_info_readme`
  * :ref:`lib_download_client`
  * :ref:`lib_spm`
  * :ref:`st25r3911b_nfc_readme`
  * :ref:`dfu_smp_c_readme` and :ref:`lbs_readme`

* Added or updated the following documentation:

  * :ref:`gs_installing` - updated for west, added instructions for macOS
  * :ref:`gs_programming` - updated for new SES version
  * :ref:`gs_modifying` - added
  * :ref:`ug_unity_testing` - added
  * :ref:`partition_manager` - added
  * :ref:`ug_nrf9160` - updated Secure Partition Manager and Network mode sections


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
* The :ref:`gatt_dm_readme` is not supported on nRF51 devices.
* Bluetooth LE samples cannot be built with the :ref:`nrfxlib:ble_controller` v0.1.0.

Bootloader
----------

* Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.
* The immutable bootlader can only be used with the following boards:

  * nrf52840_pca10056
  * nrf9160_pca10090

DFU
---

* Firmware upgrade using mcumgr or USB DFU is broken for multi-image builds, because the device tree configuration is not used.
  Therefore, it is not possible to upload the image.
  To work around this problem, build MCUboot and the application separately.


nrfxlib
=======

* GNSS sockets implementation is experimental.

 * Implementation might hard-fault when GPS is in running mode and messages are not read fast enough.
 * NMEA strings are valid c-strings (0-terminated), but the read function might return wrong length.
 * Sockets can only be closed when GPS is in stopped mode.
 * Closing a socket does not properly clean up all memory resources.
   If a socket is opened and closed multiple times, this  might starve the system.
 * Forcing a cold start and writing AGPS data is not yet supported.

nrfx 1.7.1
==========

* nrfx_saadc driver:
  Samples might be swapped when buffer is set after starting the sample process, when more than one channel is sampled.
  This can happen when the sample task is connected using PPI and setting buffers and sampling are not synchronized.
* The nrfx_uarte driver does not disable RX and TX in uninit, which can cause higher power consumption.
* The nrfx_uart driver might incorrectly set the internal tx_buffer_length variable when high optimization level is set during compilation.

In addition to the known issues above, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
