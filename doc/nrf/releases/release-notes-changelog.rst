.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.0.99
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
See `known issues for nRF Connect SDK v2.0.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Added information about :ref:`gs_supported_OS` to the :ref:`gs_recommended_versions` page.
* Added information about :ref:`app_build_output_files` on the :ref:`app_build_system` page.

Board support
-------------

* TF-M is now enabled by default on Thingy:91.

Partition Manager
-----------------

* Added :kconfig:option:`CONFIG_PM_PARTITION_REGION_LITTLEFS_EXTERNAL`, :kconfig:option:`CONFIG_PM_PARTITION_REGION_SETTINGS_STORAGE_EXTERNAL`, and :kconfig:option:`CONFIG_PM_PARTITION_REGION_NVS_STORAGE_EXTERNAL` to specify that the relevant partition must be located in external flash memory.
  You must add a ``chosen`` entry for ``nordic,pm-ext-flash`` in your devicetree to make this option available.
  See :file:`tests/subsys/partition_manager/region` for example configurations.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

* Added support for changing the radio transmitter's default power level using the :c:func:`sdc_default_tx_power_set` function.
* Added support for changing the peripheral latency mode using the :c:func:`sdc_hci_cmd_vs_peripheral_latency_mode_set` function.
* Added support for changing the default TX power using ``CONFIG_BT_CTLR_TX_PWR_*``.

For details, see :ref:`nrfxlib:softdevice_controller_changelog`.

Bluetooth mesh
--------------

* Added support for usage of :ref:`emds_readme`.
  For details, see `Bluetooth mesh samples`_ and `Bluetooth libraries and services`_.

See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.

Matter
------

* Introduced support for Matter over Wi-Fi to several samples.
* Updated :ref:`ug_matter` with new pages about Matter SDK as well as information about Matter over Wi-Fi.
* Removed the low-power configuration build type from all Matter samples.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``41cb8220744f2587413d0723e24847f07d6ac59f``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Matter device factory data.
    This includes a set of scripts for building the factory data partition content, and the ``FactoryDataProvider`` class for accessing this data.
  * Experimental support for Matter over Wi-Fi.

Thread
------

* Added information about Synchronized Sleepy End Device (SSED) and SED vs SSED activity in the :ref:`thread_ot_device_types` documentation.
* Multiprotocol support was removed from :file:`overlay-cert.config` and moved to :file:`overlay-multiprotocol.conf`.

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

* Updated:

  * Enabled the PAN ID conflict resolution in applications that uses :ref:`lib_zigbee_application_utilities` library.
    For details, see `Libraries for Zigbee`_.
  * Changed the default entropy source of Zigbee samples and unit tests to Cryptocell for SoCs that have Cryptocell.

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

ESB
---

* Fixed the ``update_radio_crc()`` function in order to correctly configure the CRC's registers (8 bits, 16 bits, or none).

nRF IEEE 802.15.4 radio driver
------------------------------

* Added option to configure IEEE 802.15.4 ACK frame timeout at build time using :kconfig:option:`CONFIG_NRF_802154_ACK_TIMEOUT_CUSTOM_US`.

Wi-Fi
-----

* Added support for nRF700x DK that supports Wi-Fi SoC, which implements IEEE 802.11 protocols.

See `Wi-Fi samples`_ for details about how to use Wi-Fi in your application.


RF Front-End Modules
====================

* Fixed a build error that occurred when building an application for nRF53 SoCs with Simple GPIO Front-End Module support enabled.
* Added the :kconfig:option:`CONFIG_MPSL_FEM_ONLY` Kconfig option that allows the :ref:`nrfxlib:mpsl_fem` API to be used without other MPSL features.
  The :ref:`MPSL library <nrfxlib:mpsl>` is linked into the build without initialization.
  You cannot use other MPSL features when this option is enabled.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

  * Added:

    * :ref:`motion_impact_detection` using the ADXL372 accelerometer.
    * Added the following Kconfig options to set the threshold and timeout values:

      * :ref:`CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD>`
      * :ref:`CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD>`
      * :ref:`CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS <CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS>`
    * Data sampling is now performed when the device detects both activity and inactivity in passive mode, notified by the :c:enum:`SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED` event of the :ref:`sensor module <asset_tracker_v2_sensor_module>`.

  * Removed:

    * ``CONFIG_APP_REQUEST_GNSS_ON_INITIAL_SAMPLING`` option.
    * ``CONFIG_APP_REQUEST_NEIGHBOR_CELLS_DATA`` option.
    * ``CONFIG_EXTERNAL_SENSORS_ACTIVITY_DETECTION_AUTO`` option.
    * ``CONFIG_MODEM_CONVERT_RSRP_AND_RSPQ_TO_DB`` option.
    * ``CONFIG_DATA_ACCELEROMETER_THRESHOLD`` option.
    * ``CONFIG_DATA_ACCELEROMETER_BUFFER_STORE`` option.
    * ``CONFIG_DATA_ACCELEROMETER_BUFFER_COUNT`` option.
    * ``SENSOR_EVT_MOVEMENT_DATA_READY`` event.

  * Updated:

    * The default value of the GNSS timeout in the application's :ref:`Real-time configurations <real_time_configs>` is now 30 seconds.
    * GNSS fixes are now published in PVT format instead of NMEA for nRF Cloud builds. To revert to NMEA, set the :ref:`CONFIG_GNSS_MODULE_NMEA <CONFIG_GNSS_MODULE_NMEA>` option.
    * The sensor module now forwards :c:enum:`SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED` and :c:enum:`SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED` events.
    * The :ref:`Real-time configurations <real_time_configs>` can now configure the upper and lower thresholds for motion detection.
      You can also configure the timeout after which the sensor reports inactivity.
      The following Kconfig options set the threshold and timeout values:

      * :ref:`CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD>`
      * :ref:`CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD>`
      * :ref:`CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS <CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS>`
    * LwM2M schema has been updated.
      To use the new parameters, upload the updated ``config_object_descript.xml`` to AVSystem.
    * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.
    * Bootstrapping has been disabled by default to be able to connect to the default LwM2M service AVSystem's `Coiote Device Management`_ using free tier accounts.
    * Added support for full modem FOTA updates for nRF Cloud builds.
    * ``CONFIG_DATA_DEVICE_MODE`` option is now a choice that can be set to either ``CONFIG_DATA_DEVICE_MODE_ACTIVE`` or ``CONFIG_DATA_DEVICE_MODE_PASSIVE`` depending on the desired device mode.
    * Restructured the documentation.

  * Fixed:

    * An issue that reports GNSS altitude, accuracy, and speed incorrectly when using LwM2M engine.
    * An issue that caused modem FOTA jobs to be reported as not validated to nRF Cloud.

nRF9160: Serial LTE modem
-------------------------

  * Added:

    * URC for GNSS timeout sleep event.
    * Selected flags support in #XRECV and #XRECVFROM commands.
    * Multi-PDN support in the Socket service.
    * The GNSS service now signifies location info to nRF Cloud.
    * New #XGPSDEL command to delete GNSS data from non-volatile memory.
    * New #XDFUSIZE command to get the size of the DFU file image.

  * Updated:

    * The AT response and the URC sent when the application enters and exits data mode.
    * ``WAKEUP_PIN`` and ``INTERFACE_PIN`` are now defined as *Active Low*. Both are *High* when the SLM application starts.

  * Removed:

    * The software toggle of ``INDICATE_PIN`` in case of reset.

nRF5340 Audio
-------------

* Added:

  * Support for Basic Audio Profile, including support for the stereo BIS (Broadcast Isochronous Stream).
  * Bonding between gateway and headsets in the CIS (Connected Isochronous Stream).
  * DFU support for internal and external flash layouts.
    See :ref:`nrf53_audio_app_configuration_configure_fota` in the application documentation for details.
  * DFU advertising name based on role.

* Updated:

  * Network controller.
  * Documentation in the :ref:`nrf53_audio_app_building_script` section.
    The text now mentions how to recover the device if programming using script fails.
  * Documentation of the operating temperature maximum range in the
    :ref:`nrf53_audio_app_dk_features` and :ref:`nrf53_audio_app_dk_legal` sections.

* Removed:

  * Support for SBC.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added configuration of :ref:`bt_le_adv_prov_readme`.
  The subsystem is now used instead of the :file:`*.def` file to configure advertising data and scan response data in :ref:`caf_ble_adv`.
* Updated Bluetooth advertising data and scan response data logic.
  The UUID128 of Nordic UART Service (NUS) is now added to the scan response data only if the NUS is enabled and the Bluetooth local identity in use has no bond.

nRF Desktop
-----------

* nRF Desktop peripherals no longer automatically send security request immediately after Bluetooth LE connection is established.
  The feature can be turned on using :kconfig:option:`CONFIG_CAF_BLE_STATE_SECURITY_REQ`.
* nRF Desktop dongles start peripheral discovery immediately after Bluetooth LE connection is established.
  The dongles no longer wait until the connection is secured.
* Added configuration of :ref:`bt_le_adv_prov_readme`.
  The subsystem is now used instead of the :file:`*.def` file to configure advertising data and scan response data in :ref:`caf_ble_adv`.
* Updated Bluetooth advertising data and scan response data logic.

  * The TX power included in the advertising packet is no longer hardcoded, the application reads it from the Bluetooth controller.
    The TX power is included in advertising packets even if the Bluetooth local identity in use has bond.
  * The UUID16 of Battery Service (BAS) and Human Interface Device Service (HIDS) are included in advertising packets only if the Bluetooth local identity in use has no bond.

Thingy:53 Zigbee weather station
--------------------------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

  * Fixed:

    * Missing return statement that caused immediate asserts when asserts were enabled.
    * Too low UART RX timeout that caused high level of fragmentation of UART RX data.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`peripheral_mds` sample, that demonstrates how to send Memfault diagnostic data through Bluetooth.

* :ref:`ble_nrf_dm` sample:

  * Split the configuration of the :ref:`mod_dm` module from the :ref:`nrf_dm`.
    This allows the use of the Nordic Distance Measurement library without the module.

* :ref:`direct_test_mode` sample:

  * Added a workaround for nRF5340 revision 1 Errata 117.

* :ref:`peripheral_hr_coded` sample:

  * Added configuration for the nRF5340 target.
  * Fixed advertising start on the nRF5340 target with the Zephyr LL controller.
    Previously, it was not possible to start advertising, because the :kconfig:option:`CONFIG_BT_EXT_ADV` option was disabled for the Zephyr LL controller.

* :ref:`bluetooth_central_hr_coded` sample:

  * Added configuration for the nRF5340 target.
  * Fixed scanning start on the nRF5340 target with the Zephyr LL controller.
    Previously, it was not possible to start scanning, because the :kconfig:option:`CONFIG_BT_EXT_ADV` option was disabled for the Zephyr LL controller.

* :ref:`ble_nrf_dm` sample:

  * Added support for the nRF5340 target.

* :ref:`peripheral_fast_pair` sample:

  * Added:

    * Possibility of toggling between show and hide UI indication in the Fast Pair not discoverable advertising.
    * Automatic switching to the not discoverable advertising with the show UI indication mode after 10 minutes of discoverable advertising.
    * Automatic switching from discoverable advertising to the not discoverable advertising with the show UI indication mode after a Bluetooth Central successfully pairs.

* :ref:`bluetooth_direction_finding_connectionless_tx` sample:

  * Fixed a build error related to the missing :kconfig:option:`CONFIG_BT_DF_CONNECTIONLESS_CTE_TX` Kconfig option.
    The option has been added and set to ``y`` in the sample's :file:`prj.conf` file.

* :ref:`ble_throughput` sample:

  * Fixed peer throughput calculations.
    These were too low because the total transfer time incorrectly included 500ms delay without including the actual transfer.
  * Optimized throughput speed by increasing MTU to 498 and using the maximum connection event time.

* :ref:`bluetooth_direction_finding_central` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

* :ref:`bluetooth_direction_finding_connectionless_rx` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

* :ref:`bluetooth_direction_finding_connectionless_tx` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

* :ref:`bluetooth_direction_finding_peripheral` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added overlay file with support for storing data with :ref:`emds_readme`.
    Also changed the sample to restore Light state after power-down.

nRF9160 samples
---------------

* Added :ref:`modem_trace_backend_sample` sample, demonstrating how to add a custom modem trace backend. The custom backend prints the amount of trace data received in bytes, trace data throughput, and CPU load.
* :ref:`lwm2m_client` sample:

  * Fixed:

    * Default configuration conforms to the LwM2M specification v1.0 instead of v1.1.
      For enabling v1.1 there is already an overlay file.
    * Bootstrap is not TLV only anymore.
      With v1.1, preferred content format is sent in the bootstrap request.
      SenML CBOR takes precedence over SenML JSON and OMA TLV, when enabled.
    * Generation of the timestamp of LwM2M Location object on obtaining location fix.

  * Added:

    * CoAP max message size is set to 1280 by default.
    * Number of SenML CBOR records is set to a higher value to cope with data exchange after registration with Coiote server.

  * Updated:

    * The sample now uses TF-M instead of SPM.

* :ref:`memfault_sample` sample:

  * Updated the sample to reflect changes in logging to the `Memfault SDK`_.

* :ref:`modem_shell_application` sample:

  * Added:

    * nRF9160 DK overlays for enabling BT support.
      When running this configuration, you can perform BT scanning and advertising using the ``bt`` command.
    * Support for injecting GNSS reference altitude for low accuracy mode.
      For a position fix using only three satellites, GNSS module must have a reference altitude that can now be injected using the ``gnss agps ref_altitude`` command.
    * New command ``startup_cmd``, which can be used to store up to three MoSh commands to be run on start/bootup.
      By default, commands are run after the default PDN context is activated, but can be set to run N seconds after bootup.
    * New command ``link search`` for setting periodic modem search parameters.
    * Added printing of modem domain events.
    * MQTT support for ``gnss`` command A-GPS and P-GPS.
    * An application-specific modem fault handler.
      The modem fault handler halts application execution in case of a modem crash.
    * Support for SEGGER's Real Time Transfer (RTT) instead of UART.

  * Updated:

    * Changed timeout parameters from seconds to milliseconds in ``location`` and ``rest`` commands.
    * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Changed:

    * This sample now uses the :ref:`lib_modem_antenna` library to configure the GNSS antenna instead of configuring it directly.
    * Minor logging and function structure improvements
    * :ref:`lib_nrf_cloud` library is no longer de-initialized and re-initialized on disconnect and reconnect.
    * The :ref:lib_nrf_cloud library function :c:func:nrf_cloud_gnss_msg_json_encode is used to send PVT location data instead of building an NMEA sentence.

  * Added:

    * Support for full modem FOTA.
    * LED status indication.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added:

    * Support for full modem FOTA updates.

* :ref:`at_monitor_sample` sample:

  * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

Thread samples
--------------

* Enabled logging of errors and hard faults in CLI sample by default.
* Extended the CLI sample documentation with SRP information.

Matter samples
--------------

* Removed the low-power configuration build type from all Matter samples.
* Optimized the usage of the QSPI NOR flash sleep mode to reduce power consumption during the Matter commissioning.

* :ref:`matter_light_switch_sample`:

  * Set :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` to be enabled by default.
  * Introduced support for Matter over Wi-Fi on ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield.

