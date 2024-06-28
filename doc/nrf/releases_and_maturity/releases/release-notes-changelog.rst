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

Build and configuration system
==============================

* Added:

  * Documentation section about the :ref:`file suffix feature from Zephyr <app_build_file_suffixes>` with a related information in the :ref:`migration guide <migration_2.7_recommended>`.
  * Documentation section about :ref:`app_build_snippets`.

* Updated:

  * All board targets for Zephyr's :ref:`Hardware Model v2 <zephyr:hw_model_v2>`, with additional information added on the :ref:`app_boards_names` page.
  * The use of :ref:`cmake_options` to specify the image when building with :ref:`configuration_system_overview_sysbuild`.
    If not specified, the option will be added to all images.

Working with nRF91 Series
=========================

* Moved :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` to the :ref:`gsg_guides` section.

Working with nRF70 Series
=========================

* Moved :ref:`ug_nrf7002_gs` to the :ref:`gsg_guides` section.

Working with nRF54L Series
==========================

* Added the :ref:`ug_nrf54l15_gs` page.
* Changed the default value for the Kconfig option :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` from 500 to 250 if :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is used.

Working with nRF53 Series
=========================

* Moved :ref:`ug_nrf5340_gs` to the :ref:`gsg_guides` section.

Working with nRF52 Series
=========================

* Moved :ref:`ug_nrf52_gs` to the :ref:`gsg_guides` section.

Working with nRF53 Series
=========================

* Added the :ref:`features_nrf53` page.
* Changed the default value for the Kconfig option :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` from 500 to 250 if :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is used.
* Replaced nrfjprog commands in :ref:`ug_nrf5340` with commands from `nRF Util`_.

Working with RF front-end modules
=================================

|no_changes_yet_note|

Security
========

* Added information about the default Kconfig option setting for :ref:`enabling access port protection mechanism in the nRF Connect SDK<app_approtect_ncs>`.

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

* Added:

  * Support for merging the generated factory data HEX file with the firmware HEX file by using the devicetree configuration, when Partition Manager is not enabled in the project.
  * Support for the unified Persistent Storage API, including the implementation of the PSA Persistent Storage.
  * Watchdog timer implementation for creating multiple :ref:`ug_matter_device_watchdog` sources and monitoring the time of executing specific parts of the code.
  * Clearing SRP host services on factory reset.
    This resolves the known issue related to the :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_ERASE_AND_PAIRING_START` (KRKNWK-18916).
  * Diagnostic logs provider that collects the diagnostic logs and sends them to the Matter controller.
    To learn more about the diagnostic logs module, see :ref:`ug_matter_configuration_diagnostic_logs`.
  * :ref:`ug_matter_diagnostic_logs_snippet` to add support for all features of the diagnostic log provider.
  * :ref:`ug_matter_gs_tools_matter_west_commands` to simplify the process of editing the ZAP files and generated the C++ Matter data model files.

* Updated:

  * Default MRP retry intervals for Thread devices to two seconds to reduce the number of spurious retransmissions in Thread networks.
  * The `ug_matter_gs_adding_cluster` user guide with the new :ref:`ug_matter_gs_tools_matter_west_commands` section.

