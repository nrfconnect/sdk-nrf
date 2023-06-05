.. _ncs_release_notes_200:

|NCS| v2.0.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and re-distributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
The feature is made available in its current state though the design and interfaces can change between release tags.
The feature will also be labeled with "EXPERIMENTAL" in Kconfig files to indicate this status.
Build warnings will be generated to indicate when features labeled EXPERIMENTAL are included in builds unless the Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is disabled.

Highlights
**********

* Simplified Software License Report generation is now possible for any |NCS| project.
  See the :ref:`sbom documentation <west_sbom>` for more information.
* The Zephyr SDK toolchain replaces the GNU ARM Embedded toolchain.
* Trusted Firmware M (TF-M) replaces the Secure Partition Manager (SPM) for secure image firmware.
  TF-M is now enabled by default for most nRF9160 and nRF5340 applications and samples.
  See TF-M known issues for details.
* Added experimental Bluetooth LE Audio support, including an application for the nRF5340 Audio DK.
* The SoftDevice Controller is now Bluetooth v5.3 qualified.
* Improved DFU over Bluetooth Low Energy (SMP server improvements).
* Support for Thread v1.2 is no longer experimental. HomeKit over Thread and Matter over Thread use Thread v1.2 libraries by default now.
* Support for the Zigbee Cluster Library specification v8 (ZCL 8) and Base Device Behavior specification v3.0.1 (BDB 3.0.1) is no longer experimental.
  ZCL 8 and BDB 3.0.1 libraries are now used by default in Zigbee samples.
* Updated Zephyr LwM2M stack to v1.1, which provides extended commands and options for more efficient data transmission.
* Added nRF Cloud Location Services support in the AVSystem's Coiote LwM2M server.
* Added power consumption optimization for poor satellite coverage when using GNSS.

A :ref:`migration guide <ncs_2.0.0_migration>` is available for users moving from |NCS| v1.x to v2.x.

Sign up for the `nRF Connect SDK v2.0.0 webinar`_ to learn more about the new features.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.0.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.0.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.0.0.
SEGGER Embedded Studio Nordic Edition is no longer tested and recommended for new projects.

:ref:`Toolchain Manager <gs_app_tcm>`, used to install the |NCS| automatically from `nRF Connect for Desktop`_, is now also available for Linux, in addition to the previously available version for Windows and macOS.
The Linux version only supports installing the |NCS| v2.0.0 and later.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v2.0.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.0.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.0.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

This section provides lists of changes related to :ref:`ug_app_dev`.

Board support
-------------

* Added necessary reset functionality for using the nRF52840 SoC on the Thingy:91 as a Bluetooth controller.

Build system
------------

* Fixed an issue with |NCS| Toolchain delimiter handling on MacOS, which could in special situations result in the build system not being able to properly locate the correct program needed.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth mesh
--------------

* Added :c:struct:`bt_mesh_sensor_srv` context to relevant callbacks and APIs to help resolve the associated sensor model instance.
  For details, see `Bluetooth mesh samples`_ and `Bluetooth libraries and services`_.

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

Matter
------

* The CHIP Tool controller is now the recommended controller for Matter.
  The documentation about :ref:`ug_matter_configuring_controller` has been updated accordingly.
  For more information about the CHIP Tool controller, read the :doc:`matter:chip_tool_guide` page in the Matter documentation.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``25e241ebcbf11b1f63dbe25546b1f10219866ad0``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added the Binding cluster and Groupcast communication to the Light Switch sample.
* Updated the nRF Connect platform by adding :file:`Kconfig.defaults`, :file:`Kconfig.mcuboot.defaults` and :file:`Kconfig.multiprotocol_rpmsg.defaults` files that contain the default configuration for all |NCS| samples.
* Added support for Thread Synchronized Sleepy End Devices.

Thread
------

* Added support for the Link Metrics and CSL Thread v1.2 features for the nRF53 Series devices.
* Removed support for the :ref:`thread_architectures_designs_cp_ncp` architecture and the related tools.
* The :ref:`thread_ot_memory` page shows requirements based on the configuration used for certification instead of minimal configuration, which has been removed.
* Updated the :ref:`ug_thread_tools_tbr_rcp` section in the :ref:`ug_thread_tools_tbr` page with the information about forcing Hardware Flow Control in J-Link.
* Updated nrfconnect/otbr docker.
* Updated OpenThread pre-built libraries for Thread v1.2.
* Removed OpenThread pre-built libraries for Thread v1.1.

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

* Added:

  * Support for nRF5340 DK (PCA10095) in the :ref:`zigbee_light_switch_sample` sample with the :ref:`lib_zigbee_fota` library enabled.
  * Production support for Weather Station application for Thingy:53.

* Updated:

  * Support for Zigbee Cluster Library ver8 (ZCL8).
    The support is not experimental anymore.
  * Support for Zigbee Base Device Behavior v3.0.1 (BDB 3.0.1).
    The support is not experimental anymore.
  * :ref:`lib_zigbee_fota` library.
    For details, see `Libraries for Zigbee`_.
  * Zigbee Network Co-processor Host package to the new version v2.2.0.
  * :ref:`lib_zigbee_shell` library.
    For details, see `Libraries for Zigbee`_.

