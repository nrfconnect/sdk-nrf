.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.7.99
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
See `known issues for nRF Connect SDK v2.7.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, and tool support
=====================

|no_changes_yet_note|

Build and configuration system
==============================

* Added the ``SB_CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM`` sysbuild Kconfig option to system to allow utilizing all available RAM when using TF-M on an nRF5340 device.

  .. note::
     This has security implications and may allow secrets to be leaked to the non-secure application in RAM.

* Added the ``SB_CONFIG_MCUBOOT_NRF53_MULTI_IMAGE_UPDATE`` sysbuild Kconfig option that enables updating the network core on the nRF5340 SoC from external flash.

Bootloaders and DFU
===================

* Added documentation for :ref:`qspi_xip_split_image` functionality.
* Added a section in the sysbuild-related migration guide about the migration of :ref:`child_parent_to_sysbuild_migration_qspi_xip` from child/parent image to sysbuild.

See also the `MCUboot`_ section.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Working with nRF54H Series
==========================

|no_changes_yet_note|

Working with nRF54L Series
==========================

|no_changes_yet_note|

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

|no_changes_yet_note|

Developing with PMICs
=====================

|no_changes_yet_note|

Security
========

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

|no_changes_yet_note|

Bluetooth® LE
-------------

* The correct SoftDevice Controller library :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE_MULTIROLE` will now be selected automatically when using coexistence based on :kconfig:option:`CONFIG_MPSL_CX` for nRF52-series devices.
* Added the APIs :c:func:`bt_hci_err_to_str` and :c:func:`bt_security_err_to_str` to allow printing error codes as strings.
  Each API returns string representations of the error codes when the corresponding Kconfig option, :kconfig:option:`CONFIG_BT_HCI_ERR_TO_STR` or :kconfig:option:`CONFIG_BT_SECURITY_ERR_TO_STR`, is enabled.
  The :ref:`ble_samples` and :ref:`nrf53_audio_app` are updated to utilize these new APIs.

Bluetooth Mesh
--------------

* Updated:

 * Added metadata as optional parameter for models Light Lightness Server, Light HSL Server, Light CTL Temperature Server, Sensor Server, and Time Server.
   To use the metadata, enable the :kconfig:option:`CONFIG_BT_MESH_LARGE_COMP_DATA_SRV` Kconfig option.

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Matter
------

* Added:

  * The Kconfig options to configure parameters impacting persistent subscriptions re-establishment:

    * :kconfig:option:`CONFIG_CHIP_MAX_ACTIVE_CASE_CLIENTS`
    * :kconfig:option:`CONFIG_CHIP_MAX_ACTIVE_DEVICES`
    * :kconfig:option:`CONFIG_CHIP_SUBSCRIPTION_RESUMPTION_MIN_RETRY_INTERVAL`
    * :kconfig:option:`CONFIG_CHIP_SUBSCRIPTION_RESUMPTION_RETRY_MULTIPLIER`

  * The :ref:`ug_matter_device_memory_profiling` section to the :ref:`ug_matter_device_optimizing_memory` page.
    The section contains useful commands for measuring memory and troubleshooting tips.


Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.3.0.0`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

Zigbee
------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Machine learning
----------------

* Added:

  * Support for sampling ADXL362 sensor from PPR core on the :ref:`zephyr:nrf54h20dk_nrf54h20`.

Asset Tracker v2
----------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

* Updated the new nrfx UARTE driver implementation by setting the :kconfig:option:`CONFIG_UART_NRFX_UARTE_LEGACY_SHIM` Kconfig option to ``n``.
  This resolves an issue where data from UART0 ends up in UART1 sometimes after the device was reset.

IPC radio firmware
------------------

|no_changes_yet_note|

Matter Bridge
-------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the application.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to application's location that contains the ZAP file and :file:`zap-generated` directory.
  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20`.
  * Optional smart plug device functionality.
  * Support for the Thread protocol.

nRF5340 Audio
-------------

* Added:

  * The APIs :c:func:`bt_hci_err_to_str` and :c:func:`bt_security_err_to_str` that are used to allow printing error codes as strings.
    Each API returns string representations of the error codes when the corresponding Kconfig option, :kconfig:option:`CONFIG_BT_HCI_ERR_TO_STR` or :kconfig:option:`CONFIG_BT_SECURITY_ERR_TO_STR`, is enabled.

* Updated the :ref:`nrf53_audio_app_overview` documentation page with the :ref:`nrf53_audio_app_overview_files` section.

