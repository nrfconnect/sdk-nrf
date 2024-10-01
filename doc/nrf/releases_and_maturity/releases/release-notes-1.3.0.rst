.. _ncs_release_notes_130:

|NCS| v1.3.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
nRF53 Series devices (which are pre-production) and Thread, Zigbee, and Bluetooth Mesh protocols are supported for development in v1.3.0 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.


Highlights
**********

* New home for the |NCS|
* Support for Thread and Zigbee
* Support for Bluetooth Mesh
* nRF Desktop supported for production
* New nRF9160 samples and libraries
* Support for using external flash for the secondary slot in MCUboot
* General updates and bugfixes

A new home
**********

Starting with this release, the |NCS| has moved to a new GitHub organization, `nrfconnect <https://github.com/nrfconnect>`_.
All repositories were renamed to provide clear and concise names that better reflect the composition of the codebase.
If you are a new |NCS| user, there are no actions you need to take.
If you used previous releases of the |NCS|, you should follow the instructions in :ref:`repo_move`.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.3.0**.
Check the ``west.yml`` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.


Supported modem firmware
************************

This version of the |NCS| has been tested with the following modem firmware for cellular IoT applications:

* mfw_nrf9160_1.2.0

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Changelog
*********

The following sections provide detailed lists of changes by component.


nRF9160
=======

* Added:

  * :ref:`lib_date_time` library - maintains the current date-time UTC from multiple time sources (modem, NTP servers).
  * :ref:`lib_ftp_client` library - can be used to download or upload FTP server files.
  * :ref:`lib_nrf_cloud_agps` library - provides a way to request and process A-GPS data from nRF Connect for Cloud to be used with the nRF9160 SiP.
    The :ref:`agps_sample` sample demonstrates how to use the library.
  * :ref:`connectivity_bridge` application with dual UART and Bluetooth LE support - replaces the USB-UART bridge sample.

* :ref:`gps_with_supl_support_sample` sample:

  * Reworked the API to make it more flexible.
  * Added an option to register an event handler.
    All event types from the GPS will be received, as opposed to the previous single-trigger source.

* :ref:`lib_nrf_cloud` library:

  * Added FOTA download progress monitoring.
  * Added :ref:`lib_nrf_cloud_agps` support.
  * Added support for MQTT persistent sessions.
  * Moved cloud connection polling out of the application into a separate nrf_cloud thread.
  * Exposed MQTT topics to the nRF Cloud library.

* :ref:`lib_aws_fota` library:

  * Switched from Zephyr's JSON parser to cJSON.
  * Added support for MQTT persistent sessions.

* :ref:`lib_fota_download` library:

  * Added support for specifying a TCP port for connections.
  * Added support for specifying the APN to be used.

* :ref:`modem_key_mgmt` library:

  * Added a :c:func:`modem_key_mgmt_cmp` function to the API, which allows to compare with a credential stored in the modem.
  * Various minor fixes.

* :ref:`modem_info_readme` library:

  * Fixed an error in reading :c:enumerator:`MODEM_INFO_RSRP`.
  * Added APN readout.

* :ref:`lte_lc_readme` library:

  * Added the concept of events.
  * Allowed to modify PSM/eDRX parameters at runtime.
  * Allowed to modify PDP context and PDN authentication at runtime.

* :file:`lib/at_host` library:

  * Updated to use CR termination by default.
    This reverts the old behavior.

* :ref:`http_application_update_sample` sample:

  * Added support for BSD library initialization at system startup.
  * Changed default download host and file name.

* :ref:`lwm2m_client` sample:

  * Fixed an invalid value of the APN resource in the Connectivity Monitoring object.
  * Set cell ID information in the Connectivity Monitoring object.
  * Added delay to the reboot procedure, to allow to send a reply to the server.
  * Added queue mode support.

* nRF9160: Asset Tracker application:

  * Improved stability.
  * Various bugfixes.

