.. _ncs_release_notes_300_preview1:

Changelog for |NCS| v3.0.0-preview1
###################################

.. contents::
   :local:
   :depth: 2

This Changelog reflects the most relevant changes from the latest official release.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and other components that implement or support protocol functionality.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.9.0-nRF54H20-1`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated:

  * The required `SEGGER J-Link`_ version to v8.10f.
  * The :ref:`installing_vsc` section on the :ref:`installation` page with the Windows-only requirement to install SEGGER USB Driver for J-Link.

Board support
=============

|no_changes_yet_note|

Build and configuration system
==============================

* Removed support for the deprecated multi-image builds (parent-child images) functionality.
  All |NCS| projects must now use :ref:`sysbuild`.
  See :ref:`child_parent_to_sysbuild_migration` for an overview of differences with parent-child image and how to migrate.

Bootloaders and DFU
===================

|no_changes_yet_note|

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

* :ref:`ug_nrf70_developing_fw_patch_ext_flash`

   * Removed support for storing the nRF70 firmware patches in external flash without the :ref:`partition_manager`.


Working with nRF54H Series
==========================

* Removed the note on installing SEGGER USB Driver for J-Link on Windows from the :ref:`ug_nrf54h20_gs` page and moved its contents to the `nRF Util prerequisites`_ documentation.
  The Windows-only requirement to install the SEGGER USB Driver for J-Link is now mentioned in the :ref:`installing_vsc` section on the :ref:`installation` page.

Developing with nRF54L Series
=============================

* Added HMAC SHA-256 with a 128-bit key type to KMU, as detailed in the :ref:`ug_nrf54l_crypto_kmu_supported_key_types` documentation section.

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

* Added support for the following:

  * :ref:`nRF21540 Front-End Module in GPIO mode <ug_radio_fem_nrf21540_gpio>` for the nRF54L Series devices.

Developing with PMICs
=====================

|no_changes_yet_note|

Security
========

  * Added:

    * Support for HKDF-Expand and HKDF-Extract in CRACEN.
    * Support for Ed25519ph(HashEdDSA) to CRACEN.

  * Updated the :ref:`app_approtect` page with nRF Util commands that replaced the nrfjprog commands.
    This is part of the ongoing work of archiving `nRF Command Line Tools`_ and replacing them with nRF Util.
  * Moved the :ref:`app_boards_spe_nspe` documentation page under :ref:`security` from the :ref:`ug_app_dev` section.

Protocols
=========

|no_changes_yet_note|

Amazon Sidewalk
---------------

|no_changes_yet_note|

Bluetooth® LE
-------------

* Fixed an issue where a flash operation executed on the system workqueue might result in ``-ETIMEDOUT``, if there is an active Bluetooth LE connection.
* Fixed an issue where Bluetooth applications built with the ``nordic-bt-rpc`` snippet (in the :ref:`ble_rpc` configuration) did not work on the nRF54H20 devices due to incorrect memory mapping.

* Updated the Bluetooth LE SoftDevice Controller driver to make the :c:func:`hci_vs_sdc_llpm_mode_set` function return an error if Low Latency Packet Mode (LLPM) is not supported or not enabled in the Bluetooth LE Controller driver configuration (:kconfig:option:`CONFIG_BT_CTLR_SDC_LLPM`).

Bluetooth Mesh
--------------

|no_changes_yet_note|

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

* Added loading of radio trims and a fix of a hardware errata for the nRF54H20 SoC to improve the RF performance.

Gazell
------

|no_changes_yet_note|

Matter
------

* Added:

  * A new documentation page :ref:`ug_matter_group_communication` in the :ref:`ug_matter_intro_overview`.
  * A new section :ref:`ug_matter_creating_accessory_vendor_cluster` in the :ref:`ug_matter_creating_accessory` page.
  * A description for the new :ref:`ug_matter_gs_tools_matter_west_commands_append` within the :ref:`ug_matter_gs_tools_matter_west_commands` page.
  * New arguments to the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool_gui` to provide a custom cache directory and add new clusters to Matter Data Model.
  * :ref:`ug_matter_debug_snippet`.