* Increased the number of available packet buffers in the Matter stack to avoid packet allocation issues.
* Removed the :file:`Kconfig.mcuboot.defaults`, :file:`Kconfig.hci_ipc.defaults` and :file:`Kconfig.multiprotocol_rpmsg.defaults` Kconfig files that stored a default configuration for the child images.
  This was done because of the :ref:`configuration_system_overview_sysbuild` integration and the child images deprecation.
  The configurations are now applied using the configuration files located in the sample's or application's directory.

  To see how to migrate an application from the previous to the current approach, see the :ref:`migration guide <migration_2.7>`.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.3.0.0`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

   * Support for the Scenes cluster.
     This enables users to control the state for devices, rooms, or their whole home, by performing multiple actions on the devices that can be triggered with a single command.
   * Support for command batching.
     This allows a controller to batch multiple commands into a single message, which minimizes the delay between execution of the subsequent commands.
   * Extended beaconing feature that allows an accessory device to advertise Matter service over Bluetooth LE for a period longer than maximum time of 15 minutes.
     This can be enabled using the :kconfig:option:`CONFIG_CHIP_BLE_EXT_ADVERTISING` Kconfig option.
   * The Kconfig options :kconfig:option:`CONFIG_CHIP_ICD_LIT_SUPPORT`, :kconfig:option:`CONFIG_CHIP_ICD_CHECK_IN_SUPPORT`, and :kconfig:option:`CONFIG_CHIP_ICD_UAT_SUPPORT` to manage ICD configuration.
   * New device types:

     * Device energy management
     * Microwave oven
     * Oven
     * Cooktop
     * Cook surface
     * Extractor hood
     * Laundry dryer
     * Electric vehicle supply equipment
     * Water valve
     * Water freeze detector
     * Water leak detector
     * Rain sensor

* Updated:

   * Network commissioning to provide more information related to the used networking technologies.
     For Wi-Fi devices, they can now report which Wi-Fi band they support and they have to perform Wi-Fi directed scanning.
     For Thread devices, the Network Commissioning cluster now includes Thread version and supported Thread features attributes.

|no_changes_yet_note|

Thread
------

* Initial experimental support for nRF54L15 to the :ref:`ot_cli_sample` and :ref:`ot_coprocessor_sample` samples.
* Added new :ref:`feature set <thread_ug_feature_sets>` option :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_RCP`.

Zigbee
------

* Updated:

  * :ref:`nrfxlib:zboss` to v3.11.4.0 and platform v5.1.5 (``v3.11.4.0+5.1.5``).
    They contain fixes for infinite boot loop due to ZBOSS NVRAM corruption and other bugs.
    For details, see :ref:`zboss_changelog`.
  * :ref:`ZBOSS Network Co-processor Host <ug_zigbee_tools_ncp_host>` package to the new version v2.2.3.

* Fixed an issue with Zigbee FOTA updates failing after a previous attempt was interrupted.
* Fixed the RSSI level value reported to the MAC layer in the Zigbee stack.

Gazell
------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

* Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.
* Added fast switching between radio states for the nRF54H20 SoC.
* Added fast radio channel switching for the nRF54H20 SoC.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

* Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board using :ref:`Shield <ug_nrf7002eb_nrf54h20dk_gs>`.
* Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board using :ref:`Shield <ug_nrf7002eb_nrf54l15pdk_gs>`.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Applications that used :ref:`zephyr:bluetooth-hci-ipc-sample`, :ref:`zephyr:nrf-ieee802154-rpmsg-sample`, or :ref:`multiprotocol-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.

Asset Tracker v2
----------------

* Updated:

  * The MQTT topic name for A-GNSS requests is changed to ``agnss`` for AWS and Azure backends.
  * GNSS heading is only sent to the cloud when it is considered accurate enough.

Serial LTE modem
----------------

* Added:

  * New behavior for when a connection is closed unexpectedly while SLM is in data mode.
    SLM now sends the :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` string when this happens.
  * Sending of GNSS data to carrier library when the library is enabled.

* Removed:

  * Mention of Termite and Teraterm terminal emulators from the documentation.
    The recommended approach is to use one of the emulators listed on the :ref:`test_and_optimize` page.
  * Sending GNSS UI service info to nRF Cloud; this is no longer required by the cloud.

* Updated:

  * AT command parsing to utilize the :ref:`at_cmd_custom_readme` library.
  * The format of the ``#XCARRIEREVT: 12`` unsolicited notification.

Connectivity Bridge
-------------------

* Updated to make the Bluetooth LE feature work for Thingy:91 X by using the load switch.

nRF5340 Audio
-------------

* Added:

  * CAP initiator for the :ref:`unicast client <nrf53_audio_unicast_client_app>`, including Coordinated Set Identification Profile (CSIP).
  * Support for any context type, not just media.
  * Rejection of connection if :ref:`unicast client <nrf53_audio_unicast_client_app>` or :ref:`broadcast source <nrf53_audio_broadcast_source_app>` (or both) tries to use an unsupported sample rate.
  * Debug prints of discovered endpoints.
  * Support for multiple :ref:`unicast servers <nrf53_audio_unicast_server_app>` in :ref:`unicast client <nrf53_audio_unicast_client_app>`, regardless of location.

* Removed:

  * The LE Audio controller for nRF5340 library.
    The only supported controller for LE Audio is :ref:`ug_ble_controller_softdevice`.
    This enables use of standard tools for building, configuring, and DFU.

