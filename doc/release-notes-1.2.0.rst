.. _ncs_release_notes_120:

|NCS| v1.2.0 Release Notes
##########################

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

|NCS| v1.2.0 supports product development with the nRF9160 Cellular IoT device.
It contains reference applications, sample source code, and libraries for developing low-power wireless applications with nRF52 and nRF53 Series devices, though support for these devices is incomplete and not recommended for production.


Highlights
**********

* Support for the new nRF5340 device
* New Multi-Protocol Service Layer (:ref:`nrfxlib:mpsl`), included in nrfxlib
* NFC supporting the new TNEP protocol
* New nRF9160 samples and libraries
* General updates and bugfixes


Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.2.0**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.


Supported modem firmware
************************

This version of the |NCS| supports the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.1.1

Use the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See `Updating the nRF9160 DK cellular modem`_ for instructions.


Tested boards
*************

The following boards have been used during testing of this release:

* PCA10090 (nRF9160 DK)
* PCA20035 (Thingy:91)
* PCA10095 (nRF5340 PDK)
* PCA10056 (nRF52840 Development Kit)
* PCA10059 (nRF52840 Dongle)
* PCA10040 (nRF52 Development Kit)

For the full list of supported devices and boards, see :ref:`zephyr:boards` in the Zephyr documentation.


Required tools
**************

In addition to the tools mentioned in :ref:`gs_installing`, the following tool versions are required to work with the |NCS|:

.. list-table::
   :header-rows: 1

   * - Tool
     - Version
     - Download link
   * - SEGGER J-Link
     - V6.60e
     - `J-Link Software and Documentation Pack`_
   * - nRF Command Line Tools
     - v10.6.0
     - `nRF Command Line Tools`_
   * - nRF Connect for Desktop
     - v3.3.0 or later
     - `nRF Connect for Desktop`_
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools`
   * - GCC
     - See :ref:`gs_installing_toolchain`
     - `GNU Arm Embedded Toolchain`_


As IDE, we recommend to use |SES| (Nordic Edition) version 4.42a.
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

  * :ref:`cloud_client` - shows how to connect to and communicate with a cloud service using the generic :ref:`cloud_api_readme`.
  * :ref:`https_client` - shows how to provision a TLS certificate and connect to an HTTPS server.
  * :ref:`serial_lte_modem` - demonstrates sending AT commands between a host and a client device.
    The sample is an enhancement to the :ref:`at_client_sample` sample.

* Added the following libraries:

  * :ref:`lib_aws_iot` - enables applications to connect to and exchange messages with the AWS IoT message broker.
    The library supports TLS-secured MQTT transmissions and firmware over-the-air upgrades.
  * :ref:`modem_key_mgmt` - provides functions to provision security credentials to the nRF9160 modem.
    The library replaces the ``nrf_inbuilt_key`` APIs from the :ref:`nrfxlib:bsdlib`.
  * :ref:`lib_zzhc` - implements the self-registration functionality that is required to connect to the China Telecom network.
  * :ref:`supl_client` - integrates the externally hosted SUPL client library.
    This library implements A-GPS data downloading from a SUPL server.


Updated samples and applications
--------------------------------

* :ref:`asset_tracker`:

  * Added functionality to configure high/low thresholds for sensor data, so that only data below/above the threshold is sent to the cloud.
  * Modified the command format to match the format that is used by nRF Cloud.
  * Implemented support for receiving modem AT commands from the cloud and returning the modem's response.
  * Added functionality to configure the interval at which sensor data is sent to the cloud.
    This makes it possible to change the poll/send interval for environmental and light sensors from the terminal card in nRF Cloud.
  * Replaced ``printk`` calls with calls to the :ref:`zephyr:logger` subsystem.
  * Added a separate workqueue for the application, instead of using the system workqueue.

* :ref:`gps_with_supl_support_sample`:

  * Added support for the new :ref:`supl_client` library, if enabled.


Updated libraries
-----------------

* :ref:`lib_download_client`:

  * Added the :option:`CONFIG_DOWNLOAD_CLIENT_MAX_TLS_FRAGMENT_SIZE` option that allows to configure fragment sizes for TLS connections and non-TLS connections independently.
  * Added support for using non-default ports.

* :ref:`lib_spm`:

  * Updated the security attribution to configure the peripherals NRF_REGULATORS and NRF_WDT as Non-secure.
  * Added the RTC0 peripheral (as Non-Secure).
  * Fixed a bug where the library attempted to set the IRQ target state of the P0 peripheral.

* :ref:`lib_fota_download`:

  * Added an optional progress event (:cpp:enumerator:`FOTA_DOWNLOAD_EVT_PROGRESS <fota_download::FOTA_DOWNLOAD_EVT_PROGRESS>`) that informs the user of the library how many bytes have been downloaded.
  * Fixed a bug where the library continued downloading even if writing to the DFU target failed.
  * Implemented a mechanism to retry downloads if a socket error occurs.

* :ref:`lib_aws_fota`:

  * Added functionality to resume jobs that are marked as being in progress, which ensures a more robust FOTA operation through AWS IoT jobs.
  * Added offset reporting through the ``statusDetails`` field in an AWS IoT job, which makes it possible to track the progress of a FOTA operation more precisely.
  * Removed the unused ``app_version`` parameter from the :cpp:func:`aws_fota_init` function.
  * Inversed the interpretation of the return value of :cpp:func:`aws_fota_mqtt_evt_handler`.
    0 now indicates success, and no further handling is required.
    1 indicates that further processing is required by the :cpp:func:`mqtt_evt_handler` that called :cpp:func:`aws_fota_mqtt_evt_handler`.

* :ref:`lib_nrf_cloud`:

  * Removed the button/switch pairing method.
  * Added functionality to handle the device configuration in the device shadow.

* :ref:`liblwm2m_carrier_readme`:

  * Updated to version 0.8.1.

* at_host (``lib/at_host``):

  * Changed the default line ending from ``CR`` to ``LF`` in Kconfig to support sending SMS.

* Moved the following libraries from ``drivers/`` to ``lib/``:

  * :ref:`at_cmd_readme`
  * ``lte_link_control``


Updated drivers
---------------

* Moved the following drivers from ``drivers/`` to ``drivers/gps/``:

  * ``gps_sim``
  * ``nrf9160_gps``


BSD library
-----------

* Updated the :ref:`nrfxlib:bsdlib` to version 0.6.1.


nRF5340
=======

This release demonstrates a dual-core solution with the Bluetooth LE Controller running on the network core and the Bluetooth LE Host and application running on the application core of the nRF5340.

Both Nordic Semiconductor's nRF Bluetooth LE Controller and Zephyr's Bluetooth LE Controller have been ported to run on the network core (nrf5340_dk_nrf5340_cpunet).
The application core (nrf5340_dk_nrf5340_cpuapp) can run Bluetooth LE samples from both the |NCS| and Zephyr.

* Added the following sample:

  * :ref:`radio_test` - runs on the network core and demonstrates how to configure the radio in a specific mode and then test its performance.
    This sample was ported from the nRF5 SDK.

* Added support for the :ref:`nRF5340 PDK board (PCA10095)<nrf5340_dk_nrf5340>` with board targets nrf5340_dk_nrf5340_cpunet and nrf5340_dk_nrf5340_cpuapp.
* Updated nrfx to support nRF5340.
* Added NFC support.


Common libraries, drivers, and samples
======================================

* Added the following libraries:

  * :ref:`fprotect_readme` - can be used to protect flash areas from writing.
    This library was extracted from the :ref:`bootloader` sample.
  * ``lib\fatal_error`` - overrides the default fatal error handling in Zephyr.
    By default, all samples perform a system reset if a fatal error occurs.


Updated samples and applications
--------------------------------

* :ref:`bootloader`:

  * Moved the provisioning data (slot sizes/addresses and public keys) to one-time programmable (OTP) memory for nRF9160 devices.
  * Implemented invalidation of public keys.


Updated libraries
-----------------

* :ref:`doc_fw_info`:

  * Renamed ABIs to EXT_APIs.
  * Restructured the :c:type:`fw_info` structure:

    * Renamed the fields ``firmware_size``, ``firmware_address``, and ``firmware_version`` to ``size``, ``address``, and ``version``.
    * Added a field to invalidate the structure.
    * Added reserved fields for future use.
    * EXT_APIs are now in a list at the end of the structure, instead of being available behind a function call.
    * EXT_APIs can now be requested by adding a request structure to a list after the EXT_API list itself.

  * Updated how EXT_API requests are checked.
    Requests are now checked against EXT_APIs by the bootloader before booting the image.
  * Added two new allowed offsets for the struct: 0x0 and 0x1000 bytes.
  * Removed ``memeq()`` in favor of regular ``memcmp()``.
  * Renamed ``__ext_api()`` to ``EXT_API()``, because names starting with ``__`` are reserved for the compiler.
  * Added new configuration options ``CONFIG_*_EXT_API_REQUIRED`` and ``CONFIG_*_EXT_API_ENABLED`` for, respectively, users and providers of EXT_APIs.

* :ref:`lib_dfu_target`:

  * Added the configuration option :option:`CONFIG_DFU_TARGET_MCUBOOT_SAVE_PROGRESS`, which uses Zephyr's :ref:`zephyr:settings` subsystem.
    When this option is enabled, the write progress of an MCUboot style upgrade is stored, so that the progress is retained when the device reboots.
  * Fixed a bug where :cpp:func:`dfu_target_done` logged the error message ``unable to deinitialize dfu resource`` when no target was initialized.

* Moved the following libraries from ``drivers/`` to ``lib/``:

  * :ref:`fprotect_readme`
  * :ref:`st25r3911b_nfc_readme`
  * ``adp536x``
  * ``flash_patch``


Crypto
======

* Added low-level cryptographic test suite using NIST, RFCs, and custom test vectors.
* :ref:`nrf_cc310_mbedcrypto_readme`/:ref:`nrf_cc310_platform_readme` v0.9.2:

  * Fixed power-efficiency issues.
  * Added experimental use of CryptoCell interrupt instead of busy-waits.
* :ref:`lib_hw_cc310`:

  * Added support for CryptoCell interrupt.

nRF Bluetooth LE Controller
===========================

* Updated the :ref:`nrfxlib:ble_controller` libraries:

  * Removed version numbers for the libraries.
  * Added preliminary support for the S140 variant with the nRF5340 device.
    The Bluetooth LE Controller for nRF5340 supports the same feature set as its nRF52 Series counterpart.
  * Moved some APIs to :ref:`nrfxlib:mpsl`.
    The library must now be linked together with MPSL.
  * Made Data Length Extensions a configurable feature.
  * Fixed an issue where an assert could occur when receiving a packet with a CRC error after performing a data length procedure on Coded PHY.

  For details, see the :ref:`nrfxlib:ble_controller_changelog`.

Multi-Protocol Service Layer (MPSL)
===================================

* Updated the :ref:`nrfxlib:mpsl` libraries:

  * Removed version numbers for the libraries.
  * Added a library version with preliminary support for the nRF5340 device.
    The feature set is the same as in the MPSL library for nRF52.

  For details, see the :ref:`nrfxlib:mpsl_changelog`.


Subsystems
==========

Bluetooth Low Energy
--------------------

* Added the following samples:

  * :ref:`peripheral_gatt_dm` - demonstrates how to use the :ref:`gatt_dm_readme`.
  * :ref:`ble_llpm` - showcases the proprietary Low Latency Packet Mode (LLPM) extension.

* Updated the Bluetooth LE samples:

  * Enabled stack protection, assertions, and logging by default.
  * Modified the samples to use the synchronous :cpp:func:`bt_enable` function.

* :ref:`nus_c_readme`, :ref:`bas_c_readme`, and :ref:`dfu_smp_c_readme`:

  * Fixed an issue where it was not possible to subscribe to the service notifications more than once.

* Updated the :ref:`central_uart` sample to handle data packets that are longer than 212 bytes.
  Enabled UART flow control to avoid data loss.

* Enabled UART flow control in the :ref:`peripheral_uart` sample to avoid data loss.

* Changed the :ref:`ble_throughput` sample to prevent it from running Bluetooth LE scanning and advertising in parallel.
  The feature to establish a connection in both master and slave role at the same time is not supported by the Zephyr Bluetooth LE Host.

* :ref:`nrf_bt_scan_readme`:

  * Added an option to update the initial connection parameters.

* :ref:`gatt_dm_readme`:

  * Fixed an issue where service or characteristic allocation failed, but the returned pointer was not checked before accessing the data it pointed to.


NFC
---

* Added the following samples:

  * :ref:`nfc_tnep_tag` and :ref:`nfc_tnep_poller` -  demonstrate how to use the Tag NDEF Exchange Protocol (TNEP).
  * :ref:`nrf-nfc-system-off-sample` - demonstrates how to wake the device from System OFF mode using NFC.
    This sample was ported from the nRF5 SDK.

* Added the following libraries:

  * :ref:`nfc_t4t_cc_file_readme` - reads and parses the Capability Container file that can be found in the Type 4 Tag.
  * :ref:`nfc_t4t_hl_procedure_readme` - performs the NDEF detection procedure for the Type 4 Tag.
  * :ref:`tnep_tag_readme` - implements the Tag NDEF Exchange Protocol (TNEP) for a Tag device.
  * :ref:`tnep_poller_readme` - implements the Tag NDEF Exchange Protocol (TNEP) for a Poller device.

* Updated the NFC samples to enable stack protection, assertions, and logging by default.
* Extended the :ref:`nfc_tag_reader` sample with parsing and printing of the Type 4 Tag content.
* Moved the NFC Platform implementation to the fw-nrfconnect-nrf repository.
  See :ref:`nrfxlib:nfc_integration_notes`.

Multi-Protocol Service Layer (MPSL)
-----------------------------------

* Added MPSL as a new subsystem.
  It integrates :ref:`nrfxlib:mpsl` into the |NCS| environment.

* Added the following sample:

  * :ref:`timeslot_sample` - demonstrates how to use :ref:`nrfxlib:mpsl` and basic MPSL Timeslot functionality.

Setting storage
---------------

* Reduced the default partition size for the settings storage from 24 kB (0x6000) to 8 kB (0x2000).
  This leaves more flash space to the application.

nRF Desktop
===========

* Added a ``ble_qos`` module to maintain channel maps.

Build system
============

* Fixed a bug where a user-defined HEX file that was provided in the static configuration of the :ref:`partition_manager` was not included in the merge operation.

nrfx
====

* Updated to v2.1.0.
  For details, see the `changelog <https://github.com/NordicSemiconductor/nrfx/blob/v2.1.0/CHANGELOG.md>`_.

