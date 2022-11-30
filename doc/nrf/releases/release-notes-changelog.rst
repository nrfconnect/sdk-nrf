.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.2.99
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
See `known issues for nRF Connect SDK v2.2.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Added a documentation page about :ref:`app_approtect`.

RF Front-End Modules
--------------------

|no_changes_yet_note|

Build system
------------

* Removed:

  * Manifest file entry ``mbedtls-nrf`` (:makevar:`ZEPHYR_MBEDTLS_NRF_MODULE_DIR`) checked out at path :file:`mbedtls`.
  * Manifest file entry ``tfm-mcuboot`` (:makevar:`ZEPHYR_TFM_MCUBOOT_MODULE_DIR`) checked out at path :file:`modules/tee/tfm-mcuboot`.

* Updated:

  * Manifest file entry ``mbedtls`` (:makevar:`ZEPHYR_MBEDTLS_MODULE_DIR`) checked out at path :file:`modules/crypto/mbedtls` now points to |NCS|'s fork of MbedTLS instead of Zephyr's fork.

Working with nRF52 Series
=========================

* Developing with nRF52 Series:

  * Added Kconfig options :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` and :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` to configure FOTA updates over Bluetooth Low Energy in the default setup.
    The default setup uses MCUmgr libraries with the Bluetooth transport layer and requires the user to enable MCUboot bootloader.
    See details in the FOTA updates section of :ref:`ug_nrf52_developing` guide.

Working with nRF53 Series
=========================

* Developing with nRF5340 DK:

  * Added Kconfig options :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` and :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` to configure FOTA updates over Bluetooth Low Energy in the default setup.
    The default setup uses MCUmgr libraries with the Bluetooth transport layer and requires the user to enable MCUboot bootloader.
    See details in the FOTA updates section of :ref:`ug_nrf5340` guide.

* Developing with Thingy:53:

  * Added the :kconfig:option:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM` Kconfig option to configure USB CDC ACM to be used as logger's backend by default.
    See details in :ref:`thingy53_app_usb` section of Thingy:53 application guide.
  * Provided support for Thingy:53 FOTA updates within the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` option.
    See details in :ref:`thingy53_app_fota_smp` section of Thingy:53 application guide.
  * Enabled MCUboot bootloader in Thingy:53 board configuration by default.
    See details in :ref:`thingy53_app_mcuboot_bootloader` section of Thingy:53 application guide.
  * Cleaned up Thingy:53 specific configuration files of samples and applications as a result of introducing simplifications.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

|no_changes_yet_note|

Bluetooth mesh
--------------

|no_changes_yet_note|

Matter
------

* Added:

  * Support for switching between Matter over Thread and Matter over Wi-Fi.
    This feature is available for the :ref:`matter_lock_sample` sample programmed on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached, using the ``thread_wifi_switched`` build type.
    See :ref:`matter_lock_sample_wifi_thread_switching` in the sample documentation for more information.
  * Documentation about :ref:`switchable Matter over Thread and Matter over Wi-Fi <ug_matter_overview_architecture_integration_designs>` platform design.
  * Support for Wi-Fi Network Diagnostic Cluster (which counts the number of packets received and transmitted on the Wi-Fi interface).
  * Default support for nRF7002 revision B.
  * Specific QR code and onboarding information in the documentation for each :ref:`Matter sample <matter_samples>` and the :ref:`Matter weather station <matter_weather_station_app>`.
  * The Bluetooth LE advertising arbiter class that enables easier coexistence of application components that want to advertise their Bluetooth LE services.
  * Support for erasing settings partition during DFU over Bluetooth LE SMP for the Nordic nRF52 Series' SoCs.
  * Enabled Wi-Fi and Bluetooth LE coexistence.
  * Mechanism to retry a failed Wi-Fi connection.
  * Documentation about :ref:`ug_matter_gs_ecosystem_compatibility_testing`.
  * Support for ZAP tool under Windows.

* Updated:

  * The default heap implementation to use Zephyr's ``sys_heap`` (:kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP`) to better control the RAM usage of Matter applications.
  * :ref:`ug_matter_device_certification` page with a section about certification document templates.
  * :ref:`ug_matter_overview_commissioning` page with information about :ref:`ug_matter_network_topologies_commissioning_onboarding_formats`.
  * :ref:`ug_matter_hw_requirements` page with a section about :ref:`ug_matter_hw_requirements_layouts`.
  * Default retry intervals used by Matter Reliability Protocol for Matter over Thread to account for longer round-trip times in Thread networks with multiple intermediate nodes.
  * The Bluetooth LE connection timeout parameters and the update timeout parameters to make communication over Bluetooth LE more reliable.
  * Default transmission output power for Matter over Thread devices to the maximum available one for all targets:
    8 dBm for nRF52840, 3 dBm for nRF5340, 20 dBm for all devices with FEM enabled, and 0 dBm for sleepy devices.
  * :ref:`ug_matter_gs_adding_cluster` page with instructions on how to use ZAP tool binaries. Before this release, the ZAP tool had to be built from sources.