* Updated:

  * Low latency configuration to be used as default setting for the nRF5340 Audio application.
  * ACL interval for service discovery to reduce setup time.
  * Default settings to be lower latency end-to-end.
  * API for creating a :ref:`broadcast source <nrf53_audio_broadcast_source_app>`, to be more flexible.
  * Migrated build system to support :ref:`configuration_system_overview_sysbuild`.
    This means that the old Kconfig used to enable FOTA updates no longer exists, and the :ref:`file suffix <app_build_file_suffixes>` ``fota`` must be used instead.

* Fixed:

  * Missing data in the advertising packet after the directed advertising has timed out.
  * Connection procedure so that a central does not try to connect to the same device twice.
  * PAC record creation in :ref:`unicast server <nrf53_audio_unicast_server_app>` so that it will not expose source records if only the sink direction is supported.
  * Presentation delay calculation so that it is railed between min and max of the :ref:`unicast server <nrf53_audio_unicast_server_app>`.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated:

  The ``ml_runner`` application module to allow running a machine learning model without anomaly support.
  The :ref:`application documentation <nrf_machine_learning_app>` by splitting it into several pages.

* Added:

  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` boards.
  * Support for :ref:`configuration_system_overview_sysbuild`.

nRF Desktop
-----------

* Added:

  * Support for the nRF54L15 PDK with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.
    The PDK can act as a sample mouse or keyboard.
    It supports the Bluetooth LE HID data transport and uses SoftDevice Link Layer with Low Latency Packet Mode (LLPM) enabled.
    The PDK uses MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and supports firmware updates using the :ref:`nrf_desktop_dfu`.
  * The ``nrfdesktop-wheel-qdec`` DT alias support to :ref:`nrf_desktop_wheel`.
    You must use the alias to specify the QDEC instance used for scroll wheel, if your board supports multiple QDEC instances (for example ``nrf54l15pdk/nrf54l15/cpuapp``).
    You do not need to define the alias if your board supports only one QDEC instance, because in that case, the wheel module can rely on the ``qdec`` DT label provided by the board.
  * A warning log for handling ``-EACCES`` error code returned by functions that send GATT notification with HID report in :ref:`nrf_desktop_hids`.
    The error code might be returned if an HID report is sent right after a remote peer unsubscribes.
    The warning log prevents displaying an error log in a use case that does not indicate an error.
  * Experimental support for the USB next stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`) to :ref:`nrf_desktop_usb_state`.

* Updated:

  * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in the nRF52840 Gaming Mouse configurations with the Fast Pair support.
    The value is now aligned with the Fast Pair requirements.
  * Enabled the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT <config_desktop_app_options>` Kconfig option for the nRF Desktop peripherals with :ref:`nrf_desktop_dfu`.
    The option mitigates HID report rate drops during DFU image transfer through the nRF Desktop dongle.
    The output report is also enabled for the ``nrf52kbd/nrf52832`` board target in the debug configuration to maintain consistency with the release configuration.
  * Replaced the :kconfig:option:`CONFIG_BT_L2CAP_TX_BUF_COUNT` Kconfig option with :kconfig:option:`CONFIG_BT_ATT_TX_COUNT` in nRF Desktop dongle configurations.
    This update is needed to align with the new approach introduced in ``sdk-zephyr`` by commit ``a05a47573a11ba8a78dadc5d3229659f24ddd32f``.
  * The :ref:`nrf_desktop_hid_forward` no longer uses a separate HID report queue for a HID peripheral connected over Bluetooth LE.
    The module relies only on the HID report queue of a HID subscriber.
    This is done to simplify implementation, reduce memory consumption and speed up retrieving enqueued HID reports.
    You can modify the enqueued HID report limit through the :ref:`CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS <config_desktop_app_options>` Kconfig option.
  * ``Error while sending report`` log level in :ref:`nrf_desktop_hid_state` from error to warning.
    The log might appear, for example, during the disconnection of a HID transport.
    In that case, it does not denote an actual error.
  * Updated the number of ATT buffers (:kconfig:option:`CONFIG_BT_ATT_TX_COUNT`) for nRF Desktop peripherals.
    This adjustment allows peripherals to simultaneously send all supported HID notifications (including HID report pipeline support), the BAS notification, and an ATT response.
    ATT uses a dedicated net buffer pool.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Matter Bridge
-------------

* Added:

   The :ref:`CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES <CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES>` Kconfig option to set the maximum number of scanned Bluetooth LE devices.
   The :ref:`CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS <CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS>` Kconfig option to set the scan timeout.

* Updated the implementation of the persistent storage to leverage ``NonSecure``-prefixed methods from the common Persistent Storage module.
* Changed data structure of information stored in the persistent storage to use less settings keys.
  The new structure uses approximately 40% of the memory used by the old structure, and provides a new field to store user-specific data.

  Backward compatibility is kept by using an internal dedicated method that automatically detects the older data format and performs data migration to the new representation.

IPC radio firmware
------------------

* Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

Bluetooth samples that used the :ref:`zephyr:bluetooth-hci-ipc-sample` radio core firmware now use the :ref:`ipc_radio`.

* Added the :ref:`bluetooth_iso_combined_bis_cis` sample showcasing forwarding isochronous data from CIS to BIS.
* Added the :ref:`bluetooth_isochronous_time_synchronization` sample showcasing time-synchronized processing of isochronous data.

* :ref:`fast_pair_input_device` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`peripheral_lbs` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`bluetooth_central_hids` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`peripheral_hids_mouse` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`peripheral_hids_keyboard` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`central_and_peripheral_hrs` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`direct_test_mode` sample:

  * Added:

    * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.
    * Support for :ref:`configuration_system_overview_sysbuild`.

* :ref:`peripheral_uart` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`central_uart` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` boards.