* Disabled the :ref:`mpsl` before performing factory reset to speed up the process.

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

Zigbee
------

* Fixed the :file:`zb_add_ota_header.py` script to allow a patch version higher than 9 in an ``APPLICATION_VERSION_STRING``.

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Machine learning
----------------

* Updated the application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system for the :zephyr:board:`nrf54h20dk` board.

Asset Tracker v2
----------------

* Updated the application to use the :ref:`lib_downloader` library instead of the deprecated Download client library.

Connectivity Bridge
-------------------

|no_changes_yet_note|

IPC radio firmware
------------------

* Updated the application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system in all devices that contain MRAM, such as the nRF54H Series devices.

Matter Bridge
-------------

* Enabled SUIT DFU support for the :ref:`matter_bridge_app` application.
  Currently, only the Matter OTA protocol is fully supported for SUIT DFU purposes.
* Enabled Link Time Optimization (LTO) by default for the ``release`` configuration.

nRF5340 Audio
-------------

|no_changes_yet_note|

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_failsafe` to use the Zephyr :ref:`zephyr:hwinfo_api` driver for getting and clearing the reset reason information (see the :c:func:`hwinfo_get_reset_cause` and :c:func:`hwinfo_clear_reset_cause` functions).
    The Zephyr :ref:`zephyr:hwinfo_api` driver replaces the dependency on the nrfx reset reason helper (see the :c:func:`nrfx_reset_reason_get` and :c:func:`nrfx_reset_reason_clear` functions).
  * The release configuration for the :zephyr:board:`nrf54h20dk` board target to enable the :ref:`nrf_desktop_failsafe` (see the :ref:`CONFIG_DESKTOP_FAILSAFE_ENABLE <config_desktop_app_options>` Kconfig option).
  * Enabled Link Time Optimization (:kconfig:option:`CONFIG_LTO` and :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION`) by default for an nRF Desktop application image.
    LTO was also explicitly enabled in configurations of other images built by sysbuild (bootloader, network core image).
  * Application configurations for nRF54L05, nRF54L10, and nRF54L15 SoCs to use Fast Pair PSA cryptography (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).
    Using PSA cryptography improves security and reduces memory footprint.
    Also increased the size of the Bluetooth receiving thread stack (:kconfig:option:`CONFIG_BT_RX_STACK_SIZE`) to prevent stack overflows.
  * Application configurations for nRF52810 and nRF52820 SoCs to reduce memory footprint:

    * Disabled Bluetooth long workqueue (:kconfig:option:`CONFIG_BT_LONG_WQ`).
    * Limited the number of key slots in the PSA Crypto core to 10 (:kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`).

  * Application configurations for HID peripherals by increasing the following thread stack sizes to prevent stack overflows during the :c:func:`settings_load` operation:

    * The system workqueue thread stack (:kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`).
    * The CAF settings loader thread stack (:kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_THREAD_STACK_SIZE`).

    This change results from the Bluetooth subsystem transition to the PSA cryptographic API.
    The GATT database hash calculation now requires larger stack size.

* Added:

  * System Power Management for the :zephyr:board:`nrf54h20dk` board target on the application and radio cores.
  * Application configurations for the nRF54L05 and nRF54L10 SoCs (emulated on the nRF54L15 DK).
    The configurations are supported through ``nrf54l15dk/nrf54l10/cpuapp`` and ``nrf54l15dk/nrf54l05/cpuapp`` board targets.
    For details, see the :ref:`nrf_desktop_board_configuration`.
  * The ``dongle_small`` configuration for the nRF52833 DK.
    The configuration enables logs and mimics the dongle configuration used for small SoCs.

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

* Added an overlay :file:`overlay-memfault.conf` file to enable Memfault.
  For more information about Memfault features in |NCS|, see :ref:`mod_memfault`.
* Updated the application to use the :ref:`lib_downloader` library instead of the deprecated Download client library.

Thingy:53: Matter weather station
---------------------------------

* Enabled Link Time Optimization (LTO) by default for the ``release`` configuration.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Amazon Sidewalk samples
-----------------------

|no_changes_yet_note|

Bluetooth samples
-----------------

* Added support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in the following samples:

  * :ref:`bluetooth_central_hids`
  * :ref:`peripheral_hids_keyboard`

* Updated:

    * The configurations of the non-secure ``nrf5340dk/nrf5340/cpuapp/ns`` board target in the following samples to properly use the TF-M profile instead of the predefined minimal TF-M profile:

      * :ref:`bluetooth_central_hids`
      * :ref:`peripheral_hids_keyboard`
      * :ref:`peripheral_hids_mouse`

      This change results from the Bluetooth subsystem transition to the PSA cryptographic standard.
      The Bluetooth stack can now use the PSA crypto API in the non-secure domain as all necessary TF-M partitions are configured properly.

    * The configurations of the following samples by increasing the main thread stack size (:kconfig:option:`CONFIG_MAIN_STACK_SIZE`) to prevent stack overflows:

      * :ref:`bluetooth_central_hids`
      * :ref:`peripheral_hids_keyboard`
      * :ref:`peripheral_hids_mouse`

      This change results from the Bluetooth subsystem transition to the PSA cryptographic API.

* :ref:`direct_test_mode` sample:

  * Added loading of radio trims and a fix of a hardware errata for the nRF54H20 SoC to improve the RF performance.

* :ref:`central_uart` sample:

  * Added reconnection to bonded devices based on their address.

Bluetooth Fast Pair samples
---------------------------

* Added support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in all Fast Pair samples.

* Updated:

  * The non-secure target (``nrf5340dk/nrf5340/cpuapp/ns`` and ``thingy53/nrf5340/cpuapp/ns``) configurations of all Fast Pair samples to use configurable TF-M profile instead of the predefined minimal TF-M profile:
    This change results from the Bluetooth subsystem transition to the PSA cryptographic standard.
    The Bluetooth stack can now use the PSA crypto API in the non-secure domain as all necessary TF-M partitions are configured properly.
  * The configuration of all Fast Pair samples by increasing the following thread stack sizes to prevent stack overflows:

    * The system workqueue thread stack (:kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`).
    * The Bluetooth receiving thread stack (:kconfig:option:`CONFIG_BT_RX_STACK_SIZE`).

    This change results from the Bluetooth subsystem transition to the PSA cryptographic API.

