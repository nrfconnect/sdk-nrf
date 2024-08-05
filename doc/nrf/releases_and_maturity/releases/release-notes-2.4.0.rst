.. _ncs_release_notes_240:

|NCS| v2.4.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

The following list includes the summary of the most relevant changes introduced in this release.

* Added :ref:`support <software_maturity>` for the following features:

  * Matter:

    * Matter 1.1 specification and Matter over Wi-Fi®.
      See the `CSA blog post`_ to learn more about the Matter 1.1 updates.

  * Bluetooth® Low Energy:

    * Periodic Advertising with Responses (PAwR).
      The advertiser role feature was added as experimental in |NCS| v2.3.0, and now both advertiser and scanner are supported.

  * Bluetooth mesh:

    * :ref:`Mesh light dimmer and scene selector sample <bluetooth_mesh_light_dim>` to show how applications could be improved to align with upcoming Bluetooth Networked Lighting Control profiles.

  * Wi-Fi®:

    * Wi-Fi Station (STA) mode on the nRF5340 host platform.
    * Emulated boards for the nRF7000 and nRF7001 ICs on the nRF7002 :term:`Evaluation Kit (EK)`.
    * Emulated board for the nRF7001 IC on the nRF7002 :term:`Development Kit (DK)`.
    * nRF7002 EB (Expansion Board) for Thingy:53.
    * :ref:`Bluetooth LE coexistence <ug_radio_mpsl_cx_nrf700x>`, a mechanism for managing the Bluetooth LE and Wi-Fi radio operations, so that the two radios operating in close proximity do not cause interference to each other.
    * :ref:`Memfault sample <memfault_sample>` supported on nRF7002 DK, using Wi-Fi for uploading data to the Memfault cloud.

  * Cellular IoT:

    * :ref:`asset_tracker_v2` now supports Wi-Fi locationing using nF9160 DK combined with nRF7002 EK.

  * Security:

    * The Oberon PSA core library provides an optimized implementation of the PSA Crypto API, which reduces the required flash footprint when compared to the Mbed TLS PSA Crypto implementation.

* Added :ref:`experimental support <software_maturity>` for the following features:

  * PMIC: nPM1300 PMIC and nPM1300 EK:

    * Charger, BUCKs, LDOs, Load Switches, and GPIOs.
    * :ref:`Sample showing fuel gauge functionality <npm1300_fuel_gauge>`.
      The sample calculates battery state of charge, time to empty, time to full and provides updates of these over the terminal once every second.
    * :ref:`Sample providing a shell interface <zephyr:npm1300_ek_sample>` that supports PMIC features: regulators (BUCKs, LDO) and GPIOs.

  * Bluetooth Low Energy:

    * Encrypted Advertising Data (EAD).
      This is a new feature introduced in Bluetooth v5.4 specification that provides a standardized way of encrypting data in advertising as well as sharing key material, which is required for data encryption.

  * Bluetooth mesh 1.1:

    * Device Firmware Update (DFU) and BLOB Transfer models.
      This feature allows updating the firmware of mesh nodes through the Bluetooth mesh network.
    * Remote provisioning.
      This feature allows provisioning devices without being in direct RF range of the provisioner.
    * Private beacons.
      This feature improves privacy of secure network beacons.
    * Mesh Enhancements.
      This includes several mesh specification enhancements namely On-demand private GATT proxy, SAR configuration models, opcode aggravator models, large Composition Data models.
    * Enhanced Provisioning Authentication algorithm.
      This feature improves security of the provisioning process.

  * nRF Cloud library:

    * :ref:`lib_nrf_cloud_alert`, which enables applications to generate and transmit messages that comply with the alert features of nRF Cloud.
    * :ref:`lib_nrf_cloud_log`, which enables applications to generate and transmit messages that comply with the logs features of nRF Cloud.

* Improved:

  * Bluetooth mesh:

    * Updated the default configuration of advertising sets used by the Bluetooth mesh subsystem, which improves performance of the Relay and GATT features with increased throughput, and provides an improved Friend feature to reduce power consumption of low power node.
    * Mesh light fixture and mesh sensor samples to align with upcoming Bluetooth Networked Lighting Control profiles.

Sign up for the `nRF Connect SDK v2.4.0 webinar`_ to learn more about the new features.