* :ref:`central_bas` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`bluetooth_central_hr_coded` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`multiple_adv_sets` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_ams_client` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_ancs_client` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_bms` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_cgms` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_cts_client` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_gatt_dm` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_hr_coded` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_nfc_pairing` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_rscs` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`peripheral_status` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`shell_bt_nus` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Bluetooth Mesh samples
----------------------

Bluetooth Mesh samples that used the :ref:`zephyr:bluetooth-hci-ipc-sample` radio core firmware now use the :ref:`ipc_radio`.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added:

    * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
    * Motion sensing, time since motion sensed and people count occupancy sensor types.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Added:

    * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
    * Motion sensing, time since motion sensed and people count occupancy sensor types.

  * Updated:

    * Actions of buttons 1 and 2.
      They are swapped to align with the elements order.
    * Log messages to be more informative.

* :ref:`bluetooth_ble_peripheral_lbs_coex` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`bt_mesh_chat` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`bluetooth_mesh_light_switch` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`bluetooth_mesh_silvair_enocean` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`bluetooth_mesh_light_dim` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`bluetooth_mesh_light` sample:

  * Added:

    * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
    * Support for DFU over Bluetooth Low Energy for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
    * Support for DFU over Bluetooth Low Energy for the :ref:`nRF5340 DK <ug_nrf5340>` board.

* :ref:`ble_mesh_dfu_target` sample:

  * Added

    * A note on how to compile the sample with new Composition Data.
    * Point-to-point DFU support with overlay file :file:`overlay-ptp_dfu.conf`.
    * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added

    * A section about the :ref:`occupancy mode <bluetooth_mesh_light_lc_occupancy_mode>`.
    * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`ble_mesh_dfu_distributor` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

Cellular samples
----------------

* :ref:`ciphersuites` sample:

  * Updated the :file:`.pem` certificate for example.com.

* :ref:`location_sample` sample:

  * Removed ESP8266 Wi-Fi DTC and Kconfig overlay files.

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for sending location data details into nRF Cloud with ``--cloud_details`` command-line option in the ``location`` command.
    * Support for Thingy:91 X Wi-Fi scanning.

  * Removed ESP8266 Wi-Fi DTC and Kconfig overlay files.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Removed:

    * The button press interface for enabling the device location card on the nRF Cloud website.
      The card is now automatically displayed.

  * Added:

    * The :ref:`CONFIG_REST_CELL_SEND_DEVICE_STATUS <CONFIG_REST_CELL_SEND_DEVICE_STATUS>` Kconfig option to control sending device status on initial connection.

* :ref:`modem_shell_application` sample:

  * Removed sending GNSS UI service info to nRF Cloud; this is no longer required by the cloud.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed:

    * An issue that prevented network connectivity when using Wi-Fi scanning with the nRF91xx.
    * An issue that caused the CoAP shadow polling thread to run too often if no data received.

  * Added:

    * The ability to control the state of the test counter using the config section in the device shadow.
    * Handling of L4 disconnect where CoAP connection is paused and socket is kept open, then resumed when L4 reconnects.
    * Checking in CoAP FOTA and shadow polling threads to improve recovery from communications failures.
    * Sysbuild overlays for Wi-Fi and external-flash builds.

* :ref:`udp` sample:

  * Updated the sample to use the :c:macro:`SO_RAI` socket option with values :c:macro:`RAI_LAST` and :c:macro:`RAI_ONGOING` instead of the deprecated socket options :c:macro:`SO_RAI_LAST` and :c:macro:`SO_RAI_ONGOING`.

Cryptography samples
--------------------

* Added:

    * :ref:`crypto_spake2p` sample.
    * Support for the :ref:`zephyr:nrf9151dk_nrf9151` board for all crypto samples.
    * Support for the :ref:`nRF9161 DK <ug_nrf9161>` board for the :ref:`crypto_test` sample.

Common samples
--------------

* Added a description about :file:`samples/common` and their purpose in the :ref:`samples` and :ref:`building_advanced` pages (:ref:`common_sample_components`).

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

* Added the :ref:`nrf_modem_dect_phy_hello` sample.

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

* Added support for the :ref:`zephyr:nrf9151dk_nrf9151` and the :ref:`nRF9161 DK <ug_nrf9161>` boards for all keys samples.

Matter samples
--------------

Matter samples that used :ref:`zephyr:nrf-ieee802154-rpmsg-sample` or :ref:`multiprotocol-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.