* :ref:`matter_lock_sample`:

  * Set :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` to be enabled by default.
  * Introduced support for Matter over Wi-Fi on ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield.

* :ref:`matter_template_sample`:

  * Introduced support for Matter over Wi-Fi on ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield.

* :ref:`matter_window_covering_sample`:

  * Set :kconfig:option:`CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT` and :kconfig:option:`CONFIG_CHIP_THREAD_SSED` to be enabled by default.
  * Added information about the :ref:`matter_window_covering_sample_ssed` in the sample documentation.

NFC samples
-----------

* Added a note to the documentation of each NFC sample about debug message configuration with the NFCT driver from the `nrfx`_ repository.

nRF5340 samples
---------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

* :ref:`zigbee_light_switch_sample` sample:

  * Fixed an issue where a buffer would not be freed after a failure occurred when sending a Match Descriptor request.

* :ref:`zigbee_shell_sample` sample:

  * Added support for :ref:`zephyr:nrf52840dongle_nrf52840`.
  * Added an option to build :ref:`zigbee_shell_sample` sample with the nRF USB CDC ACM as shell backend.

* :ref:`zigbee_ncp_sample` sample:

  * Set :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING` to be disabled by default for NCP over USB variant.

Wi-Fi samples
-------------

* Added :ref:`wifi_shell_sample` sample with the shell support.