nRF Desktop
-----------

* Added:

  * A debug configuration enabling the `Fast Pair`_ feature on the nRF54L15 PDK with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.
  * An application versioning using the :file:`VERSION` file.
    The versioning is only applied to the application configurations that use the MCUboot bootloader.
  * The :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option to :ref:`nrf_desktop_usb_state`.
    The option allows to synchronize providing HID data with USB Start of Frame (SOF).
    The feature reduces the negative impact of jitter related to USB polls, but it also increases HID data latency.
    For details, see :ref:`nrf_desktop_usb_state_sof_synchronization`.
  * Local HID report buffering in :ref:`nrf_desktop_usb_state`.
    This ensures that the memory buffer passed to the USB next stack is valid until a HID report is sent and allows to enqueue up to two HID input reports for a USB HID instance (used only when :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option is enabled).
  * Kconfig dependency that prevents enabling USB remote wakeup (:ref:`CONFIG_DESKTOP_USB_REMOTE_WAKEUP <config_desktop_app_options>`) for the nRF54H20 SoC.
    The DWC2 USB device controller driver used by the nRF54H20 SoC does not support the remote wakeup capability.

* Updated:

  * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in configurations with the Fast Pair support.
    The value is now aligned with the Fast Pair requirements.
  * The :kconfig:option:`CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE` Kconfig option value in the nRF54L15 PDK configurations to ensure short write slots.
    It prevents timeouts in the MPSL flash synchronization caused by allocating long write slots while maintaining a Bluetooth LE connection with short intervals and no connection latency.
  * The method of obtaining hardware ID using Zephyr's :ref:`zephyr:hwinfo_api` on the :ref:`zephyr:nrf54h20dk_nrf54h20`.
    Replaced the custom implementation of the :c:func:`z_impl_hwinfo_get_device_id` function in the nRF Desktop application with the native Zephyr driver function that now supports the :ref:`zephyr:nrf54h20dk_nrf54h20` board target.
    Removed the ``CONFIG_DESKTOP_HWINFO_BLE_ADDRESS_FICR_POSTFIX`` Kconfig option as a postfix constant is no longer needed for the Zephyr native driver.
    The driver uses ``BLE.ADDR``, ``BLE.IR``, and ``BLE.ER`` fields of the Factory Information Configuration Registers (FICR) to provide 8 bytes of unique hardware ID.


nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

* Added:

  * DTLS support for the ``#XUDPSVR`` and ``#XSSOCKET`` (UDP server sockets) AT commands when the :file:`overlay-native_tls.conf` configuration file is used.
  * The :kconfig:option:`CONFIG_SLM_PPP_FALLBACK_MTU` Kconfig option that is used to control the MTU used by PPP when the cellular link MTU is not returned by the modem in response to the ``AT+CGCONTRDP=0`` AT command.
  * Handler for new nRF Cloud event type ``NRF_CLOUD_EVT_RX_DATA_DISCON``.

* Removed:

  * Support for the :file:`overlay-native_tls.conf` configuration file with the ``thingy91/nrf9160/ns`` board target.
  * Support for deprecated RAI socket options ``AT_SO_RAI_LAST``, ``AT_SO_RAI_NO_DATA``, ``AT_SO_RAI_ONE_RESP``, ``AT_SO_RAI_ONGOING``, and ``AT_SO_RAI_WAIT_MORE``.

* Updated:

  * AT string parsing to utilize the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.
  * The ``#XUDPCLI`` and ``#XSSOCKET`` (UDP client sockets) AT commands to use Zephyr's Mbed TLS with DTLS when the :file:`overlay-native_tls.conf` configuration file is used.

Thingy:53: Matter weather station
---------------------------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the application.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to application's location that contains the ZAP file and :file:`zap-generated` directory.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Amazon Sidewalk samples
-----------------------

|no_changes_yet_note|

Bluetooth samples
-----------------

* Added:

  * The :ref:`ble_radio_notification_conn_cb` sample demonstrating how to use the :ref:`ug_radio_notification_conn_cb` feature.
  * The :ref:`bluetooth_conn_time_synchronization` sample demonstrating microsecond-accurate synchronization of connections that are happening over Bluetooth® Low Energy Asynchronous Connection-oriented Logical transport (ACL).

* :ref:`bluetooth_isochronous_time_synchronization`:

  * Fixed issues related to RTC wrapping that prevented the **LED** to toggle at the correct point in time.

