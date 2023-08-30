.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.4.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections is kept when the changelog is cleaned.
   "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and so on.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.4.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

MCUboot
=======

|no_changes_yet_note|

Application development
=======================

|no_changes_yet_note|

RF Front-End Modules
--------------------

* Updated the name of the ``nrf21540_ek`` shield to ``nrf21540ek``.

Build system
------------

|no_changes_yet_note|

Working with nRF91 Series
=========================

* Added support for :ref:`nrf91_modem_trace_uart_snippet`.
  Snippet is used for nRF91 modem tracing with the UART backend for the following applications and samples:

  * :ref:`asset_tracker_v2`
  * :ref:`serial_lte_modem`
  * All samples that use nRF9160 DK except for nRF9160: SLM Shell, nRF9160: Modem trace external flash backend, and nRF9160: Modem trace backend samples

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------
* Updated the Bluetooth HCI headers.
  The :file:`hci.h` header now contains only the function prototypes, and the new
  :file:`hci_types.h` header defines all HCI-related macros and structs.

  The previous :file:`hci_err.h` header has been merged into the new :file:`hci_types.h` header.
  This can break builds that were directly including :file:`hci_err.h`.

Bluetooth mesh
--------------

* Added support for Trusted Firmware-M (TF-M) PSA as the crypto backend for mesh security toolbox for the platforms with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>`.

See `Bluetooth mesh samples`_ for the list of changes in the Bluetooth mesh samples.

Matter
------

* Disabled OpenThread shell by default in Matter over Thread samples.
* Enabled :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_NVS` Kconfig option by default, including for builds without factory data support.
  The firmware now erases all flash pages in the non-volatile storage during a factory reset, instead of just clearing Matter-related settings.

* Fixed:

  * An IPC crash on nRF5340 when Zephyr's main thread takes a long time.
  * An application core crash on nRF5340 targets with the factory data module enabled.
    The crash would happen after the OTA firmware update finishes and the image is confirmed.

* Added:

  * Page about :ref:`ug_matter_device_optimizing_memory`.
  * Shell commands for printing and resetting the peak usage of critical system resources used by Matter.
    These shell commands are available when both :kconfig:option:`CHIP_LIB_SHELL` and :kconfig:option:`CHIP_STATISTICS` Kconfig options are set.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added the :kconfig:option:`CHIP_MALLOC_SYS_HEAP_WATERMARKS_SUPPORT` Kconfig option to manage watermark support.
