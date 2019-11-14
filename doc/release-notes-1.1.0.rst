.. _ncs_release_notes_110:

|NCS| v1.1.0 Release Notes
##########################

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

|NCS| v1.1.0 supports product development with the nRF9160 Cellular IoT device.
It contains reference applications, sample source code, and libraries for Bluetooth Low Energy devices in the nRF52 Series, though product development on these devices is not currently supported with the |NCS|.

.. important::
   |NCS| v1.1.0 does not provide support for nRF5340.
   For nRF5340 support, check out the master branch.

Highlights
**********

* Updated the :ref:`lib_fota_download` library (used by the :ref:`aws_fota_sample` sample, the :ref:`asset_tracker` application, and the :ref:`http_application_update_sample` sample) to support delta updates of the modem firmware in addition to application updates.
* Updated the :ref:`asset_tracker` application with new features and firmware over-the-air (FOTA) support.
* Added an :ref:`lwm2m_carrier` sample that demonstrates how to set up and use the :ref:`liblwm2m_carrier_readme` library to connect to an operator network.
  The new :ref:`liblwm2m_carrier_readme` library provides Verizon network support.
* Added an :ref:`lwm2m_client` that uses Zephyr's :ref:`zephyr:lwm2m_interface` library to push sensor data to a server.


Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.1.0**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`Getting the nRF Connect SDK code <cloning_the_repositories_win>` for more information.


Supported modem firmware
************************

* mfw_nrf9160_1.1.0

Use the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See `Updating the nRF9160 DK cellular modem`_ for instructions.


Supported boards
****************

* PCA10090 (nRF9160 DK)
* PCA20035 (Thingy:91)
* PCA10056 (nRF52840 Development Kit)
* PCA10059 (nRF52840 Dongle)
* PCA10040 (nRF52 Development Kit)
* PCA10028 (nRF51 Development Kit)


Required tools
**************

In addition to the tools mentioned in :ref:`gs_installing`, the following tool versions are required to work with the |NCS|:

.. list-table::
   :header-rows: 1

   * - Tool
     - Version
     - Download link
   * - SEGGER J-Link
     - V6.50b
     - `J-Link Software and Documentation Pack`_
   * - nRF Command Line Tools
     - v10.5.0
     - `nRF Command Line Tools`_
   * - nRF Connect for Desktop
     - v3.3.0 or later
     - `nRF Connect for Desktop`_
   * - dtc (Linux only)
     - v1.4.6 or later
     - :ref:`gs_installing_tools_linux`
   * - GCC
     - See :ref:`Installing the toolchain<gs_installing_toolchain_win>`
     - `GNU Arm Embedded Toolchain`_


As IDE, we recommend to use |SES| (Nordic Edition) version 4.20a.
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

  * :ref:`lwm2m_carrier` - demonstrates how to use the :ref:`liblwm2m_carrier_readme` library to connect to the operator LwM2M network.
  * :ref:`lwm2m_client` - demonstrates how to use Zephyr's :ref:`zephyr:lwm2m_interface` interface to implement a sample LwM2M application.
    This sample can run against an LwM2M demo server, but cannot connect to the operator network.
  * :ref:`usb_uart_bridge_sample` - acts as a serial adapter for Thingy:91, providing USB serial ports for debug output and the ability to send AT commands to the modem.
    This sample runs on the nRF52840 SoC on Thingy:91.

* Added the following libraries:

  * Cloud API (``include/net/cloud.h``) - provides a generic cloud API with an implementation for the nRF Cloud.
  * :ref:`liblwm2m_carrier_readme` (version 0.8.0) - provides support for the Verizon Wireless network support.
  * :ref:`at_notif_readme` - dispatches AT command notifications to registered modules.

* Added the following drivers:

  * nRF9160 GPS (``drivers/nrf9160_gps/``) - configures the modem for GPS operation and controls the GPS data coming from the modem.
    Applications must interact with the GPS using the GPS API (``include/gps.h``) and not use the driver directly.


Updated samples and applications
--------------------------------

* :ref:`asset_tracker`:

  * Updated to use the generic cloud API.
  * Added a user interface module to the application to facilitate the use of buttons, LEDs, buzzer, and NMOS transistors.
  * Added a cloud command decoder module that parses incoming JSON strings.
  * Added an application reboot in the case that MQTT CONNACK is missing from the nRF Cloud server.
  * Fixed a bug where invalid RSRP values (not known / not detectable) were sent to the cloud.
  * Added service information JSON to the device information shadow data.
  * Added light sensor handling.
  * Added firmware over-the-air (FOTA) support for application updates and delta updates of the modem firmware.

