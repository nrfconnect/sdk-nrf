.. _ncs_release_notes_210:

|NCS| v2.1.0 Release Notes
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

* Added :ref:`experimental support <software_maturity>` for the nRF7002 Wi-Fi 6 companion IC with samples.
  See :ref:`nrf7002dk_nrf5340` and :ref:`wifi_samples` for more information.
* Added experimental support for :ref:`Matter over Wi-Fi with nRF7002 <ug_matter_overview_architecture_integration>`.
* Feature-completed experimental support for Matter over Thread.
  It uses the :ref:`Thread v1.3 certified stack <thread_ug_supported_features_v13>` introduced in the |NCS| v2.0.2.
* Added experimental support for setting the TX power envelope with FEM power control for Bluetooth® LE and 802.15.4.
* Added experimental support for Microsoft Azure Embedded C SDK for the nRF91 Series.
* Added :ref:`support <software_maturity>` for enhanced power optimized device management and nRF Cloud Location Services through LwM2M/CoAP with AVSystem for the nRF91 Series.
* Added support for :ref:`nRF Cloud full modem FOTA <nrf9160_fota>` for the nRF91 Series.
* Added experimental support for the Nordic Distance Toolbox (NDT) on the nRF5340 SoC with samples.
* Added Find My support for the nRF5340 SoC.
* Updated Trusted Firmware-M to v1.6, with additional RAM overhead optimizations.
  It is now the default solution used in all samples.
  Secure Partition Manager (SPM) is now deprecated and removed from all samples.
* Added :ref:`mod_memfault` support over the Bluetooth LE transport using :ref:`mds_readme`.

See :ref:`ncs_release_notes_210_changelog` for the complete list of changes.

Sign up for the `nRF Connect SDK v2.1.0 webinar`_ to learn more about the new features.

The official nRF Connect for VS Code extension also received improvements for this release, including a customized debugger experience.
See the `nRF Connect for Visual Studio Code`_ page for more information.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.1.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.1.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.1.0.
SEGGER Embedded Studio Nordic Edition is no longer tested or recommended for new projects.

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
     - `Changelog <Modem library changelog for v2.1.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.1.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.1.0`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_210_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Added:

  * Information about :ref:`gs_supported_OS` to the :ref:`gs_recommended_versions` page.
  * Information about :ref:`app_build_output_files` on the :ref:`app_build_system` page.
  * Information about :ref:`gs_debugging` on the :ref:`gs_testing` page.
    Also added links to this section in different areas of documentation.
  * An option to configure IEEE 802.15.4 ACK frame timeout at build time using :kconfig:option:`CONFIG_NRF_802154_ACK_TIMEOUT_CUSTOM_US`.
  * Serial recovery of the image of nRF5340's ``cpunet`` build targets even when the simultaneous cores upgrade is disabled (:kconfig:option:`CONFIG_NRF_MULTI_IMAGE_UPDATE` set to ``n``).
    This is enabled with the :kconfig:option:`CONFIG_NRF53_RECOVERY_NETWORK_CORE`.

* Updated :ref:`app_memory` page with sections about Gazell and NFC.

RF Front-End Modules
--------------------

* Added:

  * The :kconfig:option:`CONFIG_MPSL_FEM_ONLY` Kconfig option that allows the :ref:`nrfxlib:mpsl_fem` API to be used without other MPSL features.
    The :ref:`MPSL library <nrfxlib:mpsl>` is linked into the build without initialization.
    You cannot use other MPSL features when this option is enabled.
  * The possibility to add custom models which split the requested TX power between the power on SoC output and the FEM gain in a way desired by the user.

* Fixed a build error that occurred when building an application for nRF53 SoCs with Simple GPIO Front-End Module support enabled.

Wi-Fi
-----

* Added experimental support for the nRF7002 DK that includes the nRF7002 companion IC.
  For more information, see the :ref:`ug_nrf70` guide.

See `Wi-Fi samples`_ for details about how to use Wi-Fi in your application.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

* Added:

  * Support for changing the radio transmitter's default power level using the :c:func:`sdc_default_tx_power_set` function.
  * Support for changing the peripheral latency mode using the :c:func:`sdc_hci_cmd_vs_peripheral_latency_mode_set` function.
  * Support for changing the default TX power using Kconfig options that start with ``CONFIG_BT_CTLR_TX_PWR_*``.

* Updated LTE dual-chip Coex support on the nRF52 Series.
  It is now ready for production.

For details, see the :ref:`SoftDevice Controller changelog <nrfxlib:softdevice_controller_changelog>`.

Bluetooth mesh
--------------

* Added support for using :ref:`emds_readme`.
  For details, see `Bluetooth mesh samples`_ and `Bluetooth libraries and services`_.
* Updated API in :ref:`bt_mesh_sensor_srv_readme`: Column get callback now gets called with a column index instead of a pointer to column.
  This was introduced to support a series for sensors with one or two channels.

Also see `Bluetooth mesh samples`_ for the list of changes.

Enhanced ShockBurst (ESB)
-------------------------

* Fixed the :c:func:`update_radio_crc` function in order to correctly configure the CRC's registers (8 bits, 16 bits, or none).

Matter
------