* Updated the factory data guide with an additional rotating ID information.
* Fixed RAM and ROM reports.
* Set onboarding code generation to be enabled by default if the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_BUILD` Kconfig is set.

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

* Added:

  * Integration of Wi-Fi connectivity with Connection Manager connectivity API.
  * The :kconfig:option:`CONFIG_NRF_WIFI_IF_AUTO_START` Kconfig option to enable an application to set/unset AUTO_START on an interface.
    This can be done by using the ``NET_IF_NO_AUTO_START`` flag.
  * Support for sending TWT sleep/wake events to applications.
  * The nRF5340 HFCLK192M clock divider is set to the default value ``Div4`` for lower power consumption when the QSPI peripheral is idle.
  * Extensions to the scan command to provide better control over some scan parameters.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Updated:

  * Default value of the Kconfig option :kconfig:option:`CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS` is changed to 300 seconds.
  * Enabled link time optimization to reduce the flash size of the application.
    You can disable this using the Kconfig option :kconfig:option:`CONFIG_ASSET_TRACKER_V2_LTO`.

* Fixed an issue with movement timeout handling in passive mode.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * ``#XMODEMRESET`` AT command to reset the modem while keeping the application running.
    It is expected to be used during modem firmware update, which now only requires a reset of the modem.
  * DTLS connection identifier support to the ``#XSSOCKETOPT`` and ``#XUDPCLI`` AT commands.
  * An ``auto_connect`` operation in the ``#XCARRIER`` carrier command.
    The operation controls automatic registration of UE to LTE network.

* Updated:

  * The configuration to enable support for nRF Cloud A-GPS service and nRF Cloud Location service by default.
  * UART receive refactored to utilize hardware flow control (HWFC) instead of disabling and enabling UART receiving between commands.
  * UART transmit has been refactored to utilize buffering.
    Multiple responses can now be received in a single transmission.
  * Modem FOTA to only need a modem reset to apply the firmware update.
    The full chip reset (using the ``#XRESET`` AT command) remains supported.

* Removed:

  * DFU AT commands ``#XDFUGET``, ``#XDFUSIZE`` and ``#XDFURUN`` because they were not usable without a custom application in the target (nRF52 series) device.
  * Support for bootloader FOTA update because it is not needed for Serial LTE modem.
  * Option to set or get HWFC setting from ``#XSLMUART`` AT command.
  * Operations to read or erase the MCUboot secondary slot from the ``#XFOTA`` AT command because the application update process overwrites the slot in any case.

nRF5340 Audio
-------------

* Updated the :ref:`application documentation <nrf53_audio_app>` by splitting it into several pages.
* Added back the QDID number to the documentation.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the machine learning models (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) used by the application so that they are now hosted by Nordic Semiconductor.

nRF Desktop
-----------

* Added:

  * Kconfig options to enable handling of the power management events for the following nRF Desktop modules:

    * :ref:`nrf_desktop_board` - The :ref:`CONFIG_DESKTOP_BOARD_PM_EVENTS <config_desktop_app_options>` Kconfig option.
    * :ref:`nrf_desktop_motion` - The :ref:`CONFIG_DESKTOP_MOTION_PM_EVENTS <config_desktop_app_options>` Kconfig option.
    * :ref:`nrf_desktop_ble_latency` - The :ref:`CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS <config_desktop_app_options>` Kconfig option.

    All listed Kconfig options are enabled by default and depend on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.
  * Kconfig option to configure a motion generated per second during a button press (:ref:`CONFIG_DESKTOP_MOTION_BUTTONS_MOTION_PER_SEC <config_desktop_app_options>`) in the :ref:`nrf_desktop_motion`.
    The implementation relies on the hardware clock instead of system uptime to improve accuracy of the motion data generated when pressing a button.
  * The :ref:`nrf_desktop_measuring_hid_report_rate` section in the nRF Desktop documentation.

* Updated:

  * Set the max compiled-in log level to ``warning`` for the USB HID class (:kconfig:option:`CONFIG_USB_HID_LOG_LEVEL_CHOICE`) and reduce the log message levels used in the :ref:`nrf_desktop_usb_state_pm` source code.
    This is done to avoid flooding logs during USB state changes.
  * If the USB state is set to :c:enum:`USB_STATE_POWERED`, the :ref:`nrf_desktop_usb_state_pm` restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED` instead of requiring :c:enum:`POWER_MANAGER_LEVEL_ALIVE`.
    This is done to prevent the device from powering down and waking up multiple times when an USB cable is connected.
  * Disabled ``CONFIG_BOOT_SERIAL_IMG_GRP_HASH`` in MCUboot bootloader release configurations of boards that use nRF52820 SoC.
    This is done to reduce the memory consumption.
  * To improve the accuracy, the generation of simulated movement data in the :ref:`nrf_desktop_motion` now uses a timestamp in microseconds based on the cycle count (either :c:func:`k_cycle_get_32` or :c:func:`k_cycle_get_64` function depending on the :kconfig:option:`CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER` Kconfig option).
  * Aligned Kconfig option names in the :ref:`nrf_desktop_motion` implementation that generates motion from button presses.
    The Kconfig options defining used key IDs are prefixed with ``CONFIG_MOTION_BUTTONS_`` instead of ``CONFIG_MOTION_`` to ensure consistency with configuration of other implementations of the motion module.
  * The :ref:`nrf_desktop_ble_scan` no longer stops Bluetooth LE scanning when it receives :c:struct:`hid_report_event` related to a HID output report.
    Sending HID output report is triggered by a HID host.
    Scanning stop may lead to an edge case where the scanning is stopped, but there are no peripherals connected to the dongle.

Thingy:53: Matter weather station
---------------------------------

* Added support for the nRF7002 Wi-Fi expansion board.

Matter Bridge
-------------

* Added:

  * The :ref:`Matter bridge <matter_bridge_app>` application.
  * Support for the Bluetooth LE bridged devices.
  * Support for bridging of the Bluetooth LE Environmental Sensor (ESP).

Samples
=======

Bluetooth samples
-----------------

* :ref:`direct_test_mode` sample:

  * Added support for the nRF52840 DK.

  * Updated:

    * Aligned timers' configurations to the new nrfx API.
    * Extracted the DTM radio API from the transport layer.