* Fixed:

  * An issue where the :ref:`zigbee_light_bulb_sample` sample was flickering when set to 50 kHz.
  * An issue with an assertion fail in :file:`/zephyr/include/spinlock.h:129`.
  * An issue where a wrong value was reported for attributes ``MinMeasuredvalue`` and ``MaxMeasuredValue`` in the Weather Station.
  * An issue with ZBOSS fatal error after factory reset and before bdb start.
  * An issue where the Coordinator did not form a new network after factory reset.
  * An issue with identifying when not in a network.
  * An issue with ZBOSS binding table corruption.
  * An issue where the Zigbee shell did not inform if the network address request was not sent.

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

HomeKit
-------

* Added:

  * Production support for Thread 1.2.
  * Support for 3-button actions (long press, short press, double press).
  * LED indicating BLE connectivity status.
  * Development support for OTA-DFU using the iOS Home App (over UARP - BLE and Thread).
  * LED output can be fully disabled using Kconfig.

* Updated:

  * Flash usage optimization for debug versions of samples and applications.
  * DFU mode can now be enabled by a button press.
  * CLI is no longer required for DFU configuration.
  * HAP_TESTING is now configurable using Kconfig.

* Fixed:

  * An issue where RTT logs did not work with the Light Bulb multiprotocol sample with DFU on nRF52840.
  * An issue where Nordic DFU was not compliant with HAP certification requirements.
  * An issue where a change in KVS key naming scheme caused an error for updated devices.
  * An issue where activating DFU caused increased power consumption.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Support for :ref:`bosch_software_environmental_cluster_library`.
  * Support for Indoor Air Quality (IAQ) readings retrieved from the BME680 sensor on Thingy:91.
    For more information, see the :ref:`asset_tracker_v2_sensor_module` documentation.
  * Support for QEMU x86 emulation.
  * Support for the :ref:`lib_nrf_cloud_pgps` flash memory partition under certain conditions.
  * Support for :ref:`QoS` library to handle multiple in-flight messages for MQTT based cloud backends such as AWS IoT, Azure IoT Hub, and nRF Cloud.
  * Support for Lightweight Machine to Machine (LwM2M).
  * Support for filtering updates to cloud based on LTE connection evaluation (`AT%CONEVAL`_).