* Fixed:

  * The issue where the connection would time out when attaching to a Wi-Fi access point that requires Wi-Fi Protected Access 3 (WPA3).
  * The issue where the ``NetworkInterfaces`` attribute of General Diagnostics cluster would return EUI-64 instead of MAC Extended Address for Thread network interfaces.

* Removed support for Android CHIP Tool from the documentation and release artifacts.
  Moving forward, we recommend using the development tool CHIP Tool for Linux or macOS or mobile applications from publicly available Matter Ecosystems.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``1.0.0.2`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * The initial implementation of Matter's cryptographic operations based on PSA crypto API.
  * Build-time generation of some Zigbee Cluster Library (ZCL) source files using the :file:`codegen.py` Python script.
  * An alternative factory reset implementation that erases the entire non-volatile storage flash partition.

* Updated:

  * Basic cluster has been renamed to Basic Information cluster to match the specification.

Thread
------

* Added:

  * Support for setting the default Thread output power using the :kconfig:option:`OPENTHREAD_DEFAULT_TX_POWER` Kconfig option.
  * A Thread :ref:`power consumption data <thread_power_consumption>` page.

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

|no_changes_yet_note|

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

Enhanced ShockBurst (ESB)
-------------------------

* Added support for front-end modules.
  The ESB module requires linking the :ref:`MPSL library <nrfxlib:mpsl_lib>`.

* Updated:

  * Number of PPI/DPPI channels used from three to six.
  * Events 6 and 7 from the EGU0 instance by assigning them to the ESB module.
  * The type parameter of the function :c:func:`esb_set_tx_power` to ``int8_t``.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

* Added:

  * New sample :ref:`wifi_sr_coex_sample` demonstrating Wi-Fi Bluetooth LE coexistence.
  * :ref:`ug_wifi` document.
  * :ref:`lib_wifi_credentials` library to store credentials.
  * :ref:`wifi_mgmt_ext` library to provide an autoconnect command based on Wi-Fi credentials.

* Removed:

  * nRF7002 revision A support.

* Updated:

  * WiFi Coexistence is no longer enabled by default.
    It must be enabled explicitly in Kconfig using :kconfig:option:`CONFIG_MPSL_CX`.
    On the nRF5340, this option must be selected for both the application core and the network core images.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Wi-Fi support for nRF9160 DK + nRF7002 EK configuration.
  * A section about :ref:`Custom transport <asset_tracker_v2_ext_transport>` in the :ref:`asset_tracker_v2_debug_module` documentation.

* Updated:

  * Due to the :ref:`lib_location` library updates related to combined cellular and Wi-Fi positioning,
    the following events and functions have been added replacing old ones:

    * :c:enum:`LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY` replaces ``LOCATION_MODULE_EVT_NEIGHBOR_CELLS_DATA_READY`` and ``LOCATION_MODULE_EVT_WIFI_ACCESS_POINTS_DATA_READY``
    * :c:enum:`DATA_EVT_CLOUD_LOCATION_DATA_SEND` replaces ``DATA_EVT_NEIGHBOR_CELLS_DATA_SEND`` and ``DATA_EVT_WIFI_ACCESS_POINTS_DATA_SEND``
    * :c:func:`cloud_codec_encode_cloud_location` function replaces ``cloud_codec_encode_neighbor_cells`` and ``cloud_codec_encode_wifi_access_points``
    * :c:func:`cloud_wrap_cloud_location_send` function replaces ``cloud_wrap_neighbor_cells_send`` and ``cloud_wrap_wifi_access_points_send``

  * Replaced deprecated LwM2M API calls with calls to new functions.
  * Removed static modem data handling from the application's nRF Cloud codec.
    Enabled the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS` configuration option to send static modem data.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * RFC1350 TFTP client, currently supporting only *READ REQUEST*.
  * AT command ``#XSHUTDOWN`` to put nRF9160 SiP to System Off mode.
  * Support of nRF Cloud C2D appId ``MODEM`` and ``DEVICE``.
  * Support for the :ref:`liblwm2m_carrier_readme` library.