* :ref:`peripheral_hids_keyboard` sample:

  * Fixed an interoperability issue with iOS devices by setting the report IDs of HID input and output reports to zero.

* :ref:`peripheral_fast_pair` sample:

  * Updated by disabling the :kconfig:option:`CONFIG_BT_SETTINGS_CCC_LAZY_LOADING` Kconfig option as a workaround fix for the `Zephyr issue #61033`_.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Fixed an issue with the sample not fitting into RAM size on the ``nrf52dk_nrf52832`` board.

* :ref:`bluetooth_mesh_light` sample:

  * Removed support for the configuration with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>` for :ref:`zephyr:thingy53_nrf5340`.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Fixed an issue where the sample could return an invalid Light Lightness Status message if the transition time was evaluated to zero.
  * Removed support for the configuration with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>` for :ref:`zephyr:thingy53_nrf5340`.

* :ref:`bluetooth_mesh_light_dim` sample:

  * Removed support for the configuration with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>` for :ref:`zephyr:thingy53_nrf5340`.

* :ref:`bluetooth_mesh_light_switch` sample:

  * Removed support for the configuration with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>` for :ref:`zephyr:thingy53_nrf5340`.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Added a getter for the :c:var:`bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range` sensor.
  * Removed support for the configuration with :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>` for :ref:`zephyr:thingy53_nrf5340`.
  * Fixed an issue where the :c:var:`bt_mesh_sensor_time_since_presence_detected` sensor could report an invalid value when the time delta would exceed the range of the characteristic.

* Fixed an issue where some samples copied using the `nRF Connect for Visual Studio Code`_ extension would not compile due to relative paths in :file:`CMakeLists.txt`, which were referencing files outside of the applications folder.

Cryptography samples
--------------------

* Added the :ref:`crypto_ecjpake` sample demonstrating usage of EC J-PAKE.

Cellular samples (renamed from nRF9160 samples)
-----------------------------------------------

* Renamed nRF9160 samples to :ref:`cellular_samples` and relocated them to the :file:`samples/cellular` folder.

* Added:

  * The :ref:`battery` sample to show how to use the :ref:`modem_battery_readme` library.
  * The :ref:`nrf_provisioning_sample` sample that demonstrates how to use the :ref:`lib_nrf_provisioning` service.

* :ref:`nrf_cloud_multi_service` sample:

  * Renamed Cellular: nRF Cloud MQTT multi-service to :ref:`nrf_cloud_multi_service`.
  * Added:

    * Documentation for using the :ref:`lib_nrf_cloud_alert` and :ref:`lib_nrf_cloud_log` libraries.
    * The :file:`overlay_coap.conf` file and made changes to the sample to enable the use of CoAP instead of MQTT to connect with nRF Cloud.
    * An overlay that allows the sample to be used with Wi-Fi instead of LTE (MQTT only).
    * Reporting of device and connection info to the device shadow.

  * Updated:

    * The :file:`overlay_nrfcloud_logging.conf` file to enable JSON logs by default.
    * The encoding and decoding of nRF Cloud data to use the :c:struct:`nrf_cloud_obj` structure and associated functions.
    * The connection logic by cleaning and simplifying it.
    * From using the :ref:`lte_lc_readme` library directly to using Zephyr's ``conn_mgr`` and the :kconfig:option:`CONFIG_LTE_CONNECTIVITY` Kconfig option.

  * Removed the Kconfig options :kconfig:option:`CONFIG_LTE_INIT_RETRY_TIMEOUT_SECONDS` and :kconfig:option:`CLOUD_CONNECTION_REESTABLISH_DELAY_SECONDS` as they are no longer needed.

* :ref:`http_application_update_sample` sample:

   * Updated credentials for the HTTPS connection.

* :ref:`http_full_modem_update_sample` sample:

   * Updated credentials for the HTTPS connection.

* :ref:`http_modem_delta_update_sample` sample:

   * Updated credentials for the HTTPS connection.

* :ref:`https_client` sample:

   * Updated the TF-M Mbed TLS overlay to fix an issue when connecting to the server.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added:

    * The ``disable_response`` parameter to the :c:struct:`nrf_cloud_rest_location_request` structure.
      If set to true, no location data is returned to the device when the :c:func:`nrf_cloud_rest_location_get` function is called.
    * A Kconfig option :kconfig:option:`REST_CELL_LOCATION_SAMPLE_VERSION` for the sample version.
    * Reporting of device and connection info to the device shadow.

  * Updated the sample to print its version when started.

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for accessing nRF Cloud services using CoAP through the :ref:`lib_nrf_cloud_coap` library.
    * Support for GSM 7bit encoded hexadecimal string in SMS messages.
  * Updated the sample to use the :ref:`lib_nrf_cloud` library function :c:func:`nrf_cloud_obj_pgps_request_create` to create a P-GPS request.

* :ref:`lwm2m_client` sample:

  * Added:

    * An overlay for using DTLS Connection Identifier.
      This significantly reduces the DTLS handshake overhead when doing the LwM2M Update operation.

  * Updated:

    * The sample now uses tickless operating mode from Zephyr's LwM2M engine which does not cause device wake-up in 500 ms interval anymore.
      This allows the device to achieve 2 uA of current usage while in PSM sleep mode.

* :ref:`gnss_sample` sample:

  * Added support for nRF91x1 factory almanac.
    The new almanac file format also supports QZSS satellites.

* :ref:`nrf_cloud_rest_fota` sample:

   * Added reporting of device and connection info to the device shadow.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added:

    * A DTS overlay file for LEDs on the nRF9160 DK to be compatible with the :ref:`caf_leds` module.
    * Header files for buttons and LEDs definition required by the :ref:`lib_caf` library.

  * Updated:

    * The sample to use the :ref:`lib_caf` library instead of the :ref:`dk_buttons_and_leds_readme` library.
    * Displaying an error message when the sample fails to send an alert to nRF Cloud.

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

* Updated the build target ``nrf52840dongle_nrf52840`` to use USB CDC ACM as serial transport as default.
  Samples for this target can now be built without providing extra configuration arguments.

Matter samples
--------------

* Disabled OpenThread shell by default in Matter over Thread samples.
* Enabled build with factory data enabled for all samples.

* :ref:`matter_lock_sample` sample:

  * Fixed the feature map for software diagnostic cluster.
    Previously, it was set incorrectly.
  * Fixed the cluster revision for basic information cluster.
    Previously, it was set incorrectly.

* :ref:`matter_thermostat_sample`:

  * Added the :ref:`Matter thermostat <matter_thermostat_sample>` sample.

NFC samples
-----------

|no_changes_yet_note|

Networking samples
------------------

* :ref:`aws_iot` sample:

  * Added support for Wi-Fi and LTE connectivity through the connection manager API.
  * Moved the sample from :file:`nrf9160/aws_iot` folder to :file:`net/aws_iot`.
    The documentation is now found in the :ref:`networking_samples` section.

* :ref:`azure_iot_hub` sample:

  * Added support for Wi-Fi and LTE connectivity through the connection manager API.
  * Added support for the nRF9161 development kit.
  * Moved the sample from :file:`nrf9160/azure_iot_hub` folder to :file:`net/azure_iot_hub`.
    The documentation is now found in the :ref:`networking_samples` section.

|no_changes_yet_note|

Multicore samples
-----------------

nRF5340 samples
---------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* Added:

  * :ref:`wifi_wfa_qt_app_sample` sample that demonstrates how to use the WFA QuickTrack (WFA QT) library needed for Wi-Fi Alliance QuickTrack certification.
  * :ref:`wifi_shutdown_sample` sample that demonstrates how to configure the Wi-Fi driver to shut down the Wi-Fi hardware when the Wi-Fi interface is not in use.
  * :ref:`wifi_twt_sample` sample that demonstrates how to establish TWT flow and transfer data conserving power.
  * Support for the Wi-Fi driver to several upstream Zephyr networking samples.

Other samples
-------------

* Removed the random hardware unique key sample.
  The sample is redundant since its functionality is presented as part of the :ref:`hw_unique_key_usage` sample.

* :ref:`radio_test` sample:

  * Aligned the timer's configuration to the new nrfx API.

* :ref:`802154_sniffer` sample:

  * Added the 802.15.4 sniffer sample.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* Added:

  * TCP/IP checksum offload is now enabled by default for the nRF70 Series.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

* Added :ref:`nrf_security` library, relocated from the sdk-nrfxlib repository to the :file:`subsys/nrf_security` directory.

Debug libraries
---------------

* :ref:`cpu_load` library:

  * Updated by aligning the timer's configuration to the new nrfx API.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Updated by deleting reset in progress flag from settings storage instead of storing it as ``false`` on factory reset operation.
    This is done to ensure that no Fast Pair data is left in the settings storage after the factory reset.

* :ref:`bt_mesh` library:

  * Added the :kconfig:option:`BT_MESH_LIGHT_CTRL_AMB_LIGHT_LEVEL_TIMEOUT` Kconfig option that configures a timeout before resetting the ambient light level to zero.

  * Updated:

    * The :kconfig:option:`CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT` Kconfig option, that is controlling timeout for storing of model states, is replaced by the :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT` Kconfig option.
    * The Light Lightness Actual and Generic Power Level states of the :ref:`bt_mesh_lightness_srv_readme` and :ref:`bt_mesh_plvl_srv_readme` models cannot dim to off.
      This is due to binding with Generic Level state when receiving Generic Delta Set and Generic Move Set messages.

  * Fixed:

    * An issue where the :ref:`bt_mesh_dtt_srv_readme` model could not be found for models spanning multiple elements.
    * An issue where the :ref:`bt_mesh_sensor_srv_readme` model would add a corrupted marshalled sensor data into the Sensor Status message, because the fetched sensor value was outside the range.
      If the fetched sensor value is out of range, the marshalled sensor data for that sensor is not added to the Sensor Status message.

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* Added the :ref:`modem_battery_readme` library that obtains battery voltage information or notifications from a modem.