Other samples
-------------

* :ref:`radio_test` sample:

  * Fixed the way of setting gain for the nRF21540 Front-end Module with nRF5340.

* :ref:`caf_sensor_manager_sample` sample:

  * Added :ref:`configuration<sensor_stub_config>` for the Sensor stub driver.

Devicetree configuration
========================

Thingy:91
---------

* Changed the PWM frequency of the pwmleds device from 50 Hz to 125 Hz.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added :ref:`sensor_stub`

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v0.30.1.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.
  * Projects that use the LwM2M carrier library cannot use TF-M for this release, since the LwM2M carrier library requires hard floating point.
    For more information, see the :ref:`TF-M <ug_tfm>` documentation.

Bluetooth libraries and services
--------------------------------

* Added:

  * :ref:`mds_readme`.
  * :ref:`bt_le_adv_prov_readme`.
    The subsystem manages Bluetooth LE advertising data and scan response data.
    The subsystem does not control Bluetooth LE advertising by itself.

* :ref:`bt_fast_pair_readme` service:

  * Added a SHA-256 hash check to ensure the Fast Pair provisioning data integrity.
  * Added unit test for the storage module.
  * Extended API to allow setting the flag for the hide UI indication in the Fast Pair not discoverable advertising data.

* :ref:`bt_enocean_readme` library

  * Added callback :c:member:`decommissioned` to :c:struct:`bt_enocean_callbacks` when EnOcean switch is decommissioned.

