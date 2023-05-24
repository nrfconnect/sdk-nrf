.. _ncs_release_notes_changelog:

Changelog for |NCS| v2.3.99
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
See `known issues for nRF Connect SDK v2.3.0`_ for the list of issues valid for the latest release.

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

|no_changes_yet_note|

Build system
------------

* When using :kconfig:option:`CONFIG_SB_SIGNING_KEY_FILE` with relative paths, the relative path is now located at the application configuration directory instead of the application source directory (these are the same if the application configuration directory is not set).

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

|no_changes_yet_note|

Bluetooth mesh
--------------

* Updated the protocol user guide with the information about :ref:`ble_mesh_dfu_samples`.
* Updated the default configuration of advertising sets used by the Bluetooth mesh subsystem to improve performance of the Relay, GATT and Friend features.
  This configuration is specified in the :file:`ncs/nrf/subsys/bluetooth/mesh/Kconfig` file.
* Updated samples :ref:`bluetooth_mesh_sensor_server`, :ref:`bluetooth_mesh_light_lc` and :ref:`bluetooth_mesh_light_dim` to demonstrate the Bluetooth :ref:`ug_bt_mesh_nlc`.

See `Bluetooth mesh samples`_ for the list of changes in the Bluetooth mesh samples.

Matter
------

* Added:

  * Support for the :ref:`ug_matter_configuring_optional_persistent_subscriptions` feature.
  * The Matter Nordic UART Service (NUS) feature to the :ref:`matter_lock_sample` sample.
    This feature allows using Nordic UART Service to control the device remotely through Bluetooth LE and adding custom text commands to a Matter sample.
    The Matter NUS implementation allows controlling the device regardless of whether the device is connected to a Matter network or not.
    The feature is dedicated for the Matter over Thread solution.
  * Documentation page about :ref:`ug_matter_device_configuring_cd`.
  * Matter SDK fork :ref:`documentation pages <matter_index>` with the page about CHIP Certificate Tool.
  * Documentation page about :ref:`ug_matter_device_adding_bt_services`.

* Updated:

  * The :ref:`ug_matter` protocol page with a table that lists compatibility versions for the |NCS|, the Matter SDK, and the Matter specification.
  * The :ref:`ug_matter_tools` page with installation instructions for the ZAP tool, moved from the :ref:`ug_matter_creating_accessory` page.
  * The :ref:`ug_matter_tools` page with information about CHIP Tool, CHIP Certificate Tool, and the Spake2+ Python tool.
  * The :ref:`ug_matter_device_low_power_configuration` page with information about sleepy active threshold parameter configuration.
  * The default number of user RTC channels on the nRF5340 SoC's network core from 2 to 3 (the platform default) to fix the CSL transmitter feature on Matter over Thread devices acting as Thread routers.
  * :ref:`ug_matter_hw_requirements` with updated memory requirement values valid for the |NCS| v2.4.0.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Updated the factory data generation script with the feature for generating the onboarding code.
  You can now use the factory data script to generate a manual pairing code and a QR Code that are required to commission a Matter-enabled device over Bluetooth LE.
  Generated onboarding codes should be put on the device's package or on the device itself.
  For details, see the Generating onboarding codes section on the :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.
* Introduced ``SLEEPY_ACTIVE_THRESHOLD`` parameter that makes the Matter sleepy device stay awake for a specified amount of time after network activity.
* Updated the Basic Information cluster with device finish and device color attributes and added the related entries in factory data set.

Thread
------

|no_changes_yet_note|

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

* Fixed:

  * An issue where device is not fully operational in the distributed network by adding call to the :c:func:`zb_enable_distributed` function for Zigbee Router role by default.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Support for bigger payload size.
    ESB supports a payload with a size of 64 bytes or more.
  * The ``use_fast_ramp_up`` feature that reduces radio ramp-up delay from 130 µs to 40 µs.
  * The :kconfig:option:`CONFIG_ESB_NEVER_DISABLE_TX` Kconfig option as an experimental feature that enables the radio peripheral to remain in TXIDLE state instead of TXDISABLE when transmission is pending.

* Updated:

  * The number of PPI/DPPI channels used from three to six.
  * Events 6 and 7 from the EGU0 instance by assigning them to the ESB module.
  * The type parameter of the :c:func:`esb_set_tx_power` function to ``int8_t``.

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Wi-Fi
-----