* :ref:`nrf_modem_lib_readme`:

  * Added CEREG event tracking to ``lte_connectivity``.
  * Updated:

    * The :c:func:`nrf_modem_lib_shutdown` function to allow the modem to be in flight mode (``CFUN=4``) when shutting down the modem.
    * The trace backends can now return ``-EAGAIN`` if the write operation can be retried.
    * Fixed a rare bug that caused a deadlock between two threads when one thread sent data while the other received a lot of data quickly.
    * The ``SO_IP_ECHO_REPLY``, ``SO_IPV6_ECHO_REPLY``, ``SO_TCP_SRV_SESSTIMEO`` and ``SO_SILENCE_ALL`` socket option levels to align with the modem option levels.

* :ref:`lib_location` library:

  * Added support for accessing nRF Cloud services using CoAP through the :ref:`lib_nrf_cloud_coap` library.
  * Updated the neighbor cell search to use GCI search depending on :c:member:`location_cellular_config.cell_count` value.

* :ref:`pdn_readme` library:

  * Updated the library to allow a ``PDP_type``-only configuration in the :c:func:`pdn_ctx_configure` function.

* :ref:`modem_key_mgmt` library:

  * Updated the :c:func:`modem_key_mgmt_cmp` function to return ``1`` if the buffer length does not match the certificate length.

