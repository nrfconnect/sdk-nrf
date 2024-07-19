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

IDE and tool support
====================

|no_changes_yet_note|

Build and configuration system
==============================

* Added the ``SB_CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM`` sysbuild Kconfig option to system to allow utilizing all available RAM when using TF-M on an nRF5340 device.

  .. note::
     This has security implications and may allow secrets to be leaked to the non-secure application in RAM.

Working with nRF91 Series
=========================

|no_changes_yet_note|

Working with nRF70 Series
=========================

|no_changes_yet_note|

Working with nRF54H Series
==========================

|no_changes_yet_note|

Working with nRF54L Series
==========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with RF front-end modules
=================================

|no_changes_yet_note|

Working with PMIC
=================

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

Asset Tracker v2
----------------

|no_changes_yet_note|

Connectivity Bridge
-------------------

|no_changes_yet_note|

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

nRF5340 Audio
-------------

* Added:

  * The APIs :c:func:`bt_hci_err_to_str` and :c:func:`bt_security_err_to_str` that are used to allow printing error codes as strings.
    Each API returns string representations of the error codes when the corresponding Kconfig option, :kconfig:option:`CONFIG_BT_HCI_ERR_TO_STR` or :kconfig:option:`CONFIG_BT_SECURITY_ERR_TO_STR`, is enabled.

nRF Desktop
-----------

* Added a debug configuration enabling the `Fast Pair`_ feature on the nRF54L15 PDK with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.

* Updated:

  * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in configurations with the Fast Pair support.
    The value is now aligned with the Fast Pair requirements.
  * The :kconfig:option:`CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE` Kconfig option value in the nRF54L15 PDK configurations to ensure short write slots.
    It prevents timeouts in the MPSL flash synchronization caused by allocating long write slots while maintaining a Bluetooth LE connection with short intervals and no connection latency.


nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

* Added:

  * DTLS support for the ``#XUDPSVR`` and ``#XSSOCKET`` (UDP server sockets) AT commands when the :file:`overlay-native_tls.conf` configuration file is used.

* Removed:

  * Support for the :file:`overlay-native_tls.conf` configuration file with the ``thingy91/nrf9160/ns`` board target.

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

  * Added support for setting the FOTA update check interval using the config section in the shadow.

* :ref:`nrf_cloud_multi_service` sample:

  * Updated Wi-Fi overlays from newlibc to picolib.
  * Added the :kconfig:option:`CONFIG_TEST_COUNTER_MULTIPLIER` Kconfig option to multiply the number of test counter messages sent, for testing purposes.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added support for dictionary logs using REST.

Cryptography samples
--------------------

|no_changes_yet_note|

Debug samples
-------------

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

* Added the Protocols serialization client and server samples.

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`802154_sniffer` sample:

  * Increased the number of RX buffers to reduce the chances of frame drops during high traffic periods.
  * Disabled the |NCS| boot banner.
  * Added sysbuild configuration for nRF5340.
  * Fixed the dBm value reported for captured frames.

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

    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option allowing the user to control the support for the Fast Pair subsequent pairing feature.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE` Kconfig choice option allowing the user to select their target Fast Pair use case.
      The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_UNKNOWN`, :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_INPUT_DEVICE`, :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_LOCATOR_TAG` and :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_MOUSE` Kconfig options represent the supported use cases that can be selected as part of this Kconfig choice option.

  * Removed the MbedTLS cryptographic backend support in Fast Pair, because it is superseded by the PSA backend.
    Consequently, the :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS` Kconfig option has also been removed.

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

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`lib_location` library:

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

Multiprotocol Service Layer libraries
-------------------------------------

* The Kconfig option ``CONFIG_MPSL_CX_THREAD`` has been renamed to :kconfig:option:`CONFIG_MPSL_CX_3WIRE` to better indicate multiprotocol compatibility.

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

  * Updated:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option to be available with CoAP and REST.
    * The JSON string representing longitude in ``PVT`` reports from ``lng`` to ``lon`` to align with nRF Cloud.
      nRF Cloud still accepts ``lng`` for backward compatibility.

* :ref:`lib_nrf_cloud_coap` library:

  * Fixed a hard fault that occurred when encoding AGNSS request data and the ``net_info`` field of the :c:struct:`nrf_cloud_rest_agnss_request` structure is NULL.
  * Updated to use a shorter resource string for the ``d2c/bulk`` resource.

* :ref:`lib_lwm2m_client_utils` library:

  * Fixed an issue where a failed delta update for the modem would not clear the state and blocks future delta updates.
    This only occurred when an LwM2M Firmware object was used in push mode.

* :ref:`lib_nrf_cloud_log` library:

  * Added support for dictionary logs using REST.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

* Updated the internal Bluetooth serialization API and Bluetooth callback proxy API to become part of the public NRF RPC API.
* Added:

  * An experimental serialization of Openthread APIs.
  * The logging backend that sends logs through nRF RPC events.

Other libraries
---------------

|no_changes_yet_note|

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

|no_changes_yet_note|

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