See :ref:`ncs_release_notes_240_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.4.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.4.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.4.0.

:ref:`Toolchain Manager <gs_app_tcm>`, used to install the |NCS| automatically from `nRF Connect for Desktop`_, is available for Windows, Linux, and macOS.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v2.4.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.4.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.4.0`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_240_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

Build system
------------

When using the Kconfig option :kconfig:option:`CONFIG_SB_SIGNING_KEY_FILE` with relative paths, the relative path now points to the application configuration directory instead of the application source directory (these are the same if the application configuration directory is not set).

Security
========

Updated Mbed TLS version to 3.3.0.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

* Added:

  * Support for the vendor-specific HCI command: Set Compatibility mode for window offset.
  * Support for Periodic Advertising with Responses (PAwR) Scanner.
  * Support for LE Read and Write RF Path Compensation HCI commands.
  * Support for up to 255 addresses in the Filter Accept List.
  * Support for configuring the Filter Accept List to have an arbitrary size.
  * Support for sync handles in the :c:func:`sdc_hci_cmd_vs_zephyr_write_tx_power` and :c:func:`sdc_hci_cmd_vs_zephyr_read_tx_power` commands.
  * Support for reading channel map updates that are not at the beginning of an ACAD.

* Updated:

  * Periodic Advertising with Responses (PAwR) Advertiser is now supported.

For details, see the :ref:`SoftDevice Controller changelog <nrfxlib:softdevice_controller_changelog>`.

Bluetooth mesh
--------------

* Updated:

  * The protocol user guide with the information about the :ref:`dfu_over_bt_mesh`.
  * The default configuration of advertising sets used by the Bluetooth mesh subsystem to improve performance of the Relay, GATT, and Friend features.
    This configuration is specified in the :file:`ncs/nrf/subsys/bluetooth/mesh/Kconfig` file.

See `Bluetooth mesh samples`_ for the list of changes in the Bluetooth mesh samples.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Support for bigger payload size.
    ESB supports a payload of 64 bytes or more.
  * The :ref:`esb_fast_ramp_up` feature that reduces radio ramp-up delay from 130 µs to 40 µs.
  * The :kconfig:option:`CONFIG_ESB_NEVER_DISABLE_TX` Kconfig option as an experimental feature that enables the radio peripheral to remain in TXIDLE state instead of TXDISABLE when transmission is pending.
    For more information, see :ref:`esb_never_disable_tx`.

* Updated:

  * The number of PPI/DPPI channels used from three to six.
  * Events 6 and 7 from the EGU0 instance by assigning them to the ESB module.
  * The type parameter of the :c:func:`esb_set_tx_power` function to ``int8_t``.

Matter
------

* Added:

  * Support for Matter 1.1.0 version.
  * Support for the :ref:`ug_matter_configuring_optional_persistent_subscriptions` feature.
  * Support for negotiating subscription report interval with a Matter Controller to minimalize overall power consumption.
  * The Matter Nordic UART Service (NUS) feature to the :ref:`matter_lock_sample` sample.
    This feature allows using Nordic UART Service to control the device remotely through Bluetooth LE and adding custom text commands to a Matter sample.
    The Matter NUS implementation allows controlling the device regardless of whether the device is connected to a Matter network or not.
    The feature is dedicated for the Matter over Thread solution.
  * Documentation page about :ref:`ug_matter_device_configuring_cd`.
  * Matter SDK fork :ref:`documentation pages <matter_index>` with the page about CHIP Certificate Tool.
  * Documentation page about :ref:`ug_matter_device_adding_bt_services`.