* Added support for Matter over Wi-Fi to several samples.
* Updated :ref:`ug_matter` with new pages about Matter SDK as well as information about Matter over Wi-Fi.
* Removed the overlay file for the low-power configuration build type from several Matter samples.
  The low-power communication modes are now enabled by the default for these samples.

See `Matter samples`_ for the list of changes for the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``708685f4821df2aa0304f02db2773c429ad25eb8``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Matter device factory data.
    This includes a set of scripts for building the factory data partition content, and the ``FactoryDataProvider`` class for accessing this data.
  * :ref:`Experimental support <software_maturity>` for Matter over Wi-Fi.

Thread
------

* Added information about Synchronized Sleepy End Device (SSED) and SED vs SSED activity in the :ref:`thread_ot_device_types` documentation.
* Updated values in the memory requirement tables in :ref:`thread_ot_memory` after the update to the :ref:`nrfxlib:ot_libs` in nrfxlib.
* Removed multiprotocol support from :file:`overlay-cert.config` and moved it to :file:`overlay-multiprotocol.conf`.

See `Thread samples`_ for the list of changes for the Thread samples.

Zigbee
------

* Updated:

  * The PAN ID conflict resolution is now enabled in applications that use the :ref:`lib_zigbee_application_utilities` library.
    For details, see `Libraries for Zigbee`_.
  * The default entropy source of Zigbee samples and unit tests to Cryptocell for SoCs that have Cryptocell.

See `Zigbee samples`_ for the list of changes for the Zigbee samples.

HomeKit
-------

* Added support for Thread v1.3.
* Updated:

  * OTA DFU using the iOS Home app (over UARP - BLE and Thread).
    This feature is no longer experimental.
  * HomeKit Accessory Development Kit to v6.3 (ADK v6.3 e6e82026).
  * Current consumption for Thread Sleepy End Devices (SEDs) and Bluetooth LE peripherals.
    The current consumption has been improved.
  * HomeKit documentation pages, with several improvements.

* Fixed:

  * An issue where Bluetooth LE TX configuration was set to 0 dBm by default.
  * An issue where the Stateless Switch application crashed upon factory reset.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

* All applications running on non-secure boards are documented to use TF-M as the trusted execution solution by default.
  SPM is now deprecated.
* Updated the PWM frequency of the pwmleds device from 50 Hz to 125 Hz in applications that run on Nordic Thingy:91.

.. note::
    A known issue was found that concerns :ref:`modem trace retrieval incompatibility with TF-M (NCSDK-15512) <known_issues_other>`: You can either use **UART1** for TF-M output or for modem traces, but not for both.
    This affects applications and samples based on nRF9160.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * :ref:`motion_impact_detection` using the ADXL372 accelerometer.
  * The following Kconfig options to set the threshold and timeout values:

    * :ref:`CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD>`
    * :ref:`CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD>`
    * :ref:`CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS <CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS>`

  * Support for full modem FOTA updates for nRF Cloud builds.

* Updated:

  * The application to use :ref:`TF-M <ug_tfm>` as the default secure firmware component.
  * Data sampling method.
    It is now performed when the device detects both activity and inactivity in passive mode, notified by the :c:enum:`SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED` event of the :ref:`sensor module <asset_tracker_v2_sensor_module>`.
  * ``CONFIG_MODEM_NEIGHBOR_SEARCH_TYPE`` Kconfig option.
  * Publishing method for GNSS fixes.
    GNSS fixes are now published in the PVT format instead of NMEA for nRF Cloud builds.
    To revert to NMEA, set the ``CONFIG_GNSS_MODULE_NMEA`` Kconfig option.
  * Forwarding of :c:enum:`SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED` and :c:enum:`SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED` events by the sensor module.
  * :ref:`Real-time configurations <real_time_configs>`, which can now configure the upper and lower thresholds for motion detection.
    You can also configure the timeout after which the sensor reports inactivity.
    It is now set to 30 seconds.
  * LwM2M schema.
    To use the new parameters, upload the updated :file:`config_object_descript.xml` file to AVSystem.
  * The conversions of RSRP and RSRQ.
    These now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.
  * Bootstrapping to be disabled by default.
    This allows connecting to the default LwM2M service AVSystem's `Coiote Device Management`_ using free tier accounts.
  * ``CONFIG_DATA_DEVICE_MODE`` Kconfig option to be a choice that can be set to either ``CONFIG_DATA_DEVICE_MODE_ACTIVE`` or ``CONFIG_DATA_DEVICE_MODE_PASSIVE``, depending on the desired device mode.
  * The default sample timeout for sample requests that include neighbor cell searches.
    The time is now set to 11 seconds.
  * Documentation structure.

* Fixed:

  * An issue that reports GNSS altitude, accuracy, and speed incorrectly when using LwM2M engine.
  * An issue that caused modem FOTA jobs to be reported as not validated to nRF Cloud.
  * An issue that caused the Memfault event storage buffer to get full, which in turn resulted in lost Memfault events.