* Removed:

  * The :file:`configuration` directory which contained the Partition Manager configuration file.
    It has been replaced with :file:`pm_static_<BOARD>` Partition Manager configuration files for all required target boards in the samples' directories.
  * The :file:`prj_no_dfu.conf` file.
  * Support for the ``no_dfu`` build type for the nRF5350 DK, the nRF52840 DK, and the nRF7002 DK.

* Added:

  * Test event triggers to all Matter samples and enabled them by default.
    By utilizing the test event triggers, you can simulate various operational conditions and responses in your Matter device without the need for external setup.

    To get started with using test event triggers in your Matter samples and to understand the capabilities of this feature, refer to the :ref:`ug_matter_test_event_triggers` page.

  * Support for the nRF54L15 PDK with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target to the following Matter samples:

    * :ref:`matter_template_sample` sample.
    * :ref:`matter_light_bulb_sample` sample.
    * :ref:`matter_light_switch_sample` sample.
    * :ref:`matter_thermostat_sample` sample.
    * :ref:`matter_window_covering_sample` sample.

    DFU support for the nRF54L15 PDK is available only for the ``release`` build type.

  * Support for Matter over Thread on the nRF54H20 DK with the ``nrf54h20dk/nrf54h20/cpuapp`` board target to the following Matter samples:

    * :ref:`matter_lock_sample` sample.
    * :ref:`matter_template_sample` sample.

    DFU, factory data, and PSA Crypto API are not currently supported for the nRF54H20 DK.

* Enabled the Bluetooth® LE Extended Announcement feature for all samples, and increased advertising timeout from 15 minutes to 1 hour.

* :ref:`matter_lock_sample` sample:

  * Added support for emulation of the nRF7001 Wi-Fi companion IC on the nRF7002 DK.
  * Added a door lock access manager module.
    The module is used to implement support for refined handling and persistent storage of PIN codes.
  * Added the ::ref::`matter_lock_scheduled_timed_access` feature.

Multicore samples
-----------------

* Removed the "Multicore Hello World application" sample in favor of :zephyr:code-sample:`sysbuild_hello_world`, which has equivalent functionality.
  This also removes the Multicore samples category from the :ref:`samples` page.

Networking samples
------------------

* Updated the networking samples to support import of certificates in valid PEM formats.
* Removed QEMU x86 emulation support and added support for the :ref:`native simulator <zephyr:native_sim>` board.