* Updated:

  * Configuration of factory data partition write-protection on nRF5340 SoC.
  * Configuration of logging verbosity in the debug variants of samples.
  * Overall stability and robustness of Matter over Wi-Fi solution.
  * The default number of user RTC channels on the nRF5340 SoC's network core from 2 to 3 (the platform default) to fix the CSL transmitter feature on Matter over Thread devices acting as Thread routers.
  * :ref:`ug_matter_hw_requirements` with updated memory requirement values valid for the |NCS| v2.4.0.
  * :ref:`matter_lock_sample`, :ref:`matter_window_covering_sample`, and :ref:`matter_light_bulb_sample` samples as well as the :ref:`matter_weather_station_app` application have been tested with the following ecosystems:

    * Google Home ecosystem for both Matter over Thread and Matter over Wi-Fi solutions.
      Tested with Google Nest Hub 2\ :sup:`nd` generation (software version: 47.9.4.447810048; Chromecast firmware version: 1.56.324896, and Google Home mobile application v3.0.1.9).
    * Apple Home ecosystem for both Matter over Thread and Matter over Wi-Fi solutions.
      Tested with Apple HomePod mini and Apple iPhone (iOS v16.5).

    Additionally, these samples and application were positively verified against “Works with Google” certification tests.

  * The :ref:`ug_matter` protocol page with a table that lists compatibility versions for the |NCS|, the Matter SDK, and the Matter specification.
  * The :ref:`ug_matter_tools` page with installation instructions for the ZAP tool, moved from the :ref:`ug_matter_creating_accessory` page.
  * The :ref:`ug_matter_tools` page with information about CHIP Tool, CHIP Certificate Tool, and the Spake2+ Python tool.
  * The :ref:`ug_matter_device_low_power_configuration` page with information about sleepy active threshold parameter configuration and optimizing subscription report intervals.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.1.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added the ``SLEEPY_ACTIVE_THRESHOLD`` parameter that makes the Matter sleepy device stay awake for a specified amount of time after network activity.

* Updated:

  * The factory data generation script with the feature for generating the onboarding code.
    You can now use the factory data script to generate a manual pairing code and a QR Code that are required to commission a Matter-enabled device over Bluetooth LE.
    Generated onboarding codes should be put on the device's package or on the device itself.
    For details, see the Generating onboarding codes section on the :doc:`matter:nrfconnect_factory_data_configuration` page in the Matter documentation.
  * The Basic Information cluster with device finish and device color attributes and added the related entries in factory data set.

Thread
------

Thread is impacted by CVE-2023-2626.
Due to this issue, |NCS| v2.4.0 will not undergo the certification process, and is not intended to be used in final Thread products.

Zigbee
------

* Fixed an issue where device is not fully operational in the distributed network by adding a call to the :c:func:`zb_enable_distributed` function for Zigbee Router role by default.

HomeKit
-------

* Fixed a bug where the DFU over UARP protocol does not work on the nRF5340 SoC.

Wi-Fi
-----

* Added support for the following:

  * nRF7000 EK.
  * nRF7001 EK and nRF7001 DK.
  * A new shield, nRF7002 Evaluation Board (EB) for Thingy53 (rev 1.1.0).
  * Listen interval based power save.
    Refactored existing power save configurations into one.
  * Configuring antenna gain and band-edge power backoff.
  * nRF700x non-secure build.
  * Interoperation with CMW500 configured as Wi-Fi Access Point.
  * Configuring regulatory domain.
  * Raw scan results.

* Updated:

  * The shield for nRF7002 EK (``nrf7002_ek`` to ``nrf7002ek``).
  * TWT features with improvements (units for interval changed from milliseconds to microseconds).
  * The :ref:`ug_wifi` user guide by adding a section on :ref:`ug_wifi_certification`.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added the integration of the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.
* Updated:

  * ``mcuboot_secondary`` is moved to external flash for nRF9160 DK v0.14.0 and newer.
    This requires board controller firmware v2.0.1 or newer, which enables the pin routing to external flash.
  * The application now uses the function :c:func:`nrf_cloud_location_request_msg_json_encode` to create an nRF Cloud location request message.
  * The application now uses defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.
  * Instead of sending a battery voltage, the PMIC's fuel gauge function is used to get a battery percentage.
    For nRF Cloud, the data ID ``VOLTAGE`` has been replaced with ``BATTERY``.
    For the other cloud backends, the name stays the same, but the range changes to 0-100.
  * External flash is enabled in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * AT command ``#XWIFIPOS`` to get Wi-Fi location from nRF Cloud.
  * Support for *WRITE REQUEST* in TFTP client.

* Updated the application to use defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.
* Fixed a bug in receiving a large MQTT Publish message.

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
  * Set the max compiled-in log level to ``warning`` for the Bluetooth HCI core (:kconfig:option:`CONFIG_BT_HCI_CORE_LOG_LEVEL_CHOICE`).
    This is done to avoid flooding logs during application boot.
  * Aligned the nRF52833 dongle's board DTS configuration files and nRF Desktop's application-specific DTS overlays to hardware revision 0.2.1.
  * The documentation with debug Fast Pair provisioning data obtained for development purposes.