* Updated:

  * The response for the ``#XDFUGET`` command, using unsolicited notification to report download progress.

nRF5340 Audio
-------------

* Added:

  * Support for Front End Module nRF21540.
  * Possibility to create a Public Broadcast Announcement (PBA) needed for Auracast.
  * Encryption for BISes.
  * Support for bidirectional streams to or from two headsets (True Wireless Stereo).
  * Support for interleaved packing.

* Updated:

  * Controller from version 3310 to 3322, with the following major changes:

    * Changes to accommodate BIS + ACL combinations.
    * Improvements to support creating CIS connections in any order.
    * Basic support for interleaved broadcasts.

  * Power module has been re-factored so that it uses upstream Zephyr INA23X sensor driver.
  * BIS headsets can now switch between two broadcast sources (two hardcoded broadcast names).
  * :ref:`nrf53_audio_app_ui` and :ref:`nrf53_audio_app_testing_steps_cis` sections in the application documentation with information about using **VOL** buttons to switch headset channels.
  * :ref:`nrf53_audio_app_requirements` section in the application documentation by moving the information about the nRF5340 Audio DK to `Nordic Semiconductor Infocenter`_, under `nRF5340 Audio DK Hardware`_.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added configuration option (:kconfig:option:`CONFIG_APP_SENSOR_SLEEP_TO`) to set the sensor idling timeout before suspending the sensor.

* Removed the usage of ``ml_runner_signin_event`` from the application.

nRF Desktop
-----------

* Added:

  * An application log indicating that the value of a configuration option has been updated in the :ref:`nrf_desktop_motion`.
  * Application-specific Kconfig options :ref:`CONFIG_DESKTOP_LOG <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>` to simplify the debug configurations for the Logging and Shell subsystems.
    See the debug configuration section of the :ref:`nrf_desktop` application for more details.
  * The :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO <config_desktop_app_options>` configuration option.
    It can be used to indicate the dongle peer identity with a dedicated event.
  * Synchronization between the Resolvable Private Address (RPA) rotation and the advertising data update in the Fast Pair configurations using the :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA` option.

* Updated:

  * Implemented the following adjustments to avoid flooding logs:

    * Set the max compiled-in log level to ``warning`` for the Non-Volatile Storage (:kconfig:option:`CONFIG_NVS_LOG_LEVEL`).
    * Lowered a log level to ``debug`` for the ``Identity x created`` log in the :ref:`nrf_desktop_ble_bond`.

  * By default, nRF Desktop dongles use the Bluetooth appearance (:kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE)` of keyboard.
    The new default configuration value improves consistency with the used HID boot interface.
  * The default values of the :kconfig:option:`CONFIG_BT_GATT_CHRC_POOL_SIZE` and :kconfig:option:`CONFIG_BT_GATT_UUID16_POOL_SIZE` Kconfig options are tailored to the nRF Desktop application requirements.
  * The :ref:`nrf_desktop_fast_pair_app` to remove the Fast Pair advertising payload for the dongle peer.

* Removed:

  * Fast Pair discoverable advertising payload on Resolvable Private Address (RPA) rotation during discoverable advertising session.
  * Separate configurations enabling :ref:`zephyr:shell_api` (:file:`prj_shell.conf`).
    Shell support can be enabled for a given configuration with the single Kconfig option (:ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>`).