* Updated:

  * For nRF Cloud builds, the configuration section in the shadow is now initialized during the cloud connection process.
  * The :ref:`ug_bootloader` has been enabled by default.
    To disable it, set both :kconfig:option:`CONFIG_SECURE_BOOT` and :kconfig:option:`CONFIG_BUILD_S1_VARIANT` Kconfig options to ``n``.
  * The :ref:`lib_nrf_cloud` library can now handle modem FOTA updates if :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` is enabled.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * ``#XDFUGET`` and ``#XDFURUN`` AT commands to support the cloud-to-nRF52 DFU service.
  * Native TLS support to the HTTPS client.
  * ``#XCMNG`` command to support the use of native TLS.
  * ``#XSOCKETSELECT`` AT command to support multiple sockets in the Socket service.
  * ``#XPOLL`` AT command to poll selected or all sockets for incoming data.

* Updated:

  * Enhanced the ``#XHTTPCREQ`` AT command for better HTTP upload and download support.
  * Enhanced the ``#XSLEEP`` AT command to support data indication when idle.
  * Enhanced the MQTT client to support the reception of large PUBLISH payloads.
  * The :ref:`lib_nrf_cloud` library is now used directly instead of the Cloud API.

* Fixed:

  * The secondary MCUboot partition information is no longer passed to the P-GPS library if the P-GPS partition is enabled.
  * The combined use of A-GPS and P-GPS so that ephemeris and almanac data is not requested through A-GPS, saving both power and bandwidth.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Increased the value of :kconfig:option:`CONFIG_CAF_POWER_MANAGER_TIMEOUT` to 30 seconds for Thingy:53.

nRF Desktop
-----------

* Added managing BLE connection interval depending on the USB state to reduce power consumption when USB is suspended.
* Changed default Bluetooth connection interval from 7.5 ms to 10 ms for nRF Desktop centrals that support LLPM and two simultaneous Bluetooth connections.
  This is needed to prevent Bluetooth Link Layer scheduling issues.
* Removed configurations without a bootloader.
  The B0 bootloader is enabled by default on all boards if the configuration with two image slots fits in memory.
  Alternatively, MCUboot bootloader with a single image slot and serial recovery is enabled.
  In case the configuration with the MCUboot does not fit in memory, no bootloader is enabled.

Thingy:53 Zigbee weather station
--------------------------------

* Added new :ref:`zigbee_weather_station_app` application.
* Fixed an issue where the buffer was being freed incorrectly.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`peripheral_ams_client` sample.
  * :ref:`peripheral_fast_pair` sample.
    See :ref:`ug_bt_fast_pair` for details about this feature.

* Removed Peripheral Alexa Gadgets Bluetooth sample due to Amazon pausing support for the Gadgets ecosystem.

* :ref:`direct_test_mode` sample:

  * Added the vendor-specific ``FEM_DEFAULT_PARAMS_SET`` command for restoring the default front-end module parameters.
  * Added possibility to build with the limited nRF21540 front-end module hardware pinout.
  * Fixed handling of the disable Constant Tone Extension command.
  * The front-end module test parameters are not set to their default value after the DTM reset command.
  * Changed the radio antennas array hardware description.
    It is now based on the radio bindings instead of custom configuration.

* :ref:`peripheral_hids_mouse` sample:

  * Increased the main stack size from 1024 to 1536 bytes.
  * Increased the stack size of the nRF RPC threads from 1024 to 1280 bytes.

* :ref:`peripheral_uart` sample:

  * Fixed handling of RX buffer releasing in this sample and in the UARTE driver.
    Before this fix, the RX buffer might have been released twice by the main thread.

* :ref:`central_uart` sample:

  * Fixed handling of RX buffer releasing in this sample and in the UARTE driver.
    Before this fix, the RX buffer might have been released twice by the main thread.
  * Added debug logs for the UART events.

* :ref:`bluetooth_central_dfu_smp` sample:

  * Changed the current CBOR library from TinyCBOR to `zcbor`_.

* :ref:`bluetooth-hci-lpuart-sample` sample:

  * Added support for Thingy:91.

* :ref:`ble_nrf_dm` sample:

  * Added support for the nRF52832 device.

* :ref:`peripheral_ancs_client` sample:

  * Fixed handling of the empty Generic Attribute Service.

Bluetooth mesh samples
----------------------

* Updated all samples to use the :ref:`partition_manager`, replacing the use of the Device Tree Source flash partitions.
* :ref:`bluetooth_mesh_sensor_server` sample:

  * Definitions for sensor callbacks now include the :c:struct:`bt_mesh_sensor_srv` context.

nRF9160 samples
---------------

* Added:

  * :ref:`nrf_cloud_rest_device_message` sample, demonstrating how to send an arbitrary device message with the nRF Cloud REST API.
  * :ref:`modem_callbacks_sample` sample, showcasing initialization and de-initialization callbacks.
  * :ref:`nrf_cloud_mqtt_multi_service` sample, demonstrating a simple but robust integration of location services, FOTA, sensor sampling, and more.
  * Shell functionality to HTTP Update samples.
  * :ref:`nrf_cloud_rest_cell_pos_sample` sample, demonstrating how to use the :ref:`lib_nrf_cloud_rest` library to perform cellular positioning requests.
  * :ref:`ciphersuites` sample, demonstrating how to use TLS cipher suites.

* Secure Partition Manager (rather than TF-M) is enabled by default for the applications and samples that support Thingy:91.

* :ref:`at_monitor_sample` sample:

  * Added ``denied``, ``unknown``, ``roaming``, and ``UICC failure`` CEREG status codes to :c:func:`cereg_mon`.

* :ref:`modem_shell_application` sample:

  * Added:

    * Remote control support over MQTT using the :guilabel:`Terminal` window in the nRF Cloud portal.
      It enables executing any MoSh command on the device remotely.
    * An option ``--interval`` (in seconds) to neighbor cell measurements in continuous mode  (``link ncellmeas --continuous``).
      When using this option, a new measurement is started in each interval.
    * Separate plain AT command mode that can be started with the command ``at at_cmd_mode start``.
      AT command termination methods can be configured using Kconfig options.
      The default method is CR termination.
      In AT command mode, a maximum of 10 AT commands can be pipelined with ``|`` as the delimiter character between pipelined AT commands.
    * Threading support for the ``ping`` command.
    * Iperf3 usage over Zephyr native TCP/IP stack and nRF9160 LTE default context.
    * Support for the GNSS features introduced in modem firmware v1.3.2.
      This includes several new fields in the PVT notification and a command to query the expiry times of assistance data.
    * Support for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
    * Device information is sent to nRF Cloud when connecting with MQTT using the ``cloud connect`` command.
    * New options to send acquired GNSS location to nRF Cloud for ``location`` command, either in NMEA or in PVT format.
      Both MQTT and REST transports are supported (compile-time configuration).
    * Improved the nRF9160 DK out-of-the box experience and the process of adding the DK to nRF Cloud using MoSh and LTE Link Monitor.

  * Updated:

    * The behavior of this sample when built with the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART` option enabled, is changed as follows:

      * When disabling of UART is requested either by a shell command or by a button press, modem traces are disabled before disabling UART1.
      * When the UART1 is re-enabled (either after timer expiry or button press), the modem traces are also re-enabled.

* :ref:`nrf_cloud_rest_fota` sample:

  * Enabled building of bootloader FOTA update files.
  * Corrected handling of the bootloader FOTA updates.
  * Enabled the :ref:`lib_at_host` library to make it easier to update certificates.

* :ref:`lte_sensor_gateway` sample:

  * Added support for Thingy:91.

* :ref:`lwm2m_client` sample:

  * Added:

    * Minimal Portfolio object support that is required for LwM2M conformance testing.
    * Support for using location assistance with Coiote LwM2M server.
    * Guidelines on :ref:`setting up the sample for production <lwm2m_client_provisioning>` using AVSystem’s Coiote Device Management server.

  * Updated:

    * Reworked the retry logic so that the sample can fall back to bootstrap mode and need not always restart the LTE connection.
    * Replaced the deprecated GPS driver with the new GNSS interface.
    * LwM2M v1.1 uses SenML CBOR by default as content format.

* :ref:`download_sample` sample:

  * Updated the default HTTPS URL and certificate due to the old link being broken.

* :ref:`gnss_sample` sample:

  * Added support for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
  * Enabled the :ref:`lib_at_host` library to make it easier to update certificates.
  * Added support to calculate distance from a configured reference position.

* :ref:`location_sample` sample:

  * Enabled the :ref:`lib_at_host` library to make it easier to update certificates.

* Removed the Cloud client sample.

Thread samples
--------------

* Updated:

  * Thread 1.2 version is now the default configuration option.
  * Thread Beacon payload has been removed after changes in the latest Thread Specification.
  * Minimal configuration for CLI sample has been removed.
  * BLE advertising interval has been increased from 100 ms to 300 ms for CLI sample when multiprotocol is enabled.
  * :ref:`coap_client_sample` sample with Multiprotocol Bluetooth LE extension is now compatible with :ref:`central_uart` sample.

