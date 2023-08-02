.. _ncs_release_notes_220:

|NCS| v2.2.0 Release Notes
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

  * Connected AoA transmitter.
  * Wi-Fi location in nRF Cloud Location Services.

* Added :ref:`experimental support <software_maturity>` for the following features:

  * :ref:`Distance Measurement vendor models <bt_mesh_dm_readme>` supporting distance measurement between Bluetooth mesh devices.
  * LE Power Control Request, which enables a device to request a change in TX power to a peer device.
  * Periodic Advertising Sync Transfer (only Sending supported).
  * TCP/IP and TLS over Thread.
  * ANT on nRF5340.
  * Power optimizing AS Release Assistance Indication (RAI) feature with :ref:`LwM2M <lwm2m_client_utils_additional_confg>` with samples and documentation for use on supported cellular networks.
  * Arm Platform Security Architecture best practice on nRF5340 and nRF9160.
    Includes new samples:

    * :ref:`provisioning_image` sample for production line provisioning of keys and advancing lifecycle states.
    * :ref:`tfm_psa_template` sample demonstrating secure application design using Trusted Firmware-M, nRF Secure Immutable Bootloader, and MCUboot.

  * :ref:`Target Wake Time (TWT) feature <wifi_shell_sample>` in the Wi-Fi driver for nRF7002.

* Improved:

  * Predicted GPS: Location object split into different datasets for improved flexibility and a new API in the location library for predictive GPS location.
  * Neighbor cell measurement functionality for single-cell and multi-cell location services using Modem firmware v1.3.4. Customers can optimize for lower power consumption or performance.
  * The default advertiser implementation in Bluetooth mesh subsystem by changing it from :file:`adv_legacy.c` to :file:`adv_ext.c`.
    This provides performance and reliability improvements in terms of reduced latency and increased relay throughput under high network load.
  * Enabling Power Envelope Control with hardware including nRF21540 to flatten the output power over temperature and frequency.

See :ref:`ncs_release_notes_220_changelog` for the complete list of changes.

Sign up for the `nRF Connect SDK v2.2.0 webinar`_ to learn more about the new features.

The official nRF Connect for VS Code extension also received improvements.
See the `latest release notes for nRF Connect for Visual Studio Code`_ for more information.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.2.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.2.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.2.0.
SEGGER Embedded Studio Nordic Edition is no longer tested or expected to work with |NCS| v2.2.0.

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
     - `Changelog <Modem library changelog for v2.2.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.2.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.2.0`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_220_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Updated:

  * The :ref:`software_maturity` page with a section about API deprecation.
  * The :ref:`app_boards` page with a section about processing environments.
    Also, updated terminology across the documentation to avoid the use of "secure domain" and "non-secure domain" when referring to the adoption of Cortex-M Security Extensions for the ``_ns`` build targets.

RF Front-End Modules
--------------------

* Added the nRF21540 GPIO+SPI built-in power model that keeps the nRF21540's gain constant and as close to the currently configured value of gain as possible.

Build system
------------

* Fixed:

  * An issue with the |NCS| Toolchain where ``protoc`` and ``nanopb`` would not be correctly detected by the build system, resulting in builds trying to find locally installed versions instead of the version shipped with the |NCS| Toolchain.
  * An issue with passing quoted settings to child images.

Wi-Fi
-----

See `Wi-Fi samples`_ for details about how to use Wi-Fi in your application.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

* Added:

  * Support for :c:func:`hci_driver_close`, so :c:func:`bt_disable` can now be used to disable the :ref:`softdevice_controller`.
  * Kconfig option :kconfig:option:`CONFIG_BT_UNINIT_MPSL_ON_DISABLE`.
    When enabled, it uninitializes MPSL when :c:func:`bt_disable` is used.
    This releases all peripherals used by the MPSL.
  * Support for Connection CTE Response in the angle of arrival (AoA) configuration.
  * Support for LE Set Data Related Address Changes HCI command.
  * Support for changing advertising randomness using :c:func:`sdc_hci_cmd_vs_set_adv_randomness`.
  * Support for enabling the sending of Periodic Advertising Sync Transfer (PAST) using dedicated functions such as :c:func:`sdc_support_periodic_adv_sync_transfer_sender_central`.
  * Experimental support for the LE Power Control Request feature.

For details, see the :ref:`SoftDevice Controller changelog <nrfxlib:softdevice_controller_changelog>`.

Bluetooth mesh
--------------

* Added:

  * Documentation pages:

    * :ref:`ug_bt_mesh_fota`.
    * :ref:`ug_bt_mesh_node_removal`.

Also, see `Bluetooth mesh samples`_ for the list of changes.

Enhanced ShockBurst (ESB)
-------------------------

* Added the :kconfig:option:`CONFIG_ESB_DYNAMIC_INTERRUPTS` Kconfig option to enable direct dynamic interrupts.