* :ref:`serial_lte_modem` application:

  * Moved from samples to applications.
  * Added low-power idle mode.
  * Added support for generic proprietary AT commands.
  * Added support for BSD socket, TCP/UPD, ICMP, GPS, MQTT, and FTP proprietary AT commands.
  * Added support for communicating to an external MCU over UART.
  * Added support for transmitting arbitrary hexadecimal data.
  * Added support for TCP/UDP proxy service (optional).
  * Added support for SUPL A-GPS.

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to version 0.9.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

* BSD library:

  * Updated to version 0.7.3.
    See the :ref:`nrfxlib:nrf_modem_changelog` for detailed information.

* :ref:`supl_client`:

  * Provided version 0.6.0 for download.
    This new version is required for compatibility with |NCS| v1.3.0.

nRF5
====

The following changes are relevant for the nRF52 and nRF53 Series.

nRF5340 SoC
-----------

* Added :ref:`nrf5340_empty_app_core` for samples running purely on the nRF5340 network core.
* When building a Bluetooth sample for nRF5340, the :zephyr:code-sample:`bluetooth_hci_ipc` sample is now automatically built as child image.

Multiprotocol Service Layer (MPSL)
-----------------------------------

See the :ref:`nrfxlib:mpsl_changelog` for detailed information.

* Added TX power envelope functionality.
* Added support for using a low-swing and full-swing LF clock.

Thread
------

Added the following samples:

* :ref:`ot_cli_sample` sample - very basic sample with Thread enabled and shell support to allow using the OpenThread Command Line Interface.
  This sample is needed for Thread certification.
* :ref:`coap_client_sample` sample and :ref:`coap_server_sample` sample - two samples demonstrating how to use the CoAP protocol over a Thread network.
* Thread Sleepy End Device sample - a variation of the :ref:`coap_client_sample` sample that works with lower power consumption as Minimal Thread Device type.

See the :ref:`ug_thread` user guide to get started.

Zigbee
------

Added initial support for the Zigbee network protocol:

* :ref:`nrfxlib:zboss` - port of the ZBOSS stack to the |NCS|, provided as a closed stack binary in nrfxlib.
  The stack provided in this release has not been certified by the `Zigbee Alliance`_.
* Light control sample consisting of :ref:`zigbee_light_bulb_sample`, :ref:`zigbee_light_switch_sample`, and :ref:`zigbee_network_coordinator_sample` - ported from the nRF5 SDK for Thread and Zigbee.
  The sample demonstrates all Zigbee roles (coordinator, router, end device).
  The :ref:`zigbee_light_switch_sample` sample provides sleepy end device support.

See the :ref:`ug_zigbee` user guide to get started.

Bluetooth Low Energy
--------------------

* Added the :ref:`bt_enocean_readme` library and :ref:`enocean_sample` sample.
* Introduced role section in the :ref:`ble_throughput` sample.
  The user must now type "s" or "m" to select the application role in the throughput test.
* Enabled directed advertising in the :ref:`peripheral_hids_mouse` sample.
  Added handling of directed advertising in the :ref:`central_bas` and :ref:`bluetooth_central_hids` samples.
* Optimized RAM usage in the :ref:`peripheral_gatt_dm` sample by approximately 50 percent.
* Various fixes and improvements in Bluetooth LE samples.

nRF Bluetooth LE Controller
~~~~~~~~~~~~~~~~~~~~~~~~~~~

See the :ref:`nrfxlib:softdevice_controller_changelog` for detailed information.

* Added feature to configure TX power per role/connection.

nRF Desktop
~~~~~~~~~~~

* Added support for new hardware:

  * ``nrf52833dk_nrf52833``
  * ``nrf52833dongle_nrf52833``
  * ``nrf52820dongle_nrf52820``

* Added a fail-safe erase module that, if enabled, erases settings after a crash.
* Added low-latency lock to disable slave latency until the device enters power down.
* Improved connection parameters update.
* Removed the shell module.
  Shell configuration is now done using Zephyr configuration options.