* :ref:`http_server` sample:

  * Added:

    * ``DNS_SD_REGISTER_TCP_SERVICE`` so that mDNS services can locate and address the server using its hostname.

  * Updated:

    * Set the value of the :kconfig:option:`CONFIG_POSIX_MAX_FDS` Kconfig option to ``25`` to get the Transport Layer Security (TLS) working.
    * Set the default value of the :ref:`CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX <CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX>` Kconfig option to ``1``.

NFC samples
-----------

* :ref:`record_launch_app` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`record_text` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`nfc_shell` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`nrf-nfc-system-off-sample` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`nfc_tnep_tag` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`writable_ndef_msg` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Updated:

    * The CLI command ``fem tx_power_control <tx_power_control>`` replaces ``fem tx_gain <tx_gain>`` .
      This change applies to the sample built with the :ref:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC <CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC>` set to ``n``.

  * Added:

    * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.
    * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

* :ref:`802154_phy_test` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

* :ref:`802154_sniffer` sample:

  * The sample no longer exposes two USB CDC ACM endpoints on the nRF52840 Dongle.

PMIC samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

* Added support for the :ref:`zephyr:nrf9151dk_nrf9151` and the :ref:`nRF9161 DK <ug_nrf9161>` boards for all TF-M samples (except for the :ref:`provisioning_image_net_core` sample).

Thread samples
--------------

Thread samples that used :ref:`zephyr:nrf-ieee802154-rpmsg-sample` or :ref:`multiprotocol-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.

* Initial experimental support for nRF54L15 to the :ref:`ot_cli_sample` and :ref:`ot_coprocessor_sample` samples.
* :ref:`ot_coprocessor_sample` sample:

  * Changed the default :ref:`feature set <thread_ug_feature_sets>` from Master to RCP.

Sensor samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

Zigbee samples that used :ref:`zephyr:nrf-ieee802154-rpmsg-sample` or :ref:`multiprotocol-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.

Wi-Fi samples
-------------

Wi-Fi samples that used :ref:`zephyr:bluetooth-hci-ipc-sample` or :ref:`zephyr:nrf-ieee802154-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.

* Added the :ref:`softap_wifi_provision_sample` sample.
* Added the :ref:`wifi_thread_coex_sample` sample that demonstrates Wi-Fi and Thread coexistence.

* :ref:`wifi_shell_sample` sample:

  * Modified ``connect`` command to provide better control over connection parameters.
  * Added ``Auto-Security-Personal`` mode to the ``connect`` command.
  * Added support for the ``WPA-PSK`` security mode to the ``wifi_mgmt_ext`` library.

* Added the :ref:`wifi_promiscuous_sample` sample that demonstrates how to set Promiscuous mode, establish a connection to an Access Point (AP), analyze incoming Wi-Fi packets, and print packet statistics.

* :ref:`wifi_station_sample` sample:

  * Modified to use the :ref:`lib_wifi_ready` library to manage the Wi-Fi use.

Other samples
-------------

* Added the :ref:`coremark_sample` sample that demonstrates how to easily measure a performance of the supported SoCs by running the Embedded Microprocessor Benchmark Consortium (EEMBC) CoreMark benchmark.

* :ref:`bootloader` sample:

  * Added support for the :ref:`zephyr:nrf9151dk_nrf9151` and the :ref:`nRF9161 DK <ug_nrf9161>` boards.

* :ref:`ipc_service_sample` sample:

  * Removed support for the `OpenAMP`_ library backend on the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* Removed support for setting RTS threshold through ``wifi_util`` command.
* Added support for random MAC address generation at boot using the :kconfig:option:`CONFIG_WIFI_RANDOM_MAC_ADDRESS` Kconfig option.

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

* :ref:`bt_fast_pair_readme` library:

  * Added experimental support for a new cryptographical backend that relies on the PSA crypto APIs (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).

Debug libraries
---------------

* :ref:`mod_memfault` library:

  * Fixed an issue where the library resets the LTE connectivity statistics after each read.
    This could lead to an accumulated error over time because of the byte counter resolution.
    The connectivity statistics are now only reset when the library is initialized and will be cumulative after that.