* :ref:`fast_pair_locator_tag` sample:

  * Added support for the :zephyr:board:`nrf54h20dk` board target.
  * Updated the partition layout for the ``nrf5340dk/nrf5340/cpuapp/ns`` and ``thingy53/nrf5340/cpuapp/ns`` board targets to accommodate the partitions needed due to change in the TF-M profile configuration.

Bluetooth Mesh samples
----------------------

* Added:

  * Support for nRF54L10 in the following samples:

    * :ref:`bluetooth_mesh_sensor_client`
    * :ref:`bluetooth_mesh_sensor_server`
    * :ref:`bluetooth_ble_peripheral_lbs_coex`
    * :ref:`bt_mesh_chat`
    * :ref:`bluetooth_mesh_light_switch`
    * :ref:`bluetooth_mesh_silvair_enocean`
    * :ref:`bluetooth_mesh_light_dim`
    * :ref:`bluetooth_mesh_light`
    * :ref:`ble_mesh_dfu_target`
    * :ref:`bluetooth_mesh_light_lc`
    * :ref:`ble_mesh_dfu_distributor`

  * Support for nRF54L05 in the following samples:

    * :ref:`bluetooth_mesh_sensor_client`
    * :ref:`bluetooth_mesh_sensor_server`
    * :ref:`bluetooth_ble_peripheral_lbs_coex`
    * :ref:`bt_mesh_chat`
    * :ref:`bluetooth_mesh_light_switch`
    * :ref:`bluetooth_mesh_silvair_enocean`
    * :ref:`bluetooth_mesh_light_dim`
    * :ref:`bluetooth_mesh_light`
    * :ref:`bluetooth_mesh_light_lc`