* :ref:`bt_mesh`:

  * Added:

    * Use of decommissioned callback in :ref:`bt_mesh_silvair_enocean_srv_readme` when EnOcean switch is decommissioned.
    * :ref:`emds_readme` support to:

      *  :ref:`bt_mesh_plvl_srv_readme`
      *  :ref:`bt_mesh_light_hue_srv_readme`
      *  :ref:`bt_mesh_light_sat_srv_readme`
      *  :ref:`bt_mesh_light_temp_srv_readme`
      *  :ref:`bt_mesh_light_xyl_srv_readme`
      *  :ref:`bt_mesh_lightness_srv_readme`
      * Replay protection list (RPL)


Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

* Updated:

  * :ref:`at_monitor_readme` library:

    * The :c:func:`at_monitor_pause` and :c:func:`at_monitor_resume` macros are now functions, and take a pointer to the AT monitor entry.

  * :ref:`modem_key_mgmt` library:

    * Fixed:

      * An issue that would cause the library to assert on an unhandled CME error when the AT command failed to be sent.

  * :ref:`at_cmd_parser_readme` library:

    * Fixed:

      * An issue that would cause AT command responses like ``+CNCEC_EMM`` with underscore to be filtered out.

  * :ref:`pdn_readme` library:

    * Added:

      * Support for setting multiple event callbacks for the default PDP context.
      * The :c:func:`pdn_default_ctx_cb_dereg` function to deregister a callback for the default PDP context.
      * The :c:func:`pdn_esm_strerror` function to retrieve a textual description of an ESM error reason.
        The function is compiled when :kconfig:option:`CONFIG_PDN_ESM_STRERROR` Kconfig option is enabled.

    * Updated:

      * Renamed the :c:func:`pdn_default_callback_set` function to :c:func:`pdn_default_ctx_cb_reg`.
      * Automatically subscribe to ``+CNEC=16`` and ``+CGEREP=1`` if the :ref:`lte_lc_readme` library is used to change the modem's functional mode.

    * Removed:

      * The ``CONFIG_PDN_CONTEXTS_MAX`` Kconfig option.
        The maximum number of PDP contexts is now dynamic.

  * :ref:`nrf_modem_lib_readme` library:

    * Updated:

      * Ability to add :ref:`custom trace backends <adding_custom_modem_trace_backends>`.
      * The trace module has been updated to use the new APIs in Modem library.
        The modem trace output is now handled by a dedicated thread that starts automatically.
        The trace thread is synchronized with the initialization and shutdown operations of the Modem library.
      * The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_ENABLED` is replaced by the Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE`. The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_ENABLED` is now deprecated and will be removed in the future.
      * Added the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` option to enable the :c:func:`nrf_modem_lib_diag_stats_get` function that retrieves memory runtime statistics, replacing the ``nrf_modem_lib_heap_diagnose`` and ``nrf_modem_lib_shm_tx_diagnose`` functions.
      * Consolidated ``CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC`` and ``CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC`` into the new :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC` option.
      * Consolidated ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC`` and ``CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC`` into the new :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP` option.
      * Consolidated ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS`` and ``CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS`` into the new :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP_PERIOD_MS` option.

    * Removed:

      * The following Kconfig options:

        * ``CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING``
        * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE``
        * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE_OVERRIDE``
        * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIODIC``
        * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIOD_MS``
        * ``CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC``
        * ``CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC``
        * ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC``
        * ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS``
        * ``CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC``
        * ``CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS``

      * The following functions:

        * ``nrf_modem_lib_trace_start``
        * ``nrf_modem_lib_trace_stop``
        * ``nrf_modem_lib_heap_diagnose``
        * ``nrf_modem_lib_shm_tx_diagnose``

  * :ref:`lib_location` library:

    * Changed timeout parameters' type from uint16_t to int32_t, unit from seconds to milliseconds, and value to disable them from 0 to SYS_FOREVER_MS.
      This change is done to align with Zephyr's style for timeouts.
    * Fixed an issue with P-GPS predictions not being used to speed up GNSS when first downloaded.

  * :ref:`modem_info_readme` library:

    * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

Libraries for networking
------------------------

* Updated:

  * :ref:`lib_lwm2m_client_utils` library:

    * Fixed:

      * Setting of the FOTA update result.
      * Reporting of the FOTA update result back to the LwM2M server.

    * Updated:

      * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

  * :ref:`lib_nrf_cloud` library:

    * Fixed:

      * An issue that caused the application to receive multiple disconnect events.
      * An issue that prevented full modem FOTA updates to be installed during library initialization.
      * An issue that caused the :c:func:`nrf_cloud_client_id_get` function to fail if both :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` and :kconfig:option:`CONFIG_NRF_CLOUD_REST` were enabled.

    * Added:

      * :c:func:`nrf_cloud_fota_pending_job_validate` function that enables an application to validate a pending FOTA job before initializing the :ref:`lib_nrf_cloud` library.
      * Handling for new nRF Cloud REST error code 40499.
        Moved the error log from the :c:func:`nrf_cloud_parse_rest_error` function into the calling function.
      * Support for full modem FOTA updates.
      * :c:func:`nrf_cloud_fota_is_type_enabled` function that determines if the specified FOTA type is enabled by the configuration.
      * :c:func:`nrf_cloud_gnss_msg_json_encode` function that encodes GNSS data (PVT or NMEA) into an nRF Cloud device message.
      * :c:func:`nrf_cloud_fota_pending_job_type_get` function that retreives the FOTA type of a pending FOTA job.

    * Updated:

      * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.
      * Function :c:func:`nrf_cloud_fota_is_type_enabled` no longer depends on :kconfig:option:`CONFIG_NRF_CLOUD_FOTA`.

  * :ref:`lib_multicell_location` library:

    * Added timeout parameter.
    * Made a structure for input parameters for multicell_location_get() to make updates easier in the future.
    * The conversions of RSRP and RSRQ now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

  * :ref:`lib_rest_client` library:

    * Updated timeout handling. Now using http_client library timeout also.
    * Removed CONFIG_REST_CLIENT_SCKT_SEND_TIMEOUT and CONFIG_REST_CLIENT_SCKT_RECV_TIMEOUT.
    * Updated to handle a zero timeout value as "no timeout" (wait forever) to avoid immediate timeouts.

  * :ref:`lib_nrf_cloud_rest` library:

    * Updated the :c:func:`nrf_cloud_rest_send_location` function to accept a :c:struct `nrf_cloud_gnss_data` pointer instead of an NMEA sentence.

  * :ref:`lib_nrf_cloud_pgps` library:

    * Reduced logging level for many messages to DBG.
    * Added the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_HTTP`, and :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_CUSTOM` Kconfig options.
    * Added the :c:func:`nrf_cloud_pgps_begin_update` function that prepares the P-GPS subsystem to receive downloads from a custom transport.
    * Added the :c:func:`nrf_cloud_pgps_process_update` function that stores a portion of a P-GPS download to flash.
    * Added the :c:func:`nrf_cloud_pgps_finish_update` function that a user of the P-GPS library calls when the custom download completes.

  * :ref:`lib_azure_iot_hub` library:

    * The library has been reworked to use `Azure SDK for Embedded C`_.
    * The APIs for both IoT Hub and DPS interaction have changed and the applications and samples that use the library have been updated.

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added:

  * :ref:`nrf_rpc_ipc_readme` library.

