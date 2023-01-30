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

|no_changes_yet_note|

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

  * Support for Wi-Fi Network Diagnostic Cluster (which counts the number of packets received and transmitted on the Wi-Fi interface).
  * Default support for nRF7002 revision B.
  * Specific QR code and onboarding information in the documentation for each :ref:`Matter sample <matter_samples>` and the :ref:`Matter weather station <matter_weather_station_app>`.
  * The Bluetooth LE advertising arbiter class that enables easier coexistence of application components that want to advertise their Bluetooth LE services.
  * Support for erasing settings partition during DFU over Bluetooth LE SMP for the Nordic nRF52 Series' SoCs.
  * Enabled Wi-Fi and Bluetooth LE coexistence.

* Updated:

  * The default heap implementation to use Zephyr's ``sys_heap`` (:kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP`) to better control the RAM usage of Matter applications.
  * :ref:`ug_matter_device_certification` page with a section about certification document templates.
  * :ref:`ug_matter_overview_commissioning` page with information about :ref:`ug_matter_network_topologies_commissioning_onboarding_formats`.
  * Default retry intervals used by Matter Reliability Protocol for Matter over Thread to account for longer round-trip times in Thread networks with multiple intermediate nodes.
  * The Bluetooth LE connection timeout parameters and the update timeout parameters to make communication over Bluetooth LE more reliable.

* Fixed the issue of connection timing out when attaching to a Wi-Fi access point that requires Wi-Fi Protected Access 3 (WPA3).

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

|no_changes_yet_note|

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

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Wi-Fi support for nRF9160 DK + nRF7002 EK configuration.

* Updated:

  * Replaced deprecated LwM2M API calls with calls to new functions.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * RFC1350 TFTP client, currently supporting only *READ REQUEST*.
  * AT command ``#XSHUTDOWN`` to put nRF9160 SiP to System Off mode.
  * Support of nRF Cloud C2D appId ``MODEM`` and ``DEVICE``.

* Updated:

  * The response for the ``#XDFUGET`` command, using unsolicited notification to report download progress.

nRF5340 Audio
-------------

* Added:

  * Support for Front End Module nRF21540.
  * Possibility to create a Public Broadcast Announcement (PBA) needed for Auracast.
  * Encryption for BISes.
* Updated:

  * Power module has been re-factored so that it uses upstream Zephyr INA23X sensor driver.
  * BIS headsets can now switch between two broadcast sources (two hardcoded broadcast names).
  * :ref:`nrf53_audio_app_ui` and :ref:`nrf53_audio_app_testing_steps_cis` sections in the application documentation with information about using **VOL** buttons to switch headset channels.
  * :ref:`nrf53_audio_app_requirements` section in the application documentation by moving the information about the nRF5340 Audio DK to `Nordic Semiconductor Infocenter`_, under `nRF5340 Audio DK Hardware`_.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Removed the usage of ``ml_runner_signin_event`` from the application.

nRF Desktop
-----------

* Added:

  * An application log indicating that the value of a configuration option has been updated in the :ref:`nrf_desktop_motion`.
  * Application-specific Kconfig options :ref:`CONFIG_DESKTOP_LOG <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>` to simplify the debug configurations for the Logging and Shell subsystems.
    See the debug configuration section of the :ref:`nrf_desktop` application for more details.