* :ref:`aws_fota_sample`:

  * Added a warning message when provisioning certificates stating that the certificates are stored in application core flash (readable flash) and are visible on modem traces.
  * Changed the default security tag to not be the same as nRF Cloud's security tag, to ensure that users do not overwrite their nRF Cloud certificates.
  * Created a separate nRF Cloud configuration option for the sample.
  * Added device shadow update to the sample.
  * Added support for delta updates of the modem firmware using firmware over-the-air (FOTA).

* :ref:`http_application_update_sample`:

  * Added support for delta updates of the modem firmware using firmware over-the-air (FOTA).


Updated libraries
-----------------

* :ref:`modem_info_readme`:

  * Reworked the architecture to support a parameter storage module.
  * Increased the available data.
  * Extended the modem information with IMEI, IMSI, date, and time information.
  * Changed the modem information to be handled as JSON object instead of strings.
  * Fixed known bugs.

* :ref:`lib_fota_download`:

  * Changed to use the :ref:`lib_dfu_target` abstraction.
  * Added support for performing MCUboot upgrades.
    To support this, ``CONFIG_SECURE_BOOT=y`` must be set.
    To send an upgrade, provide the path to both the S0 and the S1 candidate (separated by a space) to the file parameter of :cpp:func:`fota_download_start`.
    Both candidates are generated by the build system if ``CONFIG_MCUBOOT_BUILD_S1_VARIANT=y`` is set.

* :ref:`lib_aws_fota` and :ref:`lib_aws_jobs`:

  * Implemented fetching of AWS jobs.
    This allows a device to be updated if it is offline at the time the update is created.
  * Refactored the code (improved function names, extracted common functionality, re-used topic buffers given to AWS jobs).
  * Added unit tests for AWS jobs.
  * Removed device shadow update from the library.

* MQTT library:

  * Dropped the nRF Connect SDK copy of the MQTT library and adopted Zephyr's :ref:`zephyr:mqtt_socket_interface` library instead.

* :ref:`lib_spm`:

  * Added a new non-secure-callable function :cpp:func:`spm_firmware_info`.

* :ref:`lib_nrf_cloud`:

  * Adopted to the new device shadow format.

* at_host (``lib/at_host``):

  * Updated to use a dedicated workqueue instead of the system workqueue.
  * Miscellaneous fixes and improvements.

* :ref:`at_cmd_parser_readme`:

  * Refactored the library.

* LTE link control (``include/lte_lc.h``):

  * Added PLMN lock option (default: false).
  * Added PDN connection authentication option (default: false).
  * Added modem modes without GPS (LTE-M or NB-IoT only).
  * Added fallback to secondary LTE network mode (LTE-M/NB-IoT) if the device fails to connect using the primary network mode.
  * Added a function to get the periodic TAU and active time settings from the current network registration status.
  * Added a function to get the current functional mode.
  * Added a function to get the current network registration status (not registered, roaming, registered home network).
  * Added a function to get and set the system mode (LTE-M, NB-IoT, and GPS).
  * Other minor improvements and fixes.


Updated drivers
---------------

* :ref:`at_cmd_readme`:

  * Added an option to initialize the driver manually.
  * Fixed detection of CME / CMS errors.


BSD library
-----------

* Updated the :ref:`nrfxlib:bsdlib` to version 0.5.0.
* Updated bsdlib_init() to return the value of :cpp:func:`bsd_init` instead of (only) zero.
* Added functionality that overrides untranslated errnos set by the BSD library with a magic word (0xBAADBAAD), instead of EINVAL, and prints a log message.
  If ASSERTs are enabled, the application will assert.
* Made DFU, PDN, and RAW socket available through the socket offloading mechanism.
* Updated samples that use the BSD library to use ``CONFIG_NET_NATIVE=n`` to save RAM and ROM.

Board support
-------------

* Thingy:91 (``nrf9160_pca20035``):

  *  Removed support for earlier hardware versions of ``nrf52840_pca20035`` and ``nrf9160_pca20035``.
     From |NCS| v1.1.0, only the latest hardware version is supported.
  *  Removed configurations specific to the deprecated board versions from the :ref:`asset_tracker` application.


Common libraries and drivers
============================

* Added the following libraries:

  * :ref:`lib_dfu_target` - abstracts the specific implementation of how a DFU procedure is implemented.
    This library supports delta updates of the modem firmware and application updates.
  * :ref:`fprotect_readme` - uses hardware (BPROT, ACL, or SPU) to protect flash areas from being changed.
    This library is used by the immutable bootloader.