* Updated the application to boot using the immutable bootloader when background DFU is enabled (the immutable bootloader boots the application directly from any slot).
* MCUboot is used only on configurations with serial recovery through USB.
* The peripheral sends the first report with delay (configurable), allowing the central to be ready.
* Updated the config channel to use dynamic module IDs.
* Reworked and unified HID report data passing.
* Improved the report rate through TX power changes.
* Improved usage of setting handlers.
* Improved filtering of peripherals when the central does scanning.
* Added passkey support on keyboard (passkey required during device bonding).
* Added support for system control reports on keyboard.
* Various small improvements and bug fixes.

See the :ref:`nrf_desktop` documentation to get started.

Bluetooth Mesh
--------------

* Added the following samples:

  * :ref:`bluetooth_mesh_light` sample - demonstrates how to set up a basic Mesh server model application and control LEDs with the Bluetooth Mesh.
  * :ref:`bluetooth_mesh_light_switch` sample - demonstrates how to set up a basic Mesh client model application and control LEDs with the Bluetooth Mesh.

* Added the following Mesh models:

  * :ref:`bt_mesh_lightness_readme`
  * :ref:`bt_mesh_light_ctrl_readme`
  * :ref:`bt_mesh_sensor_models`

See the :ref:`ug_bt_mesh` user guide to get started.

Enhanced ShockBurst (ESB)
-------------------------

* Renamed the subsystem to ``esb`` and moved it from :file:`subsys/enhanced_shockburst` to :file:`subsys/esb`.
* Renamed the header file and all data types to align with the name change.

Common
======

The following changes are relevant for all device families.

Boards
------

All boards have been renamed.
The following boards are defined in Zephyr:

.. list-table::
   :header-rows: 1

   * - Old name
     - New name
   * - nrf52810_pca10040
     - nrf52dk_nrf52810
   * - nrf52_pca10040
     - nrf52dk_nrf52832
   * - nrf52833_pca10100
     - nrf52833dk_nrf52833
   * - nrf52811_pca10056
     - nrf52840dk_nrf52811
   * - nrf52840_pca10056
     - nrf52840dk_nrf52840
   * - nrf52840_pca10059
     - nrf52840dongle_nrf52840
   * - nrf9160_pca10090
     - nrf9160dk_nrf9160
   * - nrf52840_pca10090
     - nrf9160dk_nrf52840
   * - nrf52_pca20020
     - thingy52_nrf52832
   * - nrf5340_dk_nrf5340
     - nrf5340pdk_nrf5340

The following boards are defined in sdk-nrf:

.. list-table::
   :header-rows: 1

   * - Old name
     - New name
   * - nrf52840_pca20041
     - nrf52desktop_gaming_mouse_nrf52840
   * - nrf52810_pca20045
     - nrf52desktop_mouse_nrf52810
   * - nrf52_pca20044
     - nrf52desktop_mouse_nrf52832
   * - nrf52_pca20037
     - nrf52desktop_keyboard_nrf52832
   * - nrf9160_pca20035
     - thingy91_nrf9160
   * - nrf52840_pca20035
     - thingy91_nrf52840
   * - nrf52833_pca10111
     - nrf52833dongle_nrf52833
   * - nrf52833_pca10114
     - nrf52820dongle_nrf52820

nrfx
----

See the `Changelog for nrfx 2.2.0`_ for detailed information.

Crypto
------

* Added nRF Oberon v3.0.5 with a companion library that provides an mbed TLS frontend for groups of cryptographic algorithms (SHA, ECC, ECJPAKE, AES).
  See the :ref:`nrfxlib:crypto_changelog_oberon` for detailed information.
* Added nRF Oberon as a backend in the :ref:`nrf_security`.

NFC
---

* Added a :ref:`nfc_ndef_le_oob_rec_parser_readme` for decoding data used for Bluetooth LE OOB pairing.
* Added support for nRF5340 in the :ref:`nrf-nfc-system-off-sample` sample.
* Aligned file and API naming in the :ref:`lib_nfc_ndef` libraries.

Immutable bootloader
--------------------

* Exposed :c:func:`fw_info_ext_api_provide` as an :ref:`external API <doc_fw_info_ext_api>`, so that :doc:`mcuboot:index-ncs` can use it to provide external APIs from the :ref:`bootloader` to its images.
  This means that requesting external APIs in applications works even if MCUboot is included.