* Updated:

  * Implemented the following adjustments to avoid flooding logs:

    * Set the max compiled-in log level to ``warning`` for the Non-Volatile Storage (:kconfig:option:`CONFIG_NVS_LOG_LEVEL`).
    * Lowered a log level to ``debug`` for the ``Identity x created`` log in the :ref:`nrf_desktop_ble_bond`.

  * Removed separate configurations enabling :ref:`zephyr:shell_api` (:file:`prj_shell.conf`).
    Shell support can be enabled for a given configuration with the single Kconfig option (:ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>`).
  * By default, nRF Desktop dongles use the Bluetooth appearance (:kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE)` of keyboard.
    The new default configuration value improves consistency with the used HID boot interface.

Samples
=======

Bluetooth samples
-----------------

* :ref:`peripheral_uart` sample:

  * Fixed a possible memory leak in the :c:func:`uart_init` function.

* :ref:`peripheral_hids_keyboard` sample:

  * Fixed a possible out-of-bounds memory access issue in the :c:func:`hid_kbd_state_key_set` and :c:func:`hid_kbd_state_key_clear` functions.

* :ref:`ble_nrf_dm` sample:

  * Added support for high-precision distance estimate using more compute-intensive algorithms.
  * Updated:

    * Documentation by adding energy consumption information.
    * Documentation by adding a section about distance offset calibration.

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

* :ref:`modem_shell_application` sample:

  * Added:

    * External location service handling to test :ref:`lib_location` library functionality commonly used by applications.
      The :ref:`lib_nrf_cloud` library is used with MQTT for location requests to the cloud.
    * New command ``th pipeline`` for executing several MoSh commands sequentially in one thread.
    * New command ``sleep`` for introducing wait periods in between commands when using ``th pipeline``.

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

* :ref:`http_application_update_sample` sample:

  * Added support for the :ref:`liblwm2m_carrier_readme` library.

* Removed:

  * Multicell location sample because of the deprecation of the Multicell location library.
    Relevant functionality is available through the :ref:`lib_location` library.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Added:

    * MCUboot child image files to properly access external flash on newer nRF9160DK versions.
    * An :file:`overlay_mcuboot_ext_flash.conf` file to enable MCUboot use of external flash.

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

* :ref:`matter_lock_sample` sample:

  * Added:

    * ``thread_wifi_switched`` build type that enables switching between Thread and Wi-Fi network support in the field.
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

Other samples
-------------

* :ref:`esb_prx_ptx` sample:

  * Added support for front-end modules and :ref:`zephyr:nrf21540dk_nrf52840`.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* :ref:`pmw3360`:

  * Updated by reducing log verbosity.

* :ref:`paw3212`:

  * Updated by reducing log verbosity.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`mds_readme` library:

  * Fixed URI generation in the :c:func:`data_uri_read` function.

* :ref:`ble_rpc` library:

  * Fixed a possible memory leak in the :c:func:`bt_gatt_indicate_rpc_handler` function.

* :ref:`bt_le_adv_prov_readme` library:

  * Changed the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_BATTERY_DATA_MODE` Kconfig option (default value) to not include Fast Pair battery data in the Fast Pair advertising payload by default.

Bootloader libraries
--------------------

|no_changes_yet_note|

Modem libraries
---------------

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

    * Use of :ref:`lib_multicell_location` library has been removed because the library is deprecated.
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

  * Updated:

    * The MQTT disconnect event is now handled by the FOTA module, allowing for updates to be completed while disconnected and reported properly when reconnected.
    * GCI search results are now encoded in location requests.
    * The neighbor cell's time difference value is now encoded in location requests.

  * Fixed:

    * An issue where the same buffer was incorrectly shared between caching a P-GPS prediction and loading a new one, when external flash was used.
    * An issue where external flash only worked if the P-GPS partition was located at address 0.

* :ref:`lib_lwm2m_location_assistance` library:

  * Added:

    * Support for Wi-Fi based location through LwM2M.
    * API for scanning Wi-Fi access points.

  * Removed location events and event handlers.

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

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_overview_events`:

  * Added:

    * A macro intended to set the size of events member enums to 32 bits when the Event Manager Proxy is enabled.
      Applied macro to all affected CAF events.

  * Updated:

    * Inter-core compatibility has been improved.

* :ref:`caf_sensor_data_aggregator`:

  * Updated:

    * :c:struct:`sensor_data_aggregator_event` now uses the :c:struct:`sensor_value` struct data buffer and carries a number of sensor values in a single sample, which is sufficient to describe data layout.

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

|no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom`:

  * Updated so that the output contains source repository and version information for each file.

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

  * Documentation template for the :ref:`Ecosystem integration <Ecosystem_integration>` user guides.
  * The :ref:`ug_nrf70_developing` user guide.
  * A page on :ref:`ug_nrf70_features`.

* Updated:

  * The :ref:`software_maturity` page with details about Wi-Fi feature support.
  * The :ref:`app_power_opt` user guide by adding a section about power saving features.

.. |no_changes_yet_note| replace:: No changes since the latest |NCS| release.