* Added:

  * Support for nRF7000 EK.
  * Support for nRF7001 EK and nRF7001 DK.
  * Support for a new shield, nRF7002 Expansion Board (EB) for Thingy53 (rev 1.1.0).

* Updated:

  * The shield for nRF7002 EK (``nrf7002_ek`` -> ``nrf7002ek_nrf7002``).

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added the integration of the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.

* Updated:

  * Moved mcuboot_secondary to external flash for nRF9160 DK v0.14.0 and newer.
    This requires board controller firmware v2.0.1 or newer which enables the pin routing to external flash.
  * The application now uses the function :c:func:`nrf_cloud_location_request_msg_json_encode` to create an nRF Cloud location request message.
  * The application now uses defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.
  * Instead of sending a battery voltage, the PMIC's fuel gauge function is used to get a battery percentage. For nRF Cloud, the data ID "VOLTAGE" has been replaced with "BATTERY". For the other cloud backends, the name stays the same, but the range changes to 0-100.
  * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * AT command ``#XWIFIPOS`` to get Wi-Fi location from nRF Cloud.
  * Support for *WRITE REQUEST* in TFTP client.

* Updated:

  * Use defines from the :ref:`lib_nrf_cloud` library for nRF Cloud related string values.

* Fixed:

  * A bug in receiving large MQTT Publish message.

nRF5340 Audio
-------------

* Moved the LE Audio controller for the network core to the standalone :ref:`lib_bt_ll_acs_nrf53_readme` library.
* Added Kconfig options for setting periodic and extended advertising intervals.
  Search :ref:`Kconfig Reference <kconfig-search>` for ``BLE_ACL_PER_ADV_INT_`` and ``BLE_ACL_EXT_ADV_INT_`` to list all of them.
* Implemented :ref:`zephyr:zbus` for handling events from buttons and LE Audio.
* Reduced supervision timeout to reduce reconnection times for CIS.
* Updated the :ref:`nrf53_audio_app` application documentation with a note about missing support for the |nRFVSC|.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated the machine learning models (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) used by the application to ensure compatibility with the new Zephyr version.
* Simplified the over-the-air (OTA) device firmware update (DFU) configuration of nRF53 DK .
  The configuration relies on the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

nRF Desktop
-----------

* Added:

  * The :ref:`nrf_desktop_swift_pair_app`.
    The module is used to enable or disable the Swift Pair Bluetooth advertising payload depending on the selected Bluetooth peer (used local identity).
  * An application-specific string representing device generation (:ref:`CONFIG_DESKTOP_DEVICE_GENERATION <config_desktop_app_options>`).
    The generation allows to distinguish configurations that use the same board and bootloader, but are not interoperable.
    The value can be read through the :ref:`nrf_desktop_config_channel`.
    On the firmware side, fetching the values is handled by the :ref:`nrf_desktop_dfu`.
  * Unpairing old peers right after a successful erase advertising procedure.
    This prevents blocking the bond slots until the subsequent erase advertising procedure is triggered.
  * Support for the :ref:`nrf_desktop_dfu` for devices using the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``).
    In this mode, the image is booted directly from the secondary slot instead of moving it to the primary slot.
  * The :ref:`nrf_desktop_factory_reset`.
    The module is used by configurations that enable :ref:`nrf_desktop_bluetooth_guide_fast_pair` to factory reset both Fast Pair and Bluetooth non-volatile data.
    The factory reset is triggered using the configuration channel.
  * The :ref:`nrf_desktop_dfu_lock`.
    The utility provides synchronization mechanism for accessing the DFU flash.
    It is useful for application configurations that support more than one DFU method.
  * The :ref:`nrf_desktop_dfu_mcumgr` that you can enable with the :ref:`CONFIG_DESKTOP_DFU_MCUMGR_ENABLE <config_desktop_app_options>` option.
    The module handles image upload over MCUmgr SMP protocol.
    The module integrates the :ref:`nrf_desktop_dfu_lock` for synchronizing flash access with other DFU methods.