* :ref:`ble_event_trigger` sample:

  * Moved to the :file:`samples/bluetooth/event_trigger` folder.

Bluetooth Fast Pair samples
---------------------------

* Updated:

  * The values for the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option in all configurations, and for the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL` Kconfig option in configurations with the Find My Device Network (FMDN) extension support.
    The values are now aligned with the Fast Pair requirements.

* :ref:`fast_pair_locator_tag` sample:

  * Added:

    * LED indication on development kits for the Fast Pair advertising state.
    * An application versioning using the :file:`VERSION` file.
    * The DFU support which can be enabled using the ``SB_CONFIG_APP_DFU`` sysbuild Kconfig option.
      DFU is available for all supported targets except the ``debug`` configurations of :ref:`zephyr:nrf52dk_nrf52832` and :ref:`zephyr:nrf52833dk_nrf52833` due to size constraints.

  * Updated:

    * The :ref:`ipc_radio` image configuration by splitting it into the debug and release configurations.
    * The location of the sample configuration.
      It has been moved from the root sample directory to the dedicated folder (:file:`locator_tag/configuration`).
    * The ``fp_adv`` module to use the trigger requests for the Fast Pair advertising state instead of setting the Fast Pair advertising mode directly.

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Cellular samples
----------------

* :ref:`fmfu_smp_svr_sample` sample:

  * Removed the unused :ref:`at_cmd_parser_readme` library.

* :ref:`modem_shell_application` sample:

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added:

    * Support for setting the FOTA update check interval using the config section in the shadow.
    * A call to the :c:func:`nrf_cloud_print_details` function and removed redundant logging.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * The :kconfig:option:`CONFIG_TEST_COUNTER_MULTIPLIER` Kconfig option to multiply the number of test counter messages sent, for testing purposes.
    * A handler for new nRF Cloud event type ``NRF_CLOUD_EVT_RX_DATA_DISCON`` to stop sensors and location services.
    * A call to the :c:func:`nrf_cloud_print_details` function and removed redundant logging.

  * Updated:

    * Wi-Fi overlays from newlibc to picolib.
    * Handling of JITP association to improve speed and reliability.
    * Renamed the :file:`overlay_nrf7002ek_wifi_no_lte.conf` overlay to :file:`overlay_nrf700x_wifi_mqtt_no_lte.conf`.
    * Renamed the :file:`overlay_nrf7002ek_wifi_coap_no_lte.conf` overlay to :file:`overlay_nrf700x_wifi_coap_no_lte.conf`.

  * Fixed an issue where the accepted shadow was not marked as received because the config section did not yet exist in the shadow.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added:

    * Support for dictionary logs using REST.
    * A call to the :c:func:`nrf_cloud_print_details` function and removed redundant logging.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

    * Added a call to the :c:func:`nrf_cloud_print_details` function and removed redundant logging.

Cryptography samples
--------------------

|no_changes_yet_note|

Debug samples
-------------

* :ref:`memfault_sample` sample:

  * Increased the value of the :kconfig:option:`CONFIG_MAIN_STACK_SIZE` Kconfig option to 8192 bytes to avoid stack overflow.

|no_changes_yet_note|

DECT NR+ samples
----------------

|no_changes_yet_note|

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the sample.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to sample's location that contains the ZAP file and :file:`zap-generated` directory.
  * Support for :ref:`Trusted Firmware-M <ug_tfm>` on the nRF54L15 PDK.
  * The :ref:`matter_smoke_co_alarm_sample` sample that demonstrates implementation of Matter Smoke CO alarm device type.

* :ref:`matter_lock_sample` sample:

    * Added :ref:`Matter Lock schedule snippet <matter_lock_snippets>`, and updated the documentation to use the snippet.

Networking samples
------------------

|no_changes_yet_note|

NFC samples
-----------

|no_changes_yet_note|

nRF RPC
-------

* Added the :ref:`nrf_rpc_protocols_serialization_client` and the :ref:`nrf_rpc_protocols_serialization_server` samples.

nRF5340 samples
---------------

* :ref:`smp_svr_ext_xip` sample:

  * This sample has been converted to support sysbuild.
  * Support has been added to demonstrate direct-XIP building and building without network core support.

Peripheral samples
------------------

* :ref:`802154_sniffer` sample:

  * Increased the number of RX buffers to reduce the chances of frame drops during high traffic periods.
  * Disabled the |NCS| boot banner.
  * Added sysbuild configuration for nRF5340.
  * Fixed the dBm value reported for captured frames.

* :ref:`802154_phy_test` sample:

  * Added build configuration for the nRF54H20.

* :ref:`radio_test` sample:

  * Added packet reception limit for the ``start_rx`` command.

PMIC samples
------------

|no_changes_yet_note|

SDFW samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

SUIT samples
------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

* :ref:`zigbee_light_switch_sample` sample:

  * Added the option to configure transmission power.

Wi-Fi samples
-------------

* :ref:`wifi_radio_test` sample:

  * Added capture timeout as a parameter for packet capture.
  * Expanded the scope of ``wifi_radio_test show_config`` subcommand and rectified the behavior of ``wifi_radio_test tx_pkt_preamble`` subcommand.

* :ref:`softap_wifi_provision_sample` sample:

  * Increased the value of the :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_THREAD_STACK_SIZE` Kconfig option to 8192 bytes to avoid stack overflow.