* Updated the following samples to include the value of the :kconfig:option:`CONFIG_BT_COMPANY_ID` option in the Firmware ID:

    * :ref:`ble_mesh_dfu_distributor`
    * :ref:`ble_mesh_dfu_target`

Cellular samples
----------------

* Updated the following samples to use the :ref:`lib_downloader` library instead of the Download client library:

  * :ref:`http_application_update_sample`
  * :ref:`http_modem_delta_update_sample`
  * :ref:`http_modem_full_update_sample`
  * :ref:`location_sample`
  * :ref:`lwm2m_carrier`
  * :ref:`lwm2m_client`
  * :ref:`modem_shell_application`
  * nRF Cloud multi-service
  * :ref:`nrf_cloud_rest_fota`

* :ref:`modem_shell_application` sample:

  * Removed the ``CONFIG_MOSH_LINK`` Kconfig option.
    The link control functionality is now always enabled and cannot be disabled.

* nRF Cloud multi-service sample:

  * Fixed:

    * Wrong header naming in :file:`provisioning_support.h` that was causing build errors when :file:`sample_reboot.h` was included in other source files.
    * An issue with an uninitialized variable in the :c:func:`handle_at_cmd_requests` function.
    * An issue with the too small :kconfig:option:`CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE` Kconfig value
      in the :file:`overlay-coap_nrf_provisioning.conf` file.
    * Slow Wi-Fi connectivity startup by selecting ``TFM_SFN`` instead of ``TFM_IPC``.
    * The size of TLS credentials buffer for Wi-Fi connectivity to allow installing both AWS and CoAP CA certificates.

* LTE Sensor Gateway sample:

   * Fixed an issue with devicetree configuration after HCI updates in `sdk-zephyr`_.

* :ref:`pdn_sample` sample:

  * Added dynamic PDN information.

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

* Updated the :ref:`matter_template_sample` sample document with the instructions on how to build the sample on the nRF54L15 DK with support for Matter OTA DFU and DFU over Bluetooth SMP, and using internal MRAM only.
* Enabled SUIT DFU support for the :ref:`matter_lock_sample`, and :ref:`matter_template_sample` samples.
  Currently, only the Matter OTA protocol is fully supported for SUIT DFU purposes.
* Enabled Link Time Optimization (LTO) by default for the ``release`` configuration and ``nrf7002dk/nrf5340/cpuapp`` build target.

Networking samples
------------------

* Updated:

  * The :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option value to ``1280`` for all networking samples that had it set to a lower value.
    This is a requirement from zephyr and removes a build warning.
  * The following samples to use the :ref:`lib_downloader` library instead of the Download client library:

    * :ref:`aws_iot`
    * :ref:`azure_iot_hub`
    * :ref:`download_sample`

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

* Removed the ``nRF5340: Multiprotocol RPMsg`` sample.
  Use the :ref:`ipc_radio` application instead.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added loading of radio trims and a fix of a hardware errata for the nRF54H20 SoC to improve the RF performance.

PMIC samples
------------

* :ref:`nPM1300: Fuel gauge <npm13xx_fuel_gauge>` sample:

  * Updated to accommodate API changes in nRF Fuel Gauge library v1.0.0.

|no_changes_yet_note|

Protocol serialization samples
------------------------------

|no_changes_yet_note|

SDFW samples
------------

* Removed the SDFW: Service Framework Client sample as all services demonstrated by the sample have been removed.

Sensor samples
--------------

|no_changes_yet_note|