* Updated:

  * The :ref:`nrf_desktop_dfu` to integrate the :ref:`nrf_desktop_dfu_lock` for synchronizing flash access with other DFU methods.
    Use the :ref:`CONFIG_DESKTOP_DFU_LOCK <config_desktop_app_options>` option to enable this feature.
  * The nRF desktop configurations that enable :ref:`nrf_desktop_bluetooth_guide_fast_pair`.
    The configurations use the MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) instead of the ``B0`` bootloader.
    This is done to support firmware updates using both :ref:`nrf_desktop_dfu` and :ref:`nrf_desktop_dfu_mcumgr` modules.
  * The :ref:`nrf_desktop_dfu_mcumgr` is used instead of the :ref:`nrf_desktop_smp` in MCUboot SMP configuration (:file:`prj_mcuboot_smp.conf`) for the nRF52840 DK.
  * The :ref:`nrf_desktop_dfu` automatically enables 8-bit write block size emulation (:kconfig:option:`CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS`) to ensure that update images with sizes not aligned to word size can be successfully stored in the internal flash.
    The feature is not enabled if the MCUboot bootloader is used and the secondary slot is placed in an external flash (when :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY` is enabled).
  * The :ref:`nrf_desktop_ble_latency` uses low latency for the active Bluetooth connection in case of the SMP transfer event and regardless of the event submitter module.
    Previously, the module lowered the connection latency only for SMP events submitted by the :ref:`caf_ble_smp`.
  * In the Fast Pair configurations, the bond erase operation is enabled for the dongle peer, which will let you change the bonded Bluetooth Central.
  * The `Swift Pair`_ payload is, by default, included for all of the Bluetooth local identities apart from the dedicated local identity used for connection with an nRF Desktop dongle.
    If a configuration supports both Fast Pair and a dedicated dongle peer (:ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>`), the `Swift Pair`_ payload is, by default, included only for the dongle peer.
  * Set the max compiled-in log level to ``warning`` for the Bluetooth HCI core (:kconfig:option:`CONFIG_BT_HCI_CORE_LOG_LEVEL`).
    This is done to avoid flooding logs during application boot.
  * The documentation with debug Fast Pair provisioning data obtained for development purposes.
  * Aligned the nRF52833 dongle's board DTS configuration files and nRF Desktop's application-specific DTS overlays to hardware revision 0.2.1.

Samples
=======

Bluetooth samples
-----------------

* :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples register HID Service before Bluetooth is enabled (before calling the :c:func:`bt_enable` function).
  The :c:func:`bt_gatt_service_register` function can no longer be called after enabling Bluetooth and before loading settings.

* Removed the Bluetooth: External radio coexistence using 3-wire interface sample because of the removal of the 3-wire implementation.

* :ref:`peripheral_hids_mouse` sample:

  * The :kconfig:option:`CONFIG_BT_SMP` Kconfig option is included when ``CONFIG_BT_HIDS_SECURITY_ENABLED`` is selected.
  * Fixed a CMake warning by moving the nRF RPC configuration (the :kconfig:option:`CONFIG_NRF_RPC_THREAD_STACK_SIZE` Kconfig option) to a separate overlay config file.

* :ref:`direct_test_mode` sample:

  * Added:

    * Support for the :ref:`nrfxlib:mpsl_fem` Tx power split feature.
      The DTM command ``0x09`` for setting the transmitter power level takes into account the front-end module gain when this sample is built with support for front-end modules.
      The vendor-specific commands for setting the SoC output power and the front-end module gain are not available when the :kconfig:option:`CONFIG_DTM_POWER_CONTROL_AUTOMATIC` Kconfig option is enabled.
    * Support for +1 dBm, +2 dBm, and +3 dBm output power on the nRF5340 DK.

  * Changed the handling of the hardware erratas.

  * Removed a compilation warning when used with minimal pinout Skyworks FEM.

* :ref:`peripheral_uart` sample:

  * Fixed the unit of the :kconfig:option:`CONFIG_BT_NUS_UART_RX_WAIT_TIME` Kconfig option to comply with the UART API.

* :ref:`nrf_dm` sample:

  * Improved the scalability of the sample when it is being used with more devices.

* :ref:`peripheral_fast_pair` sample:

  * Added the default Fast Pair provisioning data that is used when no other provisioning data is specified.
  * Updated the documentation to align it with the new way of displaying notifications for the Fast Pair debug Model IDs.

Bluetooth mesh samples
----------------------

* Added samples :ref:`ble_mesh_dfu_target` and :ref:`ble_mesh_dfu_distributor` that can be used for evaluation of the Bluetooth mesh DFU specification and subsystem.
* Added :ref:`bluetooth_mesh_light_dim` sample that demonstrates how to set up a light dimmer and scene selector application.
* Updated the configuration of advertising sets in all samples to match the new default values.
  See `Bluetooth mesh`_ for more information.