DFU libraries
-------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_CHUNK_SZ` Kconfig option to process traces in chunks.
      This can improve the availability of trace memory, and thus reduce the chances of losing traces.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_LTE_NET_IF` Kconfig option for sending modem faults to the :ref:`nrf_modem_lib_lte_net_if` when it is enabled.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_FAULT_THREAD_STACK_SIZE` Kconfig option to allow the application to set the modem fault thread stack size.

  * Deprecated the Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ZEPHYR`.
  * Fixed an issue with the CFUN hooks when the Modem library is initialized during ``SYS_INIT`` at kernel level and makes calls to the :ref:`nrf_modem_at` interface before the application level initialization is done.
  * Removed the deprecated options ``CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ASYNC`` and ``CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_SYNC``.

  * :ref:`nrf_modem_lib_lte_net_if`:

    * Added the dependency to the :kconfig:option:`CONFIG_NET_CONNECTION_MANAGER` Kconfig option.
    * Removed the requirement of IPv4 and IPv6 support for all applications.
      IPv4 and IPv6 support can be enabled using the :kconfig:option:`CONFIG_NET_IPV4` and :kconfig:option:`CONFIG_NET_IPV6` Kconfig options, respectively.
    * Increased the required stack size of the :kconfig:option:`NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE` Kconfig option to ``1024``.

* :ref:`lib_location` library:

  * Added:

    * Convenience function to get :c:struct:`location_data_details` from the :c:struct:`location_event_data`.
    * Location data details for event :c:enum:`LOCATION_EVT_RESULT_UNKNOWN`.
    * Sending GNSS coordinates to nRF Cloud when the :kconfig:option:`CONFIG_LOCATION_SERVICE_NRF_CLOUD_GNSS_POS_SEND` Kconfig option is set.

* :ref:`pdn_readme` library:

  * Updated the ``dns4_pri``, ``dns4_sec``, and ``ipv4_mtu`` parameters of the :c:func:`pdn_dynamic_params_get` function to be optional.
    If the MTU is not reported by the SIM card, the ``ipv4_mtu`` parameter is set to zero.

* :ref:`lte_lc_readme` library:

  * Removed ``AT%XRAI`` related deprecated functions ``lte_lc_rai_param_set()`` and ``lte_lc_rai_req()``, and Kconfig option :kconfig:option:`CONFIG_LTE_RAI_REQ_VALUE`.
    The application uses the Kconfig option :kconfig:option:`CONFIG_LTE_RAI_REQ` and ``SO_RAI`` socket option instead.

Libraries for networking
------------------------

* Added:

   * :ref:`lib_softap_wifi_provision` library.
   * :ref:`lib_wifi_ready` library.

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
    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION_PARSE_ANCHORS` Kconfig option to control if anchor names are parsed.
    * The :c:func:`nrf_cloud_obj_bool_get` function to get a boolean value from an object.

  * Updated:

    * Improved FOTA job status reporting.
    * Deprecated :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI` and its related UI Kconfig options.
    * Deprecated the :c:struct:`nrf_cloud_svc_info_ui` structure contained in the :c:struct:`nrf_cloud_svc_info` structure.
      nRF Cloud no longer uses the UI section in the shadow.
    * The :c:func:`nrf_cloud_coap_shadow_get` function to include a parameter to specify the content format of the returned payload.

* :ref:`lib_mqtt_helper` library:

  * Changed the library to read certificates as standard PEM format. Previously the certificates had to be manually converted to string format before compiling the application.
  * Replaced the ``CONFIG_MQTT_HELPER_CERTIFICATES_FILE`` Kconfig option with :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`. The new option specifies the folder where the certificates are stored.

* :ref:`lib_nrf_provisioning` library:

  * Added the :c:func:`nrf_provisioning_set_interval` function to set the interval between provisioning attempts.

* :ref:`lib_nrf_cloud_coap` library:

  * Updated to request proprietary PSM mode for ``SOC_NRF9151_LACA`` and ``SOC_NRF9131_LACA`` in addition to ``SOC_NRF9161_LACA``.
  * Removed the :kconfig:option:`CONFIG_NRF_CLOUD_COAP_GF_CONF` Kconfig option.

  * Added:

    * The :c:func:`nrf_cloud_coap_shadow_desired_update` function to allow devices to reject invalid shadow deltas.
    * Support for IPv6 connections.
    * The ``SO_KEEPOPEN`` socket option to keep the socket open even during PDN disconnect and reconnect.
    * The experimental Kconfig option :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` that enables downloading FOTA and P-GPS data using CoAP instead of HTTP.