* :ref:`wifi_shell_sample` sample:

  * Added support for running the full stack on the Thingy:91 X.
     This is a special configuration that uses the nRF5340 as the host chip instead of the nRF9151.

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Updated the logging mode to minimal (:kconfig:option:`CONFIG_LOG_MODE_MINIMAL`) to reduce the sample's memory footprint and ensure no logging interference with the running benchmark.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

|no_changes_yet_note|

Wi-Fi drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_BN` Kconfig option that enables support for the Battery Notification extension.
      You must enable this option to access Fast Pair API elements associated with the Battery Notification extension.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option allowing the user to control the support for the Fast Pair subsequent pairing feature.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE` Kconfig choice option allowing the user to select their target Fast Pair use case.
      The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_UNKNOWN`, :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_INPUT_DEVICE`, :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_LOCATOR_TAG` and :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_MOUSE` Kconfig options represent the supported use cases that can be selected as part of this Kconfig choice option.

  * Removed:

    * The MbedTLS cryptographic backend support in Fast Pair, because it is superseded by the PSA backend.
      Consequently, the :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS` Kconfig option has also been removed.
    * The default overrides for the :kconfig:option:`CONFIG_BT_DIS` and :kconfig:option:`CONFIG_BT_DIS_FW_REV` Kconfig options that enable these options together with the Google Fast Pair Service.
      This configuration is now selected only by the Fast Pair use cases that require the Device Information Service (DIS).
    * The default override for the :kconfig:option:`CONFIG_BT_DIS_FW_REV_STR` Kconfig option that was set to :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` if :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` was enabled.
      The default override is now handled in the Kconfig of the Zephyr Device Information Service (DIS) module and is based on Zephyr's :ref:`zephyr:app-version-details` that uses the :file:`VERSION` file.

  * Updated the default values of the following Fast Pair Kconfig options:

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING`
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_REQ_PAIRING`
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_PN`
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID`

    These Kconfig options are now disabled by default and are selected only by the Fast Pair use cases that require them.

* :ref:`bt_le_adv_prov_readme`:

  * Updated the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_SHOW_UI_PAIRING` Kconfig option and the :c:func:`bt_le_adv_prov_fast_pair_show_ui_pairing` function to require the enabling of the :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option.
  * Added the :c:member:`bt_le_adv_prov_adv_state.adv_handle` field to the :c:struct:`bt_le_adv_prov_adv_state` structure to store the advertising handle.
    If the :kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option is enabled, you can use the :c:func:`bt_hci_get_adv_handle` function to obtain the advertising handle for the advertising set that employs :ref:`bt_le_adv_prov_readme`.
    If the Kconfig option is disabled, the :c:member:`bt_le_adv_prov_adv_state.adv_handle` field must be set to ``0``.
    This field is currently used by the TX Power provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`).

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Gazell libraries
----------------

|no_changes_yet_note|

Modem libraries
---------------

* Added:

   * The :ref:`at_parser_readme` library.
     The :ref:`at_parser_readme` is a library that parses AT command responses, notifications, and events.
     Compared to the deprecated :ref:`at_cmd_parser_readme` library, it does not allocate memory dynamically and has a smaller footprint.
     For more information on how to transition from the :ref:`at_cmd_parser_readme` library to the :ref:`at_parser_readme` library, see the :ref:`migration guide <migration_2.8_recommended>`.

* :ref:`at_cmd_parser_readme` library:

  * Deprecated:

    * The :ref:`at_cmd_parser_readme` library in favor of the :ref:`at_parser_readme` library.
      The :ref:`at_cmd_parser_readme` library will be removed in a future version.
      For more information on how to transition from the :ref:`at_cmd_parser_readme` library to the :ref:`at_parser_readme` library, see the :ref:`migration guide <migration_2.8_recommended>`.
    * The :kconfig:option:`CONFIG_AT_CMD_PARSER`.
      This option will be removed in a future version.

  * Renamed the :c:func:`at_parser_cmd_type_get` function to :c:func:`at_parser_at_cmd_type_get` to prevent a name collision.

* :ref:`lte_lc_readme` library:

  * Removed:

    * The :c:func:`lte_lc_init` function.
      All instances of this function can be removed without any additional actions.
    * The :c:func:`lte_lc_deinit` function.
      Use the :c:func:`lte_lc_power_off` function instead.
    * The :c:func:`lte_lc_init_and_connect` function.
      Use the :c:func:`lte_lc_connect` function instead.
    * The :c:func:`lte_lc_init_and_connect_async` function.
      Use the :c:func:`lte_lc_connect_async` function instead.
    * The ``CONFIG_LTE_NETWORK_USE_FALLBACK`` Kconfig option.
      Use the :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT` or :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` Kconfig option instead.
      In addition, you can control the priority between LTE-M and NB-IoT using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.

  * Updated:

    * To use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.
    * The :c:func:`lte_lc_neighbor_cell_measurement` function to return an error for invalid GCI count.

* :ref:`lib_location` library:

  * Fixed:

    * A bug causing the GNSS obstructed visibility detection to sometimes count only part of the tracked satellites.
    * A bug causing the GNSS obstructed visibility detection to be sometimes performed twice.

  * Removed the unused :ref:`at_cmd_parser_readme` library.

* :ref:`lib_zzhc` library:

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`modem_info_readme` library:

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`nrf_modem_lib_lte_net_if` library:

  * Added a log warning suggesting a SIM card to be installed if a UICC error is detected by the modem.
  * Fixed a bug causing the cell network to be treated as offline if IPv4 is not assigned.

* :ref:`nrf_modem_lib_readme`:

  * Rename the nRF91 socket offload layer from ``nrf91_sockets`` to ``nrf9x_sockets`` to reflect that the offload layer is not exclusive to the nRF91 Series SiPs.

* :ref:`modem_info_readme` library:

  * Fixed a potential issue with scanf in the :c:func:`modem_info_get_current_band` function, which could lead to memory corruption.

* :ref:`nrf_modem_lib_readme` library:

  * Updated the RTT trace backend to allocate the RTT channel at boot, instead of when the modem is activated.
  * Removed support for deprecated RAI socket options ``SO_RAI_LAST``, ``SO_RAI_NO_DATA``, ``SO_RAI_ONE_RESP``, ``SO_RAI_ONGOING``, and ``SO_RAI_WAIT_MORE``.

Multiprotocol Service Layer libraries
-------------------------------------

* The Kconfig option ``CONFIG_MPSL_CX_THREAD`` has been renamed to :kconfig:option:`CONFIG_MPSL_CX_3WIRE` to better indicate multiprotocol compatibility.
* The Kconfig option ``CONFIG_MPSL_CX_BT_1WIRE`` has been deprecated.
* Added:

  * A 1-wire coexistence implementation which can be enabled using the Kconfig option :kconfig:option:`CONFIG_MPSL_CX_1WIRE`.

* Fixed:

  * An issue where the HFXO would be left on after uninitializing MPSL when the RC oscillator was used as the Low Frequency clock source (DRGN-22809).

Libraries for networking
------------------------

* :ref:`lib_lwm2m_client_utils` library:

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`lib_nrf_cloud_rest` library:

  * Added the function :c:func:`nrf_cloud_rest_shadow_transform_request` to request shadow data using a JSONata expression.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * The function :c:func:`nrf_cloud_client_id_runtime_set` to set the device ID string if the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option is enabled.
    * The functions :c:func:`nrf_cloud_sec_tag_set` and :c:func:`nrf_cloud_sec_tag_get` to set and get the sec tag used for nRF Cloud credentials.
    * A new nRF Cloud event type ``NRF_CLOUD_EVT_RX_DATA_DISCON`` which is generated when a device is deleted from nRF Cloud.
    * The function :c:func:`nrf_cloud_print_details` to log common nRF Cloud connection information in a uniform way.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_VERBOSE_DETAILS` Kconfig option to enable the :c:func:`nrf_cloud_print_details` function to print all details instead of only the device ID.

  * Updated:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option to be available with CoAP and REST.
    * The JSON string representing longitude in ``PVT`` reports from ``lng`` to ``lon`` to align with nRF Cloud.
      nRF Cloud still accepts ``lng`` for backward compatibility.
    * The handling of MQTT JITP device association to improve speed and reliability.
    * To use nRF Cloud's custom MQTT topics instead of the default AWS topics.

  * Fixed an issue in the :c:func:`nrf_cloud_send` function that prevented data in the provided :c:struct:`nrf_cloud_obj` structure from being sent to the bulk and bin topics.