SUIT samples
------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_psa_template` sample:

  * Added support for the following attestation token fields:

    * Profile definition
    * PSA certificate reference (optional), configured using the :kconfig:option:`SB_CONFIG_TFM_OTP_PSA_CERTIFICATE_REFERENCE` sysbuild Kconfig option
    * Verification service URL (optional), configured using the :kconfig:option:`CONFIG_TFM_ATTEST_VERIFICATION_SERVICE_URL` Kconfig option

Thread samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* :ref:`wifi_station_sample` sample:

  * Added an ``overlay-zperf.conf`` overlay for :ref:`performance benchmarking and memory footprint analysis <wifi_sta_performance_testing_memory_footprint>`.

* Radio test samples:

  * Added:

    * The :ref:`wifi_radio_test_sd` sample to demonstrate the Wi-Fi and Bluetooth LE radio test running on the application core.

  * Updated:

    * The :ref:`wifi_radio_test` sample is now moved to :zephyr_file:`samples/wifi/radio_test/multi_domain`.

* :ref:`wifi_shell_sample` sample:

  * Modified support for storing the nRF70 firmware patches in external flash using the :ref:`partition_manager`.

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Added:

    * Support for the nRF54L05 and nRF54L10 SoCs (emulated on nRF54L15 DK).
    * FLPR core support for the :zephyr:board:`nrf54l15dk` board target.

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

* :ref:`liblwm2m_carrier_readme` library:

  * Updated the glue to use the :ref:`lib_downloader` library instead of the deprecated Download client library.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Added a restriction on the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER` Kconfig option in the Find My Device Network (FMDN) extension configuration.
    This Kconfig option must now be set to 0 at minimum as the Fast Pair specification now requires that the conducted Bluetooth transmit power for FMDN advertisements must not be lower than 0 dBm.

  * Updated the automatically generated ``bt_fast_pair`` partition definition (located in the :file:`subsys/partition_manager/pm.yml.bt_fast_pair`) to work correctly when building with TF-M.

* :ref:`bt_mesh` library:

  * Fixed an issue in the :ref:`bt_mesh_light_ctrl_srv_readme` model to automatically resume the Lightness Controller after recalling a scene (``NCSDK-30033`` known issue).


* :ref:`bt_fast_pair_readme` library:

  * Updated the :c:func:`bt_fast_pair_info_cb_register` API to allow registration of multiple callbacks.

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

* :ref:`lib_fmfu_fdev`:

  * Regenerated the zcbor-generated code files using v0.9.0.

Gazell libraries
----------------

|no_changes_yet_note|

Security libraries
------------------

|no_changes_yet_note|

Modem libraries
---------------

* Deprecated the AT parameters library.

* :ref:`pdn_readme` library:

  * Deprecated the :c:func:`pdn_dynamic_params_get` function.
    Use the new function :c:func:`pdn_dynamic_info_get` instead.

* :ref:`lte_lc_readme` library:

  * Fixed handling of ``%NCELLMEAS`` notification with status 2 (measurement interrupted) and no cells.
  * Added sending of ``LTE_LC_EVT_NEIGHBOR_CELL_MEAS`` event with ``current_cell`` set to ``LTE_LC_CELL_EUTRAN_ID_INVALID`` in case an error occurs while parsing the ``%NCELLMEAS`` notification.

* :ref:`modem_key_mgmt` library:

  * Added:

    * The :c:func:`modem_key_mgmt_digest` function that would retrieve the SHA1 digest of a credential from the modem.
    * The :c:func:`modem_key_mgmt_list` function that would retrieve the security tag and type of every credential stored in the modem.

  * Fixed:

    * An issue with the :c:func:`modem_key_mgmt_clear` function where it returned ``-ENOENT`` when the credential was cleared.
    * A race condition in several functions where ``+CMEE`` error notifications could be disabled by one function before the other one got a chance to run its command.
    * An issue with the :c:func:`modem_key_mgmt_clear` function where ``+CMEE`` error notifications were not restored to their original state if the ``AT%CMNG`` AT command failed.
    * The :c:func:`modem_key_mgmt_clear` function to lock the shared scratch buffer.

* Updated the :ref:`nrf_modem_lib_lte_net_if` to automatically set the actual link :term:`Maximum Transmission Unit (MTU)` on the network interface when PDN connectivity is gained.

* :ref:`nrf_modem_lib_readme`:

  * Fixed a bug where various subsystems would be erroneously initialized during a failed initialization of the library.