* Removed:

  * ``CONFIG_APP_REQUEST_GNSS_ON_INITIAL_SAMPLING`` Kconfig option.
  * ``CONFIG_APP_REQUEST_NEIGHBOR_CELLS_DATA`` Kconfig option.
  * ``CONFIG_EXTERNAL_SENSORS_ACTIVITY_DETECTION_AUTO`` Kconfig option.
  * ``CONFIG_MODEM_CONVERT_RSRP_AND_RSPQ_TO_DB`` Kconfig option.
  * ``CONFIG_DATA_ACCELEROMETER_THRESHOLD`` Kconfig option.
  * ``CONFIG_DATA_ACCELEROMETER_BUFFER_STORE`` Kconfig option.
  * ``CONFIG_DATA_ACCELEROMETER_BUFFER_COUNT`` Kconfig option.
  * ``SENSOR_EVT_MOVEMENT_DATA_READY`` event.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * URC for GNSS timeout sleep event.
  * Selected flags support in ``#XRECV`` and ``#XRECVFROM`` commands.
  * Multi-PDN support in the Socket service.
  * New ``#XGPSDEL`` command to delete GNSS data from non-volatile memory.
  * New ``#XDFUSIZE`` command to get the size of the DFU file image.

* Updated:

  * The application to use :ref:`TF-M <ug_tfm>` enabled by default.
  * The GNSS service to signify location information to nRF Cloud.
  * The AT response and the URC sent when the application enters and exits data mode.
  * ``WAKEUP_PIN`` and ``INTERFACE_PIN`` to be now defined as *Active Low*.
    Both are *High* when the SLM application starts.

* Fixed an issue where the features of the Mbed TLS v3.1 were not enabled by default, which caused the native TLS to not work.
  The documentation of the :ref:`SLM_AT_SOCKET`' socket option numbers was updated accordingly.
* Removed the software toggle of ``INDICATE_PIN`` in case of reset.

nRF5340 Audio
-------------

* Added:

  * :ref:`nrfxlib:lc3` module to the `sdk-nrfxlib`_ repository.
    The software codec does not require additional configuration steps and special access anymore.
    This affects the configuration and building process of the application.
  * Support for Basic Audio Profile, including support for the stereo :term:`Broadcast Isochronous Stream (BIS)`.
  * Bonding between gateway and headsets in the :term:`Connected Isochronous Stream (CIS)`.
  * :ref:`Experimental <software_maturity>` DFU support for internal and external flash layouts.
    See :ref:`nrf53_audio_app_configuration_configure_fota` in the application documentation for details.
  * DFU advertising name based on role.

* Updated:

  * Network controller.
  * Documentation in the :ref:`nrf53_audio_app_building_script` section.
    The text now mentions how to recover the device if programming using script fails.
  * Documentation of the operating temperature maximum range in the :ref:`nrf53_audio_app_dk_features` section.

* Removed support for SBC.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added configuration of :ref:`bt_le_adv_prov_readme`.
  The subsystem is now used instead of the :file:`*.def` file to configure advertising data and scan response data in :ref:`caf_ble_adv`.
* Updated Bluetooth advertising data and scan response data logic.
  The UUID128 of Nordic UART Service (NUS) is now added to the scan response data only if the NUS is enabled and the Bluetooth local identity in use has no bond.

nRF Desktop
-----------

* Added configuration of :ref:`bt_le_adv_prov_readme`.
  The subsystem is now used instead of the :file:`*.def` file to configure advertising data and scan response data in :ref:`caf_ble_adv`.
* Updated:

  * nRF Desktop peripherals to no longer automatically send security request immediately after Bluetooth LE connection is established.
    The feature can be turned on using :kconfig:option:`CONFIG_CAF_BLE_STATE_SECURITY_REQ`.
  * nRF Desktop dongles to start peripheral discovery immediately after Bluetooth LE connection is established.
    The dongles no longer wait until the connection is secured.
  * Bluetooth advertising data and scan response data logic:

    * The TX power included in the advertising packet is no longer hardcoded, the application reads it from the Bluetooth controller.
      The TX power is included in advertising packets even if the Bluetooth local identity in use has bond.
    * The UUID16 of Battery Service (BAS) and Human Interface Device Service (HIDS) are included in advertising packets only if the Bluetooth local identity in use has no bond.

Connectivity Bridge
-------------------

* Fixed:

  * Missing return statement that caused immediate asserts when asserts were enabled.
  * Too low UART RX timeout that caused high level of fragmentation of UART RX data.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

The following changes apply to all relevant samples:

* All samples running on non-secure boards are documented to use TF-M as the trusted execution solution.
  SPM is now deprecated.
* Updated the PWM frequency of the pwmleds device from 50 Hz to 125 Hz in samples that run on Nordic Thingy:91.

Bluetooth samples
-----------------

* Added:

  * :ref:`peripheral_mds` sample that demonstrates how to send Memfault diagnostic data through Bluetooth.
  * :ref:`power_profiling` sample that uses the system off mode and can be used for power consumption measurement.

* :ref:`ble_nrf_dm` sample:

  * Added support for the nRF5340 target.
  * Updated by splitting the configuration of the :ref:`mod_dm` module from the :ref:`nrf_dm`.
    This allows the use of the Nordic Distance Measurement library without the module.

* :ref:`direct_test_mode` sample:

  * Added a workaround for nRF5340 revision 1 Errata 117.

* :ref:`peripheral_hr_coded` sample:

  * Added configuration for the nRF5340 target.
  * Fixed advertising start on the nRF5340 target with the Zephyr LL controller.
    Previously, it was not possible to start advertising, because the :kconfig:option:`CONFIG_BT_EXT_ADV` option was disabled for the Zephyr LL controller.

