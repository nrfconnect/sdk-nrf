.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.5.99
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
See `known issues for nRF Connect SDK v2.5.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

|no_changes_yet_note|

Application development
=======================

This section provides detailed lists of changes to overarching SDK systems and components.

Build system
------------

|no_changes_yet_note|

nRF Front-End Modules
---------------------

|no_changes_yet_note|

Working with nRF91 Series
=========================

* Added new partition layout configuration options for Thingy:91.
  See :ref:`thingy91_partition_layout` for more details.

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

* Added host extensions for Nordic-only vendor-specific command APIs.
  Implementation and integration of the host APIs can be found in the :file:`host_extensions.h` header file.

Bluetooth Mesh
--------------

* Updated:

  * :ref:`bt_mesh_dm_srv_readme` and :ref:`bt_mesh_dm_cli_readme` model IDs and opcodes have been updated to avoid conflict with Simple OnOff Server and Client models.
  * :ref:`bt_mesh_sensors_readme` now use an updated API with sensor values represented by :c:struct:`bt_mesh_sensor_value` instead of :c:struct:`sensor_value`.
    This makes it possible to accurately represent all encodable sensor values.
    The old APIs based on the :c:struct:`sensor_value` type are deprecated, but are still available for backward compatibility, and can be enabled for use by setting the :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE` Kconfig option.
  * :ref:`bt_mesh_ug_reserved_ids` with model ID and opcodes for the new :ref:`bt_mesh_le_pair_resp_readme` model.

Matter
------

* Updated the page about :ref:`ug_matter_device_low_power_configuration` with the information about Intermittently Connected Devices (ICD) configuration.

* Added:

  * A Kconfig option for disabling or enabling :ref:`ug_matter_configuring_read_client`.
  * Support for PSA Crypto API for devices that use Matter over Thread.
    It is enabled by default and can be disabled by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``.
  * Migration of the Device Attestation Certificate (DAC) private key from the factory data set to the PSA ITS secure storage.

    The DAC private key can be removed from the factory data set after the migration.
    You can enable this experimental functionality by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_MIGRATE_DAC_PRIV_KEY` Kconfig option to ``y``.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.2.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

   * Support for the Intermittently Connected Devices (ICD) Management cluster.
   * The Kconfig options :kconfig:option:`CONFIG_CHIP_ICD_IDLE_MODE_DURATION`, :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_DURATION` and :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC` to manage ICD configuration.
   * New device types:

     * Refridgerator
     * Room air conditioner
     * Dishwasher
     * Laundry washer
     * Robotic vacuum cleaner
     * Smoke CO alarm
     * Air quality sensor
     * Air purifier
     * Fan

   * Product Appearance attribute in the Basic Information cluster that allows describing the product's color and finish.

* Updated:

   * Renamed the ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT`.
   * Renamed the ``CONFIG_CHIP_SED_IDLE_INTERVAL`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`.
   * Renamed the ``CONFIG_CHIP_SED_ACTIVE_INTERVAL`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL`.
   * Renamed the ``CONFIG_CHIP_SED_ACTIVE_THRESHOLD`` Kconfig option to :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD`.

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

  :ref:`wifi_raw_tx_packet_sample` sample that demonstrates how to transmit raw TX packets.

HomeKit
-------

HomeKit is now removed, as announced in the :ref:`ncs_release_notes_250`.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Asset Tracker v2
----------------

* Added:

  * The :kconfig:option:`CONFIG_DATA_SAMPLE_WIFI_DEFAULT` Kconfig option to configure whether Wi-Fi APs are included in sample requests by default.
  * The :kconfig:option:`NRF_CLOUD_SEND_SERVICE_INFO_FOTA` and :kconfig:option:`NRF_CLOUD_SEND_SERVICE_INFO_UI` Kconfig options.
    The application no longer sends a device shadow update; this is now handled by the :ref:`lib_nrf_cloud` library.

* Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

Serial LTE modem
----------------

* Added:

  * ``#XMQTTCFG`` AT command to configure MQTT client before connecting to the broker.
  * The :ref:`CONFIG_SLM_AUTO_CONNECT <CONFIG_SLM_AUTO_CONNECT>` Kconfig option to support automatic LTE connection at start-up or reset.
  * The :ref:`CONFIG_SLM_CUSTOMER_VERSION <CONFIG_SLM_CUSTOMER_VERSION>` Kconfig option for customers to define their own version string after customization.
  * The optional ``path`` parameter to the ``#XCARRIEREVT`` AT notification.
  * ``#XCARRIERCFG`` AT command to configure the LwM2M carrier library using the LwM2M carrier settings (see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SETTINGS` Kconfig option).