* Added the following drivers:

  * Flash patch (``nrf/drivers/flash_patch``) - writes to UICR to disable flash patch functionality during the first boot of the image.


Updated libraries
-----------------

* Immutable bootloader (``nrf/subsys/bootloader``):

  * Created a bl_validate_firmware() function that can be used to ensure that a received upgrade will be accepted by the immutable bootloader.
    This function is available to be called from subsequent boot steps.
  * Refactored the boot validation code.
  * Moved the provision page next to the code (instead of at the end of the flash).
  * Removed custom startup and debug code.

* :ref:`doc_fw_info`:

  * Renamed fw_metadata to fw_info.
    Most functions, macros, etc. have changed name as a result.
  * Added documentation.
  * Updated to allow the firmware information struct to be placed at one of three offsets (0x200, 0x400, 0x800).
    When looking for firmware info, you must search all these offsets.
    Changed the default to 0x200.

* :ref:`dk_buttons_and_leds_readme`:

  * Added support for boards with LED or button pins on different GPIO ports.

* :ref:`lib_download_client`:

  * Added support for specifying an access point name for the packet data network.
  * Moved the header file to ``include/net``.
  * Updated to report a :cpp:enumerator:`DOWNLOAD_CLIENT_EVT_ERROR <dl_client::DOWNLOAD_CLIENT_EVT_ERROR>` error when unable to parse the HTTP header, with error reason EBADMSG.
  * Returning 0 when receiving a :cpp:enumerator:`DOWNLOAD_CLIENT_EVT_ERROR <dl_client::DOWNLOAD_CLIENT_EVT_ERROR>` will now let the library retry the download.

Updated drivers
---------------

* ADP536X (``include/drivers/adp536x.h``):

  * Added buck discharge resistor configuration.

Crypto
======

* Added the following drivers:

  * :ref:`lib_hw_cc310` (using :ref:`nrf_cc310_platform_readme`) - a Zephyr driver providing initialization of Arm CC310 hardware accelerator.

    * Initializes Arm CC310 hardware with or without RNG support dependent on configuration.
    * Initializes Zephyr RTOS mutexes used in :ref:`nrf_cc310_platform_readme` and :ref:`nrf_cc310_mbedcrypto_readme` libraries.
    * Initializes abort handling in :ref:`nrf_cc310_platform_readme` and :ref:`nrf_cc310_mbedcrypto_readme` libraries.

  * :ref:`lib_entropy_cc310` (using :ref:`nrf_cc310_platform_readme`) - a Zephyr driver providing entropy from Arm CC310 hardware accelerator.

Updated libraries
-----------------

* :ref:`nrf_cc310_platform_readme` v0.9.1 (experimental release):

  * Added support for initialization of Arm CC310 hardware accelerator with or without RNG support.
  * Added support for setting RTOS-specific mutex and abort handling in Arm CC310 crypto libraries.
  * Added APIs to generate entropy.

* :ref:`nrf_cc310_mbedcrypto_readme` v0.9.1 (experimental release):

  * Added support to do hardware-accelerated cryptography using Arm CryptoCell CC310 in select architectures.
  * Miscellaneous bugfixes.

* :ref:`nrfxlib:nrf_security`:

  * Refactored build system and configuration.
  * Fixed bugs in the AES glue layer preventing correct decryption.
  * Upgraded to point to mbed TLS version 2.16.3.
  * Integrated with :ref:`nrf_cc310_platform_readme` and :ref:`nrf_cc310_mbedcrypto_readme` version 0.9.1.


nRF BLE Controller
==================

* Updated the :ref:`nrfxlib:ble_controller` to v0.3.0-3.prealpha.
  For details, see the :ref:`nrfxlib:ble_controller_changelog`.

* Improved the default static memory pool allocation.
  The controller now determines its static memory pool size based on the maximum Link Layer packet length.
  This is determined by the Kconfig macro :option:`CONFIG_BT_CTLR_DATA_LENGTH_MAX` (if defined), or else the minimum packet length (which is 27 B).
  The memory pool is large enough to facilitate one master and one slave link.

* Added support for connection intervals less than the standard minimum of 7.5 ms.
  Note that this a proprietary feature that is not Bluetooth Low Energy compliant.
  This proprietary feature is named `Low Latency Packet Mode (LLPM)`.


Subsystems
==========

Bluetooth Low Energy
--------------------