* :ref:`bluetooth_central_hr_coded` sample:

  * Added configuration for the nRF5340 target.
  * Fixed scanning start on the nRF5340 target with the Zephyr LL controller.
    Previously, it was not possible to start scanning, because the :kconfig:option:`CONFIG_BT_EXT_ADV` option was disabled for the Zephyr LL controller.

* Bluetooth: Fast Pair sample:

  * Added:

    * Possibility of toggling between show and hide UI indication in the Fast Pair not discoverable advertising.
    * Automatic switching to the not discoverable advertising with the show UI indication mode after 10 minutes of discoverable advertising.
    * Automatic switching from discoverable advertising to the not discoverable advertising with the show UI indication mode after a Bluetooth Central successfully pairs.

* :zephyr:code-sample:`ble_direction_finding_connectionless_tx` sample:

  * Fixed a build error related to the missing :kconfig:option:`CONFIG_BT_DF_CONNECTIONLESS_CTE_TX` Kconfig option.
    The option has been added and set to ``y`` in the sample's :file:`prj.conf` file.

* :ref:`ble_throughput` sample:

  * Fixed peer throughput calculations.
    These were too low because the total transfer time incorrectly included 500ms delay without including the actual transfer.
  * Updated by optimizing throughput speed by increasing MTU to 498 and using the maximum connection event time.

* :zephyr:code-sample:`bluetooth_direction_finding_central` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

* :zephyr:code-sample:`ble_direction_finding_connectionless_rx` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

* :zephyr:code-sample:`ble_direction_finding_connectionless_tx` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

* :zephyr:code-sample:`bluetooth_direction_finding_peripheral` sample:

  * Added devicetree overlay file for the nRF5340 application core that configures GPIO pin forwarding.
    This enables the radio peripheral's Direction Finding Extension for antenna switching.

Bluetooth mesh samples
----------------------

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added an overlay file with support for storing data with :ref:`emds_readme`.
    Also changed the sample to restore Light state after power-down.

nRF9160 samples
---------------

.. note::
    A known issue was found that concerns :ref:`modem trace retrieval incompatibility with TF-M (NCSDK-15512) <known_issues_other>`: You can either use **UART1** for TF-M output or for modem traces, but not for both.
    This affects applications and samples based on nRF9160.

* Added :ref:`modem_trace_backend_sample` sample, demonstrating how to add a custom modem trace backend.
  The custom backend prints the amount of trace data received in bytes, trace data throughput, and CPU load.
* Updated samples that support Thingy:91 to use :ref:`TF-M <ug_tfm>` enabled by default.
* Removed the AWS FOTA sample.
  The :ref:`aws_iot` sample must be used, which implements :ref:`lib_aws_fota` through :ref:`lib_aws_iot`.

* :ref:`lwm2m_client` sample:

  * Updated:

    * CoAP maximum message size to be set to 1280 by default.
    * Number of SenML CBOR records to be set to a higher value to cope with data exchange after registration with Coiote server.
    * Default configuration to be conformant to the LwM2M specification v1.0 instead of v1.1.
      For enabling v1.1, use an overlay file.
    * Bootstrap to not use TLV exclusively.
      With v1.1, the preferred content format is sent in the bootstrap request.
      SenML CBOR takes precedence over SenML JSON and OMA TLV, when enabled.

  * Fixed generation of the timestamp of LwM2M Location object on obtaining location fix.

* :ref:`memfault_sample` sample:

  * Updated the sample to reflect changes in logging to the `Memfault SDK`_.

* :ref:`modem_shell_application` sample:

  * Added:

    * nRF9160 DK overlays for enabling BT support.
      When running this configuration, you can perform BT scanning and advertising using the ``bt`` command.
    * Support for injecting GNSS reference altitude for the low accuracy mode.
      For a position fix using only three satellites, GNSS module must have a reference altitude that can now be injected using the ``gnss agps ref_altitude`` command.
    * New command ``startup_cmd``, which can be used to store up to three MoSh commands to be run on start/bootup.
      By default, commands are run after the default PDN context is activated, but can be set to run ``N`` seconds after bootup.
    * New command ``link search`` for setting periodic modem search parameters.
    * Printing of modem domain events.
    * MQTT support for ``gnss`` command A-GPS and P-GPS.
    * An application-specific modem fault handler.
      The modem fault handler halts application execution in case of a modem crash.
    * Support for SEGGER's Real Time Transfer (RTT) instead of UART.

  * Updated:

    * Timeout parameters from seconds to milliseconds in ``location`` and ``rest`` commands.
    * The conversions of RSRP and RSRQ.
      These now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * Support for full modem FOTA.
    * LED status indication.

  * Updated:

    * Usage of the :ref:`lib_modem_antenna` library to configure the GNSS antenna instead of configuring it directly.
    * :ref:`lib_nrf_cloud` library is no longer de-initialized and re-initialized on disconnect and reconnect.
    * The :ref:`lib_nrf_cloud` library's function :c:func:`nrf_cloud_gnss_msg_json_encode` is now used to send PVT location data instead of building an NMEA sentence.
    * Minor logging and function structure improvements.

  * Fixed an issue with connection initialization that would cause delta modem FOTA updates to hang and would require manual reset.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added support for full modem FOTA updates.

* :ref:`at_monitor_sample` sample:

  * Updated the conversions of RSRP and RSRQ.
    These now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.