* Fixed a bug so that the :ref:`bootloader` works with nRF5340 SPU flash regions.
* Added a :ref:`doc_bl_storage` library:

  * Renamed :file:`provision.h` and :file:`provision_flash.h` to ``bl_storage`` and allowed including the library in the application.
  * Added documentation and tests.
  * Added a monotonic version counter.
    The immutable bootloader will not boot an application that has a lower version than the monotonic counter.

Secure Partition Manager (SPM)
------------------------------

* Added support for disabling some services in the nRF9160: Secure Services sample.
  It now works in more bootloader configurations.
* Disabled ``CONFIG_SPM_SERVICE_PREVALIDATE`` in the Secure Partition Manager (SPM) library, because this option requires the immutable bootloader.
* Updated the Secure Partition Manager (SPM) library to make it compatible with nRF5340 (with or without `anomaly 19`_).

CPU load measurement
--------------------

Added :ref:`cpu_load`, a debug module for measuring CPU load.

iCalendar parser
----------------

Added :ref:`icalendar_parser_readme`, a library to parse iCalendar data format.

MCUboot
=======

* Added external flash secondary slot MCUboot.
  See :ref:`ug_bootloader_external_flash`.
* Added an option to build Ed25519 signature validation without using mbedTLS, by relying on a bundled tinycrypt-based SHA-512 implementation.
* Replaced CBOR decoding in serial recovery with code generated from a CDDL description.
  This results in a flash footprint reduction of more than 1 KB while serial recovery is enabled.
* Added support for X25519-encrypted images.
* Added rollback protection.
  There is support for a hardware rollback counter that must be provided as part of the platform, as well as a software solution that protects against some types of rollback.
* Added an optional boot record in shared memory to communicate boot attributes to code that is run later.
* Added a cleanup of the Arm core before the application is booted.
* Allowed recovery over USB CDC ACM with logs enabled.
* Various fixes to work with the latest Zephyr version.

Build system
============

* Added support for :ref:`ug_multi_image` for multi-core projects.
* Facilitated defining non-secure boards out of tree.
  Any board that matches ``*_ns`` or ``*ns`` is now considered non-secure, and its child images board is set to the secure variant.
* Added support for defining external flash in the :ref:`partition_manager`.

Zephyr
======

* Updated the time-out type to :c:type:`k_timeout_t`, because the Zephyr kernel deprecated its integer type as time-out in different APIs (timeout, scheduling, ...).
* Updated all files to use the C/C++ Devicetree generic API, because the C/C++ Devicetree value fetching API was reworked in Zephyr so that it uses compatible strings and new function-like macros to match properties.
  See :ref:`zephyr:dt-from-c`.


Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* Moved Kconfig options to a separate documentation set
* :ref:`doc_build` - updated to reflect that Kconfig options are now built as a separate documentation set
* :ref:`doc_styleguide` - updated
* :ref:`gs_assistant` - updated to recommend the use of the toolchain manager
* :ref:`gs_installing` - updated to align the instructions for all operating systems, added :ref:`repo_move`
* :ref:`gs_programming` - updated :ref:`programming_board_names`
* :ref:`gs_testing` - added How to connect with LTE Link Monitor
* :ref:`gs_modifying`  - added :ref:`gs_modifying_build_types`
* :ref:`app_build_system` - updated |NCS| additions
* :ref:`ug_nrf9160` - added :ref:`Concurrent GPS and LTE <nrf9160_gps_lte>`
* :ref:`ug_nrf5340` - updated
* :ref:`ug_nrf52` - added
* :ref:`ug_thingy91` - added :ref:`thingy91_serialports`
* :ref:`ug_nfc` - added
* :ref:`ug_bootloader` - added :ref:`ug_bootloader_adding`
* Cloud client - updated
* :ref:`crypto_test` - added
* :ref:`libraries` - improved the structure of the library documentation
* :ref:`bt_mesh` (and subpages) - added
* :ref:`nrf_bt_scan_readme` - updated
* ``at_cmd`` library - added
* :ref:`coap_utils_readme` - added
* :ref:`tnep_poller_readme` and :ref:`tnep_tag_readme` - updated
* :ref:`nrf_desktop_config_channel_script` - updated
* BSD library - added documentation about GNSS socket.
* :ref:`nrfxlib:mpsl` - added documentation about :ref:`nrfxlib:mpsl_timeslot`, Radio notifications, and :ref:`nrfxlib:mpsl_tx_power_control`
* :ref:`nrfxlib:nfc` - added documentation about :ref:`nrfxlib:type_2_tag` and :ref:`nrfxlib:type_4_tag`, updated the :ref:`nrfxlib:nfc_integration_notes`
* :ref:`nrf_security` - updated