* :ref:`sms_readme` library:

  * Added support for providing input text as a GSM 7bit encoded hexadecimal string to send some special characters that cannot be sent using ASCII string.

Libraries for networking
------------------------

* Added:

  * :ref:`lib_nrf_provisioning` library for device provisioning.
  * :ref:`lib_nrf_cloud_coap` library for accessing nRF Cloud services using CoAP.

* :ref:`lib_nrf_cloud_log` library:

  * Added:

    * An explanation of text versus dictionary logs.
    * Functions to query whether text-based or dictionary (binary-based) logging is enabled.
    * Support for sending direct log messages using CoAP.

  * Fixed the memory leak.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * :c:struct:`nrf_cloud_obj` structure and functions for encoding and decoding nRF Cloud data.
    * :c:func:`nrf_cloud_obj_pgps_request_create` function that creates a P-GPS request for nRF Cloud.
    * A new internal codec function :c:func:`nrf_cloud_obj_location_request_payload_add`, which excludes local Wi-Fi access point MAC addresses from the location request.
    * Support for CoAP CBOR type handling to :c:struct:`nrf_cloud_obj`.
    * Warning message discouraging use of :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` for purposes other than testing.
    * Reporting of protocol (MQTT, REST, or CoAP) as well as method (LTE or Wi-Fi) to the device shadow.
    * Kconfig choice :kconfig:option:`CONFIG_NRF_CLOUD_WIFI_LOCATION_ENCODE_OPT` for selecting the data that is encoded in Wi-Fi location requests.

  * Updated:

    * JSON manipulation moved from :file:`nrf_cloud_fota.c` to :file:`nrf_cloud_codec_internal.c`.
    * :c:func:`nrf_cloud_obj_location_request_create` to use the new function :c:func:`nrf_cloud_obj_location_request_payload_add`.
    * Retry handling for P-GPS data download errors to retry ``ECONNREFUSED`` errors.
    * By default, Wi-Fi location requests include only the MAC address and RSSI value.

  * Fixed:

    * A build issue that occurred when MQTT and P-GPS are enabled and A-GPS is disabled.
    * A bug preventing ``AIR_QUAL`` from being enabled in shadow UI service info.
    * A bug that prevented an MQTT FOTA job from being started.

  * Removed:

    * Unused internal codec function ``nrf_cloud_format_single_cell_pos_req_json()``.
    * ``nrf_cloud_location_request_msg_json_encode()`` function and replaced with :c:func:`nrf_cloud_obj_location_request_create`.
    * ``nrf_cloud_location_req_json_encode()`` internal codec function.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated the :c:func:`nrf_cloud_rest_location_get` function to use the new function :c:func:`nrf_cloud_obj_location_request_payload_add`.

* :ref:`lib_lwm2m_client_utils` library:

  * Added:

    * Support for using pre-provisioned X.509 certificates.
    * Support for using DTLS Connection Identifier

  * Updated:

    * Zephyr's LwM2M Connectivity Monitor object now uses a 16-bit value for radio signal strength so it does not roll over on values smaller than -126 dBm.

* :ref:`lib_aws_fota` library:

  * Added:

    * Support for a single ``url`` field in job documents.
      Previously the host name and path of the download URL could only be specified separately.

  * Updated:

    * The :kconfig:option:`CONFIG_AWS_FOTA_HOSTNAME_MAX_LEN` Kconfig option has been replaced by the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE` Kconfig option.
    * The :kconfig:option:`CONFIG_AWS_FOTA_FILE_PATH_MAX_LEN` Kconfig option has been replaced by the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE` Kconfig option.
    * AWS FOTA jobs are now marked as failed if the job document for the update is invalid.
    * The protocol (HTTP or HTTPS) is now automatically chosen based on the ``protocol`` or ``url`` fields in the job document for the update.

* :ref:`lib_azure_fota` library:

  * Updated:

    * The :kconfig:option:`CONFIG_AZURE_FOTA_HOSTNAME_MAX_LEN` Kconfig option has been replaced by the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE` Kconfig option.
    * The :kconfig:option:`CONFIG_AZURE_FOTA_FILE_PATH_MAX_LEN` Kconfig option has been replaced by the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE` Kconfig option.

* :ref:`lib_download_client` library:

  * Updated:

    * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE` Kconfig option's default value to ``255``.
    * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE` Kconfig option's default value to ``255``.

* :ref:`lib_fota_download` library:

  * Updated:

    * The library now verifies whether the download started with the same URI and resumes the interrupted download.

* :ref:`lib_nrf_cloud_alert` library:

  * Added support for sending alerts using CoAP.

* Removed the Multicell location library as the relevant functionality is available through the :ref:`lib_location` library.

Libraries for NFC
-----------------

* Fixed a potential issue where the NFC interrupt context switching could result in loss of interrupt data.
  This could happen if interrupts would be executed much faster than the NFC workqueue or thread.

* Fixed an issue where an assertion could be triggered when requesting clock from the NFC platform interrupt context.
  The NFC interrupt is no longer a zero latency interrupt.

* :ref:`nfc_t4t_isodep_readme` library:

  * Fixed the ISO-DEP error recovery process in case where the R(ACK) frame is received in response to the R(NAK) frame from the poller device.
    The poller device raised a false semantic error instead of resending the last I-block.

Nordic Security Module
----------------------

* :ref:`nrf_security` library:

  * Removed:

    * Option to build Mbed TLS builtin PSA core (:kconfig:option:`CONFIG_PSA_CORE_BUILTIN`).
    * Option to build Mbed TLS builtin PSA crypto driver (:kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_BUILTIN`) and all its associated algorithms (``CONFIG_MBEDTLS_PSA_BUILTIN_ALG_xxx``).