Thread samples
--------------

* :ref:`ot_cli_sample` sample:

  * Added logging of errors and hard faults in CLI sample by default.
  * Updated the sample documentation with SRP information.

Matter samples
--------------

* Added optimized usage of the QSPI NOR flash sleep mode to reduce power consumption during the Matter commissioning.
* Updated the size of MCUBoot partition on ``nrf5340dk_nrf5340_cpuapp`` by reducing it by 16 kB.

* :ref:`matter_light_switch_sample`:

  * Added support for Matter over Wi-Fi on ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield.
  * Updated ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` to be enabled by default.
  * Removed the overlay file for the low-power configuration build type.
    The low-power communication modes is now enabled by the default for this sample.

* :ref:`matter_lock_sample`:

  * Added support for Matter over Wi-Fi on ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield.
  * Updated ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` to be enabled by default.
  * Removed the overlay file for the low-power configuration build type.
    The low-power communication modes is now enabled by the default for this sample.

* :ref:`matter_template_sample`:

  * Added support for Matter over Wi-Fi on ``nrf7002dk_nrf5340_cpuapp`` and on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield.

* :ref:`matter_window_covering_sample`:

  * Added information about the :ref:`matter_window_covering_sample_ssed` in the sample documentation.
  * Updated ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` and :kconfig:option:`CONFIG_CHIP_THREAD_SSED` to be enabled by default.
  * Removed the overlay file for the low-power configuration build type.
    The low-power communication modes is now enabled by the default for this sample.

NFC samples
-----------

* Added a note to the documentation of each NFC sample about debug message configuration with the NFCT driver from the `nrfx`_ repository.

Zigbee samples
--------------

* :ref:`zigbee_light_switch_sample` sample:

  * Fixed an issue where a buffer would not be freed after a failure occurred when sending a Match Descriptor request.

* :ref:`zigbee_shell_sample` sample:

  * Added:

    * Support for :ref:`zephyr:nrf52840dongle_nrf52840`.
    * An option to build :ref:`zigbee_shell_sample` sample with the nRF USB CDC ACM as shell backend.

* :ref:`zigbee_ncp_sample` sample:

  * Updated by setting :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING` to be disabled by default for NCP over USB variant.

Wi-Fi samples
-------------

* Added :ref:`wifi_shell_sample` sample with the shell support.

Other samples
-------------

* Added two samples related to the identity key stored in the Key Management Unit (KMU):

  * :ref:`identity_key_generate` sample to demonstrate the generation of the identity key.
  * :ref:`identity_key_usage` sample to demonstrate how to make use of the identity key.

* :ref:`radio_test` sample:

  * Fixed the way of setting gain for the nRF21540 Front-end Module with nRF5340.

* :ref:`caf_sensor_manager_sample` sample:

  * Added configuration for the Sensor stub driver.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added :ref:`sensor_stub`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v0.30.2.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* Added:

  * :ref:`mds_readme`.
  * :ref:`bt_le_adv_prov_readme`.
    The subsystem manages Bluetooth LE advertising data and scans response data.
    The subsystem does not control Bluetooth LE advertising by itself.

* :ref:`bt_fast_pair_readme` service:

  * Added:

    * A SHA-256 hash check to ensure the Fast Pair provisioning data integrity.
    * Unit test for the storage module.
    * Cryptographic backend using :ref:`nRF Oberon <nrfxlib:nrf_oberon_readme>` API.
    * Implementation of cryptographic functions required by Fast Pair extensions.
      Also expanded unit test to verify the implementation.
      The Fast Pair extensions are not yet supported by the Fast Pair service.
    * Using the per-connection authentication callbacks to handle Bluetooth authentication during Fast Pair procedure.
    * Internal implementation improvements.

  * Updated API to allow setting the flag for the hide UI indication in the Fast Pair not discoverable advertising data.

* :ref:`bt_enocean_readme` library:

  * Added callback :c:member:`decommissioned` to :c:struct:`bt_enocean_callbacks` when EnOcean switch is decommissioned.

* :ref:`bt_mesh`:

  * Added:

    * Use of decommissioned callback in :ref:`bt_mesh_silvair_enocean_srv_readme` when EnOcean switch is decommissioned.
    * :ref:`emds_readme` support to:

      * :ref:`bt_mesh_plvl_srv_readme`
      * :ref:`bt_mesh_light_hue_srv_readme`
      * :ref:`bt_mesh_light_sat_srv_readme`
      * :ref:`bt_mesh_light_temp_srv_readme`
      * :ref:`bt_mesh_light_xyl_srv_readme`
      * :ref:`bt_mesh_lightness_srv_readme`
      * Replay protection list (RPL).

  * Updated the ``bt_mesh_sensor_ch_str_real`` function by replacing it with the :c:func:`bt_mesh_sensor_ch_str` function, which was previously a macro.

Bootloader libraries
--------------------

* :ref:`lib_dfu_target` library:

   * Updated by moving the :c:func:`dfu_ctx_mcuboot_set_b1_file` function to the :ref:`lib_fota_download` library and renaming it to :c:func:`fota_download_parse_dual_resource_locator`.

Modem libraries
---------------

* :ref:`lte_lc_readme` library:

  * Fixed an issue that caused stack corruption in the :c:func:`lte_lc_nw_reg_status_get` function.