* Removed the :file:`hci_rpmsg.conf` file from all samples that support nRF5340 DK or Thingy:53.
  This configuration is moved to the :file:`ncs/nrf/subsys/bluetooth/mesh/hci_rpmsg_child_image_overlay.conf` file.
* :ref:`bluetooth_mesh_light_lc` sample:

   * Updated the sample to demonstrate the use of sensor server to report additional useful information about the device.

* :ref:`bluetooth_mesh_sensor_server` and :ref:`bluetooth_mesh_sensor_client` samples:

  * Added:

    * Support for motion threshold as a setting for the presence detection.
    * Support for ambient light level sensor.
    * Shell support to :ref:`bluetooth_mesh_sensor_client`.

nRF9160 samples
---------------

* :ref:`http_full_modem_update_sample` sample:

  * The sample now uses modem firmware versions 1.3.3 and 1.3.4.
  * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

* :ref:`http_modem_delta_update_sample` sample:

  * The sample now uses modem firmware v1.3.4 to do a delta update.

* :ref:`modem_shell_application` sample:

  * Added:

    * Sending of GNSS data to carrier library when the library is enabled.

  * Updated:

    * The sample now uses defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.
      Removed the inclusion of the file :file:`nrf_cloud_codec.h`.
    * Modem FOTA now updates the firmware without rebooting the application.
    * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

* :ref:`https_client` sample:

  * Added IPv6 support and wait time for PDN to fully activate (including IPv6, if available) before looking up the address.

* :ref:`slm_shell_sample` sample:

  * Added support for the nRF7002 DK PCA10143.

* :ref:`lwm2m_client` sample:

  * Added:

    * Integration of the connection pre-evaluation functionality using the :ref:`lib_lwm2m_client_utils` library.

  * Updated:

    * The sample now integrates the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.
    * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

* :ref:`pdn_sample` sample:

  * Updated the sample to show how to get interface address information using the :c:func:`nrf_getifaddrs` function.

* :ref:`nrf_cloud_mqtt_multi_service` sample:

  * Updated:

    * Increased the MCUboot partition size to the minimum necessary to allow bootloader FOTA.
    * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

  * Added:

    * Sending of log messages directly to nRF Cloud.
    * Overlay to enable `Zephyr Logging`_ backend for full logging to nRF Cloud.

* :ref:`nrf_cloud_rest_fota` sample:

    * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added:

    * Overlays to use RTT instead of UART for testing purposes.
    * Sending of log messages directly to nRF Cloud.
    * Overlay to enable `Zephyr Logging`_ backend for full logging to nRF Cloud.

  * Updated:

    * The Hello World message sent to nRF Cloud now contains a timestamp (message ID).

* :ref:`memfault_sample` sample:

  * Moved from :file:`nrf9160/memfault` to :file:`debug/memfault`.
    The documentation is now found in the :ref:`debug_samples` section.
  * Added support for the nRF7002 DK.
  * Added a Kconfig fragment to enable ETB trace.

* :ref:`modem_trace_flash` sample:

  * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`provisioning_image` sample:

  * The network core logic is now moved to the new sample :ref:`provisioning_image_net_core` instead of being a Zephyr module.

Thread samples
--------------

|no_changes_yet_note|

Matter samples
--------------

* Updated the default settings partition size for all Matter samples from 16 kB to 32 kB.

  .. caution::
      This change can affect the Device Firmware Update (DFU) from the older firmware versions that were using the 16-kB settings size.
      Read more about this in the :ref:`ug_matter_device_bootloader_partition_layout` section of the Matter documentation.
      You can still perform DFU from the older firmware version to the latest firmware version, but you will have to change the default settings size from 32 kB to the value used in the older version.

* :ref:`matter_lock_sample` sample:

  * Added the Matter Nordic UART Service (NUS) feature, which allows controlling the door lock device remotely through Bluetooth LE using two simple commands: ``Lock`` and ``Unlock``.
    This feature is dedicated for the nRF52840 and the nRF5340 DKs.
    The sample supports one Bluetooth LE connection at a time.
    Matter commissioning, DFU, and NUS over Bluetooth LE must be run separately.

NFC samples
-----------

|no_changes_yet_note|

Multicore samples
-----------------