Matter samples
--------------

* Added:

  * Release configuration for all samples.
  * :ref:`matter_window_covering_sample` sample, based on the Matter upstream sample.
    This sample utilizes Thread Synchronized Sleepy End Device role.

* Updated:

  * Simplified the :file:`prj.conf` file of each sample by using the default configuration from the :file:`Kconfig.defaults` file in Matter upstream.
  * All ZAP configurations due to changes in Matter upstream.

* :ref:`matter_template_sample`:

  * Added OTA DFU support.

* :ref:`matter_light_switch_sample` sample:

  * Added:

    * A binding cluster to the sample.
    * Groupcast communication.
    * Overlay enabling low power mode support.

  * Updated the Pairing process to Binding process in the sample.

* :ref:`matter_lock_sample` sample:

  * Added support for the Door Lock cluster, which replaced the previous temporary solution based on the On/Off cluster.

NFC samples
-----------

* :ref:`nfc_tag_reader` sample:

  * Skips NDEF content printing when message parsing returns an error.

nRF5340 samples
---------------

* Added:

  * :ref:`nrf5340_remote_shell` sample.
  * :ref:`multicore_hello_world` sample.

* Updated:

  * Changed the transport layer for inter-core communication on the nRF5340 device from the RPMsg to the IPC service library.
    The IPC service library can work with different transport backends and uses the RPMsg backend with static VRINGs by default.
    This transport layer change affects all samples that use Bluetooth HCI driver over RPMsg, 802.15.4 spinel backend over IPC or nRF RPC libraries.

* :ref:`nrf_rpc_entropy_nrf53` sample:

  * Converted from TinyCBOR to `zcbor`_.

Zigbee samples
--------------

* Added support for the factory reset functionality from :ref:`lib_zigbee_application_utilities` in the following samples:

  * :ref:`zigbee_light_bulb_sample`
  * :ref:`zigbee_light_switch_sample`
  * :ref:`zigbee_network_coordinator_sample`
  * :ref:`zigbee_shell_sample`
  * :ref:`zigbee_template_sample`

* :ref:`zigbee_light_switch_sample` sample:

  * Added identify handler.

* :ref:`zigbee_light_bulb_sample` sample:

  * Removed implementation of Home Automation Profile Specification logic.
    This logic added dependency between On/Off and Level clusters, so changes in Level cluster were affecting the On/Off one.
  * Updated the frequency of the LED PWM signal to 100 Hz to remove excessive flickering.

Other samples
-------------

* Added:

  * :ref:`ipc_service_sample` sample that demonstrates throughput of the IPC service with available backends.
  * :ref:`event_manager_proxy_sample` sample, which demonstrates the usage of :ref:`event_manager_proxy` library.
  * :ref:`caf_sensor_manager_sample` sample for single-core and multi-core SoCs.

* :ref:`radio_test` sample:

  * Added new configuration that builds the sample with support for remote IPC Service shell on nRF5340 application core through USB.
  * Added possibility to build with the limited nRF21540 front-end module hardware pinout.
  * Improved the calculation of the total payload size for the radio duty cycle.
  * Fast ramp-up is enabled for all radio modes.
  * The duty cycle for modulated transmission is limited to 1-90%.
  * Improved the DFU throughput in the :ref:`smp_svr_sample` for the Bluetooth transport by optimizing Bluetooth MTU configuration and by leveraging the MCUmgr packet reassembly feature.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Removed the deprecated GPS driver.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v0.30.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.
  * Projects that use the LwM2M carrier library cannot use TF-M for this release, since the LwM2M carrier library requires hard floating point.
    For more information, see the :ref:`TF-M <ug_tfm>` documentation.

Bluetooth libraries and services
--------------------------------

* Added:

  * :ref:`ams_client_readme` library.
  * :ref:`bt_fast_pair_readme`.

* Removed Alexa Gadgets Service library due to Amazon pausing support for the Gadgets ecosystem.

* :ref:`gatt_dm_readme` library:

  * Added option to discover several service instances using the UUID.
  * Fixed discovery of empty services.

* :ref:`bt_mesh` library:

  * Added:

    * :c:struct:`bt_mesh_sensor_srv` context to the following API and callbacks:

      * :c:func:`sensor_column_encode` API.
      * :c:member:`get` and :c:member:`set` callbacks in :c:struct:`bt_mesh_sensor_setting`.
      * :c:member:`get` callback in :c:struct:`bt_mesh_sensor_series`.
      * :c:member:`get` callback in :c:struct:`bt_mesh_sensor`.

    * Shell commands for client models.

* :ref:`ble_rpc` library:

  * Added host callback handlers for the ``write`` and ``match`` operations of the CCC descriptor.
  * Converted from TinyCBOR to `zcbor`_.

  * Fixed:

    * Serialization of the write callback applied to the GATT attribute.
    * Serialization of the :c:func:`bt_gatt_service_unregister` function call.

Bootloader libraries
--------------------

* :ref:`doc_bl_validation`:

  * Fixed an issue in :c:func:`bl_validate_firmware` where the reset vector validation check would not work properly.

DFU libraries
-------------

* Added :ref:`lib_dfu_multi_image` library.

* :ref:`lib_dfu_target` library:

  * Fixed a NULL dereference bug, which could happen if :c:func:`settings_load` was called outside of the library.