Other libraries
---------------

* :ref:`lib_identity_key` library:

  * Updated:

    * :c:func:`identity_key_write_random`, :c:func:`identity_key_write_key` and :c:func:`identity_key_write_dummy` functions to return an error code and not panic on error.
    * :c:func:`identity_key_read` function to always return an error code from the library-defined codes.
    * The defined error code names with prefix IDENTITY_KEY_ERR_*.

* :ref:`lib_hw_unique_key` library:

  * Updated:

    * :c:func:`hw_unique_key_write`, :c:func:`hw_unique_key_write_random` and :c:func:`hw_unique_key_load_kdr` functions to return an error code and not panic on error.
    * :c:func:`hw_unique_key_derive_key` function to always return an error code from the library-defined codes.
    * The defined error code names with prefix ``HW_UNIQUE_KEY_ERR_*``.

* :ref:`st25r3911b_nfc_readme` library:

  * Fixed an issue where the :c:func:`st25r3911b_nfca_process` function returns an error in case the Rx complete event is received together with FIFO water level event.

Common Application Framework (CAF)
----------------------------------

* Added :ref:`caf_shell` for triggering CAF events.

* :ref:`caf_buttons`:

  * Added selective wakeup functionality.
    The module's configuration file can specify a subset of buttons that is not used to trigger an application wakeup.
    Each row and column specifies an additional flag (:c:member:`gpio_pin.wakeup_blocked`) that can be set to prevent an entire row or column of buttons from acting as a wakeup source.