nRF5340 Audio
-------------

* Added Kconfig options for setting periodic and extended advertising intervals.
  For more information on the options, see all options prefixed with ``CONFIG_BLE_ACL_PER_ADV_INT_`` and ``CONFIG_BLE_ACL_EXT_ADV_INT_``.
* Updated:

  * LE Audio controller for the network core has been moved to the standalone LE Audio controller for nRF5340 library.
  * :ref:`zephyr:zbus` is now implemented for handling events from buttons and LE Audio.
  * The supervision timeout has been reduced to reduce reconnection times for CIS.
  * The application documentation with a note about missing support for the |nRFVSC|.

* Fixed an issue related to anomaly 160 in nRF5340.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated:

  * The machine learning models (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) used by the application to ensure compatibility with the new Zephyr version.
  * The over-the-air (OTA) device firmware update (DFU) configuration of nRF53 DK has been simplified.
    The configuration relies on the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

Samples
=======

Bluetooth samples
-----------------

* :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples:

  * Updated the samples to register HID Service before Bluetooth is enabled (before calling the :c:func:`bt_enable` function).
    The :c:func:`bt_gatt_service_register` function can no longer be called after enabling Bluetooth and before loading settings.

* :ref:`peripheral_hids_mouse` sample:

  * Updated the sample to include the :kconfig:option:`CONFIG_BT_SMP` Kconfig option when ``CONFIG_BT_HIDS_SECURITY_ENABLED`` is selected.
  * Fixed a CMake warning by moving the nRF RPC configuration (the :kconfig:option:`CONFIG_NRF_RPC_THREAD_STACK_SIZE` Kconfig option) to a separate overlay config file.

* :ref:`direct_test_mode` sample:

  * Added:

    * Support for the :ref:`nrfxlib:mpsl_fem` TX power split feature.
      The DTM command ``0x09`` for setting the transmitter power level takes into account the front-end module gain when this sample is built with support for front-end modules.
      The vendor-specific commands for setting the SoC output power and the front-end module gain are not available when the :ref:`CONFIG_DTM_POWER_CONTROL_AUTOMATIC <CONFIG_DTM_POWER_CONTROL_AUTOMATIC>` Kconfig option is enabled.
    * Support for +1 dBm, +2 dBm, and +3 dBm output power on the nRF5340 DK.

  * Updated the handling of the hardware erratas.
  * Removed a compilation warning when used with minimal pinout Skyworks FEM.

* :ref:`peripheral_uart` sample:

  * Fixed the unit of the ``CONFIG_BT_NUS_UART_RX_WAIT_TIME`` Kconfig option to comply with the UART API.

* :ref:`nrf_dm` sample:

  * Updated the sample by improving the scalability when it is used with multiple devices.

* Bluetooth: Fast Pair sample:

  * Added the default Fast Pair provisioning data that is used when no other provisioning data is specified.
  * Updated the documentation to align it with the new way of displaying notifications for the Fast Pair debug Model IDs.

* Removed the Bluetooth: External radio coexistence using 3-wire interface sample because of the removal of the 3-wire implementation.

Bluetooth mesh samples
----------------------

* Added:

  * Samples :ref:`ble_mesh_dfu_target` and :ref:`ble_mesh_dfu_distributor` that can be used for evaluation of the Bluetooth mesh DFU specification and subsystem.
  * :ref:`bluetooth_mesh_light_dim` sample that demonstrates how to set up a light dimmer and scene selector application.

* Updated:

  * The configuration of advertising sets in all samples to match the new default values.
    See `Bluetooth mesh`_ for more information.
  * The link layer payload size over GATT communication from default 27 to maximum 37 bytes (maximum Bluetooth mesh advertiser data) for all samples.
    This was done to avoid fragmentation of outgoing messages to the proxy client.

* Removed the :file:`hci_rpmsg.conf` file from all samples that support nRF5340 DK or Thingy:53.
  This configuration is moved to the :file:`ncs/nrf/subsys/bluetooth/mesh/hci_rpmsg_child_image_overlay.conf` file.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added support for the Scene Server model.
  * Updated to demonstrate the use of a Sensor Server model to report additional useful information about the device.

* :ref:`bluetooth_mesh_sensor_server` and :ref:`bluetooth_mesh_sensor_client` samples:

  * Added:

    * Support for motion threshold as a setting for the presence detection.
    * Support for ambient light level sensor.
    * Shell support to :ref:`bluetooth_mesh_sensor_client` sample.