* :ref:`lib_flash_patch` library:

  * Allow the :kconfig:option:`CONFIG_DISABLE_FLASH_PATCH` Kconfig option to be used on the nRF52833 SoC.

* :ref:`doc_fw_info` module:

  * Fixed a bug where MCUboot would experience a fault when using the :ref:`doc_fw_info_ext_api` feature.

* :ref:`nrf_rpc_ipc_readme`:

  * This library can use different transport implementation for each nRF RPC group.
  * Memory for remote procedure calls is now allocated on a heap instead of the calling thread stack.

* :ref:`emds_readme`

  * Updated :c:func:`emds_entry_add` to no longer use heap, but instead require a pointer to the dynamic entry structure :c:struct `emds_dynamic_entry`.
    The dynamic entry structure should be allocated in advance.

* :ref:`mod_memfault`:

  * Added default metrics for Bluetooth.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_adv`:

  * Added :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST` Kconfig option.
    The option is used instead of :kconfig:option:`CONFIG_BT_FILTER_ACCEPT_LIST` option to enable the filter accept list.
  * Integrated :ref:`bt_le_adv_prov_readme`.
    The subsystem is now used instead of the :file:`*.def` file to configure advertising data and scan response data.
  * Bluetooth device name is no longer automatically included in scan response data.
    A dedicated data provider (:kconfig:option:`CONFIG_BT_LE_ADV_PROV_DEVICE_NAME`) can be used to add the Bluetooth device name to the scan response data.
  * Added :c:struct:`ble_adv_data_update_event` that can be used to trigger update of advertising data and scan response data during undirected advertising.
    When the event is received, the module gets new data from providers and updates advertising payload.