nRF IEEE 802.15.4 radio driver
------------------------------

* Added:

  * Functionality where Radio trim values are reapplied after a ``POWER`` register write as a workaround for the hardware Errata 158 of the nRF5340 chip.
  * API that allows for Coordinated Sampled Listening (CSL) Phase calculation based on an absolute anchor time and CSL Period.

Matter
------

* Added the following documentation pages:

    * :ref:`ug_matter_overview_dfu`.
    * :ref:`ug_matter_overview_multi_fabrics` and entry about binding to :ref:`ug_matter_network_topologies_concepts`.
    * :ref:`ug_matter_overview_commissioning`, which is based on an earlier subsection of :ref:`ug_matter_overview_network_topologies`.
    * :ref:`ug_matter_overview_security`, which is based on an earlier subsection of :ref:`ug_matter_overview_network_topologies`.
    * :ref:`ug_matter_device_certification_reqs_security` on the page about :ref:`ug_matter_device_certification`.

* Updated:

  * :ref:`ug_matter_device_certification` with several new sections that provide an overview of the certification process.
  * :ref:`ug_matter_overview_int_model` with an example of the interaction.
  * :ref:`ug_matter_overview_data_model` with an example of the Data Model of a door lock device.
  * :ref:`ug_matter_gs_adding_cluster` documentation with new code snippets to align it with the source code of refactored Matter template sample.
  * :ref:`ug_matter_hw_requirements` with the latest RAM and flash memory requirements.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``bc6b43882a56ddb3e94d3e64956bd5f3292b4058``.

Thread
------

* Added experimental TCP support as required by Thread 1.3 Specification.

See `Thread samples`_ for the list of changes for the Thread samples.

Wi-Fi
-----

See `Wi-Fi samples`_ for the list of changes for the Wi-Fi samples.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Handling for the new data receive events in the :ref:`lib_nrf_cloud` library.
    This is a major change in the application code but a minor change from the user perspective.
  * :ref:`asset_tracker_v2_location_module`.
    GNSS is used through the :ref:`lib_location` library.

* Updated:

  * The application now uses the new LwM2M location assistance objects through the :ref:`lib_lwm2m_location_assistance` library.
  * The application now uses passive mode as the default mode for Thingy:91 builds.
  * The application now uses the :ref:`lib_location` library for retrieving location information.
    This is a major change in the application code but a minor change from the user perspective.
  * Neighbor cell handling moved from :ref:`asset_tracker_v2_modem_module` to :ref:`asset_tracker_v2_location_module` to be used through Location library.
  * The :ref:`asset_tracker_v2_location_module` triggers a :c:func:`location_request` with GNSS being the first priority method and cellular the second priority if they are enabled in the application configuration.
  * A-GPS/P-GPS are not requested based on triggers in the application but only based on :ref:`lib_location` library events :c:enum:`LOCATION_EVT_GNSS_ASSISTANCE_REQUEST` and :c:enum:`LOCATION_EVT_GNSS_PREDICTION_REQUEST`.
  * Currently, you cannot configure or define the LTE LC neighbor cell search type with the :ref:`lib_location` library.
    The default search type is always used.
  * The Kconfig option :kconfig:option:`CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION` (calling :c:func:`nrf_cloud_pgps_set_location`) is not supported in the :ref:`lib_location` library.

* Removed:

  * GNSS module.
  * A-GPS and P-GPS processing.
    It is now handled by the :ref:`lib_nrf_cloud` library.
  * NMEA support.
    This was there only because originally, nRF Cloud did not support PVT but that has changed.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * Optional data modem flow control Kconfig option :ref:`CONFIG_SLM_DATAMODE_URC <CONFIG_SLM_DATAMODE_URC>`.
  * Handling for the new data receive events in the :ref:`lib_nrf_cloud` library.

* Updated the service info JSON payload to use ``GNSS`` instead of ``GPS``.
* Removed automatic quit of data mode in GNSS, FTP, and HTTP services.

nRF5340 Audio
-------------

* Added:

  * Kconfig options for different sample rates and BAP presets.
  * Bidirectional mode for the CIS mode.
  * A :ref:`walkie talkie demo <nrf53_audio_app_configuration_enable_walkie_talkie>` for bidirectional CIS.
  * Minimal Media Control Service (MCS) functionality to the Play/Pause button.
  * Coordinated Set Identification Service (CSIS) for the CIS headset.
  * Functionality for supporting multiple streams on BIS headsets.

* Updated:

  * LE Audio Controller Subsystem for nRF53 from version 3303 to version 3307.
    This version provides improved Android compatibility.