* :ref:`bluetooth_mesh_sensor_server`, :ref:`bluetooth_mesh_light_lc` and :ref:`bluetooth_mesh_light_dim` samples:

  * Updated to demonstrate the Bluetooth :ref:`ug_bt_mesh_nlc`.

Crypto samples
--------------

* Updated all samples and their documentation as the supported logging backend changed to UART instead of both SEGGER RTT and UART.

Debug samples
-------------

* :ref:`memfault_sample` sample:

  * Added:

    * Support for the nRF7002 DK.
    * A Kconfig fragment to enable ETB trace.

  * Updated:

    * The sample has been moved from :file:`nrf9160/memfault` to :file:`debug/memfault`.
      The documentation is now found in the :ref:`debug_samples` section.

Matter samples
--------------

* Updated:

  * The default settings partition size for all Matter samples from 16 kB to 32 kB.

    .. caution::
       This change can affect the Device Firmware Update (DFU) from the older firmware versions that were using the 16-kB settings size.
       Read more about this in the :ref:`ug_matter_device_bootloader_partition_layout` section of the Matter documentation.
       You can still perform DFU from the older firmware version to the latest firmware version, but you will have to change the default settings size from 32 kB to the value used in the older version.

 * :ref:`matter_lock_sample` sample:

  * Added the Matter Nordic UART Service (NUS) feature, which allows controlling the door lock device remotely through Bluetooth LE using two simple commands: ``Lock`` and ``Unlock``.
    This feature is dedicated for the nRF52840 and the nRF5340 DKs.
    The sample supports one Bluetooth LE connection at a time.
    Matter commissioning, DFU, and NUS over Bluetooth LE must be run separately.
  * Updated the default value of the feature map.

* :ref:`matter_light_bulb_sample` sample:

  * Added handling of TriggerEffect command.

* :ref:`matter_window_covering_sample` sample:

  * Updated by aligning default values of Window Covering attributes with the specification.

Multicore samples
-----------------

* ``multicore_hello_world`` sample:

  * Added :ref:`zephyr:sysbuild` support to the sample.
  * Updated the sample documentation by renaming it as ``multicore_hello_world`` from nRF5340: Multicore application and moved it from :ref:`nrf5340_samples` to Multicore samples.

nRF9160 samples
---------------

* :ref:`http_modem_full_update_sample` sample:

  * The sample now uses modem firmware versions 1.3.3 and 1.3.4.
  * Enabled external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

* :ref:`http_modem_delta_update_sample` sample:

  * The sample now uses modem firmware v1.3.4 to do a delta update.

* :ref:`modem_shell_application` sample:

  * Added sending of GNSS data to carrier library when the library is enabled.

  * Updated:

    * The sample now uses defines from the :ref:`lib_nrf_cloud` library for string values related to nRF Cloud.
      Removed the inclusion of the file :file:`nrf_cloud_codec.h`.
    * Modem FOTA now updates the firmware without rebooting the application.
    * External flash is now enabled in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.
    * Renamed :file:`overlay-modem-trace.conf` to :file:`overlay-modem-trace-flash.conf`.

* :ref:`https_client` sample:

  * Added IPv6 support and wait time for PDN to fully activate (including IPv6, if available) before looking up the address.

* :ref:`slm_shell_sample` sample:

  * Added support for the nRF7002 DK (PCA10143).

* :ref:`lwm2m_client` sample:

  * Added:

   * The integration of the connection pre-evaluation functionality using the :ref:`lib_lwm2m_client_utils` library.
   * Support for experimental Advanced Firmware Update object.
     See :ref:`lwm2m_client_fota`.

  * Updated:

    * The sample now integrates the :ref:`lib_lwm2m_client_utils` FOTA callback functionality.
    * External flash is now enabled in the nRF9160 DK devicetree overlays for v0.14.0 or later versions (it is now disabled in the Zephyr board definition).

* :ref:`pdn_sample` sample:

  * Updated the sample to show how to get interface address information using the :c:func:`nrf_getifaddrs` function.