* :ref:`caf_ble_state`:

  * Running on Bluetooth Peripheral no longer automatically send security request immediately after Bluetooth LE connection is established.
    The :kconfig:option:`CONFIG_CAF_BLE_STATE_SECURITY_REQ` can be used to enable this feature.
    The option can be used for both Bluetooth Peripheral and Bluetooth Central.

* :ref:`caf_sensor_data_aggregator`:

  * Added unit tests for the library.

* :ref:`caf_sensor_manager`:

  * No longer uses floats to calculate and determine if the sensor trigger is activated.
    This is because the float uses more space.
    Also, data sent to :c:struct:`sensor_event` uses :c:struct:`sensor_value` instead of float.

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

* :ref:`lib_zigbee_application_utilities` library:

  * Added :kconfig:option:`CONFIG_ZIGBEE_PANID_CONFLICT_RESOLUTION` for enabling automatic PAN ID conflict resolution.
    This option is enabled by default.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`bt_fast_pair_provision_script`:

  * Added a SHA-256 hash of the Fast Pair provisioning data to ensure its integrity.

Unity
-----

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``e86f575f68fdac2cab1898e0a893c8c6d8fd0fa1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* |no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``fb802fb6c0af80dbd383e744065bcf1745ecbc66``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline fb802fb6c0 ^45ef0d2

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^fb802fb6c0