Known issues
************

nRF9160
=======

* The nRF9160: Asset Tracker application prints warnings and error messages during successful FOTA. (NCSDK-5574)
* The :ref:`lte_sensor_gateway` sample crashes when Thingy:52 is flipped. (NCSDK-5666)

From v1.2.0
-----------

* The :c:func:`nrf_send` function in the BSD library might be blocking for several minutes, even if the socket is configured for non-blocking operation.
  The behavior depends on the cellular network connection.
* The nRF9160: Asset Tracker sample might show up to 2.5 mA current consumption in idle mode with ``CONFIG_POWER_OPTIMIZATION_ENABLE=y``.
* The SEGGER Control Block cannot be found by automatic search by the RTT Viewer/Logger.
  As a workaround, set the RTT Control Block address to 0 and it will try to search from address 0 and upwards.
  If this does not work, look in the ``builddir/zephyr/zephyr.map`` file to find the address of the ``_SEGGER_RTT`` symbol in the map file and use that as input to the viewer/logger.
* nRF91 fails to receive large packets (over 4000 bytes).
* nrf_connect fails if called immediately after initialization of the device.
  A delay of 1000 ms is required for this to work as intended.

nRF5
====

nRF5340
-------

* FOTA with the :zephyr:code-sample:`smp-svr` does not work.

nRF52820
--------

* The :file:`CMakeLists.txt` file for developing applications that emulate nRF52830 on the nRF52833 DK is missing.

  As a workaround, create a :file:`CMakeLists.txt` file in the :file:`ncs/zephyr/boards/arm/nrf52833dk_nrf52820` folder with the following content::

    zephyr_compile_definitions(DEVELOP_IN_NRF52833)
    zephyr_compile_definitions(NRFX_COREDEP_DELAY_US_LOOP_CYCLES=3)

  You can `download this file <nRF52820 CMakeLists.txt_>`_ from the upstream Zephyr repository.
  After you add it, the file is automatically included by the build system.



Multi-Protocol Service Layer (MPSL)
-----------------------------------

* The Antenna Diversity feature is not supported on nRF52840 devices. (KRKNWK-6361)

Thread
------

* It is not possible to build Thread samples using SEGGER Embedded Studio (SES).
  SES does not support .cpp files in |NCS| projects. (NCSDK-5014)
* It is not possible to provision the :ref:`coap_client_sample` sample to servers that it cannot directly communicate with.
  This is because Link Local Address is used for communication. (KRKNWK-6358)
* The "diag" command is not yet supported by Thread in the |NCS|. (KRKNWK-6408)

Zigbee
------

* There might be a noticeable delay (~220 ms) between calling the ZBOSS API and on-the-air activity.
* ZBOSS alarms are inaccurate.
  On average, they last longer by 6.4 percent.
  It is recommended to use Zephyr alarms.

Bluetooth Low Energy
--------------------

* High-throughput transmission can deadlock the receive thread if the connection is suddenly disconnected. (NCSDK-5711)
* Bi-directional high-throughput traffic can deadlock the transmit thread. (NCSDK-5711)

Bluetooth Mesh
--------------

* On nRF5340, only the :ref:`nrfxlib:softdevice_controller` is supported for Bluetooth Mesh. (NCSDK-5580)

Common
======

Samples and applications
------------------------

* The build configuration consisting of :ref:`bootloader`, Secure Partition Manager, and application does not work.
  As a workaround, either include MCUboot in the build or use MCUboot instead of the immutable bootloader.