* :ref:`nrf_cloud_multi_service` sample:

  * Updated:

    * The MCUboot partition size is increased to the minimum necessary to allow bootloader FOTA.
    * External flash is enabled in the nRF9160 DK devicetree overlays for v0.14.0 or later versions (it is now disabled in the Zephyr board definition).

  * Added:

    * Sending of log messages directly to nRF Cloud.
    * Overlay to enable `Zephyr Logging`_ backend for full logging to nRF Cloud.

* :ref:`nrf_cloud_rest_fota` sample:

    * Updated the sample by enabling external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added:

    * Overlays to use RTT instead of UART for testing purposes.
    * Sending of log messages directly to nRF Cloud.
    * Overlay to enable `Zephyr Logging`_ backend for full logging to nRF Cloud.

  * Updated the Hello World message sent to nRF Cloud, which now contains a timestamp (message ID).

* :ref:`modem_trace_flash` sample:

  * Updated the sample by enabling external flash in the nRF9160 DK devicetree overlays for v0.14.0 or later versions, as it is now disabled in the Zephyr board definition.

PMIC samples
------------

* Added :ref:`npm1300_fuel_gauge` sample that demonstrates how to calculate the battery state of charge using the :ref:`nrfxlib:nrf_fuel_gauge`.

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`provisioning_image` sample:

  * Updated by moving the network core logic to the new sample :ref:`provisioning_image_net_core` instead of being a Zephyr module.

Other samples
-------------

* :ref:`ei_wrapper_sample` sample:

  * Updated the machine learning model (:kconfig:option:`CONFIG_EDGE_IMPULSE_URI`) to ensure compatibility with the new Zephyr version.

* :ref:`radio_test` sample:

  * Added a workaround for the following:

    * Hardware `Errata 254`_ of the nRF52840 SoC .
    * Hardware `Errata 255`_ of the nRF52833 SoC.
    * Hardware `Errata 256`_ of the nRF52820 SoC.
    * Hardware `Errata 257`_ of the nRF52811 SoC.
    * Hardware `Errata 117`_ of the nRF5340 SoC.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added nRF Wi-Fi driver.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* Added the standalone LE Audio controller for nRF5340 library, originally a part of the :ref:`nrf53_audio_app` application.
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

    * The salt size in the Fast Pair not discoverable advertising from 1 byte to 2 bytes, to align with the Fast Pair specification update.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON` Kconfig option is now the default Fast Pair cryptographic backend.

* :ref:`nrf_bt_scan_readme` library:

  * Fixed the output arguments of the :c:func:`bt_scan_filter_status_get` function.
    The :c:member:`bt_filter_status.manufacturer_data.enabled` field is now correctly set to reflect the status of the filter when the function is called.

Debug libraries
---------------

* Added the :ref:`etb_trace` library for instruction traces.

Modem libraries
---------------

* :ref:`lte_lc_readme` library:

  * Added the Kconfig option :kconfig:option:`CONFIG_LTE_PSM_REQ` that automatically requests PSM on modem initialization.
    If this option is disabled, PSM will not be requested when attaching to the LTE network.
    This means that the modem's NVS (Non-Volatile Storage) contents are ignored.

  * Updated:

    * The Kconfig option :kconfig:option:`CONFIG_LTE_EDRX_REQ`.
      The option will now prevent the modem from requesting eDRX in case the option is disabled, in contrast to the previous behavior, where eDRX was requested even if the option was disabled (in the case where the modem has preserved requesting eDRX in its non-volatile storage).
    * The library to handle notifications from the modem when eDRX is not used by the current cell.
      The application now receives an :c:enum:`LTE_LC_EVT_EDRX_UPDATE` event with the network mode set to :c:enum:`LTE_LC_LTE_MODE_NONE` in these cases.
      Modem firmware version v1.3.4 or newer is required to receive these events.
    * The Kconfig option ``CONFIG_LTE_AUTO_INIT_AND_CONNECT`` is now deprecated.
      The application calls the :c:func:`lte_lc_init_and_connect` function instead.
    * New events added to enumeration :c:enum:`lte_lc_modem_evt` for RACH CE levels and missing IMEI.

* :ref:`at_cmd_custom_readme` library:

  * Updated:

    * The :c:macro:`AT_CUSTOM_CMD` macro has been renamed to :c:macro:`AT_CMD_CUSTOM`.
    * The :c:func:`at_custom_cmd_respond` function  has been renamed to  :c:func:`at_cmd_custom_respond`.

  * Removed:

    * The macros :c:macro:`AT_CUSTOM_CMD_PAUSED` and :c:macro:`AT_CUSTOM_CMD_ACTIVE`.
    * The functions :c:func:`at_custom_cmd_pause` and :c:func:`at_custom_cmd_active`.

* :ref:`nrf_modem_lib_readme`:

  * Added:

    * The :c:func:`nrf_modem_lib_bootloader_init` function to initialize the Modem library in bootloader mode.
    * The function :c:func:`nrf_modem_lib_fault_strerror` to retrieve a statically allocated textual description of a given modem fault.
      The function can be enabled using the new Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_FAULT_STRERROR`.

  * Updated:

    * The :c:func:`nrf_modem_lib_init` function now initializes the Modem library in normal operating mode only and the ``mode`` parameter is removed.
      Use the :c:func:`nrf_modem_lib_bootloader_init` function to initialize the Modem library in bootloader mode.
    * The Kconfig option ``CONFIG_NRF_MODEM_LIB_SYS_INIT`` is now deprecated.
      The application initializes the modem library using the :c:func:`nrf_modem_lib_init` function instead.
    * The :c:func:`nrf_modem_lib_shutdown` function now checks that the modem is in minimal functional mode (``CFUN=0``) before shutting down the modem.
      If not, a warning is given to the application, and minimal functional mode is set before calling the :c:func:`nrf_modem_shutdown` function.
    * The Kconfig option ``CONFIG_NRF_MODEM_LIB_IPC_PRIO_OVERRIDE`` is now deprecated.

  * Removed:

    * The deprecated function ``nrf_modem_lib_get_init_ret``.
    * The deprecated function ``nrf_modem_lib_shutdown_wait``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_TRACE_ENABLED``.