* Updated:

  * ``#XMQTTCON`` AT command to exclude MQTT client ID from the parameter list.
  * ``#XSLMVER`` AT command to report CONFIG_SLM_CUSTOMER_VERSION if it is defined.
  * The ``#XTCPCLI``, ``#XUDPCLI`` and ``#XHTTPCCON`` AT commands with options to:

    * Set the ``PEER_VERIFY`` socket option.
      Set to ``TLS_PEER_VERIFY_REQUIRED`` by default.
    * Set the ``TLS_HOSTNAME`` socket option to ``NULL`` to disable the hostname verification.

* Removed Kconfig options ``CONFIG_SLM_CUSTOMIZED`` and ``CONFIG_SLM_SOCKET_RX_MAX``.

nRF5340 Audio
-------------

* Changed:

    * ISO data sending has been refactored, and is now done in a single file: bt_le_audio_tx.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated:

  * The MCUboot and HCI RPMsg child images debug configurations to disable the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option.
    Disabling this Kconfig option improves the debugging experience.
  * The MCUboot and HCI RPMsg child images release configurations to explicitly enable the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option.
    Enabling this Kconfig option improves the reliability of the firmware.

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_dfu` to use :ref:`partition_manager` definitions for determining currently booted image slot in build time.
    The other image slot is used to store an application update image.
  * The :ref:`nrf_desktop_dfu_mcumgr` to use MCUmgr SMP command status callbacks (the :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` Kconfig option) instead of MCUmgr image and OS management callbacks.
  * The dependencies of the :kconfig:option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` Kconfig option.
    The option can be enabled even when the Bluetooth controller is not enabled as part of the application that uses :ref:`nrf_desktop_ble_latency`.
  * The :ref:`nrf_desktop_bootloader` and :ref:`nrf_desktop_bootloader_background_dfu` sections in the nRF Desktop documentation to explicitly mention the supported DFU configurations.
  * The documentation describing the :ref:`nrf_desktop_memory_layout` configuration to simplify the process of getting started with the application.
  * Changed the term *flash memory* to *non-volatile memory* for generalization purposes.
  * The :ref:`nrf_desktop_watchdog` to use ``watchdog0`` DTS alias instead of ``wdt`` DTS node label.
    Using the alias makes the configuration of the module more flexible.
  * Introduced information about priority, pipeline depth and maximum number of HID reports to :c:struct:`hid_report_subscriber_event`.
  * The :ref:`nrf_desktop_hid_state` uses :c:struct:`hid_report_subscriber_event` to handle HID data subscribers connection and disconnection.
    The :c:struct:`ble_peer_event` and ``usb_hid_event`` are no longer used for this purpose.
  * The ``usb_hid_event`` is removed.
  * The :ref:`nrf_desktop_usb_state` to use the :c:func:`usb_hid_set_proto_code` function to set the HID Boot Interface protocol code.
    The ``CONFIG_USB_HID_BOOT_PROTOCOL`` Kconfig option was removed and dedicated API needs to be used instead.
  * Disabled MCUboot's logs over RTT (:kconfig:option:`CONFIG_LOG_BACKEND_RTT` and :kconfig:option:`CONFIG_USE_SEGGER_RTT`) on ``nrf52840dk_nrf52840`` in :file:`prj_mcuboot_qspi.conf` configuration to reduce MCUboot memory footprint and avoid flash overflows.
    Explicitly enabled the UART log backend (:kconfig:option:`CONFIG_LOG_BACKEND_UART`) together with its dependencies in the configuration file to ensure log visibility.
  * The MCUboot, B0, and HCI RPMsg child images debug configurations to disable the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option.
    Disabling this Kconfig option improves the debugging experience.
  * The MCUboot, B0, and HCI RPMsg child images release configurations to explicitly enable the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option.
    Enabling this Kconfig option improves the reliability of the firmware.

Thingy:53: Matter weather station
---------------------------------

* Removed instantiation of OTATestEventTriggerDelegate, which was usable only for Matter Test Event purposes.
* Changed the deployment of configuration files to align with other Matter applications.

Matter Bridge
-------------

* Added:

  * Support for groupcast communication to the On/Off Light device implementation.
  * Support for controlling the OnOff Light simulated data provider by using shell commands.
  * Support for Matter Generic Switch bridged device type.
  * Support for On/Off Light Switch bridged device type.
  * Support for bindings to the On/Off Light Switch device implementation.
  * Guide about extending the application by adding support for a new Matter device type, a new Bluetooth LE service or a new protocol.
  * Support for Bluetooth LE Security Manager Protocol that allows to establish secure session with bridged Bluetooth LE devices.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* :ref:`ble_throughput` sample:

  * Enabled encryption in the sample.
    The measured throughput is calculated over the encrypted data, which is how most of the Bluetooth products use this protocol.

* :ref:`direct_test_mode` sample:

  * Added the configuration option to disable the Direction Finding feature.

Bluetooth Mesh samples
----------------------

* :ref:`ble_mesh_dfu_distributor` sample:

  * Added:

    * Support for pairing with display capability and the :ref:`bt_mesh_le_pair_resp_readme`.

|no_changes_yet_note|

Cellular samples
----------------

* :ref:`location_sample` sample:

  * Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for full modem FOTA.
    * Printing of the last reset reason when the sample starts.
    * Support for printing the sample version information using the ``version`` command.

  * Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

  * Updated the MQTT and CoAP overlays to enable the Kconfig option :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI`.
    The sample no longer sends a device shadow update for MQTT and CoAP builds; this is now handled by the :ref:`lib_nrf_cloud` library.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:
    * A generic processing example for application-specific shadow data.
    * Configuration and overlay files to enable MCUboot to use the external flash on the nRF1961 DK.

  * Fixed:

    * The sample now waits for a successful connection before printing ``Connected to nRF Cloud!``.
    * Building for the Thingy:91.
    * The PSM Requested Active Time is now reduced from 1 minute to 20 seconds.
      The old value was too long for PSM to activate.

  * Changed:

    * The sample now explicitly uses the :c:func:`conn_mgr_all_if_connect` function to start network connectivity, instead of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START` and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT` Kconfig options.
    * The sample to use the FOTA support functions in the :file:`nrf_cloud_fota_poll.c` and :file:`nrf_cloud_fota_common.c` files.
    * The sample now enables the Kconfig options :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_FOTA` and :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI`.
      It no longer sends a device status update on initial connection; this is now handled by the :ref:`lib_nrf_cloud` library.
    * Increased the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` and :kconfig:option:`CONFIG_AT_MONITOR_HEAP_SIZE` Kconfig options to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.
    * The sample reboot logic is now in a dedicated file so that it can be used in multiple locations.

  * Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added credential check before connecting to network.
  * Changed the sample use the functions in the :file:`nrf_cloud_fota_poll.c` and :file:`nrf_cloud_fota_common.c` files.
  * Increased the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` Kconfig option to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added credential check before connecting to network.
  * Increased the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` and :kconfig:option:`CONFIG_AT_MONITOR_HEAP_SIZE` Kconfig options to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.

* :ref:`nrf_provisioning_sample` sample:

  * Added event handling for events from device mode callback.

* :ref:`gnss_sample` sample:

  * Added the configuration overlay file :file:`overlay-supl.conf` for building the sample with SUPL assistance support.

* :ref:`udp` sample:

  * Added the :ref:`CONFIG_UDP_DATA_UPLOAD_ITERATIONS <CONFIG_UDP_DATA_UPLOAD_ITERATIONS>` Kconfig option for configuring the number of data transmissions to the server.

Cryptography samples
--------------------

* Updated:

  * All crypto samples to use ``psa_key_id_t`` instead of ``psa_key_handle_t``.
    These concepts have been merged and ``psa_key_handle_t`` is removed from the PSA API specification.

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

* Unified common code for buttons, LEDs and events in all Matter samples:

  * Created the task executor module which is responsible for posting and dispatching tasks.
  * Moved common methods for managing buttons and LEDs that are located on the DK to the board module.
  * Divided events to application and system events.
  * Defined common LED and button constants in the dedicated board configuration files.
  * Created the Kconfig file for the Matter common directory.

* Disabled :ref:`ug_matter_configuring_read_client` in most Matter samples using the new :kconfig:option:`CONFIG_CHIP_ENABLE_READ_CLIENT` Kconfig option.

* :ref:`matter_light_bulb_sample` sample:

  * Added support for `AWS IoT Core`_.

* :ref:`matter_lock_sample` sample:

  * Fixed an issue that prevented nRF Toolbox for iOS in version 5.0.9 from controlling the sample using :ref:`nus_service_readme`.
  * Changed the design of the :ref:`matter_lock_sample_wifi_thread_switching` feature so that support for both Matter over Thread and Matter over Wi-Fi is included in a single firmware image.

* Disabled:

  * :ref:`ug_matter_configuring_read_client` in most Matter samples using the new :kconfig:option:`CONFIG_CHIP_ENABLE_READ_CLIENT` Kconfig option.
  * WPA Supplicant advanced features in all Matter samples using the :kconfig:option:`WPA_SUPP_ADVANCED_FEATURES` Kconfig option.
    This saves roughly 25 KB of FLASH memory for firmware images with Wi-Fi support.

Multicore samples
-----------------

|no_changes_yet_note|

Networking samples
------------------

* :ref:`net_coap_client_sample` sample:

  * Added support for Wi-Fi and LTE connectivity through the connection manager API.
  * Updated:

    * The sample is moved from the :file:`cellular/coap_client` folder to :file:`net/coap_client`.
      The documentation is now found in the :ref:`networking_samples` section.
    * The sample to use the :ref:`coap_client_interface` library.

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * The "start_tx_modulated_carrier" command, when used without an additional parameter, does not enable the radio end interrupt.

PMIC samples
------------

* Added :ref:`npm1300_one_button` sample that demonstrates how to support wake-up, shutdown and user interactions through a single button connected to the nPM1300.

* :ref:`npm1300_fuel_gauge` sample:

  * Updated to accommodate API changes in the :ref:`nrfxlib:nrf_fuel_gauge`.

Sensor samples
--------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

|no_changes_yet_note|

Thread samples
--------------

* Changed building method to use :ref:`zephyr:snippets` for predefined configuration.

* In the :ref:`thread_ug_feature_sets` provided as part of the |NCS|, the following features have been removed from the FTD and MTD variants:

  * ``DHCP6_CLIENT``
  * ``JOINER``
  * ``SNTP_CLIENT``
  * ``LINK_METRICS_INITIATOR``

  All mentioned features are still available in the master variant.

* Added experimental support for Thread Over Authenticated TLS.

Sensor samples
--------------

|no_changes_yet_note|

Zigbee samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* Added the :ref:`wifi_throughput_sample` sample that demonstrates how to measure the network throughput of a Nordic Wi-Fi enabled platform under the different Wi-Fi stack configuration profiles.

Other samples
-------------

* :ref:`radio_test` sample:

  * Corrected the way of setting the TX power with FEM.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* OS agnostic code is moved to |NCS| (``sdk-nrfxlib``) repository.

  * Low-level API documentation is now available on the :ref:`Wi-Fi driver API <nrfxlib:nrf_wifi_api>`.

* Added TX injection feature to the nRF70 Series device.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Updated:

    * Improved the :ref:`bt_fast_pair_readme` library documentation to include the description of the missing Kconfig options.

* :ref:`bt_mesh` library:

  * Added:

    * The :ref:`bt_mesh_le_pair_resp_readme` model to allow passing a passkey used in LE pairing over a mesh network.

 :ref:`nrf_bt_scan_readme`:

  * Added the :c:func:`bt_scan_update_connect_if_match` function to update the autoconnect flag after a filter match.

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

* :ref:`lib_dfu_target` library:

  * Updated:

    * The :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION` Kconfig option to be automatically enabled when ``nordic,pm-ext-flash`` is chosen in the devicetree.
      See :ref:`partition_manager` for details.