Samples
=======

Bluetooth samples
-----------------

* Added the :ref:`peripheral_status` sample.

* :ref:`peripheral_uart` sample:

  * Fixed a possible memory leak in the :c:func:`uart_init` function.

* :ref:`peripheral_hids_keyboard` sample:

  * Fixed a possible out-of-bounds memory access issue in the :c:func:`hid_kbd_state_key_set` and :c:func:`hid_kbd_state_key_clear` functions.

* :ref:`ble_nrf_dm` sample:

  * Added support for high-precision distance estimate using more compute-intensive algorithms.
  * Updated:

    * Documentation by adding energy consumption information.
    * Documentation by adding a section about distance offset calibration.
    * Configuration of the GPIO pins used by the DM module using the device tree.

* :ref:`peripheral_nfc_pairing` and :ref:`central_nfc_pairing` samples:

  * Fixed OOB pairing between these samples.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_ble_peripheral_lbs_coex` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

* :ref:`bt_mesh_chat` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

* :ref:`bluetooth_mesh_light` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

* :ref:`bluetooth_mesh_light_switch` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Updated:

    * Enabled :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` configuration option.

nRF9160 samples
---------------

* Added:

  * :ref:`nidd_sample` sample that demonstrates how to use Non-IP Data Delivery (NIDD).

* :ref:`modem_shell_application` sample:

  * Added:

    * External location service handling to test :ref:`lib_location` library functionality commonly used by applications.
      The :ref:`lib_nrf_cloud` library is used with MQTT for location requests to the cloud.
    * New command ``th pipeline`` for executing several MoSh commands sequentially in one thread.
    * New command ``sleep`` for introducing wait periods in between commands when using ``th pipeline``.
    * New command ``heap`` for printing kernel and system heap usage statistics.

  * Updated:

    * Timeout command-line arguments for the ``location get`` command changed from integers in milliseconds to floating-point values in seconds.
    * Replaced deprecated LwM2M API calls with calls to new functions.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added the usage of GCI search option if running modem firmware v1.3.4.
  * Updated the sample, so that it waits for RRC idle mode before requesting neighbor cell measurements.