* ``west flash`` and ``ninja flash`` only program one core, even if multiple cores are included in the build.
  As a workaround, execute the flash command from inside the build directory of the child image that is placed on the other core (for example, :file:`build/hci_rpmsg`).


Crypto
------

* nRF Oberon v3.0.5 is missing symbols for HKDF using SHA1, which will be fixed in an upcoming version of the library.
  As a workaround, use a different backend (for example, vanilla mbed TLS) for HKDF/HMAC using SHA1. (NCSDK-5546)
* The :ref:`nrf_security` is currently only fully supported on nRF52840 and nRF9160 devices.
  It gives compile errors on nRF52832, nRF52833, nRF52820, nRF52811, and nRF52810.
  These errors can be fixed by cherry-picking commits in `nrfxlib PR #205 <https://github.com/nrfconnect/sdk-nrfxlib/pull/205>`_.

Secure Partition Manager (SPM)
------------------------------

* Enabling default logging in the Secure Partition Manager sample makes it crash if the sample logs any data after the application has booted (for example, during a SecureFault, or in a secure service).
  At that point, RTC1 and UARTE0 are non-secure.
  As a workaround, do not enable logging and add a breakpoint in the fault handling, or try a different logging backend. (NCSIDB-114)

Build system
============

* It is not possible to build and program Secure Partition Manager and the application individually. (from v1.2.0)

Zephyr
======

* If the Zephyr kernel preempts the current thread and performs a context switch to a new thread while the current thread is executing a secure service, the behavior is undefined and might lead to a kernel fault.
  To prevent this situation, a thread that aims to call a secure service must temporarily lock the kernel scheduler (:c:func:`k_sched_lock`) and unlock the scheduler (:c:func:`k_sched_unlock`) after returning from the secure call. (NCSIDB-108)



In addition to the known issues above, check the current issues in the `official Zephyr repository`_, since these might apply to the |NCS| fork of the Zephyr repository as well.
To get help and report issues that are not related to Zephyr but to the |NCS|, go to Nordic's `DevZone`_.


Fixed known issues from |NCS| v1.2.0
************************************

nRF9160
=======

* The :ref:`gps_with_supl_support_sample` sample stops working if :ref:`supl_client` support is enabled, but the SUPL host name cannot be resolved.
  As a workaround, insert a delay (``k_sleep()``) of a few seconds after the ``printf`` on line 294 in :file:`main.c`. (fixed)

Bluetooth Low Energy
====================

* The :ref:`peripheral_hids_keyboard` sample cannot be used with the :ref:`nrfxlib:softdevice_controller` because the NFC subsystem does not work with the controller library.
  The library uses the MPSL Clock driver, which does not provide an API for asynchronous clock operation.
  NFC requires this API to work correctly. (fixed)
* When the :ref:`peripheral_hids_mouse` sample is used with the Zephyr Bluetooth LE Controller, directed advertising does not time out and the regular advertising cannot be started. (fixed)
* The :ref:`bluetooth_central_hids` sample cannot connect to a peripheral that uses directed advertising. (fixed)
* When running the :ref:`bluetooth_central_dfu_smp` sample, the :kconfig:option:`CONFIG_BT_SMP` configuration must be aligned between this sample and the Zephyr counterpart (:zephyr:code-sample:`smp-svr`).
  However, security is not enabled by default in the Zephyr sample. (fixed)
* On some operating systems, the nrf_desktop application is unable to reconnect to a host. (fixed)


NFC
===

* The :ref:`nfc_tnep_poller` and :ref:`nfc_tag_reader` samples cannot be run on the nRF5340 PDK.
  There is an incorrect number of pins defined in the MDK files, and the pins required for :ref:`st25r3911b_nfc_readme` cannot be configured properly. (fixed)
* NFC tag samples are unstable when exhaustively tested (performing many repeated read and/or write operations).
  NFC tag data might be corrupted. (fixed)
* For nRF5340, the pins **P1.12** to **P1.15** are unavailable due to an incorrect pin number definition in the MDK. (fixed)

MCUboot
=======

* The MCUboot recovery feature using the USB interface does not work. (fixed)