* :ref:`lib_nrf_cloud_coap` library:

  * Fixed:

    * A hard fault that occurred when encoding AGNSS request data and the ``net_info`` field of the :c:struct:`nrf_cloud_rest_agnss_request` structure is NULL.
    * An issue where certain CoAP functions could return zero, indidicating success, even though there was an error.

  * Updated:

    * To use a shorter resource string for the ``d2c/bulk`` resource.
    * The function :c:func:`nrf_cloud_coap_shadow_get` to return ``-E2BIG`` if the received shadow data was truncated because the provided buffer was not big enough.

* :ref:`lib_lwm2m_client_utils` library:

  * Fixed an issue where a failed delta update for the modem would not clear the state and blocks future delta updates.
    This only occurred when an LwM2M Firmware object was used in push mode.

* :ref:`lib_nrf_cloud_log` library:

  * Added support for dictionary logs using REST.
  * Added support for dictionary (binary) logs when connected to nRF Cloud using CoAP.

* :ref:`lib_nrf_cloud_fota` library:

* Updated:

  * The :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE` Kconfig option is made available and used also when the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_POLL` Kconfig option is enabled.
    The range of the option is now from 128 to 1900 bytes, and the default value is 1700 bytes.
  * The function :c:func:`nrf_cloud_fota_poll_process` can now be used asynchrounously if a callback to handle errors is provided.