* :ref:`multicore_hello_world` sample:

  * Added :ref:`zephyr:sysbuild` support to the sample.

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

|no_changes_yet_note|

Other samples
-------------

* :ref:`ei_wrapper_sample` sample:

  * Updated the machine learning model (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) to ensure compatibility with the new Zephyr version.

* :ref:`radio_test` sample:

  * Added:

    * A workaround for the hardware `Errata 254`_ of the nRF52840 chip.
    * A workaround for the hardware `Errata 255`_ of the nRF52833 chip.
    * A workaround for the hardware `Errata 256`_ of the nRF52820 chip.
    * A workaround for the hardware `Errata 257`_ of the nRF52811 chip.
    * A workaround for the hardware `Errata 117`_ of the nRF5340 chip.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added :ref:`nrf700x_wifi`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* Added the standalone :ref:`lib_bt_ll_acs_nrf53_readme` library, originally a part of the :ref:`nrf53_audio_app` application.

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v3.2.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_le_adv_prov_readme` library:

  * Added API to enable or disable the Swift Pair provider (:c:func:`bt_le_adv_prov_swift_pair_enable`).

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * The :c:func:`bt_fast_pair_info_cb_register` function and the :c:struct:`bt_fast_pair_info_cb` structure to register Fast Pair information callbacks.
      The :c:member:`bt_fast_pair_info_cb.account_key_written` callback can be used to notify the application about the Account Key writes.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_USER_RESET_ACTION` Kconfig option to enable a custom user reset action that executes together with the Fast Pair factory reset operation triggered by the :c:func:`bt_fast_pair_factory_reset` function.

  * Updated:

    * Salt size in the Fast Pair not discoverable advertising from 1 byte to 2 bytes, to align with the Fast Pair specification update.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON` Kconfig option is now the default Fast Pair cryptographic backend.

* :ref:`nrf_bt_scan_readme` library:

  * Fixed:

    * The output arguments of the :c:func:`bt_scan_filter_status_get` function.
      The :c:member:`bt_filter_status.manufacturer_data.enabled` field is now correctly set to reflect the status of the filter when the function is called.

Bootloader libraries
--------------------

|no_changes_yet_note|

Debug libraries
---------------

* Added the :ref:`etb_trace` library for instruction traces.

Modem libraries
---------------

* :ref:`lte_lc_readme` library:

  * Added:

    * The Kconfig option :kconfig:option:`CONFIG_LTE_PSM_REQ` that automatically requests PSM on modem initialization.
      If this option is disabled, PSM will not be requested when attaching to the LTE network.
      This means that the modem's NVS (Non-Volatile Storage) storage contents are ignored.

  * Updated:

    * The Kconfig option :kconfig:option:`CONFIG_LTE_EDRX_REQ` will now prevent the modem from requesting eDRX in case the option is disabled, in contrast to the previous behavior, where eDRX was requested even if the option was disabled (in the case where the modem has preserved requesting eDRX in its NVS storage).

* :ref:`at_cmd_custom_readme` library:

  * Updated:

    * Renamed the :c:macro:`AT_CUSTOM_CMD` macro to :c:macro:`AT_CMD_CUSTOM`.
    * Renamed the :c:func:`at_custom_cmd_respond` function to :c:func:`at_cmd_custom_respond`.

  * Removed:

    * Macros :c:macro:`AT_CUSTOM_CMD_PAUSED` and :c:macro:`AT_CUSTOM_CMD_ACTIVE`.
    * Functions :c:func:`at_custom_cmd_pause` and :c:func:`at_custom_cmd_active`.

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The :c:func:`nrf_modem_lib_bootloader_init` function to initialize the Modem library in bootloader mode.
    * The function :c:func:`nrf_modem_lib_fault_strerror` to retrieve a statically allocated textual description of a given modem fault.
      The function can be enabled using the new Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_FAULT_STRERROR`.

  * Updated:

    * The :c:func:`nrf_modem_lib_init` function now initializes the Modem library in normal operating mode only and the ``mode`` parameter is removed.
      Use the :c:func:`nrf_modem_lib_bootloader_init` function to initialize the Modem library in bootloader mode.
    * The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_SYS_INIT` is now deprecated.
      The application initializes the modem library using the :c:func:`nrf_modem_lib_init` function instead.
    * The :c:func:`nrf_modem_lib_shutdown` function now checks that the modem is in minimal functional mode (``CFUN=0``) before shutting down the modem.
      If not, a warning is given to the application, and minimal functional mode is set before calling the :c:func:`nrf_modem_shutdown` function.
    * The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_PRIO_OVERRIDE` is now deprecated.

  * Removed:

    * The deprecated function ``nrf_modem_lib_get_init_ret``.
    * The deprecated function ``nrf_modem_lib_shutdown_wait``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_TRACE_ENABLED``.

