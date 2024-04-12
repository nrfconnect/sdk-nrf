.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.6.99
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
See `known issues for nRF Connect SDK v2.6.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

Working with nRF91 Series
=========================

|no_changes_yet_note|

Working with nRF70 Series
=========================

|no_changes_yet_note|

Working with nRF52 Series
=========================

|no_changes_yet_note|

Working with nRF53 Series
=========================

|no_changes_yet_note|

Working with RF front-end modules
=================================

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth Mesh
--------------

* Updated:

  * The Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` to no longer depend on the Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA`.
  * The Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_CLI` to no longer enable the Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` by default.
    The Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` can still be manually enabled.
  * The JSON file, added to :file:`dfu_application.zip` during the automatic DFU metadata generation, to now contain a field for the ``core_type`` used when encoding the metadata.

Matter
------

* Added support for merging the generated factory data HEX file with the firmware HEX file by using the devicetree configuration, when Partition Manager is not enabled in the project.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.2.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Updated:

   * The scripts for factory data generation and related :doc:`matter:nrfconnect_factory_data_configuration` documentation page.
     Now, you can use a single script to generate both JSON and HEX files that include the factory data.
     Previously, you would have to do that in two steps using two separate scripts.
     The older method is still supported for backward compatibility.

|no_changes_yet_note|

Thread
------

* Initial experimental support for nRF54L15 for Thread CLI and Co-processor samples.

Zigbee
------

* Fixed an issue with Zigbee FOTA updates failing after a previous attempt was interrupted.

Gazell
------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

* Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`zephyr:nrf54l15pdk_nrf54l15` boards.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Asset Tracker v2
----------------

* Updated:

  * The MQTT topic name for A-GNSS requests is changed to ``agnss`` for AWS and Azure backends.

Serial LTE modem
----------------

* Removed:

  * Mention of Termite and Teraterm terminal emulators from the documentation.
    The recommended approach is to use one of the emulators listed on the :ref:`test_and_optimize` page.
  * Sending GNSS UI service info to nRF Cloud; this is no longer required by the cloud.

* Updated:

  * AT command parsing to utilize the :ref:`at_cmd_custom_readme` library.

nRF5340 Audio
-------------

* Updated:

  * Removed the LE Audio controller for nRF5340 library.
    The only supported controller for LE Audio is :ref:`ug_ble_controller_softdevice`.
    This enables use of standard tools for building, configuring, and DFU.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the ``ml_runner`` application module to allow running a machine learning model without anomaly support.

nRF Desktop
-----------

* Added:

  * Support for the nRF54L15 PDK with the ``nrf54l15pdk_nrf54l15_cpuapp`` board target.
    The PDK can act as a sample mouse or keyboard.
    It supports the Bluetooth LE HID data transport and uses SoftDevice Link Layer with Low Latency Packet Mode (LLPM) enabled.
    The PDK uses MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and supports firmware updates using the :ref:`nrf_desktop_dfu`.
  * The ``nrfdesktop-wheel-qdec`` DT alias support to :ref:`nrf_desktop_wheel`.
    You must use the alias to specify the QDEC instance used for scroll wheel, if your board supports multiple QDEC instances (for example ``nrf54l15pdk_nrf54l15_cpuapp``).
    You do not need to define the alias if your board supports only one QDEC instance, because in that case, the wheel module can rely on the ``qdec`` DT label provided by the board.

* Updated:

  * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in the nRF52840 Gaming Mouse configurations with the Fast Pair support.
    The value is now aligned with the Fast Pair requirements.
  * Enabled the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT <config_desktop_app_options>` Kconfig option for the nRF Desktop peripherals with :ref:`nrf_desktop_dfu`.
    The option mitigates HID report rate drops during DFU image transfer through the nRF Desktop dongle.
    The output report is also enabled for the ``nrf52kbd_nrf52832`` build target in the debug configuration to maintain consistency with the release configuration.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Matter Bridge
-------------

* Added:

   The :kconfig:option:`CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES` kconfig option to set the maximum number of scanned Bluetooth LE devices.
   The :kconfig:option:`CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS` kconfig option to set the scan timeout.

IPC radio firmware
------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added the :ref:`bluetooth_isochronous_time_synchronization` sample showcasing time-synchronized processing of isochronous data.

* :ref:`fast_pair_input_device` sample:

    * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`peripheral_lbs` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_central_hids` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`peripheral_hids_mouse` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`peripheral_hids_keyboard` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`central_and_peripheral_hrs` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`direct_test_mode` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Bluetooth Mesh samples
----------------------