* :ref:`at_monitor_readme` library:

  * Updated by reworking what previously were macros to :c:func:`at_monitor_pause` and :c:func:`at_monitor_resume` functions.
    These new functions take a pointer to the AT monitor entry.

* :ref:`modem_key_mgmt` library:

  * Fixed an issue that would cause the library to assert on an unhandled CME error when the AT command failed to be sent.

* :ref:`at_cmd_parser_readme` library:

  * Fixed an issue that would cause AT command responses like ``+CNCEC_EMM`` with underscore to be filtered out.

* :ref:`pdn_readme` library:

  * Added:

    * Support for setting multiple event callbacks for the default PDP context.
    * The :c:func:`pdn_default_ctx_cb_dereg` function to deregister a callback for the default PDP context.
    * The :c:func:`pdn_esm_strerror` function to retrieve a textual description of an ESM error reason.
      The function is compiled when :kconfig:option:`CONFIG_PDN_ESM_STRERROR` Kconfig option is enabled.

  * Updated:

    * The :c:func:`pdn_default_callback_set` function name to :c:func:`pdn_default_ctx_cb_reg`.
    * Automatic subscription to ``+CNEC=16`` and ``+CGEREP=1`` if the :ref:`lte_lc_readme` library is used to change the modem's functional mode.

  * Removed the ``CONFIG_PDN_CONTEXTS_MAX`` Kconfig option.
    The maximum number of PDP contexts is now dynamic.

* :ref:`nrf_modem_lib_readme`:

  * Added:

    * :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option that replaces :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_ENABLED`.
      The Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_ENABLED` is now deprecated and will be removed in the future.
    * A section about :ref:`modem_trace_backend_uart_custom_board`.
    * :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` option to enable the :c:func:`nrf_modem_lib_diag_stats_get` function that retrieves memory runtime statistics, replacing the ``nrf_modem_lib_heap_diagnose`` and ``nrf_modem_lib_shm_tx_diagnose`` functions.

  * Updated:

    * Ability to add :ref:`custom trace backends <adding_custom_modem_trace_backends>`.
    * The trace module to use the new APIs in the modem library.
      The modem trace output is now handled by a dedicated thread that starts automatically.
      The trace thread is synchronized with the initialization and shutdown operations of the Modem library.
    * The following Kconfig options by refactoring them:

      * ``CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC`` and ``CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC`` into the new :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC` Kconfig option.
      * ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC`` and ``CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC`` into the new :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP` Kconfig option.
      * ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS`` and ``CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS`` into the new :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP_PERIOD_MS` Kconfig option.

  * Removed:

    * The following Kconfig options:

      * ``CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING``
      * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE``
      * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_SIZE_OVERRIDE``
      * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIODIC``
      * ``CONFIG_NRF_MODEM_LIB_TRACE_HEAP_DUMP_PERIOD_MS``
      * ``CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC``
      * ``CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC``
      * ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC``
      * ``CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIOD_MS``
      * ``CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC``
      * ``CONFIG_NRF_MODEM_LIB_SHMEM_TX_DUMP_PERIOD_MS``

    * The following functions:

      * ``nrf_modem_lib_trace_start``
      * ``nrf_modem_lib_trace_stop``
      * ``nrf_modem_lib_heap_diagnose``
      * ``nrf_modem_lib_shm_tx_diagnose``

    * The ``nrf_modem_lib_get_init_ret`` function is now deprecated.

* :ref:`lib_location` library:

  * Updated the timeout parameters' type from uint16_t to int32_t, unit from seconds to milliseconds, and value to disable them from 0 to ``SYS_FOREVER_MS``.
    This change is done to align with Zephyr's style for timeouts.
  * Fixed an issue with P-GPS predictions not being used to speed up GNSS when first downloaded.
  * Removed PoLTE support as the service is discontinued.

* :ref:`modem_info_readme` library:

  * Updated to use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_ for the conversions of RSRP and RSRQ.

Libraries for networking
------------------------