* :ref:`pdn_readme` library:

  * Updated the library to use ePCO mode if the Kconfig option :kconfig:option:`CONFIG_PDN_LEGACY_PCO` is not enabled.

  * Fixed:

    * A bug in the initialization of a new PDN context without a PDN event handler.
    * A memory leak in the :c:func:`pdn_ctx_create` function.

* :ref:`lte_lc_readme` library:

  * Updated:

    * Updated the library to handle notifications from the modem when eDRX is not used by the current cell.
      The application now receives an :c:enum:`LTE_LC_EVT_EDRX_UPDATE` event with the network mode set to :c:enum:`LTE_LC_LTE_MODE_NONE` in these cases.
      Modem firmware version v1.3.4 or newer is required to receive these events.
    * The Kconfig option :kconfig:option:`CONFIG_LTE_AUTO_INIT_AND_CONNECT` is now deprecated.
      The application calls the :c:func:`lte_lc_init_and_connect` function instead.
    * New events added to enumeration :c:enum:`lte_lc_modem_evt` for RACH CE levels and missing IMEI.

Libraries for networking
------------------------

* Added the :ref:`lib_nrf_cloud_log` library for logging to nRF Cloud.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * A public header file :file:`nrf_cloud_defs.h` that contains common defines for interacting with nRF Cloud and the :ref:`lib_nrf_cloud` library.
    * A new event :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR` to indicate an error while the transport connection is being established when the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD` Kconfig option is enabled.
      Earlier this was indicated with a second :c:enum:`NRF_CLOUD_EVT_TRANSPORT_CONNECTING` event with an error status.
    * A public header file :file:`nrf_cloud_codec.h` that contains encoding and decoding functions for nRF Cloud data.
    * Defines to enable parameters to be omitted from a P-GPS request.

  * Removed unused internal codec function ``nrf_cloud_format_single_cell_pos_req_json()``.

  * Updated:

    * The :c:func:`nrf_cloud_device_status_msg_encode` function now includes the service info when encoding the device status.
    * Renamed files :file:`nrf_cloud_codec.h` and :file:`nrf_cloud_codec.c` to :file:`nrf_cloud_codec_internal.h` and :file:`nrf_cloud_codec_internal.c` respectively.
    * Standardized encode and decode function names in the codec.
    * Moved the :c:func:`nrf_cloud_location_request_json_get` function from the :file:`nrf_cloud_location.h` file to :file:`nrf_cloud_codec.h`.
      The function is now renamed to :c:func:`nrf_cloud_location_request_msg_json_encode`.
    * Allow only one file download at a time within the library.
      MQTT-based FOTA, :kconfig:option:`CONFIG_NRF_CLOUD_FOTA`, has priority.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated:

    * The mask angle parameter can now be omitted from an A-GPS REST request by using the value ``NRF_CLOUD_AGPS_MASK_ANGLE_NONE``.
    * Use defines from the :file:`nrf_cloud_pgps.h` file for omitting parameters from a P-GPS request.
      Removed the following values: ``NRF_CLOUD_REST_PGPS_REQ_NO_COUNT``, ``NRF_CLOUD_REST_PGPS_REQ_NO_INTERVAL``, ``NRF_CLOUD_REST_PGPS_REQ_NO_GPS_DAY``, and ``NRF_CLOUD_REST_PGPS_REQ_NO_GPS_TOD``.
    * A-GPS request encoding now uses the common codec function and new nRF Cloud API format.