* :ref:`pdn_readme` library:

  * Updated the library to use ePCO mode if the Kconfig option :kconfig:option:`CONFIG_PDN_LEGACY_PCO` is not enabled.

  * Fixed:

    * A bug in the initialization of a new PDN context without a PDN event handler.
    * A memory leak in the :c:func:`pdn_ctx_create` function.

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

  * Updated:

    * The :c:func:`nrf_cloud_device_status_msg_encode` function now includes the service info when encoding the device status.
    * The files :file:`nrf_cloud_codec.h` and :file:`nrf_cloud_codec.c` have been renamed to :file:`nrf_cloud_codec_internal.h` and :file:`nrf_cloud_codec_internal.c` respectively.
    * Encode and decode function names have been standardized in the codec.
    * The :c:func:`nrf_cloud_location_request_json_get` function has been moved from the :file:`nrf_cloud_location.h` file to :file:`nrf_cloud_codec.h`.
      The function is now renamed to :c:func:`nrf_cloud_location_request_msg_json_encode`.
    * The library to allow only one file download at a time.
      MQTT-based FOTA, :kconfig:option:`CONFIG_NRF_CLOUD_FOTA`, has priority.

  * Removed unused internal codec function ``nrf_cloud_format_single_cell_pos_req_json()``.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated:

    * The mask angle parameter can now be omitted from an A-GPS REST request by using the value ``NRF_CLOUD_AGPS_MASK_ANGLE_NONE``.
    * The library now uses defines from the :file:`nrf_cloud_pgps.h` file for omitting parameters from a P-GPS request.
      Removed the following values:

      * ``NRF_CLOUD_REST_PGPS_REQ_NO_COUNT``
      * ``NRF_CLOUD_REST_PGPS_REQ_NO_INTERVAL``
      * ``NRF_CLOUD_REST_PGPS_REQ_NO_GPS_DAY``
      * ``NRF_CLOUD_REST_PGPS_REQ_NO_GPS_TOD``

    * A-GPS request encoding now uses the common codec function and new nRF Cloud API format.

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for the connection pre-evaluation feature using the Kconfig option :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LTE_CONNEVAL`.
  * Updated the file :file:`lwm2m_client_utils.h`, which includes new API for FOTA to register application callback to receive state changes and requests for the update process.
  * Removed the old API ``lwm2m_firmware_get_update_state_cb()``.

* :ref:`lib_download_client` library:

  * Added the :c:func:`download_client_get` function that combines the functionality of functions :c:func:`download_client_set_host`, :c:func:`download_client_start`, and :c:func:`download_client_disconnect`.

  * Updated:

    * The ``download_client_connect`` function has been refactored to :c:func:`download_client_set_host` and made it non-blocking.
    * The configuration from one security tag to a list of security tags.
    * The library reports error ``ERANGE`` when HTTP range is requested but not supported by server.

  * Removed functions ``download_client_pause()`` and ``download_client_resume()``.

* :ref:`lib_lwm2m_location_assistance` library:

  * Updated:

    * :file:`lwm2m_client_utils_location.h` includes new API for location assistance to register application callback to receive result codes from location assistance.
    * :file:`lwm2m_client_utils_location.h` by removing deprecated confirmable parameters from location assistance APIs.

* :ref:`pdn_readme` library:

  * Added the event ``PDN_EVENT_NETWORK_DETACH`` to indicate a full network detach.

Other libraries
---------------

* Added the :ref:`fem_al_lib` library for use in radio samples.

* :ref:`dk_buttons_and_leds_readme` library:

  * Updated the library to support using the GPIO expander for the buttons, switches, and LEDs on the nRF9160 DK.

* :ref:`app_event_manager` library:

  * Added the :c:macro:`APP_EVENT_ID` macro.

* :ref:`event_manager_proxy` library:

  * Removed the ``remote_event_name`` argument from the :c:func:`event_manager_proxy_subscribe` function.

* :ref:`mod_memfault` library:

  * Added support for the ETB trace to be included in coredump.

sdk-nrfxlib
-----------

* Added:

  * New library :ref:`nrf_fuel_gauge`.
  * PSA core implementation provided by nrf_oberon PSA core.

* Updated:

  * Deprecated PSA core implementation from Mbed TLS.
  * Deprecated PSA driver implementation from Mbed TLS.

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

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
    This archive file contains images for both slots, primary and secondary.
  * Encoding of the image start address into the header when the :kconfig:option:`CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT` Kconfig option is enabled.
    The encoding is done using the ``--rom-fixed`` argument of the :file:`imgtool.py` script.
    If the currently running application also has the :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT` Kconfig option enabled, the MCUmgr rejects application image updates signed without the start address.