* :ref:`bluetooth_mesh_sensor_client` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_sensor_server` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
   * Updated:

     * Actions of buttons 1 and 2.
       They are swapped to align with the elements order.
     * Log messages to be more informative.

* :ref:`bluetooth_ble_peripheral_lbs_coex` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bt_mesh_chat` sample:

   * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_light_switch` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_silvair_enocean` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_light_dim` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`bluetooth_mesh_light` sample:

  * Added:

    * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
    * Support for DFU over Bluetooth Low Energy for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`ble_mesh_dfu_target` sample:

  * Added a note on how to compile the sample with new Composition Data.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added a section about the :ref:`occupancy mode <bluetooth_mesh_light_lc_occupancy_mode>`.

Cellular samples
----------------

* :ref:`ciphersuites` sample:

  * Updated the :file:`.pem` certificate for example.com.

* :ref:`location_sample` sample:

  * Removed ESP8266 Wi-Fi DTC and Kconfig overlay files.

* :ref:`modem_shell_application` sample:

  * Added support for sending location data details into nRF Cloud with ``--cloud_details`` command-line option in the ``location`` command.
  * Removed ESP8266 Wi-Fi DTC and Kconfig overlay files.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Removed:

    * The button press interface for enabling the device location card on the nRF Cloud website.
      The card is now automatically displayed.

  * Added:

    * The :kconfig:option:`CONFIG_REST_CELL_SEND_DEVICE_STATUS` Kconfig option to control sending device status on initial connection.

* :ref:`modem_shell_application` sample:

  * Removed sending GNSS UI service info to nRF Cloud; this is no longer required by the cloud.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed issue that prevented network connectivity when using Wi-Fi scanning with the nRF91xx.

Cryptography samples
--------------------

* Added :ref:`crypto_spake2p` sample.

Debug samples
-------------

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

* Removed:

  * The :file:`configuration` directory which contained the Partition Manager configuration file.
    It has been replaced replace with :file:`pm_static_<BOARD>` Partition Manager configuration files for all required target boards in the samples' directories.
  * The :file:`prj_no_dfu.conf` file.
  * Support for ``no_dfu`` build type for nRF5350 DK, nRF52840 DK and nRF7002 DK.

* Added:

  * Test event triggers to all Matter samples and enabled them by default.
    By utilizing the test event triggers, you can simulate various operational conditions and responses in your Matter device without the need for external setup.

    To get started with using test event triggers in your Matter samples and to understand the capabilities of this feature, refer to the :ref:`ug_matter_test_event_triggers` page.

  * Support for the nRF54L15 PDK with the ``nrf54l15pdk_nrf54l15_cpuapp`` build target to the following Matter samples:

    * :ref:`matter_template_sample` sample.
    * :ref:`matter_light_bulb_sample` sample.
    * :ref:`matter_light_switch_sample` sample.
    * :ref:`matter_thermostat_sample` sample.

* :ref:`matter_lock_sample` sample:

  * Added support for emulation of the nRF7001 Wi-Fi companion IC on the nRF7002 DK.

Multicore samples
-----------------

|no_changes_yet_note|

Networking samples
------------------

* Updated:

  *  The networking samples to support import of certificates in valid PEM formats.

* :ref:`http_server` sample:

  * Added ``DNS_SD_REGISTER_TCP_SERVICE`` so that mDNS services can locate and address the server using its hostname.

  * Updated:

    * Set the value of the :kconfig:option:`CONFIG_POSIX_MAX_FDS` Kconfig option to ``25`` to get the Transport Layer Security (TLS) working.
    * Set the default value of the :kconfig:option:`HTTP_SERVER_SAMPLE_CLIENTS_MAX` Kconfig option to ``1``.

NFC samples
-----------

* :ref:`record_launch_app` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`record_text` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`nfc_shell` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`nrf-nfc-system-off-sample` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`nfc_tnep_tag` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`writable_ndef_msg` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Updated:

    * The CLI command ``fem tx_power_control <tx_power_control>`` replaces ``fem tx_gain <tx_gain>`` .
      This change applies to the sample built with the :kconfig:option:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC` set to ``n``.

  * Added:

    * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
    * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

PMIC samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

* Initial experimental support for nRF54L15 for Thread CLI and Co-processor samples.

Sensor samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

* Added the :ref:`coremark_sample` sample that demonstrates how to easily measure a performance of the supported SoCs by running the Embedded Microprocessor Benchmark Consortium (EEMBC) CoreMark benchmark.
  Included support for the nRF52840 DK, nRF5340 DK, and nRF54L15 PDK.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

.. _lib_bt_ll_acs_nrf53_readme:

* Removed the LE Audio controller for nRF5340 library, which was deprecated in the :ref:`v2.6.0 release <ncs_release_notes_260>`.
  As mentioned in the :ref:`migration_2.6`, make sure to transition to Nordic Semiconductor's standard :ref:`ug_ble_controller_softdevice` (:ref:`softdevice_controller_iso`).

Bluetooth libraries and services
--------------------------------

* :ref:`bt_mesh` library:

  * Updated the :ref:`bt_mesh_light_ctrl_srv_readme` model documentation to explicitly mention the Occupany On event.