The current |NCS| main branch is based on revision ``fb802fb6c0`` of Zephyr.


Additions specific to |NCS|
---------------------------

Added Wi-Fi L2 layer, this is a fork of Zephyr's Wi-Fi L2 with the added support for WPA supplicant.

zcbor
=====

The `zcbor`_ module has been updated from version 0.4.0 to 0.5.1.
Release notes for 0.5.0 and 0.5.1 can be found in :file:`ncs/nrf/modules/lib/zcbor/RELEASE_NOTES.md`.
:ref:`lib_fmfu_fdev` code has been regenerated using zcbor 0.5.1.


Trusted Firmware-M
==================

* Fixed:

  * |no_changes_yet_note|

cJSON
=====

* |no_changes_yet_note|

Documentation
=============

* Added:

  * Software maturity entries for security features: TF-M, PSA crypto, Immutable bootloader, HW unique key.
  * Documentation for :ref:`ug_nrf70`.
  * Documentation for the :ref:`lib_flash_map_pm` library.
  * Documentation for the :ref:`lib_adp536x` library.
  * Documentation for the :ref:`lib_flash_patch` library.
  * :ref:`gs_debugging` section on the :ref:`gs_testing`.
    Also added links to this section in different areas of documentation.
  * :ref:`ug_thread_prebuilt_libs` as a separate page instead of being part of :ref:`ug_thread_configuring`.
  * A section about NFC in the :ref:`app_memory` page.
  * A section about :ref:`modem_trace_backend_uart_custom_board` in the :ref:`nrf_modem_lib_readme` page.
  * A note in the :ref:`ug_ble_controller` about the usage of the Zephyr LE Controller.
  * A section about :ref:`modem_trace` in the :ref:`ug_nrf91_features` page.

* Updated:

  * :ref:`gs_recommended_versions` page with a new section about :ref:`gs_supported_OS`.
  * :ref:`ug_nrf91` with the following changes:

    * In the :ref:`ug_nrf9160_gs` guide, :ref:`nrf9160_gs_updating_fw_modem` section is now moved before :ref:`nrf9160_gs_updating_fw_application` because updating modem firmware erases application firmware.
    * In the :ref:`ug_nrf9160` guide, the :ref:`build_pgm_nrf9160` section now mentions |VSC| and command line instructions.
    * In the :ref:`ug_thingy91_gsg` guide, :ref:`programming_thingy` and :ref:`connect_nRF_cloud` sections now have different structure.
    * The instructions and images in the :ref:`ug_thingy91_gsg` and :ref:`ug_nrf9160_gs` documentation pages now also mention accepting :term:`eUICC Identifier (EID)` when activating your iBasis SIM card from the `nRF Cloud`_ website.

  * :ref:`ug_thread_configuring` page to better indicate what is required and what is optional.
    Also added further clarifications to the page to make everything clearer.
  * :ref:`ug_matter_tools` page with a new section about the ZAP tool.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