Modem libraries
---------------

* :ref:`lib_location` library:

  * Updated:

    * The use of neighbor cell measurements for cellular positioning.
      Previously, 1-2 searches were performed and now 1-3 will be done depending on the requested number of cells and the number of found cells.
      Also, only GCI cells are counted towards the requested number of cells, and normal neighbors are ignored from this perspective.
    * Cellular positioning not to use GCI search when the device is in RRC connected mode, because the modem cannot search for GCI cells in that mode.
    * GNSS is not started at all if the device does not enter RRC idle mode within two minutes.

* :ref:`lte_lc_readme` library:

  * Added:

    * The :c:func:`lte_lc_psm_param_set_seconds` function and Kconfig options :kconfig:option:`CONFIG_LTE_PSM_REQ_FORMAT`, :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU_SECONDS`, and :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT_SECONDS` to enable setting of PSM parameters in seconds instead of using bit field strings.

  * Updated:

    * The default network mode to :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` from :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_GPS`.
    * The default LTE mode preference to :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE_LTE_M_PLMN_PRIO` from :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE_AUTO`.
    * The :kconfig:option:`CONFIG_LTE_NETWORK_USE_FALLBACK` Kconfig option is deprecated.
      Use the :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT` or :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` Kconfig option instead.
      In addition, you can control the priority between LTE-M and NB-IoT using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.
    * The :c:func:`lte_lc_init` function is deprecated.
    * The :c:func:`lte_lc_deinit` function is deprecated.
      Use the :c:func:`lte_lc_power_off` function instead.
    * The :c:func:`lte_lc_init_and_connect` function is deprecated.
      Use the :c:func:`lte_lc_connect` function instead.
    * The :c:func:`lte_lc_init_and_connect_async` function is deprecated.
      Use the :c:func:`lte_lc_connect_async` function instead.

  * Removed the deprecated Kconfig option ``CONFIG_LTE_AUTO_INIT_AND_CONNECT``.