* Fixed:

  * An issue with the figure for :ref:`nrf53_audio_app_overview_architecture_i2s` in the :ref:`nrf53_audio_app` documentation.
    The figure now correctly shows the interaction with the Bluetooth modules.
  * An issue with Simple Management Protocol (SMP) not advertising in the CIS mode.
  * An issue with the ``mcumgr`` command being unable to receive in the BIS mode.
  * The :ref:`nrf53_audio_app_porting_guide` section in the documentation did not mention long-pressing **BTN 4** while resetting the development kit to start DFU.
    This has now been added to the documentation.

* Removed support for the nRF5340 Audio DK (PCA10121) board version 0.7.1 or older.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Removed the support for Thingy:52 (``thingy52_nrf52832``).

nRF Desktop
-----------

* Added:

  * :ref:`nrf_desktop_fast_pair_app`.
    The module is used in configurations that integrate Google `Fast Pair`_.
  * :ref:`CONFIG_DESKTOP_LED_STATE_DEF_PATH <config_desktop_app_options>`.
    The option can be used to specify the file defining the used LED effects.
  * The application configurations that enable `Fast Pair`_.
    See nRF Desktop's :ref:`nrf_desktop_bluetooth_guide_fast_pair` documentation section for details.

  * :ref:`CONFIG_DESKTOP_USB_REMOTE_WAKEUP <config_desktop_app_options>` Kconfig option for :ref:`nrf_desktop_usb_state`.
    The option enables the USB wakeup functionality in the application.
    The option selects :kconfig:option:`CONFIG_USB_DEVICE_REMOTE_WAKEUP`.

  * Application-specific Kconfig options to simplify the configuration.
    Part of an application Kconfig configuration that is common for the selected HID device type is introduced as overlays for default Kconfig values.
    See :ref:`nrf_desktop_porting_guide` for details.

* Updated:

  * The ``CONFIG_DESKTOP_BLE_SCANNING_ENABLE`` Kconfig option has been renamed to :ref:`CONFIG_DESKTOP_BLE_SCAN_ENABLE <config_desktop_app_options>`.
  * The UUID16 values of GATT Human Interface Device Service (HIDS) and GATT Battery Service (BAS) have been moved from advertising data to scan response data.
  * The :kconfig:option:`CONFIG_BT_GATT_SERVICE_CHANGED` Kconfig option is disabled on nRF Desktop dongles to reduce memory footprint.
  * The :kconfig:option:`CONFIG_BT_ID_UNPAIR_MATCHING_BONDS` is now enabled by default.
    This is done to pass the Fast Pair Validator's end-to-end integration tests and to improve the user experience during the erase advertising procedure.
  * The :kconfig:option:`CONFIG_BT_ID_ALLOW_UNAUTH_OVERWRITE` Kconfig option is enabled by default for the HID peripherals.
    This setting improves the user experience as it is no longer required to delete the bonding information from the old identity to pair using the new one.


Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added the :ref:`peripheral_cgms` sample.

* :ref:`ble_throughput` sample:

  * Added terminal commands for selecting the role.
  * Updated the ASCII art used for showing progress to feature the current Nordic Semiconductor logo.

* :ref:`peripheral_fast_pair` sample:

  * Added:

    * Bond removal functionality.
    * Battery information to demonstrate the Fast Pair Battery Notification extension.
    * TX power correction value to align the advertised TX power with the Fast Pair expectations.

  * Updated:

    * The sample now uses :ref:`bt_le_adv_prov_readme` to generate Bluetooth advertising and scan response data.
    * The advertising pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`) is enabled only in the Fast Pair discoverable advertising mode.
      The device also rejects normal Bluetooth LE pairing when not in the pairing mode.
    * After the device reaches the maximum number of paired devices (set by :kconfig:option:`CONFIG_BT_MAX_PAIRED`), the device stops looking for new peers.
      The device no longer advertises with the pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`) enabled, and only the Fast Pair not discoverable advertising with hide UI indication mode includes the Fast Pair payload.

* :ref:`peripheral_mds` sample:

  * Updated the sample documentation by adding a section about testing with the `nRF Memfault for Android`_ and the `nRF Memfault for iOS`_ mobile applications.

* :ref:`direct_test_mode` sample:

  * Updated:

    * Front-end module support is now provided by the :ref:`nrfxlib:mpsl_fem` API instead of the custom driver that was part of this sample.
    * On the nRF5340 development kit, the :ref:`nrf5340_remote_shell` sample is now a mandatory sample that must be programmed to the application core.
    * On the nRF5340 development kit, the application core UART interface is used for communication with testers instead of the network core UART interface.
    * On the nRF5340 development kit, added support for the USB CDC ACM interface.

* :ref:`peripheral_uart` sample:

  * Fixed the code build with the :kconfig:option:`CONFIG_BT_NUS_SECURITY_ENABLED` Kconfig option disabled.