Modem libraries
---------------

* Added :ref:`lib_modem_antenna` library, a new library for setting the antenna configuration on an nRF9160 DK or a Thingy:91.

* :ref:`sms_readme` library:

  * Fixed time zone handling for received SMSs.
  * The time zone is now returned in quarters of an hour.
  * Added handling for SMS client unregistration notification from the modem.
    When the notification is received, the library re-registers the SMS client automatically.

* :ref:`lib_location` library:

  * Added:

    * Support for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
    * Improved integration of A-GPS and P-GPS when both are enabled.
    * A missing call to the :c:func:`nrf_cloud_pgps_notify_prediction` function, when using the REST interface with P-GPS.
    * Support for P-GPS data retrieval from an external source, implemented separately by the application.
      Enabled by setting the ``CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL`` option.
      The library triggers a :c:enum:`LOCATION_EVT_GNSS_PREDICTION_REQUEST` event when assistance is needed.
    * Obstructed satellite visibility detection feature for GNSS.
      When this feature is enabled, the library tries to detect occurrences where getting a GNSS fix is unlikely or would consume a lot of energy.
      When such an occurrence is detected, GNSS is stopped without waiting for a fix or a timeout.
    * In addition to the current default fallback mode for acquiring a location, it can also be acquired using the :c:enum:`LOCATION_REQ_MODE_ALL` mode that runs all methods in the list sequentially.
      Each run method receives a location event, either a success or a failure.

  * Updated:

    * The :c:member:`request` member of the :c:struct:`location_event_data` structure renamed to :c:member:`agps_request`.
    * Current system time is attached to the ``location_datetime`` parameter of the location request response with Wi-Fi and cellular methods.
      The timestamp comes from the moment of scanning or neighbor measurements.
    * Removed dependency on the :ref:`lib_modem_jwt` library.
      The :ref:`lib_location` library now selects :kconfig:option:`CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT` when using :kconfig:option:`CONFIG_NRF_CLOUD_REST`.

  * Removed support for Skyhook.

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * :c:macro:`NRF_MODEM_LIB_ON_INIT` macro for compile-time registration of callbacks on modem initialization.
    * :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macro for compile-time registration of callbacks on modem de-initialization.
    * :kconfig:option:`CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID` to enable logging for both FW version and UUID at the end of the library initialization step.
    * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING` to process modem traces in a thread (experimental).

  * Updated:

    * The modem fault handler's signature now takes a pointer as parameter to the :c:struct:`nrf_modem_fault_info` structure.
    * The modem fault handler callback is now configurable through the :c:struct:`nrf_modem_init_params_t` structure.
    * By default, the :c:func:`nrf_modem_fault_handler` function fault handler prints the modem fault.
      If any other handling is required, the :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT` Kconfig option must be set accordingly.

  * Deprecated the :c:func:`nrf_modem_lib_shutdown_wait` function, in favor of :c:macro:`NRF_MODEM_LIB_ON_INIT`.

* :ref:`lte_lc_readme` library:

  * Added:

    * :c:macro:`LTE_LC_ON_CFUN` macro for compile-time registration of callbacks on modem functional mode changes using :c:func:`lte_lc_func_mode_set`.
    * Support for simple shell commands.

* :ref:`lib_modem_attest_token` library:

  * CBOR parsing is now performed with the `zcbor`_ module.
    TinyCBOR is deprecated.

* Removed the deprecated A-GPS library.
* Fixed an issue where the compiler would generate an error when building :file:`ncs/nrf/include/modem/lte_lc.h` with C++ applications.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud_rest` library:

  * Added JSON Web Token (JWT) autogeneration feature.
    If enabled, the nRF Cloud REST library automatically generates a JWT if none is provided by the user when making REST requests.

  * Updated:

    * Centralized error handling.
    * Error return values.
    * For cellular positioning responses, the type is now set based on the ``fulfilledWith`` response from the nRF Cloud.
    * nRF Cloud error codes are now parsed and set in the :c:struct:`nrf_cloud_rest_context` structure.

* :ref:`lib_download_client` library:

  * Fixed an issue where downloads of COAP URIs would fail when they contained multiple path elements.
  * Added the :c:member:`set_native_tls` parameter in the configuration structure to configure native TLS support at runtime.

* :ref:`lib_fota_download` library:

  * Added :c:func:`fota_download_s0_active_get` function that gets the active B1 slot.
  * Added :kconfig:option:`CONFIG_FOTA_DOWNLOAD_NATIVE_TLS` to configure the socket to be native for TLS instead of offloading TLS operations to the modem.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * :c:func:`nrf_cloud_bootloader_fota_slot_set` function that sets the active bootloader slot flag during bootloader FOTA updates.
    * :c:func:`nrf_cloud_pending_fota_job_process` function that processes the state of pending FOTA jobs.
    * :c:func:`nrf_cloud_handle_error_message` function that handles error message responses (MQTT) from nRF Cloud.

  * Updated:

    * Shadow data behavior during the connection process.
      The data is now sent to the application even if no ``"config"`` section is present.
    * The application can now send shadow updates earlier in the connection process.
    * nRF Cloud error message responses to location service MQTT requests are now handled.
    * The value of the :kconfig:option:`CONFIG_NRF_CLOUD_HOST_NAME` option is now ``mqtt.nrfcloud.com``.
    * Removed support for the Cloud API.

  * Fixed the validation of bootloader FOTA updates.

* :ref:`lib_aws_iot` library:

    * Renamed ``aws_iot_topic_type`` to ``aws_iot_shadow_topic_type`` and removed ``AWS_IOT_SHADOW_TOPIC_UNKNOWN``.
    * Removed support for the Cloud API.

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for using location assistance when using the Coiote LwM2M server.
  * Updated the library to store credentials and server settings permanently on bootstrap.
  * Updated the library to let an application control the network connection state.

* :ref:`lib_azure_iot_hub` library:

  * Added :kconfig:option:`CONFIG_AZURE_IOT_HUB_NATIVE_TLS` option to configure the socket to be native for TLS instead of offloading TLS operations to the modem.
  * Removed support for the Cloud API.

* :ref:`lib_nrf_cloud_pgps` library:

  * Added:

    * The passing of the current prediction, if one is available, along with the ``PGPS_EVT_READY`` event.
    * The following three ways to define the storage location in the flash memory:

      * A dedicated P-GPS partition, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_PARTITION` option.
      * The use of the MCUboot secondary partition as storage, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_MCUBOOT_SECONDARY` option.
      * An application-specific storage, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_STORAGE_CUSTOM` option.