* :ref:`lwm2m_client` sample:

  * Added:

    * Support for nRF7002 EK shield and Wi-Fi based location.
    * Location events and event handlers.

  * Updated:

    * The sensor module has been simplified.
      It does not use application events, filtering, or configurable periods anymore.
    * Replaced deprecated LwM2M API calls with calls to new functions.
    * Enabled LwM2M queue mode and updated documentation accordingly.
    * Moved configuration options from :file:`overlay-queue.conf` to default configuration :file:`prj.conf`.
    * Removed :file:`overlay-queue.conf`.
    * Enabled :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS` configuration option.

* :ref:`http_application_update_sample` sample:

  * Added support for the :ref:`liblwm2m_carrier_readme` library.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Updated:

    * The sample now uses a partition in external flash for full modem FOTA updates.

  * Added:

    * MCUboot child image files to properly access external flash on newer nRF9160DK versions.
    * An :file:`overlay_mcuboot_ext_flash.conf` file to enable MCUboot use of external flash.
    * Sending an alert to the cloud on boot and when a temperature limit is exceeded.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added sending an alert to nRF Cloud on boot.

* :ref:`slm_shell_sample` sample:

  * Added:

    * Sample for nRF52 and nRF53 Series devices to send AT commands to nRF9160 SiP from shell.

* Removed:

  * Multicell location sample because of the deprecation of the Multicell location library.
    Relevant functionality is available through the :ref:`lib_location` library.
  * nRF9160: Simple MQTT sample.
    This is now replaced by a new :ref:`mqtt_sample` sample that supports Wi-Fi and LTE connectivity.

* :ref:`nrf_cloud_rest_fota` sample:

  * Updated:

    * Device status information, including FOTA enablement, is now sent to nRF Cloud when the device connects.
    * Removed user prompt and button press handling for FOTA enablement.
    * The sample now uses a partition in external flash for full modem FOTA updates.

* :ref:`azure_fota_sample` sample:

  * The sample now uses the logging subsystem for console output.

* :ref:`azure_iot_hub` sample:

  * The sample now uses the logging subsystem for console output.

* :ref:`aws_iot` sample:

  * The sample now uses the logging subsystem for console output.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added support for the nRF7002 board.
  * Fixed sample building with support for the Skyworks front-end module.
  * Updated the documentation to clarify that this sample is dedicated for the short-range radio (Bluetooth LE, IEEE 802.15.4, and proprietary modes).

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------
* Added:

  * :file:`overlay-low_power.conf` and :file:`low_power.overlay` to the CLI sample to facilitate power consumption measurements.

* Updated:

  * Overlay structure:

    * :file:`overlay-rtt.conf` removed from all samples.
    * :file:`overlay-log.conf` now uses RTT backend by default.
    * Logs removed from default configuration (moved to :file:`overlay-logging.conf`).
    * Asserts removed from default configuration (moved to :file:`overlay-debug.conf`).


Matter samples
--------------

* Enabled Matter shell commands for all build types except ``release`` in all Matter samples.
* Removed FEM-related Kconfig options from all samples.
  Now, the transmission output power for Matter over Thread can be set using the :kconfig:option:`OPENTHREAD_DEFAULT_TX_POWER` Kconfig option.

* :ref:`matter_lock_sample` sample:

  * Added:

    * ``thread_wifi_switched`` build type that enables switching between Thread and Wi-Fi network support in the field.
      See :ref:`matter_lock_sample_wifi_thread_switching` in the sample documentation for more information.
    * Wi-Fi low power configuration using Wi-Fi's :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.

* :ref:`matter_light_switch_sample`:

  * Added Wi-Fi low power configuration using Wi-Fi's :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.

NFC samples
-----------

* Fixed an issue where NFC samples that use the NFC Reader feature returned false error code with value ``1`` during the NFC T4T operation.

|no_changes_yet_note|

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

  * :ref:`wifi_sr_coex_sample` sample demonstrating Wi-Fi Bluetooth LE coexistence.

* Removed:

  * nRF7002 revision A support.

* Changed:

* The :ref:`wifi_shell_sample` sample now uses the :ref:`lib_wifi_credentials` and :ref:`wifi_mgmt_ext` libraries.

* The :ref:`wifi_provisioning` sample now uses the :ref:`lib_wifi_credentials` and :ref:`wifi_prov_readme` libraries.

Other samples
-------------

* Added :ref:`mqtt_sample` sample that supports Wi-Fi and LTE connectivity.

* :ref:`esb_prx_ptx` sample:

  * Added support for front-end modules and :ref:`zephyr:nrf21540dk_nrf52840`.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* :ref:`pmw3360`:

  * Updated by reducing log verbosity.

* :ref:`paw3212`:

  * Updated by reducing log verbosity.

* :ref:`lib_bh1749`:

  * Fixed an issue where the driver would attempt to use APIs before the sensor was ready, which in turn could make the application hang.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Removed the dependency on the :ref:`lte_lc_readme` library.

Bluetooth libraries and services
--------------------------------

* Added the :ref:`nsms_readme` library.

* Added the :ref:`wifi_prov_readme` library.

* :ref:`mds_readme` library:

  * Fixed URI generation in the :c:func:`data_uri_read` function.

* :ref:`ble_rpc` library:

  * Fixed a possible memory leak in the :c:func:`bt_gatt_indicate_rpc_handler` function.

* :ref:`bt_le_adv_prov_readme` library:

  * Added the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_STOP_DISCOVERABLE_ON_RPA_ROTATION` Kconfig option to drop the Fast Pair advertising payload on RPA rotation.
  * Extended the :c:struct:`bt_le_adv_prov_adv_state` structure to include new fields.
    The new :c:member:`bt_le_adv_prov_adv_state.rpa_rotated` field is used to notify registered providers about Resolvable Private Address (RPA) rotation.
    The new :c:member:`bt_le_adv_prov_adv_state.new_adv_session` field is used to notify registered providers that the new advertising session is about to start.
  * Changed the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_BATTERY_DATA_MODE` Kconfig option (default value) to not include Fast Pair battery data in the Fast Pair advertising payload by default.

* :ref:`bt_fast_pair_readme` service:

  * Added the :c:func:`bt_fast_pair_factory_reset` function to clear the Fast Pair storage.

Bootloader libraries
--------------------

* :ref:`doc_bl_storage` library:

  * Updated:

    * The monotonic counter functions can now return errors.
    * The :c:func:`get_monotonic_version` function is split into functions :c:func:`get_monotonic_version` and :c:func:`get_monotonic_slot`.
    * The monotonic counter functions now have a counter description parameter to be able to distinguish between different counters.

* :ref:`doc_bl_validation` library:

  * Updated:

    * The :c:func:`get_monotonic_version` function can now return an error.

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OFF` Kconfig option to set the modem trace level to off by default.
    * The flash trace backend that enables the application to store trace data to flash for later retrieval.

  * Updated:

    * It is now possible to poll Modem library and Zephyr sockets at the same time using the :c:func:`poll` function.
      This includes special sockets such as event sockets created using the :c:func:`eventfd` function.
    * The minimal value of :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE` to meet the requirements of modem firmware 1.3.4.
    * The :c:func:`nrf_modem_lib_diag_stats_get` function now returns an error if called when the :ref:`nrf_modem_lib_readme` library has not been initialized.
    * The trace backend interface to be exposed to the :ref:`modem_trace_module` using the :c:struct:`nrf_modem_lib_trace_backend` struct.
    * The :ref:`modem_trace_module` to support backends that store the trace data for later retrieval.

* :ref:`lib_location` library:

  * Added:

    * Support for the application to send the Wi-Fi access point list to the cloud.
    * :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL` Kconfig option that replaces the following configurations:

      * ``CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL``
      * ``CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL``
      * ``CONFIG_LOCATION_METHOD_CELLULAR_EXTERNAL``

      The new configuration also handles Wi-Fi positioning.
    * Introduced several new Kconfig options for default location request configurations, including default method priority configuration.
      These new Kconfig options are applied when :c:func:`location_config_defaults_set` function is called.

  * Updated:

    * Neighbor cell measurements and Wi-Fi scan results are combined into a single cloud request.
      This also means that cellular and Wi-Fi positioning are combined into a single cloud positioning method
      if they are one after the other in the method list of the location request.
      Because of this, some parts of the API are replaced with new ones as follows:

      * Event :c:enum:`LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST` replaces old events
        ``LOCATION_EVT_CELLULAR_EXT_REQUEST`` and ``LOCATION_EVT_WIFI_EXT_REQUEST`` that are removed.
      * Function :c:func:`location_cloud_location_ext_result_set` replaces old functions
        ``location_cellular_ext_result_set`` and ``location_wifi_ext_result_set`` that are removed.
      * Member variable :c:var:`cloud_location_request` replaces old members
        ``cellular_request`` and ``wifi_request`` that are removed in :c:struct:`location_event_data`.

      :kconfig:option:`CONFIG_LOCATION_SERVICE_CLOUD_RECV_BUF_SIZE` replaces ``CONFIG_LOCATION_METHOD_CELLULAR_RECV_BUF_SIZE`` and ``CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE``.

    * The :ref:`lib_multicell_location` library is deprecated.
      Relevant functionality from the library is moved to this library.
      The following features were not moved:

      * Definition of HTTPS port for HERE service, that is :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT`.
      * HERE v1 API.
      * nRF Cloud CA certificate handling.

    * Improved GNSS assistance data need handling.
    * GNSS filtered ephemerides are no longer used when the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME` Kconfig option is enabled.
    * Renamed:

      * ``enum location_cellular_ext_result`` to ``enum location_ext_result``, because Wi-Fi will use the same enumeration.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_NRF_CLOUD`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_API_KEY`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_API_KEY`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_HOSTNAME`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_HOSTNAME`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_TLS_SEC_TAG`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG`.

  * Fixed an issue causing the A-GPS data download to be delayed until the RRC connection release.

* :ref:`lib_modem_slm` library:

  * Added:

    * Library for external MCU to work with nRF9160 SiP through the Serial LTE Modem application.

* :ref:`modem_info_readme` library:

  * Added :c:func:`modem_info_get_hw_version` function to obtain the hardware version string using the ``AT%HWVERSION`` command.

* :ref:`lte_lc_readme` library:

  * Fixed an issue where cell update events could be sent without the cell information from the modem actually being updated.


Libraries for networking
------------------------

* Added :ref:`lib_mqtt_helper` library that simplifies Zephyr MQTT API and socket handling.

* :ref:`lib_azure_iot_hub` library:

  * Pulled out the :file:`azure_iot_hub_mqtt.c` file that is now implemented by a new library :ref:`lib_mqtt_helper`.

* :ref:`lib_multicell_location` library:

  * This library is now deprecated and relevant functionality is available through the :ref:`lib_location` library.

* :ref:`lib_fota_download` library:

  * Fixed:

    * An issue where the :c:func:`download_client_callback` function was continuing to read the offset value even if :c:func:`dfu_target_offset_get` returned an error.
    * An issue where the cleanup of the downloading state was not happening when an error event was raised.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * Support for full modem FOTA updates using a partition in external flash.

  * Updated:

    * The MQTT disconnect event is now handled by the FOTA module, allowing for updates to be completed while disconnected and reported properly when reconnected.
    * GCI search results are now encoded in location requests.
    * The neighbor cell's time difference value is now encoded in location requests.

  * Fixed:

    * An issue where the same buffer was incorrectly shared between caching a P-GPS prediction and loading a new one, when external flash was used.
    * An issue where external flash only worked if the P-GPS partition was located at address 0.

  * Added:

    * Device status information is automatically sent to nRF Cloud when the device connects if the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS` Kconfig option is enabled.
      Network information is included if the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_NETWORK` Kconfig option is enabled.
      SIM card information is included if the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_SIM` Kconfig option is enabled.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_DEVICE_STATUS_ENCODE_VOLTAGE` Kconfig option controls if device voltage is included when device status data is encoded.
    * An application version string can now be included in the :c:struct:`nrf_cloud_init_param` struct.

* :ref:`lib_lwm2m_location_assistance` library:

  * Added:

    * Support for Wi-Fi based location through LwM2M.
    * API for scanning Wi-Fi access points.

  * Removed location events and event handlers.

* :ref:`lib_nrf_cloud_alerts` library:

  * A new library for sending notifications of critical device events to nRF Cloud, using either REST or MQTT connections.

* :ref:`lib_nrf_cloud_rest` library:

  * Added:

    * :c:func:`nrf_cloud_rest_device_status_message_send` function to send the device status information as an nRF Cloud device message.

Libraries for NFC
-----------------

* Added:

  * The possibility of moving an NFC callback to a thread context.
  * Support for zero-latency interrupts for NFC.

* Updated by aligning the :file:`ncs/nrf/subsys/nfc/lib/platform.c` file with new library implementation.

* :ref:`nfc_ndef_ch_rec_parser_readme` library:

  * Fixed a bug where the AC Record Parser was not functional and returned invalid results.

Other libraries
---------------

* :ref:`lib_contin_array` library:

  * Updated by separating the library from the :ref:`nrf53_audio_app` application and moving it to :file:`lib/contin_array`.
    Updated code and documentation accordingly.

* :ref:`lib_pcm_stream_channel_modifier` library:

  * Updated by separating the library from the :ref:`nrf53_audio_app` application and moving it to :file:`lib/pcm_stream_channel_modifier`.
    Updated code and documentation accordingly.

* :ref:`lib_data_fifo` library:

  * Updated by separating the library from the :ref:`nrf53_audio_app` application and moving it to :file:`lib/data_fifo`.
    Updated code and documentation accordingly.

* :ref:`QoS` library:

  * Updated by removing the ``QOS_MESSAGE_TYPES_REGISTER`` macro.

* Secure Partition Manager (SPM):

  * Removed Secure Partition Manager (SPM) and the Kconfig option ``CONFIG_SPM``.
    It is replaced by the Trusted Firmware-M (TF-M) as the supported trusted execution solution.
    See :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` for more information about the TF-M.