* :ref:`ble_nrf_dm` sample:

  * Updated the sample to generate a new seed value after each synchronization to provide different hopping sequences.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_light_switch` sample:

  * Added support for running the light switch as a Low Power node.

* :ref:`bluetooth_mesh_light` sample:

  * Added point-to-point Device Firmware Update (DFU) support over the Simple Management Protocol (SMP) for supported nRF52 Series development kits.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Added:

    * Ability to limit the reported temperatures based on :c:var:`bt_mesh_sensor_dev_op_temp_range_spec` as a setting for the :c:var:`bt_mesh_sensor_present_dev_op_temp` sensor type.
    * Ability to persistently store the sensor type setting.
    * A sensor descriptor of the temperature sensor.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added the ability to use buttons to send ``get`` and ``set`` messages for a sensor setting, as well as a ``get`` message for a sensor descriptor.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Updated:

    * The :c:func:`bt_disable` function call has been removed from the interrupt context before calling the :c:func:`emds_store` function.
    * The :c:func:`mpsl_uninit` function has been replaced with :c:func:`mpsl_lib_uninit`.


nRF9160 samples
---------------

* Added :ref:`modem_trace_flash` sample that demonstrates how to add a modem trace backend that stores the trace data to external flash.

* :ref:`lwm2m_client` sample:

  * Added:

    * Ability to use buttons to generate location assistance requests.
    * Documentation on :ref:`lwmwm_client_testing_shell`.

  * Updated:

    * The sample now uses the new LwM2M location assistance objects through the :ref:`lib_lwm2m_location_assistance` library.
    * Removed all read callbacks from sensor code because of an issue of read callbacks not working properly when used with LwM2M observations.
      This is due to the fact that the engine does not know when data is changed.
    * Sensor samples are now enabled by default for Thingy:91 and disabled by default on nRF9160 DK.

* :ref:`modem_shell_application` sample:

  * Added:

    * The functionality where **LED 1** (nRF9160 DK)/Purple LED (Thingy:91) is lit for five seconds indicating that the current location has been acquired by using the ``location get`` command.
    * Overlay files for nRF9160 DK with nRF7002 EK to enable Wi-Fi scanning support.
      With this configuration, you can, for example, obtain the current location using the ``location get`` command.
    * Support for new GCI (Global Cell ID) search types for ``link ncellmeas`` command, which are supported by the modem firmware versions from and including 1.3.4.
    * Handling for the new data receive events in the :ref:`lib_nrf_cloud` library.
    * Support for connecting to cloud using the :ref:`lib_lwm2m_client_utils` library.
      An overlay file is provided for building with the LwM2M support and an optional overlay to enable P-GPS.

  * Removed A-GPS and P-GPS processing.
    It is now handled by the :ref:`lib_nrf_cloud` library.

* :ref:`location_sample` sample:

  * Added overlay files for nRF9160 DK with nRF7002 EK to obtain the current location by using Wi-Fi scanning results.

* :ref:`lte_sensor_gateway` sample:

  * Added handling for the new data receive events in the :ref:`lib_nrf_cloud` library.
  * Removed A-GPS and P-GPS processing.
    It is now handled by the :ref:`lib_nrf_cloud` library.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * Support for the :ref:`lib_location` library Wi-Fi location method with the nRF7002 EK.
    * Handling for the new data receive events in the :ref:`lib_nrf_cloud` library.
    * Board overlay file for nRF9160 DK with external flash.
    * Overlay file to enable P-GPS data storage in external flash.

  * Removed A-GPS and P-GPS processing.
    It is now handled by the :ref:`lib_nrf_cloud` library.

* Renamed the nRF9160: nRF Cloud REST cellular position sample to :ref:`nrf_cloud_rest_cell_pos_sample` sample.
  Sample files are moved from ``samples/nrf9160/nrf_cloud_rest_cell_pos`` to ``samples/nrf9160/nrf_cloud_rest_cell_location``.

Trusted Firmware-M (TF-M) samples
---------------------------------

* Added:

  * :ref:`tfm_psa_test` for validating compliance with Arm PSA Certified requirements.
  * :ref:`tfm_regression_test` to run secure and non-secure Trusted Firmware-M (TF-M) regression tests.
  * :ref:`tfm_psa_template`, providing a template for Arm PSA best practices on nRF devices and enforcing correct transition and usage of the PSA lifecycle states.
  * :ref:`provisioning_image` sample that provisions the PSA platform root of trust parameters (such as the PSA Implementation ID and lifecycle state) in a manner compatible with Trusted Firmware-M (TF-M).

Thread samples
--------------

* :ref:`ot_cli_sample` sample:

  * Removed the Thread Certification support files in favor of regular sample overlays.

Matter samples
--------------

.. note::
   All Matter samples in the |NCS| v2.2.0 that offer Wi-Fi support have been tested using the nRF7002 DK (PCA10143) rev. A and are built with rev. A support by default.
   You can configure Matter samples to use rev. B by setting the :kconfig:option:`CONFIG_NRF700X_REV_A` Kconfig option to ``n``.
   Make sure that you build the samples for the revision of the nRF7002 DK that you are using.

* Updated ZAP configuration of the samples to conform with device types defined in Matter 1.0 specification.

* :ref:`matter_light_bulb_sample` sample:

  * Added:

    * Support for Matter over Wi-Fi on standalone ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached.
    * Deferred attribute persister class to reduce the flash wear caused by handling the ``MoveToLevel`` command from the Level Control cluster.


nRF5340 samples
---------------

* :ref:`multiprotocol-rpmsg-sample` sample:

  * Updated by decreasing the maximum supported number of concurrent Bluetooth LE connections to four.


Wi-Fi samples
-------------

* Added:

  * :ref:`wifi_radio_test` sample with the radio test support and :ref:`subcommands for FICR/OTP programming <wifi_ficr_prog>`.
  * :ref:`wifi_scan_sample` sample that demonstrates how to scan for the access points.
  * :ref:`wifi_station_sample` sample that demonstrates how to connect the Wi-Fi station to a specified access point.
  * :ref:`wifi_provisioning` sample that demonstrates how to provision a device with Nordic Semiconductor's Wi-Fi chipsets over Bluetooth® Low Energy.

* :ref:`wifi_shell_sample` sample:

  * Added configuration support for the Wi-Fi power saving feature.


Other samples
-------------

* Added :ref:`hw_id_sample` sample.
* :ref:`radio_test` sample:

  * Added:

    * Support for the :ref:`nrfxlib:mpsl_fem` TX power split feature.
      The new ``total_output_power`` shell command is introduced for sample builds with front-end module support.
      It enables automatic setting of the SoC output power in a radio peripheral and front-end module gain to achieve requested output power or less if an exact value is not supported.
    * Support for +1 dBm, +2 dBm, and +3 dBm output power on nRF5340 DK.

  * Updated:

    * Front-end module support is now provided by the :ref:`nrfxlib:mpsl_fem` API instead of the custom driver that was part of this sample.
    * On the nRF5340 development kit, the :ref:`nrf5340_remote_shell` sample is now a mandatory sample that must be programmed to the application core.
    * On the nRF5340 development kit, this sample uses the :ref:`shell_ipc_readme` library to forward shell data through the physical application core UART interface.


CAF samples
-----------

* :ref:`caf_sensor_manager_sample` sample:

  * Removed the sensor sim configuration.
    The sample now uses the :ref:`sensor_stub` by default.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added :ref:`uart_ipc`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v3.1.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* Added :ref:`cgms_readme` library.

* :ref:`bt_le_adv_prov_readme` library:

  * Added:

    * Google Fast Pair advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`).
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option to TX power advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`).
      The option adds a predefined value to the TX power, which is included in the advertising data.
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_GAP_APPEARANCE_SD` Kconfig option to GAP appearance data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_GAP_APPEARANCE`).
      The option can be used to move the GAP appearance value to the scan response data.
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME_SD` Kconfig option to Bluetooth device name data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`).
      The option can be used to move the Bluetooth device name to the advertising data.

  * Updated the library by changing :c:member:`bt_le_adv_prov_adv_state.bond_cnt` to :c:member:`bt_le_adv_prov_adv_state.pairing_mode`.
    The information about whether the advertising device is looking for a new peer is more meaningful for the Bluetooth LE data providers.

* :ref:`nrf_bt_scan_readme`:

  * Added the ability to use the module when the Bluetooth Observer role is enabled.

* :ref:`bt_fast_pair_readme` service:

  * Added:

    * API to check Account Key presence (:c:func:`bt_fast_pair_has_account_key`).
    * Support for the Personalized Name extension.
    * Support for the Battery Notification extension.

  * Updated the library by disabling automatic security re-establishment request as a peripheral (:kconfig:option:`CONFIG_BT_GATT_AUTO_SEC_REQ`) to allow the Fast Pair Seeker to control the security re-establishment.

* :ref:`hids_readme`:

  * Fixed:

    * A possible out-of-bounds memory access issue in the :c:func:`hids_protocol_mode_write` and :c:func:`bt_hids_boot_kb_inp_rep_send` functions.
    * The :c:func:`hids_ctrl_point_write` function behavior.

* :ref:`bt_mesh` library:

  * Added the vendor :ref:`bt_mesh_dm_readme` supporting distance measurement between Bluetooth mesh devices.
  * Updated:

    * Bluetooth mesh client models to reflect the changed mesh shell module structure.
      All Bluetooth mesh model commands are now located under **mesh models** in the shell menu.

    * :ref:`bt_mesh_dk_prov` module:

      * The UUID generation has been updated to prevent trailing zeros in the UUID.
        Migration note: To retain the legacy generation of UUID, enable the :kconfig:option:`CONFIG_BT_MESH_DK_LEGACY_UUID_GEN` Kconfig option.

  * Removed the Kconfig options controlling timing and delta for the :ref:`bt_mesh_silvair_enocean_srv_readme`.
    These values are now specified in the specification and cannot be changed.



See `Bluetooth mesh samples`_ for the list of changes for the Bluetooth mesh samples.


Bootloader libraries
--------------------

* :ref:`doc_bl_storage` library:

  * Added:

    * PSA compatible lifecycle state.
    * PSA compatible implementation ID.

  * Removed the option of application using the library to read OTP memory when the |NSIB| (NSIB) is enabled.


Modem libraries
---------------

* Added the :ref:`at_cmd_custom_readme` library to add custom AT commands with application callbacks.

* :ref:`modem_info_readme` library:

  * Removed:

    * The :c:func:`modem_info_json_string_encode` and :c:func:`modem_info_json_object_encode` functions.
    * The ``network_mode`` field from :c:struct:`network_param` structure.
    * The macro ``MODEM_INFO_NETWORK_MODE_MAX_SIZE``.
    * The ``CONFIG_MODEM_INFO_ADD_BOARD`` Kconfig option.

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE` Kconfig option to override the IPC IRQ priority from the devicetree.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO` Kconfig option to configure the IPC IRQ priority when the Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE` is enabled.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE` Kconfig option to enable the measurement of the modem trace backend bitrate.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_LOG` Kconfig option to enable logging of the modem trace backend bitrate.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BITRATE_LOG` Kconfig option to enable logging of the modem trace bitrate.

  * Updated:

    * The IPC IRQ priority is now set using the devicetree.
    * The EGU peripheral is no longer used to generate software interrupts to process network data.
    * The :c:func:`getaddrinfo` function to return ``EAFNOSUPPORT`` instead of ``EPROTONOSUPPORT`` when socket family is not supported.
    * The :c:func:`bind` function to return ``EAFNOSUPPORT`` instead of ``ENOTSUP`` when socket family is not supported.
    * The :c:func:`sendto` function to return ``EAFNOSUPPORT`` instead of ``ENOTSUP`` when socket family is not supported.
    * The :c:func:`connect` function to not override the error codes set by the Modem library when called with raw parameters (non-IP).

  * Fixed:

    * An issue where the :c:func:`getsockopt` function causes segmentation fault when the ``optlen`` parameter is provided as ``NULL``.
    * An issue where the :c:func:`recvfrom` function causes segmentation fault when the ``from`` and ``fromlen`` parameters are provided as ``NULL``.

* :ref:`lib_location` library:

  * Added:

    * Timeout for the entire location request.
    * Location data details such as entire PVT data.
    * MQTT support for nRF Cloud Wi-Fi positioning.
    * Improved LTE-GNSS interworking and added possibility to trigger GNSS priority mode if GNSS does not get long-enough time windows due to LTE idle mode operations.

  * Updated:

    * Location method has been moved from the :c:struct:`location_data` structure to :c:struct:`location_event_data`.
    * The library now uses :ref:`lib_nrf_cloud_location` library for nRF Cloud Wi-Fi positioning.

* :ref:`lte_lc_readme` library:

  * Added support for GCI (Global Cell ID) neighbor cell measurement search types, which are supported by the modem firmware versions from and including v1.3.4.

  * Updated the parameter type in the :c:func:`lte_lc_neighbor_cell_measurement` function to :c:struct:`lte_lc_ncellmeas_params`.
    It includes both search type and GCI count that have an impact only on GCI search types.

* :ref:`modem_key_mgmt` library:

  * Added:

    * The ``-EALREADY`` return value for the :c:func:`modem_key_mgmt_write` function when the credential already exists and cannot be overwritten.
    * The ``-ECANCELED`` return value for the :c:func:`modem_key_mgmt_write` and :c:func:`modem_key_mgmt_delete` functions when the voltage is low.

  * Updated:

    * All the functions to return ``-EACCES`` instead of ``-EPERM`` when the access to the credential is not allowed.
    * All the functions to return ``-EPERM`` instead of ``-EACCES`` when the operation is not permitted because the LTE link is active.

* Renamed the AT SMS Cert library to :ref:`lib_gcf_sms_readme`.
  The :ref:`lib_gcf_sms_readme` library now uses the :ref:`at_cmd_custom_readme` library to register filtered AT commands.

Libraries for networking
------------------------

* Added :ref:`lib_lwm2m_location_assistance` library that supports using A-GPS, P-GPS, and ground fix assistance from nRF Cloud through an LwM2M server.

* Multicell location library:

  * Removed the Kconfig option :kconfig:option:`CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS`.
    The maximum number of supported neighbor cell measurements for HERE location services depends on the :kconfig:option:`CONFIG_LTE_NEIGHBOR_CELLS_MAX` Kconfig option.

* :ref:`lib_download_client` library:

  * Updated the library so that it does not retry download on disconnect.
  * Fixed a race condition when starting the download.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * A possibility to override used default OS memory alloc/free functions.
    * More unit tests for the library.
    * Events :c:enum:`NRF_CLOUD_EVT_RX_DATA_CELL_POS` and :c:enum:`NRF_CLOUD_EVT_RX_DATA_SHADOW`.

  * Updated:

    * The stack size of the MQTT connection monitoring thread can now be adjusted by setting the :kconfig:option:`CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD_STACK_SIZE` Kconfig option.
    * The library now subscribes to a wildcard cloud-to-device (/c2d) topic.
      This enables the device to receive nRF Cloud Location Services data on separate topics.
    * The event :c:enum:`NRF_CLOUD_EVT_RX_DATA` is replaced with :c:enum:`NRF_CLOUD_EVT_RX_DATA_GENERAL`.
    * The library now processes A-GPS and P-GPS data; it is no longer passed to the application.
    * The status field of :c:enum:`NRF_CLOUD_EVT_ERROR` events uses values from the enumeration :c:enumerator:`nrf_cloud_error_status`.
    * UI service info and sensor type strings now refer to ``GNSS`` instead of ``GPS``.
    * Renamed the enumeration value ``NRF_CLOUD_EVT_RX_DATA_CELL_POS`` to :c:enum:`NRF_CLOUD_EVT_RX_DATA_LOCATION`.

  * Removed:

    * An unused parameter of the :c:func:`nrf_cloud_connect` function.
    * An unused :c:func:`nrf_cloud_shadow_update` function.

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for using X509 certificates.

* :ref:`lib_fota_download` library:

  * Added an error code :c:enumerator:`FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL` to indicate that the source of error is not network related.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated by replacing the ``nrf_cloud_rest_cell_pos_get()`` function with :c:func:`nrf_cloud_rest_location_get`.

* :ref:`lib_nrf_cloud_pgps` library:

  * Added access to P-GPS predictions in external flash.

  * Fixed:

    * An issue where zero predictions would be requested from the cloud.
    * An issue where subsequent updates were locked out after the first one completes.
      This happened when custom download transport was not used.

* Renamed the nRF Cloud cellular positioning library to :ref:`lib_nrf_cloud_location`.
  In addition to cellular location, the library now supports device location from nRF Cloud using Wi-Fi network information.

Libraries for NFC
-----------------

* :ref:`lib_nfc_ndef`:

  * Fixed a write to a constant field in the :c:func:`ac_rec_payload_parse` function.

* :ref:`nfc_t4t_isodep_readme`:

  * Fixed out of bounds access in the :c:func:`ats_parse` function.

Other libraries
---------------

* Added:

  * :ref:`lib_sfloat` library.
  * :ref:`lib_hw_id` library to retrieve a unique hardware ID.

* :ref:`emds_readme` library:

  * Updated:

    * The library implementation bypasses the flash driver when storing the emergency data.
      This allows calling the :c:func:`emds_store` function from an interrupt context.
    * The internal thread for storing the emergency data has been removed.
      The emergency data is now stored by the :c:func:`emds_store` function.

* :ref:`mod_dm` module:

  * Added a window length configuration to be used runtime, when a new measurement request is added.

  * Updated:

    * The calculation of MPSL timeslot length has been improved by using the :ref:`nrfxlib:nrf_dm` functionality.
    * Renamed the ``access_address`` field to ``rng_seed`` in the :c:struct:`dm_request` structure.

* :ref:`app_event_manager`:

  * Added :kconfig:option:`CONFIG_APP_EVENT_MANAGER_SHELL` Kconfig option.
    The option can be used to disable Event Manager shell commands.

* :ref:`nrf_profiler`:

  * Added the :kconfig:option:`CONFIG_NRF_PROFILER_SHELL` Kconfig option.
    The option can be used to disable the nRF Profiler shell commands.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_state`:

  * Added :kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS`.
    The option allows to specify the maximum number of allowed bonds in each Bluetooth local identity for a Bluetooth Peripheral.

* :ref:`caf_sensor_manager`:

  * Added:

    * Dynamic control for the :ref:`sensor sample period <sensor_sample_period>`.
    * Test for the sample.

DFU libraries
-------------
* :ref:`lib_dfu_target` library:

  * Added new types :c:enum:`DFU_TARGET_IMAGE_TYPE_ANY_MODEM` and :c:enum:`DFU_TARGET_IMAGE_TYPE_ANY_APPLICATION`.
    This makes any supported modem update type acceptable when downloading.

  * Updated the library such that calling the :c:func:`dfu_target_reset()` function clears all images that have already been downloaded into a target area.
    This allows cancelling any update packages even if they are already marked to be updated.

MPSL libraries
--------------

* Added :ref:`mpsl_lib`.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

* :ref:`west_sbom`:

  * Updated the SPDX License List database to version 3.18.

* :ref:`partition_manager`:

  * Added:

    * :kconfig:option:`CONFIG_PM_PARTITION_ALIGN_SETTINGS_STORAGE` Kconfig option to specify the alignment of the settings storage partition.
    * :kconfig:option:`CONFIG_PM_EXTERNAL_FLASH_HAS_DRIVER` Kconfig option.
      This option must be selected by drivers providing support for external flash.
      It is automatically selected by :kconfig:option:`CONFIG_NORDIC_QSPI_NOR` and :kconfig:option:`CONFIG_SPI_NOR`.
      MCUboot might fail to boot when the external flash memory is used for non-primary application images and when the driver for the external flash memory is not enabled.
      See :ref:`ug_bootloader_external_flash` and :ref:`pm_external_flash` for details.
    * P-GPS partition section to the :file:`ncs/nrf/subsys/partition_manager/Kconfig` file.
    * Board-specific static Partition Manager configuration shared among build types.
      The configuration can be defined in the board's directory.

  * Updated the :file:`ncs/nrf/subsys/partition_manager/pm.yml.pgps` file to place P-GPS partition in external flash when so configured.

  * Fixed an issue with the driver and devicetree symbol for the external flash memory where the driver was sometimes NULL, even if the DT node was chosen.

    .. note::
       If your project includes MCUboot and uses external flash for storing secondary images, after applying this fix, you need to configure ``nordic,pm-ext-flash`` property in a devicetree overlay for the MCUboot child image.
       Otherwise, MCUboot will not be able to read a flash area in the external flash to pick up a new firmware image.

* Unity:

  * Added:

    * Support for excluding functions from mock generation.
      This provides a framework to implement custom mocks.

  * Updated:

    * The generated mock functions have been renamed from ``__wrap`` to ``__cmock``.
    * The generated mock headers have been renamed from :file:`mock_<header_file>.h` to :file:`cmock_<header_file>.h`.
    * The compiler option ``--defsym`` is used instead of ``--wrap``.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``cfec947e0f8be686d02c73104a3b1ad0b5dcf1e6``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Precise check of the image size.
  * :kconfig:option:`CONFIG_USE_NRF53_MULTI_IMAGE_WITHOUT_UPGRADE_ONLY` Kconfig option to specify that you want to use nRF53 multi-image upgrade without the upgrade only setting in MCUboot.
    Enabling this option has drawbacks.
    See the :ref:`ug_nrf5340` user guide for details.
  * :kconfig:option:`CONFIG_BOOT_SERIAL_MAX_RECEIVE_SIZE` Kconfig option to specify the size of the serial recovery command buffer, which was previously fixed at 512 bytes.
    The new default value is 1024 bytes to allow for larger commands (and increased transfer speed).

* Updated loader by adding post copy hook to swap function.

* Fixed:

  * RAM loading for Arm with correct handling of vector table when code has moved to RAM.
  * An issue with QSPI stack alignment in serial recovery, which prevented writing data to QSPI slots.
  * An issue in serial recovery where rc is wrongly returned as an unsigned integer.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``cd16a8388f71a6cce0cea871f75f6d4ac8f56da9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline cd16a8388f ^71ef669ea4

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^cd16a8388f

The current |NCS| main branch is based on revision ``cd16a8388f`` of Zephyr.

Trusted Firmware-M
==================

* Added:

  * Reading, updating, and attesting the lifecycle state stored in nRF OTP.
    The storage is managed by the :ref:`doc_bl_storage` library.
  * Reading and attesting an implementation ID stored in nRF OTP.
    The storage is managed by the :ref:`doc_bl_storage` library.
  * Enabling the APPROTECT and the device reset when transitioning to the SECURED lifecycle state.
  * Support for provisioning as described in the Platform Security Architecture (PSA) security model.

Documentation
=============

* Added:

  * The :ref:`ug_nrf52_gs` user guide.
  * Documentation for the :ref:`lib_bh1749`.


* Updated:

  * :ref:`app_memory` documentation by adding configuration options affecting memory footprint for Bluetooth mesh, which can be used to optimize the application.
  * Steps on :ref:`Installing nRF Connect SDK automatically <gs_assistant>` to reflect the fact that the |nRFVSC| is the default recommended IDE.
  * Working with the nRF52 Series information by splitting the information into :ref:`ug_nrf52_features` and :ref:`ug_nrf52_developing` pages.
  * :ref:`ug_tfm` user guide with improved TF-M logging documentation on getting the secure output on nRF5340 DK.
  * Documentation of :ref:`nrf_bt_scan_readme`, :ref:`ancs_client_readme`, :ref:`hogp_readme` and :ref:`lib_hrs_client_readme` libraries to improve readability.
  * :ref:`ug_nrf52_developing` and :ref:`ug_nrf5340` with sections describing FOTA in Bluetooth mesh.
  * :ref:`ug_thread_tools` with new nRF Util installation information when configuring radio co-processor.