Zephyr
======

This release is based on Zephyr v2.1.99 (more precisely, Zephyr revision 40175fd3bd), which is between the upstream Zephyr v2.1 and v2.2 releases.

To see a comprehensive list of changes introduced since |NCS| v1.1.0, use the following Git command:

.. code-block:: console

   git log 7d7fed0d2b..40175fd3bd

MCUboot
=======

* Updated to include new features from upstream:

  * New downgrade prevention feature (available when the overwrite-based image update strategy is used)
  * New swap method that removes the need for a scratch partition
  * Bug fixes

  See the `MCUboot release notes <https://github.com/JuulLabs-OSS/mcuboot/blob/master/docs/release-notes.md#version-150>`_ for more information.
  Note that not all features from v1.5.0 are included.

Documentation
=============

* Added or updated documentation for the following samples:

  * nRF9160:

    * :ref:`cloud_client` - added
    * :ref:`gps_with_supl_support_sample` - added
    * :ref:`https_client` - added
    * :ref:`serial_lte_modem` - added
    * :ref:`nrf_coap_client_sample` - updated

  * Bluetooth Low Energy:

    * :ref:`ble_llpm` - added
    * :ref:`peripheral_gatt_dm` - added
    * :ref:`ble_throughput` - updated

  * NFC:

    * :ref:`nrf-nfc-system-off-sample` - added
    * :ref:`nfc_tnep_poller` - added
    * :ref:`nfc_tnep_tag` - added

  * Other:

    * :ref:`radio_test` - added
    * :ref:`timeslot_sample` - added
    * :ref:`bootloader` - updated
    * :ref:`nrf_desktop` - updated