* :ref:`lib_lwm2m_client_utils` library:

  * The following initialization functions have been deprecated as these modules are now initialized automatically on boot:

    * :c:func:`lwm2m_init_location`
    * :c:func:`lwm2m_init_device`
    * :c:func:`lwm2m_init_cellular_connectivity_object`
    * :c:func:`lwm2m_init_connmon`

  * :c:func:`lwm2m_init_firmware` is deprecated in favour of :c:func:`lwm2m_init_firmware_cb` that allows application to set a callback to receive FOTA events.
  * Fixed an issue where the Location Area Code was not updated when the Connection Monitor object version 1.3 was enabled.
  * Added support for the ``SO_KEEPOPEN`` socket option to keep the socket open even during PDN disconnect and reconnect.

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed a NULL pointer issue that could occur when there are some valid predictions in flash but not the one required at the current time.

* :ref:`lib_download_client` library:

  * Removed the deprecated ``download_client_connect`` function.

* :ref:`lib_fota_download` library:

  * Added:

    * The function :c:func:`fota_download_b1_file_parse` to parse a bootloader update file path.
    * Experimental support for performing FOTA updates using an external download client with the Kconfig option :kconfig:option:`CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL` and functions :c:func:`fota_download_external_start` and Function :c:func:`fota_download_external_evt_handle`.

Libraries for NFC
-----------------

|no_changes_yet_note|

Security libraries
------------------

* :ref:`trusted_storage_readme` library:

  * Added the Kconfig option :kconfig:option:`CONFIG_TRUSTED_STORAGE_STORAGE_BACKEND_CUSTOM` that enables use of custom storage backend.

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

* Added the :file:`thingy91x_dfu.py` script in the :file:`scripts/west_commands` folder.
  The script adds the west commands ``west thingy91x-dfu`` and ``west thingy91x-reset`` for convenient use of the serial recovery functionality.

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

* Added:

  * Support for PSA PAKE APIs from the PSA Crypto API specification 1.2.
  * Support for PBKDF2 algorithms as of |NCS| v2.6.0.

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * :ref:`ug_dect` user guide under :ref:`protocols`.
  * The :ref:`test_framework` section for gathering information about unit tests, with a new page about :ref:`running_unit_tests`.
  * List of :ref:`debugging_tools` on the :ref:`debugging` page.
  * Recommendation for the use of a :file:`VERSION` file for :ref:`ug_fw_update_image_versions_mcuboot` in the :ref:`ug_fw_update_image_versions` user guide.
  * The :ref:`ug_coremark` page.
  * The :ref:`bt_mesh_models_common_blocking_api_rule` section to the :ref:`bt_mesh_models_overview` page.
  * Steps for nRF54 devices across all supported samples to reflect the new button and LED numbering on the nRF54H20 DK and the nRF54L15 PDK.

* Updated:

  * The :ref:`cmake_options` section on the :ref:`configuring_cmake` page with the list of most common CMake options and more information about how to provide them.
  * The table listing the :ref:`boards included in sdk-zephyr <app_boards_names_zephyr>` with the nRF54L15 PDK and nRF54H20 DK boards.

  * The :ref:`ug_wifi_overview` page by separating the information about Wi-Fi certification into its own :ref:`ug_wifi_certification` page under :ref:`ug_wifi`.
  * The :ref:`ug_bt_mesh_configuring` page with an example of possible entries in the Settings NVS name cache.
  * The :ref:`lib_security` page to include all security-related libraries.
  * The trusted storage support table in the :ref:`trusted_storage_in_ncs` section by adding nRF52833 and replacing nRF9160 with nRF91 Series.

  * Reworked the :ref:`ble_rpc` page to be more informative and aligned with the library template.
  * Improved the :ref:`ug_radio_fem` user guide to be up-to-date and more informative.

  * The :ref:`ug_nrf52_developing` and :ref:`ug_nrf5340` by adding notes about how to perform FOTA updates with samples using random HCI identities, some specifically relevant when using the iOS app.

* Fixed:

  * Replaced the occurrences of the outdated :makevar:`OVERLAY_CONFIG` with the currently used :makevar:`EXTRA_CONF_FILE`.