* :ref:`lib_lwm2m_client_utils` library:

  * Added:

    * Support for the connection pre-evaluation feature using the Kconfig option :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LTE_CONNEVAL`.

  * Updated:

    * :file:`lwm2m_client_utils.h` includes new API for FOTA to register application callback to receive state changes and requests for the update process.

  * Removed the old API ``lwm2m_firmware_get_update_state_cb()``.

* :ref:`lib_download_client` library:

  * Refactored the :c:func:`download_client_connect` function to :c:func:`download_client_set_host` and made it non-blocking.
  * Added the :c:func:`download_client_get` function that combines the functionality of functions :c:func:`download_client_set_host`, :c:func:`download_client_start`, and :c:func:`download_client_disconnect`.
  * Changed configuration from one security tag to a list of security tags.
  * Updated to report error ``ERANGE`` when HTTP range is requested but not supported by server.
  * Removed functions :c:func:`download_client_pause` and :c:func:`download_client_resume`.

* :ref:`lib_lwm2m_location_assistance` library:

  * Updated:

    * :file:`lwm2m_client_utils_location.h` includes new API for location assistance to register application callback to receive result codes from location assistance.
    * :file:`lwm2m_client_utils_location.h` by removing deprecated confirmable parameters from location assistance APIs.

* :ref:`pdn_readme` library:

  * Added:

    * ``PDN_EVENT_NETWORK_DETACH`` event to indicate a full network detach.

Libraries for NFC
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added

  * The :ref:`fem_al_lib` library for use in radio samples.

* :ref:`dk_buttons_and_leds_readme` library:

  * The library now supports using the GPIO expander for the buttons, switches, and LEDs on the nRF9160 DK.

* :ref:`app_event_manager` library:

  * Added :c:macro:`APP_EVENT_ID` macro.

* :ref:`event_manager_proxy` library:

  * Removed the ``remote_event_name`` argument from the :c:func:`event_manager_proxy_subscribe` function.

* :ref:`mod_memfault`:

  * Added support for the ETB trace to be included in coredump.

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

* Added:

  * :ref:`nrf_fuel_gauge`.
  * PSA core implementation provided by nrf_oberon PSA core.

* Updated:

  * Deprecated PSA core implementation from Mbed TLS.
  * Deprecated PSA driver implementation from Mbed TLS.


See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

DFU libraries
-------------

|no_changes_yet_note|

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`partition_manager`:

  * Fixed an issue that prevents an empty gap after a static partition for a region with the ``START_TO_END`` strategy.

* :ref:`nrf_desktop_config_channel_script`:

  * Added:

    * Support for the device information (``devinfo``) option fetching.
      The option provides device's Vendor ID, Product ID and generation.
    * Support for devices using MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``).
      In this mode, the image is booted directly from the secondary slot without moving it to the primary slot.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``6902abba270c0fbcbe8ee3bb56fe39bc9acc2774``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Support for the downgrade prevention feature using hardware security counters (:kconfig:option:`CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION`).
  * Generation of a new variant of the :file:`dfu_application.zip` when the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option is enabled.
    Mentioned archive now contains images for both slots, primary and secondary.
  * Encoding of the image start address into the header when the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option is enabled.
    The encoding is done using the ``--rom-fixed`` argument of the :file:`imgtool.py` script.
    If the currently running application also has the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option enabled, the MCUmgr will reject application image updates signed without the start address.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``4bbd91a9083a588002d4397577863e0c54ba7038``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 4bbd91a908 ^fcaa60a99f

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^4bbd91a908

The current |NCS| main branch is based on revision ``4bbd91a908`` of Zephyr.

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

  * A page on :ref:`ug_nrf70_developing_regulatory_support` in the :ref:`ug_nrf70_developing` user guide.
  * A section on :ref:`building Wi-Fi applications<thingy53_build_pgm_targets_wifi>` to the :ref:`ug_thingy53` page.

* Updated:

  * The structure of sections on the :ref:`known_issues` page.
    Known issues were moved around, but no changes were made to their description.
    The hardware-only sections were removed and replaced by the "Affected platforms" list.
  * The :ref:`software_maturity` page with details about Bluetooth feature support.
  * The :ref:`ug_nrf5340_gs`, :ref:`ug_thingy53_gs`, :ref:`ug_nrf52_gs`, and :ref:`ug_ble_controller` pages with a link to the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.
  * The :ref:`zigbee_weather_station_app` documentation to match the application template.
  * The :ref:`ug_nrf9160` page with a section about :ref:`external flash <nrf9160_external_flash>`.
  * The :ref:`nrf_modem_lib_readme` page with a section about :ref:`modem trace flash backend <modem_trace_flash_backend>`.

Moved:

  * The :ref:`mod_memfault` library documentation from :ref:`lib_others` to :ref:`lib_debug`.

Removed:

  * The section "Pointing the repositories to the right remotes after they were moved" from the :ref:`gs_updating` page.