* :ref:`nrf_modem_lib_readme`:

  * Added:

    * A mention about enabling TF-M logging while using modem traces in the :ref:`modem_trace_module`.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_DOWN_DEFAULT_LTE_DISCONNECT` option, allowing the user to change the behavior of the driver's :c:func:`net_if_down` implementation at build time.

  * Updated by renaming ``lte_connectivity`` module to ``lte_net_if``.
    All related Kconfig options have been renamed accordingly.
  * Changed the default value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START`, :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT`, and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_DOWN` Kconfig options from enabled to disabled.

  * Fixed:

    * The ``lte_net_if`` module now handles the :c:enumerator:`~pdn_event.PDN_EVENT_NETWORK_DETACH` PDN event.
      Not handling this caused permanent connection loss and error message (``ipv4_addr_add, error: -19``) in some situations when reconnecting.

  * Removed:

    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_SYS_INIT``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE``.
    * The ``NRF_MODEM_LIB_NET_IF_DOWN`` flag support in the ``lte_net_if`` network interface driver.

* :ref:`lib_modem_slm`:

    * Changed the GPIO used to be configurable using devicetree.

* :ref:`pdn_readme` library:

   * Added the :c:func:`pdn_dynamic_params_get` function to retrieve dynamic parameters of an active PDN connection.
   * Fixed a potential issue where the library tries to free the PDN context twice, causing the application to crash.
   * Updated the library to add PDP auto configuration to the :c:enumerator:`LTE_LC_FUNC_MODE_POWER_OFF` event.