* :ref:`lib_lwm2m_client_utils` library:

  * Updated the conversions of RSRP and RSRQ.
    These now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.
  * Fixed:

    * Setting of the FOTA update result.
    * Reporting of the FOTA update result back to the LwM2M Server.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * :c:func:`nrf_cloud_fota_pending_job_validate` function that enables an application to validate a pending FOTA job before initializing the :ref:`lib_nrf_cloud` library.
    * Handling for new nRF Cloud REST error code 40499.
      Moved the error log from the :c:func:`nrf_cloud_parse_rest_error` function into the calling function.
    * Support for full modem FOTA updates.
    * :c:func:`nrf_cloud_fota_is_type_enabled` function that determines if the specified FOTA type is enabled by the configuration.
    * :c:func:`nrf_cloud_gnss_msg_json_encode` function that encodes GNSS data (PVT or NMEA) into an nRF Cloud device message.
    * :c:func:`nrf_cloud_fota_pending_job_type_get` function that retrieves the FOTA type of a pending FOTA job.
    * Unit test for the :c:func:`nrf_cloud_init` function.

  * Updated:

    * The conversions of RSRP and RSRQ.
      These now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.
    * The function :c:func:`nrf_cloud_fota_is_type_enabled` no longer depends on :kconfig:option:`CONFIG_NRF_CLOUD_FOTA`.

  * Fixed:

    * An issue that caused the application to receive multiple disconnect events.
    * An issue that prevented full modem FOTA updates to be installed during library initialization.
    * An issue that caused the :c:func:`nrf_cloud_client_id_get` function to fail if both :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` and :kconfig:option:`CONFIG_NRF_CLOUD_REST` were enabled.

* Multicell location library:

  * Added:

    * Timeout parameter.
    * Structure for input parameters for ``multicell_location_get`` function to make updates easier in the future.

  * Updated the conversions of RSRP and RSRQ.
    These now use common macros that follow the conversion algorithms defined in the `AT Commands Reference Guide`_.
  * Removed PoLTE support as the service is discontinued.

* :ref:`lib_rest_client` library:

  * Updated:

    * Timeout handling.
      Now using http_client library timeout also.
    * A zero timeout value is now handled as "no timeout" (wait forever) to avoid immediate timeouts.

  * Removed ``CONFIG_REST_CLIENT_SCKT_SEND_TIMEOUT`` and ``CONFIG_REST_CLIENT_SCKT_RECV_TIMEOUT`` Kconfig options.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated the :c:func:`nrf_cloud_rest_send_location` function to accept a :c:struct:`nrf_cloud_gnss_data` pointer instead of an NMEA sentence.

* :ref:`lib_nrf_cloud_pgps` library:

  * Added:

    * :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_HTTP` and :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_CUSTOM` Kconfig options.
    * :c:func:`nrf_cloud_pgps_begin_update` function that prepares the P-GPS subsystem to receive downloads from a custom transport.
    * :c:func:`nrf_cloud_pgps_process_update` function that stores a portion of a P-GPS download to flash.
    * :c:func:`nrf_cloud_pgps_finish_update` function that a user of the P-GPS library calls when the custom download completes.

  * Reduced logging level for many messages to debug (``DBG``).

* :ref:`lib_azure_iot_hub` library:

  * Updated:

    * Reworked the library to use `Azure SDK for Embedded C`_.
    * The APIs are modified for both IoT Hub and DPS interaction.
      The applications and samples that use the library have been updated accordingly.

* :ref:`lib_download_client` library:

  * Fixed:

    * Handling of duplicated CoAP packets.
    * Handling of timeout errors when using CoAP.

nRF RPC libraries
-----------------

* Added documentation for the :ref:`nrf_rpc_ipc_readme` library.
* Updated memory for remote procedure calls, which is now allocated on a heap instead of the calling thread stack.

Other libraries
---------------

* Added:

  * Documentation for the :ref:`lib_adp536x` library.
  * Documentation for the :ref:`lib_flash_map_pm` library.
  * :ref:`lib_identity_key` library.

* :ref:`lib_flash_patch` library:

  * Added documentation page.
  * Updated by modifying the :kconfig:option:`CONFIG_DISABLE_FLASH_PATCH` Kconfig option, so that it can be used on the nRF52833 SoC.

* :ref:`doc_fw_info` module:

  * Fixed a bug where MCUboot would experience a fault when using the :ref:`doc_fw_info_ext_api` feature.

* :ref:`emds_readme`:

  * Updated :c:func:`emds_entry_add` to no longer use heap, but instead require a pointer to the dynamic entry structure :c:struct:`emds_dynamic_entry`.
    The dynamic entry structure should be allocated in advance.

* :ref:`mod_memfault`:

  * Added default metrics for Bluetooth.

* Secure Partition Manager (SPM):

  * Deprecated Secure Partition Manager (SPM) and the Kconfig option ``CONFIG_SPM``.
    It is replaced by the Trusted Firmware-M (TF-M) as the supported trusted execution solution.
    See :ref:`ug_tfm` for more information about the TF-M.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_adv`:

  * Added:

    * :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST` Kconfig option.
      The option is used instead of :kconfig:option:`CONFIG_BT_FILTER_ACCEPT_LIST` option to enable the filter accept list.
    * :c:struct:`ble_adv_data_update_event` that can be used to trigger update of advertising data and scan response data during undirected advertising.
      When the event is received, the module gets new data from providers and updates advertising payload.
    * A wakeup call when connection is made in the grace period.
      With this change, the call wakes up the whole system to avoid inconsistent power state between modules.

  * Updated:

    * The :ref:`bt_le_adv_prov_readme` subsystem is now used instead of the :file:`*.def` file to configure advertising data and scan response data.
    * Bluetooth device name is no longer automatically included in scan response data.
      A dedicated data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`) can be used to add the Bluetooth device name to the scan response data.

* :ref:`caf_ble_state`:

  * Updated to no longer automatically send security request immediately after Bluetooth LE connection is established when running on Bluetooth Peripheral.
    The :kconfig:option:`CONFIG_CAF_BLE_STATE_SECURITY_REQ` Kconfig option can be used to enable this feature.
    The option can be used for both Bluetooth Peripheral and Bluetooth Central.

* :ref:`caf_sensor_data_aggregator`:

  * Added unit tests for the library.

* :ref:`caf_sensor_manager`:

  * Updated to no longer use floats to calculate and determine if the sensor trigger is activated.
    This is because the float uses more space.
    Also, data sent to :c:struct:`sensor_event` uses :c:struct:`sensor_value` instead of float.