* Added or updated documentation for the following libraries:

  * nRF9160:

    * :ref:`lib_aws_iot` - added
    * :ref:`cloud_api_readme` - added
    * :ref:`modem_key_mgmt` - added
    * :ref:`sms_readme` - added
    * :ref:`supl_client` - added
    * :ref:`at_cmd_readme` - updated
    * :ref:`at_cmd_parser_readme` - updated
    * :ref:`lib_download_client` - updated

  * Bluetooth Low Energy:

    * :ref:`bt_mesh_dk_prov` - added
    * :ref:`latency_c_readme` - added
    * :ref:`latency_readme` - added
    * :ref:`shell_bt_nus_readme` - updated

  * NFC:

    * :ref:`nfc_t4t_cc_file_readme` - added
    * :ref:`nfc_t4t_hl_procedure_readme` - added
    * :ref:`tnep_poller_readme` - added
    * :ref:`tnep_tag_readme` - added

  * Other:

    * :ref:`doc_bl_crypto` - added
    * :ref:`doc_bl_validation` - added
    * :ref:`fprotect_readme` - added
    * :ref:`lib_dfu_target` - updated
    * :ref:`doc_fw_info` - updated
    * :ref:`partition_manager` - updated


* Added or updated the following documentation:

  * Getting started:

    * :ref:`gs_installing` - restructured parts of the content
    * :ref:`gs_programming` - restructured the content and added information about building on the command line
    * :ref:`gs_modifying` - updated the content and added information about configuring an application

  * User guides:

    * :ref:`ug_nrf5340` - added
    * :ref:`ug_thingy91` - added
    * :ref:`ug_ble_controller` - added
    * :ref:`ug_multi_image` - updated with content that was removed from the Zephyr fork

  * nrfxlib:

    * :ref:`nrfxlib:bsdlib` - extended and restructured the content
    * :ref:`nrfxlib:mpsl` - added
    * :ref:`nrfxlib:ble_controller_readme` - updated to match current version of the nRF Bluetooth LE Controller