* :ref:`lib_at_host` library:

  * Added the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` Kconfig option.
    This option allows the stack size of the AT host workqueue thread to be adjusted.

Libraries for networking
------------------------

* :ref:`lib_aws_iot` library:

  * Added library tests.
  * Updated the library to use the :ref:`lib_mqtt_helper` library.
    This simplifies the handling of the MQTT stack.

* :ref:`lib_nrf_cloud_coap` library:

  * Added:

    * Automatic selection of proprietary PSM mode when building for the SOC_NRF9161_LACA.
    * Support for bulk transfers to the :c:func:`nrf_cloud_coap_json_message_send` function.

  * Updated:

    * The :c:func:`nrf_cloud_coap_shadow_delta_process` function to include a parameter for application-specific shadow data.
    * The :c:func:`nrf_cloud_coap_shadow_delta_process` function to process default shadow data added by nRF Cloud, which is not used by CoAP.
    * The CDDL file for AGNSS to align with cloud changes and regenerated the AGNSS encoder accordingly.

* :ref:`lib_nrf_cloud_log` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_LOG_INCLUDE_LEVEL_0` Kconfig option.
    * Support for nRF Cloud CoAP text mode logging.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * The :c:func:`nrf_cloud_credentials_configured_check` function to check if credentials exist based on the application's configuration.
    * The :c:func:`nrf_cloud_obj_object_detach` function to get an object from an object.
    * The :c:func:`nrf_cloud_obj_shadow_update` function to update the device's shadow with the data from an :c:struct:`nrf_cloud_obj` structure.
    * An :c:struct:`nrf_cloud_obj_shadow_data` structure to the :c:struct:`nrf_cloud_evt` structure to be used during shadow update events.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_FOTA` Kconfig option to enable sending configured FOTA service info on the device's initial connection to nRF Cloud.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI` Kconfig option to enable sending configured UI service info on the device's initial connection to nRF Cloud.
    * Support for handling location request responses fulfilled by a Wi-Fi anchor.

  * Updated:

    * The :c:func:`nrf_cloud_obj_object_add` function to reset the added object on success.
    * Custom shadow data is now passed to the application during shadow update events.
    * The AGNSS handling to use the AGNSS app ID string and corresponding MQTT topic instead of the older AGPS app ID string and topic.