* :ref:`gatt_pool_readme`:

  * Removed HID dependencies.
  * Fixed pool definition macros.
  * Added UUID check when allocating descriptor.

* :ref:`hids_readme`:

  * Added an option to configure report permissions.
    The user can configure permissions globally or individually for each report.

* :ref:`hids_readme` and :ref:`hids_c_readme`:

  * Extracted common declarations to a separate header file.

* :ref:`gatt_dm_readme`:

  * Fixed assert that occurs when discovering all services and the end handle is set to 0xFFFF.

* :ref:`lbs_readme`, :ref:`nus_service_readme`, and :ref:`throughput_readme`:

  * Updated to allow to define services as static using ``BT_GATT_SERVICE_DEFINE``.

* :ref:`bas_c_readme`:

  * Extended API to enable periodical reading of the characteristic value.

* :ref:`nrf_bt_scan_readme`:

  * Added matching data in the filter match event, to notify which data triggered the match.

* Updated the Bluetooth Low Energy samples:

  * Added logging when security status changes.
  * Enabled bonding support.
  * Fixed Work Queue stack setting in :ref:`central_uart` and :ref:`bluetooth_central_hids`.
  * Removed needless Work Queue instance in :ref:`peripheral_hids_mouse`.
  * Fixed SMP time-out on nRF51 in HIDS samples.
  * Added "Numeric Comparison" pairing support and aligned LED usage in peripheral samples.
  * Added nRF52840 Dongle support in :ref:`peripheral_lbs`.

* Fixed default connections configuration when selecting :option:`CONFIG_BT_LL_NRFXLIB`.


NFC
---

* Added the following libraries:

  * :ref:`nfc_t2t_parser_readme` - reads and parses NFC Type 2 Tags.
  * :ref:`nfc_ndef_parser_readme` - interprets NDEF messages and records.
  * :ref:`nfc_t4t_apdu_readme` - provides functions to encode and decode C-APDU and raw R-APDU data.
  * :ref:`nfc_t4t_isodep_readme` - implements the NFC ISO-DEP protocol.

* Extended the :ref:`nfc_tag_reader` sample with parsing and printing of the Type 2 Tag content, including NDEF messages.

* Added a tag sleep callback to the :ref:`st25r3911b_nfc_readme` driver.

nrfx
====

* Updated to v1.8.1.
  For details, see the `changelog <https://github.com/NordicSemiconductor/nrfx/blob/v1.8.1/CHANGELOG.md>`_.


Documentation
=============

* Added or updated documentation for the following samples:

  * nRF9160:

    * :ref:`at_client_sample`
    * :ref:`lwm2m_carrier`
    * :ref:`lwm2m_client`
    * :ref:`aws_fota_sample`
    * :ref:`http_application_update_sample`

  * Bluetooth Low Energy:

    * :ref:`peripheral_hids_keyboard`
    * :ref:`peripheral_hids_mouse`
    * :ref:`peripheral_lbs`
    * :ref:`peripheral_uart`

  * Other:

    * :ref:`bootloader`
    * :ref:`usb_uart_bridge_sample`

* Added or updated documentation for the following libraries:

  * nRF9160:

    * :ref:`at_notif_readme`
    * :ref:`doc_fw_info`
    * :ref:`lib_aws_fota`
    * :ref:`lib_aws_jobs`
    * :ref:`lib_fota_download`
    * :ref:`modem_info_readme`

  * Bluetooth Low Energy:

    * :ref:`bas_c_readme`
    * :ref:`latency_readme`
    * :ref:`latency_c_readme`
    * :ref:`bt_mesh`


  * Other:

    * :ref:`lib_dfu_target`
    * :ref:`fprotect_readme`
    * :ref:`lib_entropy_cc310`
    * :ref:`lib_hw_cc310`
    * :ref:`nfc_ndef_le_oob`
    * :ref:`nfc_ndef_parser_readme`
    * :ref:`nfc_t2t_parser_readme`
    * :ref:`nfc_t4t_apdu_readme`
    * :ref:`nfc_t4t_isodep_readme`
    * :ref:`profiler`
    * :ref:`lib_secure_services`


* Added or updated the following documentation:

  * :ref:`gs_assistant`
  * :ref:`gs_installing`
  * :ref:`doc_styleguide`
  * :ref:`ncs-app-dev`
  * :ref:`ug_bootloader`
  * :ref:`dev-model`
  * :ref:`ug_nrf9160`
  * :ref:`nrfxlib:ble_controller`
  * :ref:`nrfxlib:bsdlib`
  * :ref:`nrfxlib:nrf_cc310_platform_readme`
  * :ref:`nrfxlib:mpsl`
  * :ref:`nrf_security_readme`
  * :ref:`mcuboot:mcuboot_wrapper`
  * :ref:`mcuboot:mcuboot_ncs`