Known issues
************

nRF9160
=======

* The :cpp:func:`nrf_send` function in the :ref:`nrfxlib:bsdlib` might be blocking for several minutes, even if the socket is configured for non-blocking operation.
  The behavior depends on the cellular network connection.
* The :ref:`gps_with_supl_support_sample` sample stops working if :ref:`supl_client` support is enabled, but the SUPL host name cannot be resolved.
  As a workaround, insert a delay (``k_sleep()``) of a few seconds after the ``printf`` on line 294 in :file:`main.c`.
* The :ref:`asset_tracker` sample might show up to 2.5 mA current consumption in idle mode with ``CONFIG_POWER_OPTIMIZATION_ENABLE=y``.
* The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.
  As a workaround, set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the ``builddir/zephyr/zephyr.map`` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.
* nRF91 fails to receive large packets (over 4000 bytes).
* nrf_connect fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.


Subsystems
==========

Bluetooth Low Energy
--------------------

* Bluetooth LE cannot be used in a non-secure application, for example, an application built for the nrf5340_dk_nrf5340_cpuappns board.
  Use the nrf5340_dk_nrf5340_cpuapp board instead.
* The :ref:`peripheral_hids_keyboard` sample cannot be used with the :ref:`nrfxlib:ble_controller` because the NFC subsystem does not work with the controller library.
  The library uses the MPSL Clock driver, which does not provide an API for asynchronous clock operation.
  NFC requires this API to work correctly.
* When the :ref:`peripheral_hids_mouse` sample is used with the Zephyr Bluetooth LE Controller, directed advertising does not time out and the regular advertising cannot be started.
* The :ref:`bluetooth_central_hids` sample cannot connect to a peripheral that uses directed advertising.
* When running the :ref:`bluetooth_central_dfu_smp` sample, the :option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:ref:`zephyr:smp_svr_sample`).
  However, security is not enabled by default in the Zephyr sample.
* The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.
* On some operating systems, the nrf_desktop application is unable to reconnect to a host.


Bootloader
----------

* Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.
* The immutable bootloader can only be used with the following boards:

  * nrf52840_pca10056
  * nrf9160_pca10090

  It does not work properly on nRF51 and nRF53.


NFC
---

* The :ref:`nfc_tnep_poller` and :ref:`nfc_tag_reader` samples cannot be run on the nRF5340 PDK.
  There is an incorrect number of pins defined in the MDK files, and the pins required for :ref:`st25r3911b_nfc_readme` cannot be configured properly.
* NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted.

Build system
============

* It is not possible to build and program :ref:`secure_partition_manager` and the application individually.

nrfxlib
=======

* In the BSD library, the GNSS sockets implementation is experimental.

MDK (part of nrfx)
==================

* For nRF5340, the pins **P1.12** to **P1.15** are unavailable due to an incorrect pin number definition in the MDK.

MCUboot
=======

* The MCUboot recovery feature using the USB interface does not work.


In addition to the known issues above, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