* :ref:`bt_enocean_readme` library:

  * Fixed an issue where the sensor data of a certain length was incorrectly parsed as switch commissioning.

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

  * Fixed an issue with the CFUN hooks when the Modem library is initialized during ``SYS_INIT`` at kernel level and makes calls to the :ref:`nrf_modem_at` interface before the application level initialization is done.

* :ref:`lib_location` library:

  * Added:

    * Convenience function to get :c:struct:`location_data_details` from the :c:struct:`location_event_data`.
    * Location data details for event :c:enum:`LOCATION_EVT_RESULT_UNKNOWN`.

Libraries for networking
------------------------

* :ref:`lib_wifi_credentials` library:

  * Added:

    * Function :c:func:`wifi_credentials_delete_all` to delete all stored Wi-Fi credentials.
    * Function :c:func:`wifi_credentials_is_empty` to check if the Wi-Fi credentials storage is empty.
    * New parameter ``channel`` to the structure :c:struct:`wifi_credentials_header` to store the channel information of the Wi-Fi network.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * Support for Wi-Fi anchor names in the :c:struct:`nrf_cloud_location_result` structure.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST` Kconfig option to enable including Wi-Fi anchor names in the location callback.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION_ANCHOR_LIST_BUFFER_SIZE` Kconfig option to control the buffer size used for the anchor names.

  * Updated:

    * Improved FOTA job status reporting.
    * Deprecated :kconfig:option:`NRF_CLOUD_SEND_SERVICE_INFO_UI` and its related UI Kconfig options.
    * Deprecated the :c:struct:`nrf_cloud_svc_info_ui` structure contained in the :c:struct:`nrf_cloud_svc_info` structure.
      nRF Cloud no longer uses the UI section in the shadow.

* :ref:`lib_mqtt_helper` library:

  * Changed the library to read certificates as standard PEM format. Previously the certificates had to be manually converted to string format before compiling the application.
  * Replaced the ``CONFIG_MQTT_HELPER_CERTIFICATES_FILE`` Kconfig option with :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`. The new option specifies the folder where the certificates are stored.

* :ref:`lib_nrf_provisioning` library:

  * Added the :c:func:`nrf_provisioning_set_interval` function to set the interval between provisioning attempts.

* :ref:`lib_nrf_cloud_coap` library:

  * Updated to request proprietary PSM mode for ``SOC_NRF9151_LACA`` and ``SOC_NRF9131_LACA`` in addition to ``SOC_NRF9161_LACA``.

  * Added the :c:func:`nrf_cloud_coap_shadow_desired_update` function to allow devices to reject invalid shadow deltas.

* :ref:`lib_lwm2m_client_utils` library:

  * The following initialization functions have been deprecated as these modules are now initialized automatically on boot:

    * :c:func:`lwm2m_init_location`
    * :c:func:`lwm2m_init_device`
    * :c:func:`lwm2m_init_cellular_connectivity_object`
    * :c:func:`lwm2m_init_connmon`

  * :c:func:`lwm2m_init_firmware` is deprecated in favour of :c:func:`lwm2m_init_firmware_cb` that allows application to set a callback to receive FOTA events.

Libraries for NFC
-----------------

|no_changes_yet_note|

Security libraries
------------------

|no_changes_yet_note|

Other libraries
---------------

* Added the :ref:`lib_uart_async_adapter` library.

* :ref:`app_event_manager`:

  * Added the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_REBOOT_ON_EVENT_ALLOC_FAIL` Kconfig option.
    The option allows to select between system reboot or kernel panic on event allocation failure for default event allocator.

Common Application Framework (CAF)
----------------------------------

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

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``0051731a41fa2c9057f360dc9b819e47b2484be5``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 0051731a41 ^23cf38934c

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^0051731a41

The current |NCS| main branch is based on revision ``0051731a41`` of Zephyr.

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

* Support PSA PAKE APIs from the PSA Crypto API specification 1.2.

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * The :ref:`ug_nrf54l` and :ref:`ug_nrf54l15_gs` pages.
  * List of :ref:`debugging_tools` on the :ref:`debugging` page.

  * Recommendation for the use of a :file:`VERSION` file for :ref:`ug_fw_update_image_versions_mcuboot` in the :ref:`ug_fw_update_image_versions` user guide.
  * The :ref:`ug_coremark` page.

* Updated:

  * The :ref:`cmake_options` section on the :ref:`configuring_cmake` page with the list of most common CMake options and more information about how to provide them.
  * The table listing the :ref:`boards included in sdk-zephyr <app_boards_names_zephyr>` with the nRF54L15 PDK and nRF54H20 DK boards.

  * The :ref:`ug_wifi_overview` page by separating the information about Wi-Fi certification into its own :ref:`ug_wifi_certification` page under :ref:`ug_wifi`.
  * The :ref:`ug_bt_mesh_configuring` page with an example of possible entries in the Settings NVS name cache.

* Fixed:

  * Replaced the occurrences of the outdated :makevar:`OVERLAY_CONFIG` with the currently used :makevar:`EXTRA_CONF_FILE`.