Known issues
************


nRF9160
=======

* Deprecation warning: The nrf_inbuilt_key API in the :ref:`nrfxlib:bsdlib` will be removed in a future release.
  A replacement library that wraps the AT commands for ``AT%CMNG`` will be available in the |NCS|.
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

* :option:`CONFIG_BT_HCI_TX_STACK_SIZE` must be set to 1536 when selecting :option:`CONFIG_BT_LL_NRFXLIB`.
* The :ref:`nrfxlib:ble_controller` 0.3.0-3.prealpha might assert when receiving a packet with an CRC error on LE Coded PHY after performing a DLE procedure where RX Octets is changed to a value above 140.
* :option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` must be set to 2048 when selecting :option:`CONFIG_BT_LL_NRFXLIB` on :ref:`central_uart` and :ref:`central_bas`.
* :option:`CONFIG_NFCT_IRQ_PRIORITY` must be set to 5 or less when selecting :option:`CONFIG_BT_LL_NRFXLIB` on :ref:`peripheral_hids_keyboard`.
* When selecting :option:`CONFIG_BT_LL_NRFXLIB`:
  If a directed high duty cycle advertiser times out, the application might have to wait a short time before starting a new connectable advertiser.
  Otherwise, starting the advertiser will fail.
* Bluetooth Low Energy peripheral samples are unstable in some conditions (when pairing and bonding are performed and then disconnections/re-connections happen).
* When running the :ref:`bluetooth_central_dfu_smp` sample, the :option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:ref:`zephyr:smp_svr_sample`).
  However, security is not enabled by default in the Zephyr sample.
* The central samples (:ref:`central_uart`, :ref:`bluetooth_central_hids`) do not support any pairing methods with MITM protection.
* On some operating systems, the nrf_desktop application is unable to reconnect to a host.
* central_uart: A too long 212-byte string cannot be handled when entered to the console to send to peripheral_uart.
* On nRF51 devices, BLE samples that use GPIO might crash when buttons are pressed frequently.
  In such case, the GPIO ISR introduces latency that violates real-time requirements of the Radio ISR.
  nRF51 is more sensitive to this issue than nRF52 (faster core).


Bootloader
----------

* Public keys are not revoked when subsequent keys are used.
* The bootloader does not work properly on nRF51.
* Building and programming the immutable bootloader (see :ref:`ug_bootloader`) is not supported in SEGGER Embedded Studio.
* The immutable bootlader can only be used with the following boards:

  * nrf52840_pca10056
  * nrf9160_pca10090


DFU and FOTA
------------

* When using :ref:`lib_aws_fota`, no new jobs are received on the device if the device is reset during a firmware upgrade or loses the MQTT connection.
  As a workaround, delete the stalled in progress job from AWS IoT.
* :ref:`lib_fota_download` does not resume a download if the device loses the connection.
  As a workaround, call :cpp:func:`fota_download_start` again with the same arguments when the connection is re-established to resume the download.
* When using the mcuboot target in :ref:`lib_dfu_target`, the write/downloaded offset is not retained when the device is reset.
* In the :ref:`aws_fota_sample` and :ref:`http_application_update_sample` samples, the download is stopped if the socket connection times out before the modem can delete the modem firmware.
  As a workaround, call :cpp:func:`fota_download_start` again with the same arguments.
  A fix for this issue is available in commit `38625ba7 <https://github.com/NordicPlayground/fw-nrfconnect-nrf/commit/38625ba775adda3cdc7dbf516eeb3943c7403227>`_.
* If the last fragment of a :ref:`lib_fota_download` is received but is corrupted, or if the last write is unsuccessful, the library emits an error event as expected.
  However, it also emits an apply/request update event, even though the downloaded data is invalid.

NFC
---

* NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted.

Build system
============

* It is not possible to build and program :ref:`secure_partition_manager` and the application individually.

nrfxlib
=======

* In the BSD library, the GNSS sockets implementation is experimental.


nrfx v1.8.1
===========

* nrfx_saadc driver:
  Samples might be swapped when buffer is set after starting the sample process, when more than one channel is sampled.
  This can happen when the sample task is connected using PPI and setting buffers and sampling are not synchronized.
* The nrfx_uarte driver does not disable RX and TX in uninit, which can cause higher power consumption.


In addition to the known issues above, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.