* :ref:`lib_nrf_cloud_agps` library:

  * Fixed premature assistance suppression when the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED` option is enabled.
    Added a 10 minute margin of error to ensure A-GPS assistance is downloaded every two hours even if the modem requests assistance a little early.

* Multicell location library:

  * Removed support for Skyhook.

* Removed the Cloud API library.

Libraries for NFC
-----------------

* :ref:`nfc_ndef_parser_readme`:

  * Updated:

    * :c:func:`nfc_ndef_msg_parse` with a fix to the declaration, a new assertion to avoid a potential usage fault, and added a note in the API documentation.
    * ``NFC_NDEF_PARSER_REQUIRED_MEMO_SIZE_CALC`` macro has been renamed to :c:macro:`NFC_NDEF_PARSER_REQUIRED_MEM`.

Other libraries
---------------

* Added:

  * :ref:`event_manager_proxy` library.
  * :ref:`QoS` library.
  * :ref:`emds_readme` library.

* :ref:`app_event_manager`:

  * Added:

    * Event type flags to represent if event type should be logged, traced, and whether it has dynamic data.
      To update your application, pass a flag variable as a parameter in :c:macro:`APP_EVENT_TYPE_DEFINE` instead of ``init_log``.
      Use :c:macro:`APP_EVENT_FLAGS_CREATE` to set multiple flags:

      .. code-block:: c

         APP_EVENT_TYPE_DEFINE(my_event,
           log_my_event,
           &my_event_info,
           APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_1, APP_EVENT_TYPE_FLAGS_2));

    * :c:func:`app_event_manager_event_size` function with corresponding :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE` option.
    * Universal hooks for Application Event Manager initialization, event submission, preprocessing, and postprocessing.
      This includes implementation of macros that register hooks, grouped as follows:

        * :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_LAST`
        * :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_LAST`
        * :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST`

    * Renamed Event Manager to Application Event Manager.

* :ref:`app_event_manager_profiler_tracer`:

  * The library is no longer directly referenced from the Application Event Manager.
    Instead, it uses the Application Event Manager hooks to connect with the manager.


* :ref:`esb_readme`:

  * Fixed a compilation error for nRF52833.

* :ref:`nrf_profiler`:

    * Renamed Profiler to nRF Profiler.
    * Updated versions of required python modules (pynrfjprog and matplotlib).

* :ref:`ei_wrapper`:

    * Added :kconfig:option:`CONFIG_EI_WRAPPER_PROFILING` for logging time of classifier execution.

* :ref:`lib_hw_unique_key` library:

  * Fixed a bug where the random key would not be deleted from RAM after being generated and written.

* :ref:`lib_ram_pwrdn` library:

  * Added the :c:func:`power_up_unused_ram` API.

Common Application Framework (CAF)
----------------------------------

* Added :ref:`caf_sensor_data_aggregator`, which buffers sensor events and sends them as packages to the listener.

Shell libraries
---------------

* Added :ref:`shell_ipc_readme` library.

Libraries for Zigbee
--------------------

* :ref:`lib_zigbee_application_utilities` library:

  * Added factory reset functionality in :ref:`lib_zigbee_application_utilities` library.

* :ref:`lib_zigbee_shell` library:

  * Added:

    * ``nbr monitor`` shell command for monitoring the list of active Zigbee neighbors.
    * Set of ``zcl groups`` shell commands for managing Zigbee groups.
    * :kconfig:option:`CONFIG_ZIGBEE_SHELL_ZCL_CMD_TIMEOUT` for timing out ZCL cmd commands.

  * Updated:

    * :ref:`lib_zigbee_shell` structure to make it an independent library.
    * File names ``zigbee_cli*`` and changed it to ``zigbee_shell*``.
    * Function names ``zigbee_cli*`` and changed it to ``zigbee_shell*``.
    * ``bdb factory_reset`` command.
      Now the command checks if the ZBOSS stack is started before performing the factory reset.
    * ``zcl cmd`` shell command extended to allow sending groupcasts.
    *  ``zdo`` shell commands extended to allow binding to a group address.
    * Internal context manager structures.

  * Fixed an issue where the ``zcl cmd`` shell command was using the incorrect index of a context manager entry during cleanup after the command was sent.

* :ref:`lib_zigbee_zcl_scenes` library:

  * Updated the library, so that it is allowed to store empty scenes.

* :ref:`lib_zigbee_osif` library:

  * Updated:

    * Crypto library used for performing software AES encryption.
      Now, the :ref:`nrfxlib:nrf_oberon_readme` is used instead of the Tinycrypt library.
    * Optimize calling ZBOSS API in |NCS| platform.
      If the ZBOSS API is called in the ZBOSS thread context, processing by the workqueue is now skipped.

* :ref:`lib_zigbee_fota` library:

  * Added:

    * New :kconfig:option:`CONFIG_ZIGBEE_FOTA_SERVER_DISOVERY_INTERVAL_HRS` Kconfig option to configure the interval between queries for the Zigbee FOTA server.
    * New :kconfig:option:`CONFIG_ZIGBEE_FOTA_IMAGE_QUERY_INTERVAL_MIN` Kconfig option to configure the interval between queries for the available Zigbee FOTA images.
    * Support for the combined application and network core updates for the nRF5340 SoC.

  * Updated:

    * Download logic to use the :ref:`lib_dfu_multi_image` library API and image structure.
    * The image generation script by introducing the sub-element structure inside the Zigbee OTA image.
      Enable :kconfig:option:`CONFIG_ZIGBEE_FOTA_GENERATE_LEGACY_IMAGE_TYPE` to generate images compatible with previous |NCS| releases.
    * Default value of the :kconfig:option:`CONFIG_ZIGBEE_FOTA_IMAGE_TYPE` to ``0x0141``.
    * Setting of the :kconfig:option:`CONFIG_NRF53_ENFORCE_IMAGE_VERSION_EQUALITY` for nRF5340 SoC to ensure integrity of the upgrade image.

  * Removed the :c:enum:`ZIGBEE_FOTA_EVT_ERASE_PENDING` and :c:enum:`ZIGBEE_FOTA_EVT_ERASE_DONE` events.

* Fixed:

  * An issue where printing binding table containing group-binding entries results in corrupted output.
  * An issue where Zigbee shell coordinator would not form a new network after the factory reset operation.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

* :ref:`nrfxlib:crypto`:

  * The Oberon library now supports PKCS#7 padding with CBC and 16 bytes IV with GCM mode in PSA APIs.
  * Updated:

    * :ref:`nrfxlib:nrf_cc3xx_platform_readme` to v0.9.14.
    * :ref:`nrfxlib:nrf_cc3xx_mbedcrypto_readme` to v0.9.14.
    * :ref:`nrfxlib:nrf_oberon_readme` to v3.0.11.

* :ref:`nrfxlib:softdevice_controller`:

  * Added:

    * Support for connectionless angle of arrival (AoA) transmitter.
    * Support for peripheral-initiated feature exchange.
    * A ``nak_count`` field into QoS Connection event reports.

* :ref:`nrfxlib:nrf_dm`:

  * Added support for nRF52832 and nRF5340.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* Added new ``west ncs-sbom`` command that generates :ref:`Software Bill of Materials <west_sbom>`.
* Added :ref:`bt_fast_pair_provision_script`.

* :ref:`partition_manager`:

  * Added the :file:`ncs/nrf/subsys/partition_manager/pm.yml.pgps` file.
  * Added the :file:`ncs/nrf/subsys/partition_manager/pm.yml.emds` file.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``e86f575f68fdac2cab1898e0a893c8c6d8fd0fa1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added :kconfig:option:`CONFIG_NRF53_ENFORCE_IMAGE_VERSION_EQUALITY` to attach the dependencies between application and network core images.
  This option links the upgrade images in such a way that either both or none is applied.
* Added support for the Dual-slot execute-in-place (XIP) feature in the |NCS| build system.
  See the :file:`ncs/nrf/tests/modules/mcuboot/direct_xip` test project for an example of how to leverage this feature in your application.
* Adapted ``boot_serial`` to Zephyr's new CRC APIs (boot_serial implements the serial recovery).
* Modified ``zephyr/boot_serial_extension`` to use ``BOOT_LOG`` instead of ``LOG_`` (``boot_serial_extension`` implements the serial recovery extension).
* Loader was reworked so it allows larger minimum flash write size (was 8 B, now 32 B).
* Added optional timeout to enter serial recovery for ``zephyr`` under ``boot_serial``.
  See :kconfig:option:`CONFIG_BOOT_SERIAL_WAIT_FOR_DFU`.
* Modified ``zephyr`` to use a smaller SHA-256 implementation.
* Added support for the echo command to ``boot_serial``.
  See :kconfig:option:`CONFIG_BOOT_MGMT_ECHO`.
* Modified the single loader to fix image decryption for any SoC flash of the pages with size not fitting in 1024 B.
* Removed deprecated ``DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL``.
* Fixed usage of ``CONFIG_LOG_IMMEDIATE``.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``53fbf40227de087423620822feedde6c98f3d631``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 53fbf40227 ^45ef0d2

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^53fbf40227

The current |NCS| main branch is based on revision ``53fbf40227`` of Zephyr.

cddl-gen/zcbor
==============

.. note::
    In March 2022, cddl-gen has been renamed to zcbor.
    zcbor is now provided through Zephyr instead of directly in |NCS|.

The `zcbor`_ module has been updated from version 0.3.0 to 0.4.0.
Release notes for 0.4.0 can be found in :file:`ncs/nrf/modules/lib/zcbor/RELEASE_NOTES.md`.

The following major changes have been implemented:

* Renamed ``cddl-gen`` to ``zcbor`` throughout the repository.
* Regenerated fmfu code from cddl.
* Added Kconfig options to control the zcbor configuration options.
* Updated tests to run with updated zcbor.

Trusted Firmware-M
==================

* Fixed:

  * NCSDK-13949 known issue where the TF-M secure image would copy FICR to RAM on the nRF9160 SiP.
  * NCSDK-12306 known issue where a usage fault would be triggered in the debug build on nRF9160 SiP.
  * NCSDK-14015 known issue that would cause crash during boot when the :kconfig:option:`CONFIG_RPMSG_SERVICE` Kconfig option is enabled on the nRF5340 SoC.

cJSON
=====

*  Fixed an issue with floats in the cJSON module when using NEWLIB_LIBC without the :kconfig:option:`CONFIG_NEWLIB_LIBC_FLOAT_PRINTF` Kconfig option.

Documentation
=============

* Added:

  * :ref:`ncs_2.0.0_migration` reflecting major changes in |NCS| v2.0.0 that might require a migration action.
  * Documentation for :ref:`debugging of nRF5340 <debugging>` in :ref:`working with nRF5340 DK<ug_nrf5340>` user guide.
  * Section about :ref:`ug_nrf5340_intro_xip` in :ref:`working with nRF5340 DK<ug_nrf5340>` user guide.
  * Section describing how to enable Amazon Frustration-Free Setup (FFS) support in :ref:`ug_matter_configuring_device_identification` user guide.
  * Notes to the :ref:`bluetooth_central_dfu_smp` sample document specifying the intended use of the sample.
  * DevAcademy links to the :ref:`index` and :ref:`getting_started` pages.
  * Additional user guidance to the :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages and the corresponding Developing with pages.
  * A page on :ref:`software_maturity` listing the different software maturity levels for the available features.
  * A page on :ref:`ug_pinctrl`.
  * Documentation for :ref:`ug_thingy53_gs`.
  * Documentation page about :ref:`ug_zigbee_commissioning`.
  * Documentation for Asset tracker v2 :ref:`asset_tracker_unit_test`.
  * New :ref:`security` page on the top level, with a brief introduction to core security features available in Nordic Semiconductor products.

* Updated:

  * The |NCS| documentation website with a new look-and-feel to improve its usability.
  * :ref:`ug_nrf5340` and :ref:`ug_nrf91` user guides with information about Trusted Firmware-M replacing Secure Partition Manager as the default solution for creating a Trusted Execution Environment.
  * Some samples and applications built as a non-secure firmware image for the ``_ns`` build target to reflect that the :ref:`Trusted Firmware-M <ug_tfm>` (TF-M) is automatically included instead of Secure Partition Manager (SPM).
  * :ref:`app_power_opt` guide with information on how to disable serial logging when using TF-M.
  * Replaced reference to Secure Partition Manager with reference to Trusted Firmware-M for multi-image project builds (nRF9160 samples) in :ref:`gs_programming` page.
  * :ref:`gs_updating` page with information about updating |VSC| and the toolchain.
  * :ref:`ug_nrf52` and :ref:`ug_nrf5340` user guides with information about FOTA upgrades for Matter, Thread, and Zigbee.
  * Protocol architecture diagram in :ref:`ug_matter_architecture` page.
  * :ref:`app_memory` page with sections for Matter and Zigbee.
  * :ref:`thread_ug_feature_updating_libs` section to clarify the use, and added |VSC| instructions.
  * :ref:`ug_thread_communication` by moving it to a separate page instead of it being under :ref:`ug_thread_architectures`.
  * Added a note to several nRF Cloud samples using the `nRF Cloud REST API`_ indicating the need for valid and registered request signing credentials.
  * :ref:`thread_ot_memory` page with definitions of variants listed on the tables.
  * :ref:`ug_nrf_cloud` page with more information about security.
  * Working with Thingy:53 user guide with new information, and renamed it to :ref:`ug_thingy53`.
  * nRF Desktop documentation with the following additions:

    * Information about nRF21540 EK shield support.
    * Documentation for selective HID report subscription in :ref:`nrf_desktop_usb_state` using :ref:`CONFIG_DESKTOP_USB_SELECTIVE_REPORT_SUBSCRIPTION <config_desktop_app_options>` option.

  * :ref:`lwm2m_client` sample documentation with a state diagram.
  * The relevant Thread sample documentation pages with information about support for :ref:`Trusted Firmware-M <ug_tfm>`.
  * All Thread samples documentation with a Configuration section, and organized relevant information under that section.
  * Gazell samples documentation by separating the :ref:`gazell_samples` into their own pages for Host and Device.
    There are now four different sample pages, where each Host sample must be used along with its corresponding Device sample.
  * Moved the :ref:`ug_nrf_cloud` page to the top level of the navigation.

* Removed:

  * Documentation on the Getting Started Assistant, as this tool is no longer in use.
    Linux users can install the |NCS| by using the `Installing using Visual Studio Code <Installing on Linux_>`_ instructions or by following the steps on the :ref:`gs_installing` page.
  * Documentation on the SEGGER Embedded Studio, as this tool will no longer be supported moving forward.
    The previous |NCS| releases still support SEGGER Embedded Studio (Nordic edition).
    To migrate from SEGGER Embedded Studio IDE or on the command line to |VSC|, use the `Open an existing application <Migrating IDE_>`_ option in the |nRFVSC| to migrate your application.
  * Added |VSC| instructions on the following documentation:

    * :ref:`gs_modifying`
    * :ref:`ug_thingy91`
    * :ref:`ug_nrf5340`
    * :ref:`bootloader`