* :ref:`lib_nrf_provisioning` library:

  * Renamed nRF Device provisioning to :ref:`lib_nrf_provisioning`.
  * Updated the device mode callback to send an event when the provisioning state changes.

* :ref:`lib_nrf_cloud_fota` library:

  * Added the :file:`nrf_cloud_fota_poll.c` file to consolidate the FOTA polling code from the :ref:`nrf_cloud_multi_service` and :ref:`nrf_cloud_rest_fota` samples.

* :ref:`lib_mqtt_helper` library:

  * Added support for using a password when connecting to a broker.

Libraries for NFC
-----------------

* Fixed an issue with handling zero size data (when receiving empty I-blocks from poller) in the :file:`platform_internal_thread` file.

nRF Security
------------

|no_changes_yet_note|

Other libraries
---------------

* :ref:`lib_adp536x` library:

  * Fixed issue where the adp536x driver was included in the immutable bootloader on Thingy:91 when :kconfig:option:`CONFIG_SECURE_BOOT` was enabled.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_state`:

  * Updated the dependencies of the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` Kconfig option.
    The option can be enabled even when the Bluetooth controller is not enabled as part of the application that uses :ref:`caf_ble_state`.

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

* :ref:`nrf_desktop_config_channel_script`:

  * Separated functions that are specific to handling the :file:`dfu_application.zip` file format.
    The ZIP format is used for update images in the nRF Connect SDK.
    The change simplifies integrating new update image file formats.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``11ecbf639d826c084973beed709a63d51d9b684e``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