Libraries for NFC
-----------------

* Added an experimental serialization of NFC tag 2 and tag 4 APIs.
* Fixed a potential issue with handling data pointers in the function ``ring_buf_get_data`` in the :file:`platform_internal_thread` file.

nRF RPC libraries
-----------------

* Updated the internal Bluetooth serialization API and Bluetooth callback proxy API to become part of the public NRF RPC API.
* Added:

  * An experimental serialization of Openthread APIs.
  * The logging backend that sends logs through nRF RPC events.

Other libraries
---------------

* Added a compression/decompression library with support for the LZMA decompression.
* :ref:`lib_date_time` library:

  * Fixed a bug that caused date-time updates to not be rescheduled under certain circumstances.

  * Added:

    * A retry feature that reattempts failed date-time updates up to a certain number of consecutive times.
    * The Kconfig options :kconfig:option:`CONFIG_DATE_TIME_RETRY_COUNT` to control whether and how many consecutive date-time update retries may be performed, and :kconfig:option:`CONFIG_DATE_TIME_RETRY_INTERVAL_SECONDS` to control how quickly date-time update retries occur.

Security libraries
------------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

Libraries for Zigbee
--------------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

|no_changes_yet_note|

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

|no_changes_yet_note|

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

|no_changes_yet_note|

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integation
--------------------

|no_changes_yet_note|

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``ea02b93eea35afef32ebb31f49f8e79932e7deee``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline ea02b93eea ^23cf38934c

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^ea02b93eea

The current |NCS| main branch is based on revision ``ea02b93eea`` of Zephyr.

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

* Added possibility to read UICR.OTP registers through platform services.

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * The :ref:`ug_app_dev` section, which includes pages from the :ref:`configuration_and_build` section and from the removed Device configuration guides section.
  * The :ref:`peripheral_sensor_node_shield` page.

* Restructured the :ref:`app_bootloaders` documentation and combined the DFU and bootloader articles.
  Additionally, created a new bootloader :ref:`bootloader_quick_start`.
* Separated the instructions about building from :ref:`configure_application` and moved it to a standalone :ref:`building` page.

* Removed:

  * Removed the Device configuration guides section and moved its contents to :ref:`ug_app_dev`.
  * The Advanced building procedures page and moved its contents to the :ref:`building` page.

* Updated:

  * The :ref:`ug_nrf70_developing_debugging` page with the new snippets added for the nRF70 driver debug and WPA supplicant debug logs.