* :ref:`lib_pcm_mix` library:

  * New library.
    This was previously a component of the :ref:`nrf53_audio_app` application, now moved to :file:`lib/pcm_mix`.

* :ref:`app_event_manager`:

  * Updated the way section names are created for event subscribers.
    This allows you to use any event naming scheme.
    For more information, see the :ref:`NCSIDB-925 <ncsidb_925>` issue description on the :ref:`known_issues` page.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_smp`:

  * Updated:

    * Aligned the module with the recent MCUmgr API, following the :ref:`zephyr:mcumgr_callbacks` migration guide in the Zephyr documentation.
      The module now requires :kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` and :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK` configuration options to be enabled.

* :ref:`caf_ble_adv`:

  * Added :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA`.
    The option synchronizes Resolvable Private Address (RPA) rotation with the advertising data update in the Bluetooth Privacy mode.
    Added dependent :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT` and :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT_RAND` options.
    They are used to specify the rotation period and its randomization factor.

* :ref:`caf_overview_events`:

  * Added:

    * A macro intended to set the size of events member enums to 32 bits when the Event Manager Proxy is enabled.
      Applied macro to all affected CAF events.

  * Updated:

    * Inter-core compatibility has been improved.

* :ref:`caf_sensor_data_aggregator`:

  * Updated:

    * :c:struct:`sensor_data_aggregator_event` now uses the :c:struct:`sensor_value` struct data buffer and carries a number of sensor values in a single sample, which is sufficient to describe data layout.

    * The way buffers are declared is updated when the instance is created.
      Now the memory-region devicetree property works independently for each instance and does not require the specific instance name.