* :ref:`caf_ble_adv`:

  * Updated:

    * The dependencies of the :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST` Kconfig option so that it can be used when the Bluetooth controller is running on the network core.
    * The library by improving broadcast of :c:struct:`module_state_event`.
      The event informing about entering either :c:enum:`MODULE_STATE_READY` or :c:enum:`MODULE_STATE_OFF` is not submitted until the CAF Bluetooth LE advertising module is initialized and ready.

* :ref:`caf_power_manager`:

  * Reduced verbosity of logs denoting allowed power states from ``info`` to ``debug``.

Shell libraries
---------------

* Added:

  * The :ref:`shell_nfc_readme` library.
    It adds shell backend using the NFC T4T ISO-DEP protocol for data exchange.

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

* Removed the relocated :ref:`nrf_security` library.

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

DFU libraries
-------------

* Added a new DFU SMP target for the image update to an external MCU by using the MCUmgr SMP Client.


Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`partition_manager`:

  * The size of the span partitions was changed to include the alignment partitions (``EMPTY_x``) appearing between other partitions, but not alignment partitions at the beginning or end of the span partition.
    The size of the span partitions now reflects the memory space taken from the start of the first of its elements to the end of the last, not just the sum of the sizes of the included partitions.

* :ref:`west_sbom`:

  * Changed:

    * To reduce RAM usage, the script now runs the `Scancode-Toolkit`_ detector in a single process.
      This change slows down the licenses detector, because it is no longer executed simultaneously on all files.
    * SPDX License List database updated to version 3.21.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``74c4d1c52fd51d07904b27a7aa9b2303e896a4e3``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``a8b28f13c195a00bdf50f5c24092981124664ed9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline a8b28f13c1 ^4bbd91a908

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^a8b28f13c1

The current |NCS| main branch is based on revision ``a8b28f13c1`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * A page on :ref:`ug_wireless_coexistence` in :ref:`protocols`.
  * Pages on :ref:`thread_device_types` and :ref:`thread_sed_ssed` to the :ref:`ug_thread` documentation.
  * A new section :ref:`ug_pmic`, containing :ref:`ug_npm1300_features` and :ref:`ug_npm1300_gs`.
  * A section about :ref:`nrf70_gs_shields_expansion_boards` in :ref:`nrf7002dk_nrf5340` user guide.

* Updated:

  * The :ref:`emds_readme` library documentation with :ref:`emds_readme_application_integration` section about the formula used to compute the required storage time at shutdown in a worst case scenario.
  * The structure of the :ref:`nrf_modem_lib_readme` documentation.
  * The structure of the |NCS| documentation at its top level, with the following major changes:

    * The getting started section has been replaced with :ref:`Installation <installation>`.
    * The guides previously located in the application development section have been moved to :ref:`configuration_and_build`, :ref:`test_and_optimize`, :ref:`device_guides`, and :ref:`security_index`.
      Some of these new sections also include guides that were previously in the getting started section.
    * "Working with..." device guides are now located under :ref:`device_guides`.
    * :ref:`release_notes`, :ref:`software_maturity`, :ref:`known_issues`, :ref:`glossary`, and :ref:`dev-model` are now located under :ref:`releases_and_maturity`.

  * The :ref:`ug_thread` documentation to improve the overall presentation and add additional details where necessary.