* Updated:

  * Serial recovery is now able to return hash of image slots, when :kconfig:option:`CONFIG_BOOT_SERIAL_IMG_GRP_HASH` is enabled.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``4bbd91a9083a588002d4397577863e0c54ba7038``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 4bbd91a908 ^e1e06d05fa

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^4bbd91a908

The current |NCS| main branch is based on revision ``4bbd91a908`` of Zephyr.

Trusted Firmware-M
==================

* Updated TF-M version to 1.7.0.

Documentation
=============

* Added:

  * A page on :ref:`ug_nrf70_developing_regulatory_support` in the :ref:`ug_nrf70_developing` user guide.
  * New sample categories :ref:`pmic_samples`, :ref:`debug_samples`, Multicore samples, and :ref:`networking_samples`.

* Updated:

  * The structure of sections on the :ref:`known_issues` page.
    Known issues were moved around, but no changes were made to their description.
    The hardware-only sections were removed and replaced by the "Affected platforms" list.
  * The :ref:`software_maturity` page with details about Bluetooth feature support.
  * The :ref:`ug_nrf5340_gs`, :ref:`ug_thingy53_gs`, :ref:`ug_nrf52_gs`, and :ref:`ug_ble_controller` guides by adding a link to the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.
  * The :ref:`zigbee_weather_station_app` application documentation to match the application template.
  * The :ref:`ug_nrf9160` guide, relevant application and sample documentation with a section about :ref:`external flash <nrf9160_external_flash>`.
  * The :ref:`nrf_modem_lib_readme` library documentation with a section about :ref:`modem trace flash backend <modem_trace_flash_backend>`.
  * The :ref:`mod_memfault` library documentation has been moved from :ref:`lib_others` to :ref:`lib_debug`.
  * The :ref:`mqtt_sample` sample documentation has been moved from nRF9160 samples to :ref:`networking_samples`.
  * The :ref:`ug_thingy53` guide by adding a section on :ref:`building Wi-Fi applications <thingy53_build_pgm_targets_wifi>`.
  * The :ref:`ug_bt_mesh_configuring` page with additional configuration options used to configure the behavior and performance of a Bluetooth mesh network.

* Removed the section "Pointing the repositories to the right remotes after they were moved" from the :ref:`gs_updating` page.