* :ref:`lib_location` library:

  * Removed references to HERE location services.

* :ref:`lib_at_host` library:

  * Fixed a bug where AT responses would erroneously be written to the logging UART instead of being written to the chosen ``ncs,at-host-uart`` UART device when the :kconfig:option:`CONFIG_LOG_BACKEND_UART` Kconfig option was set.

* :ref:`modem_info_readme` library:

  * Added:

    * The :c:enum:`modem_info_data_type` type for representing LTE link information data types.
    * The :c:func:`modem_info_data_type_get` function for requesting the data type of the current modem information type.

  * Deprecated the :c:func:`modem_info_type_get` function in favor of the :c:func:`modem_info_data_type_get` function.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* Added:

  * The :ref:`lib_downloader` library.
  * A backend for the :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` that stores the credentials in the modem, see :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_NRF_MODEM`.

* Deprecated the Download client library.
  See the :ref:`migration guide <migration_3.0_recommended>` for recommended changes.

* Updated the following libraries to use the :ref:`lib_downloader` library instead of the Download client library:

  * :ref:`lib_nrf_cloud`
  * :ref:`lib_aws_fota`
  * :ref:`lib_azure_fota`
  * :ref:`lib_fota_download`

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed the warning due to missing ``https`` download protocol.

* :ref:`lib_downloader` library:

  * Updated to support Proxy-URI option and an authentication callback after connecting.

* :ref:`lib_fota_download` library:

  * Added error codes related to unsupported protocol, DFU failures, and invalid configuration.
  * Updated to use the :ref:`lib_downloader` library for CoAP downloads.

* :ref:`lib_nrf_cloud` library:

  * Added the :kconfig:option:`CONFIG_NRF_CLOUD` Kconfig option to prevent unintended inclusion of nRF Cloud Kconfig variables in non-nRF Cloud projects.
  * Updated to use the :ref:`lib_downloader` library for CoAP downloads.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Removed the following unused SDFW services: ``echo_service``, ``reset_evt_service``, and ``sdfw_update_service``.

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

* Added instructions on how to provision the Fast Pair data onto devices without the :ref:`partition_manager` support, specifically for the :zephyr:board:`nrf54h20dk`.

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

* Added a new feature to automatically post coredumps to Memfault when network connectivity is available.
  To enable this feature, set the :kconfig:option:`CONFIG_MEMFAULT_NCS_POST_COREDUMP_ON_NETWORK_CONNECTED` Kconfig option to ``y``.
  Only supported for nRF91 Series devices.

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

|no_changes_yet_note|

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a2bc982b3379d51fefda3e17a6a067342dce1a8b``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed an issue where an unusable secondary slot was cleared three times instead of once during cleanup.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``fdeb7350171279d4637c536fcceaad3fbb775392``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline fdeb735017 ^beb733919d

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^fdeb735017

The current |NCS| main branch is based on revision ``fdeb735017`` of Zephyr.

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

  * New section :ref:`ug_custom_board`.
    This section includes the following pages:

    * :ref:`defining_custom_board` - Previously located under :ref:`app_boards`.
    * :ref:`programming_custom_board` - New page.

  * New page :ref:`thingy53_precompiled` under :ref:`ug_thingy53`.
    This page includes some of the information previously located on the standalone page for getting started with Nordic Thingy:53.
  * New page :ref:`add_new_led_example` under :ref:`configuring_devicetree`.
    This page includes information previously located in the |nRFVSC| documentation.

* Updated:

  * The :ref:`create_application` page with the :ref:`creating_add_on_index` section.
  * The :ref:`ug_nrf91` documentation to use `nRF Util`_ instead of nrfjprog.
  * The :ref:`dm-revisions` section of the :ref:`dm_code_base` page with information about the preview release tag, which replaces the development tag.

* Removed the standalone page for getting started with Nordic Thingy:53.
  The contents of this page have been moved to the :ref:`thingy53_precompiled` page and to the `Programming Nordic Thingy:53 <Programming Nordic Thingy53_>`_ section in the Programmer app documentation.