Libraries for Zigbee
--------------------

* :ref:`lib_zigbee_application_utilities` library:

  * Added :kconfig:option:`CONFIG_ZIGBEE_PANID_CONFLICT_RESOLUTION` for enabling automatic PAN ID conflict resolution.
    This option is enabled by default.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`bt_fast_pair_provision_script`:

  * Added a SHA-256 hash of the Fast Pair provisioning data to ensure its integrity.

* :ref:`partition_manager`:

  * Added:

    * :kconfig:option:`CONFIG_PM_PARTITION_REGION_LITTLEFS_EXTERNAL`, :kconfig:option:`CONFIG_PM_PARTITION_REGION_SETTINGS_STORAGE_EXTERNAL`, and :kconfig:option:`CONFIG_PM_PARTITION_REGION_NVS_STORAGE_EXTERNAL` Kconfig options to specify that the relevant partition must be located in external flash memory.
    * :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK` to override the external driver check.
      This is needed when using an external flash which is not using the :dtcompatible:`nordic,qspi-nor` driver from Zephyr.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``1d4404116a9a6b54d54ea9aa3dd2575286e666cd``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes specific to the |NCS|:

* Added initial support for leveraging the RAM-LOAD mode with the zephyr-rtos port.
* Added the MCUboot status callback support.
  See :kconfig:option:`CONFIG_MCUBOOT_ACTION_HOOKS`.
* Edited includes to have the ``zephyr/`` prefix.
* Edited the DFU detection's GPIO-pin configuration to be done through DTS using the ``mcuboot-button0`` pin alias.
* Edited the LED usage to prefer DTS' ``mcuboot-led0`` alias over the ``bootloader-led0`` alias.
* Removed :c:func:`device_get_binding()` usage in favor of :c:func:`DEVICE_DT_GET()`.

* boot_serial:

  * Upgraded from cddl-gen v0.1.0 to zcbor v0.4.0.
  * Refactored and optimized the code, mainly in what affects the progressive erase implementation.
  * Fixed a compilation issue with the echo command code.

* imgtool: Added support for providing signature through a third party.



* Documentation:

  * Updated:

    * :ref:`mcuboot:mcuboot_ncs` now includes information regarding the bootloader user guides in the |NCS| documentation.
    * :ref:`ug_nrf5340` now includes information about using MCUboot's serial recovery of the network core image.

  * Removed:

    * The "Zephyr Test Plan" page in the MCUboot documentation set.
    * The "Building and using MCUboot with Zephyr" page in the MCUboot documentation set.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``71ef669ea4a73495b255f27024bcd5d542bf038c``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 71ef669ea4 ^45ef0d2

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^71ef669ea4

The current |NCS| main branch is based on revision ``71ef669ea4`` of Zephyr.

The following list summarizes the major additions specific to the |NCS|:

* Added Wi-Fi L2 layer.
  This is a fork of Zephyr's Wi-Fi L2 with the added support for the WPA supplicant.

zcbor
=====

* Updated the `zcbor`_ module from v0.4.0 to v0.5.1.
  Release notes for v0.5.0 and v0.5.1 are located at :file:`ncs/modules/lib/zcbor/`.
* Regenerated :ref:`lib_fmfu_fdev` code using zcbor v0.5.1.

Trusted Firmware-M
==================

* Added:

  * Support for an identity key that can be used as a PSA attestation key.
  * TF-M support for FPU Hard ABI.

* Updated:

  * TF-M version to 1.6.0.
  * :ref:`TF-M <ug_tfm>` is now enabled by default on Thingy:91.

* Fixed:

  * An issue with Thingy:91 v1.5.0 and lower.
  * An issue where TF-M used more RAM compared to SPM in the minimal configuration.
  * An issue with non-secure storage partitions in external flash.

Documentation
=============

* Updated:

  * :ref:`ug_ble_controller` with a note about the usage of the Zephyr LE Controller.
  * :ref:`software_maturity` with entries for security features: TF-M, PSA crypto, Immutable bootloader, HW unique key.
  * :ref:`ug_nrf91` with the following changes:

    * In the :ref:`ug_nrf91_features` page, added a section about :ref:`modem_trace`.
    * In the :ref:`ug_nrf9160_gs` guide, :ref:`nrf9160_gs_updating_fw_modem` section is now moved before :ref:`nrf9160_gs_updating_fw_application` because updating modem firmware erases application firmware.
    * In the :ref:`ug_nrf9160` guide, the :ref:`build_pgm_nrf9160` section now mentions |VSC| and command-line instructions.
    * In the :ref:`ug_thingy91_gsg` guide, Programming firmware and :ref:`connect_nRF_cloud` sections now have different structure.
    * The instructions and images in the :ref:`ug_thingy91_gsg` and :ref:`ug_nrf9160_gs` guides now also mention accepting :term:`eUICC Identifier (EID)` when activating your iBasis SIM card from the `nRF Cloud`_ website.

  * :ref:`ug_thread_configuring` page to better indicate what is required and what is optional.
    Also added further clarifications to the page to make everything clearer.
    As part of this change, the former :ref:`ug_thread_prebuilt_libs` section has been moved to a separate page.
  * :ref:`ug_matter_tools` page with a new section about the ZAP tool.
  * :ref:`caf_settings_loader` page with a section about the file system used as settings backend.