|no_changes_yet_note|

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``23cf38934c0f68861e403b22bc3dd0ce6efbfa39``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 23cf38934c ^a768a05e62

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^23cf38934c

The current |NCS| main branch is based on revision ``23cf38934c`` of Zephyr.

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

* The minimal TF-M build profile no longer silences TF-M logs by default.

  .. note::
     This can be a breaking change if the UART instance used by TF-M is already in use, for example by modem trace with a UART backend.

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added

  * :ref:`ug_nrf9161` user guide.
  * A page on :ref:`ug_nrf70_developing_debugging` in the :ref:`ug_nrf70_developing` user guide.
  * A page on :ref:`ug_nrf70_developing_fw_patch_ext_flash` in the :ref:`ug_nrf70_developing` user guide.
  * A page on :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission` in the :ref:`ug_nrf70_developing` user guide.
  * :ref:`contributions_ncs` page in a new :ref:`contributions` section that also includes the development model pages, previously listed under :ref:`releases_and_maturity`.

* Updated:

  * The :ref:`installation` section by replacing two separate pages about installing the |NCS| with just one (:ref:`install_ncs`).
  * The :ref:`requirements` page with new sections about :ref:`requirements_clt` and :ref:`toolchain_management_tools`.
  * The :ref:`configuration_and_build` section:

    * :ref:`app_build_system` gathers conceptual information about the build and configuration system previously listed on several other pages.
      The :ref:`app_build_additions` section on this page now provides more information about :ref:`app_build_additions_build_types` specific to the |NCS|.
    * :ref:`app_boards` is now a section and its contents have been moved to several subpages.
    * New :ref:`configuring_devicetree` subsection now groups guides related to configuration of hardware using the devicetree language.
    * New reference page :ref:`app_build_output_files` gathers information previously listed on several other pages.
    * :ref:`app_dfu` and :ref:`app_bootloaders` are now separate sections, with the DFU section summarizing the available DFU methods in a table.

  * The :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages so that instructions in the :ref:`nrf9160_gs_connecting_dk_to_cloud` and :ref:`thingy91_connect_to_cloud` sections, respectively, match the updated nRF Cloud workflow.
  * The :ref:`ug_nrf9160_gs` by replacing the Updating the DK firmware section with a new :ref:`nrf9160_gs_installing_software` section.
    This new section includes steps for using Quick Start, a new application in `nRF Connect for Desktop`_ that streamlines the getting started process with the nRF91 Series DKs.
  * The :ref:`tfm_enable_share_uart` section on :ref:`ug_nrf9160`.
  * Integration steps in the :ref:`ug_bt_fast_pair` guide.
    Reorganized extension-specific content into dedicated subsections.
  * The :ref:`ug_nrf70_developing_powersave_power_save_mode` section in the :ref:`ug_nrf70_developing_powersave` user guide.
  * The :ref:`nrf7002dk_nrf5340` page with a link to the `Wi-Fi Fundamentals course`_ in the `Nordic Developer Academy`_.

  * The :ref:`dev-model` section with the :ref:`documentation` pages, previously listed separately.
  * The :ref:`glossary` page by moving it back to the main hierarchy level.

* Removed the Welcome to the |NCS| page.
  This page is replaced with existing :ref:`ncs_introduction` page.