* :ref:`caf_sensor_manager`:

  * Updated by cleaning up :file:`sensor_event.h` and :file:`sensor_manager.h` files.
    Moved unrelated declarations to a separate :file:`caf_sensor_common.h` file.

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

DFU libraries
-------------

* :ref:`lib_dfu_target` library:

  * Added:
    * :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION` configuration option to support the ``FMFU_STORAGE`` partition in external flash.
    * :c:func:`dfu_target_full_modem_fdev_get` function that gets the configured flash device information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom`:

  * Updated so that the output contains source repository and version information for each file.

* :ref:`partition_manager`:

  * Added:

    * The :file:`ncs/nrf/subsys/partition_manager/pm.yml.fmfu` file.
    * Support for the full modem FOTA update (FMFU) partition: ``FMFU_STORAGE``.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``cfec947e0f8be686d02c73104a3b1ad0b5dcf1e6``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added an option to prevent inclusion of default nRF5340 network core DFU image hook, which allows a custom implementation by users if the :kconfig:option:`CONFIG_BOOT_IMAGE_ACCESS_HOOK_NRF5340` Kconfig option is disabled (enabled by default).
  CMake can be used to add additional hook files.
  See :file:`modules/mcuboot/hooks/CMakeLists.txt` for an example of how to achieve this.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``e1e06d05fa8d1b6ac1b0dffb1712e94e308861f8``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline cd16a8388f ^71ef669ea4

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^cd16a8388f

The current |NCS| main branch is based on revision ``cd16a8388f`` of Zephyr.

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

  * Documentation page about :ref:`ug_matter_device_security` in the Matter protocol section.
  * Documentation template for the :ref:`Ecosystem integration <Ecosystem_integration>` user guides.
  * Documentation on :ref:`ug_avsystem`.
  * The :ref:`ug_nrf70_developing` user guide.
  * A page on :ref:`ug_nrf70_features`.
  * Documentation template for :ref:`Applications <application>`.
  * The :ref:`ug_nrf5340_gs` guide.

* Updated:

  * The :ref:`software_maturity` page with details about Wi-Fi feature support.
  * The :ref:`app_power_opt` user guide by adding sections about power saving features and PSM usage.
  * The :ref:`ug_thingy53` by aligning with current simplified configuration.
  * The :ref:`ug_nrf52_developing` by aligning with current simplified FOTA configuration.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
