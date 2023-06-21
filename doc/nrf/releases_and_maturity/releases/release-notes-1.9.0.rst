.. _ncs_release_notes_190:

|NCS| v1.9.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and re-distributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
The feature is made available in its current state though the design and interfaces can change between release tags.
The feature will also be labelled with "EXPERIMENTAL" in Kconfig files to indicate this status.
Build warnings will be generated to indicate when features labelled EXPERIMENTAL are included in builds unless the Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is disabled.

Highlights
**********

* Added experimental support for Nordic Distance Toolbox with examples. The software package adds phase-based distance measurement support for the nRF52 and nRF53 Series of devices.
* Added support for Amazon Frustration-Free Setup (FFS) for Matter devices.
* Added deep power-down mode to QSPI driver to optimize energy usage for solutions using an external flash memory.
* Added support for micro:bit v2 board.
* Added support for filtered ephemeris that improves A-GPS download size.
* Updated the support for Periodic Advertising with the SoftDevice Controller. It is no longer experimental.
* Updated the Zigbee solution on nRF5340. It is no longer experimental and is now Zigbee Platform Certification compliant.
* Replaced proprietary sockets with nrf_modem APIs that include link-time optimization, thereby reducing memory usage.

Sign up for the `nRF Connect SDK v1.9.0 webinar`_ to know more about the new features.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.9.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

For information on the included repositories and revisions, see `Repositories and revisions`_.

IDE support
***********

|NCS| v1.9.0 is supported by both `nRF Connect for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ and SEGGER Embedded Studio Nordic Edition.
In future releases, documentation on the setup and usage of Segger Embedded Studio Nordic Edition will no longer be available and all references to IDE support will instruct users with nRF Connect for Visual Studio Code.
In future releases, Segger Embedded Studio will no longer be tested with the build system and it is not recommended for new projects.

nRF Connect extensions for Visual Studio Code will be under continued active development and supported and tested with future |NCS| releases.
It is recommended for all new projects.

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
     - `Changelog <Modem library changelog for v1.9.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v1.9.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.9.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Updated:

  * :ref:`app_build_system` by adding information about default enabling of experimental Kconfig option and the generation of warnings when experimental features are selected.
  * :ref:`ug_edge_impulse` by adding instructions on how to download a model from a public Edge Impulse project.
  * :ref:`ug_nrf_cloud` by adding a section that describes and compares MQTT and REST.

* Removed:

  * Using Pelion with the |NCS| user guide as part of removal of Pelion DM support.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Matter
------

* Added:

  *  ``EXPERIMENTAL`` select in Kconfig that informs that Matter support is experimental.

* Updated:

  * :ref:`ug_matter_configuring` by embedding the Matter tutorial video.
  * :ref:`ug_matter_configuring_protocol` by adding information about setting Matter log levels and available device identification optional features. See sections :ref:`ug_matter_configuring_optional_log` and :ref:`ug_matter_configuring_device_identification` for details.
  * :ref:`ug_matter_creating_accessory` by updating the described procedure to match the current Matter API.


Thread
------

* Updated:

  * Enabling of Thread 1.2. Refer to :ref:`thread_ug_thread_specification_options` and :ref:`thread_ug_feature_sets` for more information.
  * :ref:`ug_thread_cert` by adding instructions for using VS Code extension as an alternative to command line when building the CLI sample for Thread certification using the Thread Test Harness.
  * :ref:`thread_ug_feature_sets` by adding support for SRP client and removing rarely used features.

Zigbee
------

* Added:

  * Experimental support for Zigbee Cluster Library ver8 (ZCL8), included in :ref:`nrfxlib:zboss` v3.11.1.177.
  * Support for Zigbee Base Device Behavior v3.0.1 (BDB 3.0.1).
  * Set of Zigbee libraries with binary trace logs enabled.
  * Stability and performance improvements.
  * Documentation for collecting Zigbee trace logs. See :ref:`ug_zigbee_configuring_zboss_traces` for more information.


* Updated:

  * Support for nRF5340 and for the combination of nRF5340 and nRF21540. The support is not experimental anymore.
  * :ref:`nrfxlib:zboss` to v3.11.1.0 and platform v5.1.1 (``v3.11.1.0+v5.1.1``).
  * :ref:`ZBOSS Network Co-processor Host <ug_zigbee_tools_ncp_host>` package to the new version v2.1.1.
  * :ref:`lib_zigbee_osif` library with Kconfig options that allow to either reset or halt the device upon ZBOSS stack assert. Reset is enabled by default.

* Fixed:

  * Several bugs.

HomeKit
-------

* Added:

  * Nordic proprietary Device Firmware Update over Bluetooth LE (Nordic DFU) based on mcumgr.
  * Development support for Weather Station application on Thingy:53 for HAP over Thread.

* Fixed:

  * An issue where CLI command hap services returns incorrect results (KRKNWK-11666).
  * An issue where HAP tool’s provision command freezes (KRKNWK-11365).
  * An issue where deactivating HAP logging leads to build error (KRKNWK-12397).

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Support for A-GPS filtered ephemerides.
  * Functionality that allows the application to wait for A-GPS data to be processed before starting GNSS positioning.
  * New documentation for the following modules in :ref:`asset_tracker_v2`:

    * :ref:`asset_tracker_v2_app_module`
    * :ref:`asset_tracker_v2_cloud_module`
    * :ref:`api_cloud_wrapper`
    * :ref:`asset_tracker_v2_data_module`
    * GNSS module
    * :ref:`asset_tracker_v2_modem_module`
    * :ref:`api_modules_common`
    * :ref:`asset_tracker_v2_sensor_module`
    * :ref:`asset_tracker_v2_ui_module`
    * :ref:`asset_tracker_v2_util_module`

  * Support for atmospheric pressure readings retrieved from the BME680 sensor on Thingy:91. See :ref:`asset_tracker_v2_sensor_module` for more information.

* Updated:

  * Code and documentation to use the acronym GNSS instead of GPS when not referring explicitly to the GPS system.
  * Support for P-GPS preemptive updates and P-GPS coexistence with A-GPS.


* Fixed:

  * An issue where PSM was requested from the network even though it was disabled in Kconfig.
  * An nRF Cloud FOTA issue by only enabling image types on the cloud if they are enabled on the device.

nRF9160: Asset Tracker
----------------------

* Removed nRF9160: Asset Tracker.
  It is recommended to upgrade to the :ref:`asset_tracker_v2` application.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added:

  * ``CONFIG_ML_APP_SENSOR_EVENT_DESCR`` option that globally defines the sensor used by the application modules.
  * Bluetooth LE bonding functionality.
    The functionality relies on :ref:`caf_ble_bond`.

* Updated:

  * ``ml_state`` module by renaming it to ``ml_app_mode`` module.
  * Architecture diagram.
  *	Documentation by adding a section on Power management.

Thingy:53: Matter weather station
---------------------------------

* Added:

  * Initial support for the Matter-compliant over-the-air (OTA) device firmware upgrade (DFU) method.

nRF Desktop
-----------

* Added:

  * Possibility to ask for bootloader variant using config channel. See :ref:`module_variant <dfu_bootloader_var>` for more information.
  * Kconfig options that allow erasing dongle bond on the gaming mouse using buttons or config channel. See the Erasing dongling peers section in :ref:`nrf_desktop_ble_bond` for more information.
  * Two states to enable erasing dongle peer - ``STATE_DONGLE_ERASE_PEER`` and ``STATE_DONGLE_ERASE_ADV``. See the Dongle states section in :ref:`nrf_desktop_ble_bond` for more information.
  * New application specific Kconfig option to enable :ref:`nrf_desktop_ble_bond`.
  * Configuration for ``nrf52833dk_nrf52820`` to allow testing of nRF Desktop dongle on the nRF52820 SoC.
  * Configuration for `Works With ChromeBook (WWCB)`_.
  * New documentation :ref:`nrf_desktop_hid_state_pm`.

* Updated:

  * Fn key related macros by moving them to an application specific header file (:file:`configuration/common/fn_key_id.h`).
  * :ref:`nrf_desktop_buttons` by adding a description about Key ID.
  * Config channel to no longer use orphaned sections to store module Id information.
    Hence, the :kconfig:option:`CONFIG_LINKER_ORPHAN_SECTION_PLACE` option is no longer required in the config file.
  * Bluetooth transmission power level for the :ref:`nrf_desktop_ble_adv`.
    It is now assumed to be 0 dBm during advertising.

  * Documentation and diagrams for the :ref:`nrf_desktop_ble_bond`.
  * Architecture diagrams.
  * Documentation with information about fwupd support.

nRF Pelion client
-----------------

* Removed the nRF Pelion client application as part of removing the Pelion DM support.

nRF9160: Serial LTE modem
-------------------------

* Updated:

  * :ref:`slm_description` as part of adding the overlay for Thingy:91 target:

    * Added:

      * :ref:`slm_testing_twi` in :ref:`slm_testing`.
      * :ref:`slm_connecting_thingy91`.

    * Updated:

      * :ref:`SLM_AT_TWI`.
      * Additional configuration section by adding information about the configuration files for Thingy:91.

  * SLM UART #XSLMUART section in :ref:`SLM_AT_gen`.
  * :ref:`slm_config` with the configuration option related to SLM UART.


Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added:

  * :ref:`multiple_adv_sets` sample.
  * :ref:`bluetooth_direction_finding_central` sample.
  * :ref:`direction_finding_peripheral` sample.
  * :ref:`ble_nrf_dm` sample, which uses the new GATT library :ref:`ddfs_readme`.

* Updated:

  * :ref:`peripheral_rscs` sample:

    * Corrected the number of bytes for setting the Total Distance Value and specified the data units. See :ref:`peripheral_rscs_testing` for more details.

  * :ref:`direct_test_mode` sample:

    * Added support for front-end module devices that support 2-pin PA/LNA interface with additional support for the Skyworks SKY66114-11 and the Skyworks SKY66403-11. See the Skyworks front-end module and Vendor-specific packet payload sections in the sample documentation.

  * :ref:`peripheral_hids_mouse` and :ref:`peripheral_hids_keyboard` samples:

    * Added a notice about encryption requirement.

  * :ref:`central_and_peripheral_hrs` sample:

    * Clarified the Requirements section and fixed a link in the Testing section of the sample documentation.

  * :ref:`peripheral_uart` sample:

    * Increased the system workqueue stack size for the :file:`prj_minimal.conf` file.

  * Aligned all samples to use the Bluetooth helper macros :c:macro:`BT_UUID_16_ENCODE` and :c:macro:`BT_CONN_CB_DEFINE`.

Matter samples
--------------

* Updated:

  * :ref:`matter_lock_sample` and :ref:`matter_light_bulb_sample` samples:

    * Added initial support for the Matter-compliant over-the-air (OTA) device firmware upgrade (DFU) method. See Device Firmware Upgrade support (Configuration section) in :ref:`matter_lock_sample` sample documentation.

nRF9160 samples
---------------

* Updated:

  * :ref:`modem_shell_application` sample:

    * Added:

      * A new shell command ``cloud`` for establishing an MQTT connection to nRF Cloud. See the Cloud section in the sample documentation.
      * A new shell command ``filtephem`` to enable or disable nRF Cloud A-GPS filtered ephemerides mode (REST only).
      * Various PPP updates. For example, the sample uses Zephyr async PPP driver configuration to provide better throughput for dial-up. See the PPP support section in the sample documentation for instructions to set up PPP on Linux.
      * The functionality to use LED 1 on the development kit to indicate the LTE registration status.

    * Removed:

      * Support for the GPS driver.


  * :ref:`http_application_update_sample` sample:

    * Added support for application downgrade.
      The sample now alternates updates between two different application images.

  * :ref:`gnss_sample` sample:

    * Added:

     * Support for minimal assistance using factory almanac, time, and location. See the Minimal assistance section in the sample documentation for details.
     * Support for TTFF test mode. See the Operation modes section in the sample documentation for details.
     * Support for nRF Cloud A-GPS filtered mode.

  * nRF9160: HTTP update samples:

    * Updated the samples to set the modem in power off mode after the firmware image download completes. This avoids ungraceful disconnects from the network upon pressing the reset button on the kit.

  * nRF9160: AWS FOTA sample:

    * Updated the Troubleshooting section in the sample documentation.

  * :ref:`lwm2m_client` sample:

    * Added support for triggering neighbor cell measurements.

  * Secure Partition Manager sample:

    * Updated the sample by reducing the amount of RAM reserved in the default configuration of the sample for nRF9160, freeing up 32 Kb of RAM for the application.

OpenThread samples
------------------

* Updated:

  * :ref:`ot_cli_sample` sample:

    * Removed :file:`overlay-thread_1_2.conf` as Thread 1.2 is now supported as described in :ref:`thread_ug_thread_specification_options`.
    * Updated :ref:`ot_cli_sample_activating_variants` to add information about the overlays to enable RTT logging and thread awareness.

Other samples
-------------

* Updated:

  * :ref:`radio_test`:

    * Added support for front-end module devices that support 2-pin PA/LNA interface with additional support for the Skyworks SKY66114-11 and the Skyworks SKY66403-11. See the section Skyworks front-end module in the sample documentation for more information.

  * Secure Partition Manager sample:

    * Fixed:

      * An issue where low baud rates would trigger a fault by selecting as system clock driver ``SYSTICK`` instead of ``RTC1`` (NCSDK-12230).

Zigbee samples
--------------

* Updated:

  * :ref:`zigbee_light_bulb_sample` sample and :ref:`zigbee_network_coordinator_sample` sample:

    * Updated the User interface section in the respective sample documentation.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Flash driver
------------

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* Updated:

  * :ref:`liblwm2m_carrier_readme` library:

    * Updated to v0.22.0.
      See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* Added:

  * New library :ref:`ddfs_readme`.

* Updated:

  * :ref:`rscs_readme` library:

    * Added units for :c:struct:`bt_rscs_measurement` members.

  * :ref:`ble_rpc` library:

    * Fixed the issue related to missing buffer size variables for the user PHY update and the user data length update procedures.

  * :ref:`hids_readme`:

    * Added information about encryption configuration.

Bluetooth mesh
++++++++++++++

* Updated:

  * :ref:`bt_mesh` library:

    * Extracted proportional-integral (PI) regulator module from :ref:`bt_mesh_light_ctrl_srv_readme`, enabling easier implementation of custom regulators. For more information, see the documentation on :ref:`bt_mesh_light_ctrl_reg_readme` and :ref:`bt_mesh_light_ctrl_reg_spec_readme`.

Common Application Framework (CAF)
----------------------------------

* Added:

  * A simple implementation of the :ref:`caf_ble_bond`.
    The implementation allows to erase bonds for default Bluetooth local identity.

  * :ref:`nRF Desktop settings loader <nrf_desktop_settings_loader>` is migrated to :ref:`lib_caf` as :ref:`CAF: Settings loader module <caf_settings_loader>`.

* Updated:

  * :ref:`caf_leds`:

    * Added:

      * New LED effect macro :c:macro:`LED_EFFECT_LED_BLINK2`.
      * A macro to pass color arguments between macro calls :c:macro:`LED_COLOR_ARG_PASS`.

  * :ref:`caf_buttons`:

    * Clarified what Key ID means in a button event.

  * :ref:`caf_ble_adv`:

    * Added:

      * Possibility of setting custom Bluetooth LE advertising intervals.

   * Updated:

     * Unify module ID reference location:

       * The array holding module reference objects is explicitly defined in linker script to avoid creating an orphan section. ``MODULE_ID`` macro and :c:func:`module_id_get` function now returns module reference from dedicated section instead of module name. The module name cannot be obtained from reference object directly. Instead, a helper function (:c:func:`module_name_get`) must be used.

* Fixed:

  * The known issue related to directed advertising in CAF (NCSDK-13058).
  * NULL dereferencing in :ref:`caf_sensor_manager` during :c:struct:`wake_up_event` handling for sensors that do not support trigger.

Bootloader libraries
--------------------

* Added a separate section for :ref:`lib_bootloader`.

Libraries for networking
------------------------

* Updated:

  * :ref:`lib_fota_download` library:

    * Updated:

      * The library to skip host name check when connecting to TLS service using just IP address.
      * Standardized bootloader FOTA download to accept only full dual path names (for example :file:`path/to/s0.bin path/to/s1.bin`). Truncated dual path names (:file:`path/to/s0.bin s1.bin`) are no longer supported.

    * Fixed:

      * An issue where the application would not be notified of errors originating from :c:func:`download_with_offset`. In the HTTP update samples, this would result in the dfu start button interrupt being disabled after a connect error in :c:func:`download_with_offset` and after a disconnect during firmware download.
      * An issue where the :ref:`lib_fota_download` library restarted the firmware image download from an incorrect offset after a link-connection timeout.
        This resulted in an invalid image and required a restart of the firmware update process.

  * :ref:`lib_nrf_cloud_agps` library:

    * Added a section on Optimizing cloud data downloads as part of adding support for  A-GPS filtered ephemerides mode when using MQTT with nRF Cloud.

  * :ref:`lib_azure_iot_hub` library:

    * Added a note about the known connectivity issue with Azure IoT Hub when the device is not reachable.

  * :ref:`lib_location` library:

    * Added A-GPS filtered ephemerides support for both MQTT and REST transports.

  * :ref:`lib_nrf_cloud` library:

    * Added:

      * A-GPS filtered ephemerides support, with the ability to set matching threshold mask angle, for both MQTT and REST transports.

        * When filtered ephemerides is enabled, A-GPS assistance requests to cloud are limited to no more than once every two hours.

    * Updated:

      * MQTT connection error handling.
        Now, unacknowledged pings and other errors result in a transition to the disconnected state.
        This ensures that reconnection can take place.

      * The library to correctly trigger MQTT disconnection on nRF Cloud device removal.


  * :ref:`lib_nrf_cloud_rest` library:

    * Updated:

      * The library to use the :ref:`lib_rest_client` library for REST API calls.
      * The library by adding the :c:func:`nrf_cloud_rest_send_device_message` function that sends a JSON message to nRF Cloud using the ``SendDeviceMessage`` endpoint.
      * The library by adding the :c:func:`nrf_cloud_rest_send_location` function that sends an NMEA sentence to nRF Cloud as a GPS-type device message.

  * :ref:`lib_lwm2m_client_utils` library:

    * Added support for LwM2M object ECID-Signal Measurement Information (object ID 10256).

  * :ref:`lib_nrf_cloud_pgps` library:

    * Removed the word ALL from the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REQUEST_UPON_INIT` option. Changed the :c:func:`nrf_cloud_pgps_init` function to use this to disable all three reasons for downloading P-GPS data, not just the full set.

DFU libraries
-------------

* Updated:

  * :ref:`lib_dfu_target` library:

    * Updated the implementation of modem delta upgrades in the DFU target library to use the new socketless interface provided by the :ref:`nrf_modem`. See the Modem delta upgrades section in the library documentation.

Modem libraries
---------------

* Updated:

  * :ref:`nrf_modem_lib_readme`:

    * The modem trace handling is moved from :file:`nrf_modem_os.c` to a new file :file:`nrf_modem_lib_trace.c`, which also provides the API for starting a trace session for a given time interval or until a given size of trace data is received.
      Updated the documentation with information about the Modem trace module.

    * Fixed:

      * A bug in the socket offloading component, where the :c:func:`recvfrom` wrapper could do an out-of-bounds copy of the sender's address, when the application is compiled without IPv6 support. In some cases, the out of bounds copy could indefinitely block the :c:func:`send` and other socket API calls.
      * A bug in the socket offloading component, where the :c:func:`accept` wrapper did not free the reserved file descriptor if the call to :c:func:`nrf_accept` failed. On subsequent failing calls to :c:func:`accept`, this bug could result in the OS running out of file descriptors.

  * :ref:`at_monitor_readme` library:

    * Added :c:macro:`AT_MONITOR_ISR` macro to monitor AT notifications in an interrupt service routine. See Deferred dispatching and Direct dispatching sections in the library documentation for more information.
    * Removed :c:func:`at_monitor_init` function and :kconfig:option:`CONFIG_AT_MONITOR_SYS_INIT` option. The library now initializes automatically when enabled.

  * :ref:`at_cmd_parser_readme` library:

    * Updated the library to parse AT command responses containing the response result, for example, ``OK`` or ``ERROR``.

  * :ref:`lib_location` library:

    * Updated the documentation by adding information about the changes to the library to support A-GPS filtered ephemerides mode when using MQTT with nRF Cloud and by listing the support of high location accuracy.

  * :ref:`sms_readme` library:

    * Updated the documentation to reflect the use of new AT API.

  * :ref:`lte_lc_readme` library:

    * Updated:

      * API calls that initialize the library will now return 0 if the library has already been initialized.
      * Documentation by adding information about LTE fallback mode. See the section Connection fallback mode in the library documentation.

    * Fixed:

      * An issue where subsequent calls to :c:func:`lte_lc_connect` or :c:func:`lte_lc_init_and_connect` could hang until timeout, and in some cases change the LTE mode.

* Removed:

  * AT command interface library.
  * AT command notifications library.

Zigbee libraries
----------------

* Updated:

  * :ref:`lib_zigbee_osif` library documentation by updating the Configuration section with new Kconfig options.
  * :ref:`lib_zigbee_error_handler` library documentation with information about the new function :c:func:`zb_osif_abort`.

Other libraries
---------------

* Added:

  * New library :ref:`mod_dm`.

* Updated:

  * :ref:`lib_bootloader` library:

    * Moved :ref:`lib_bootloader` to a section of their own.
    * Added write protection by default for the image partition.

  * :ref:`lib_date_time` library:

    * Removed the :kconfig:option:`CONFIG_DATE_TIME_IPV6` Kconfig option.
      The library now automatically uses IPv6 for NTP when available.
    * Updated the documentation as part of refactoring the NTP server selection logic.

  * :ref:`lib_location` library:

    * Added support for GNSS high accuracy.

  * :ref:`lib_ram_pwrdn` library:

    * Added:

      * Functions for powering up and down RAM sections for a given address range.
      * Experimental functionality to automatically power up and down RAM sections based on the libc heap usage.
      * Support for nRF5340 application core to power up and power down RAM sections.

  * :ref:`app_event_manager`:

    * Added:

      * ``APP_EVENT_SUBSCRIBE_FIRST`` subscriber priority and updated the documentation about this functionality.

    * Updated:

      * The sections used by the event manager and stopped using orphaned sections.

        * Event manager no longer uses orphaned sections to store information about event types, listeners, and subscribers.    Hence, the :kconfig:option:`CONFIG_LINKER_ORPHAN_SECTION_PLACE` option is no longer required in the config file.

      * Reworked priorities.


    * Removed:

      *  Forced alignment for x86.

  * :ref:`app_event_manager_profiler_tracer`:

    * Updated:

      * Event manager Profiler Tracer to no longer use orphaned sections to store nRF Profiler information.
        Hence, the :kconfig:option:`CONFIG_LINKER_ORPHAN_SECTION_PLACE` option is no longer required in the config file.

TF-M libraries
--------------

* Updated:

  * :ref:`lib_tfm_ioctl_api`:

    * Updated the documentation about the IOCTL request types that have been moved to the upstream TF-M platform support.


sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

* Added:

  * :ref:`nrfxlib:nrf_dm` and corresponding overview, usage, changelog, and API documentation.

* Updated:

  * :ref:`crypto`:

    * Added PSA crypto drivers for CryptoCell and Oberon.
    * Updated:

      * :ref:`nrfxlib:nrf_cc3xx_platform_readme` to v0.9.13.
      * :ref:`nrfxlib:nrf_cc3xx_mbedcrypto_readme` to v0.9.13.
      * :ref:`nrfxlib:nrf_oberon_readme` to v3.0.10.

    * Deprecated the ability of Mbed TLS API to enable multiple cryptographic backends in a build, such as the Oberon accelerated library or the CryptoCell hardware accelerator. This applies only to the APIs with the ``mbedtls_`` prefix.

  * :ref:`softdevice_controller`:

    * Updated:

      * :ref:`softdevice_controller_scheduling` by updating :ref:`scheduling_priorities_table` and adding a section on Timing when synchronized to a periodic advertiser.

  * :ref:`nrf_modem`:

    * Updated:

      * :ref:`nrfxlib:nrf_modem` to v1.5.1.
      * Architecture diagram in :ref:`nrfxlib:architecture`.
      * :ref:`nrfxlib:nrf_modem_delta_dfu`.

    * Removed:

      * Overview diagram in :ref:`nrfxlib:nrf_modem`.
      * AT socket page.
      * Extensions page.

  * :ref:`nrf_security`:

    * Added experimental support for PSA crypto drivers in :ref:`nrf_security`.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Unity
-----

* Fixed a bug that resulted in the mocks for some functions not having the ``__wrap_`` prefix. This happened for functions declared with whitespaces between identifier and parameter list.

HID Configurator for nRF Desktop
--------------------------------

* Updated:

  * The script to recognize the bootloader variant as a DFU module variant for the configuration channel communication. The new implementation is backward compatible - the new version of the script checks for module name and acts accordingly. See the Performing DFU section in :ref:`nrf_desktop_config_channel_script` documentation.

  * Documentation on setting up the dependencies for Debian/Ubuntu/Linux Mint operating system.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``1eedec3e79``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Check of reset address in the incoming image validation phase. See :kconfig:option:`CONFIG_MCUBOOT_VERIFY_IMG_ADDRESS`.

* Updated:

  * Always call :c:func:`sys_clock_disable` in main since the empty :c:func:`sys_clock_disable` callback is provided if the platform does not support system clock disable capability.
  * Bootutil library to close flash_area after failure to read swap status information.
  * Allow image header bigger than 1 KB for encrypted images.
  * Zephyr:RSA to use a smaller SHA256 implementation.

* Fixed:

  * Zephyr/boot_serial_extension build failure on :c:macro:`MCUBOOT_LOG_MODULE` when LOG is disabled.
  * Status offset calculation in case of scratch area.
  * Build with image encryption using RSA (issue introduced by migration to MbedTLS 3.0.0).
  * Support for single application with serial recovery.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``45ef0d2``, plus some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 45ef0d2 ^zephyr-v2.7.0

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^45ef0d2

The current |NCS| main branch is based on revision ``45ef0d2`` of Zephyr, which is located between v2.7.0 and v3.0.0 of Zephyr.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``77ab003e5fcd409cd225b68daa3cdaf506ed1107``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for General Diagnostics, Software Diagnostics and Thread Network Diagnostics clusters.
  * Initial support for the Matter OTA (Over-the-air) Requestor role, used for updating the device firmware.

cddl-gen
========

The `cddl-gen`_ module has been updated from version 0.1.0 to 0.3.0.
Release notes for 0.3.0 can be found in :file:`ncs/nrf/modules/lib/cddl-gen/RELEASE_NOTES.md`.

The change prompted some changes in the CMake for the module, notably:

* The CMake function ``target_cddl_source()`` was removed.
* The non-generated source files (:file:`cbor_encode.c` and :file:`cbor_decode.c`) and their accompanying header files are now added to the build when :kconfig:option:`CONFIG_CDDL_GEN` is enabled.

Also, it prompted the following:

* The code of some libraries using cddl-gen (like :ref:`lib_fmfu_fdev`) has been regenerated.
* The sdk-nrf integration test has been expanded into three new tests.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* Added:

  * :ref:`ug_thingy91_gsg` guide moved to |NCS| documentation from infocenter. Reworked and renamed the Working with Thingy:91 user guide to :ref:`ug_thingy91`.
  * :ref:`ug_nrf9160_gs` guide moved to |NCS| documentation from infocenter. Renamed the Working with nRF9160 DK user guide to :ref:`ug_nrf9160` and removed the redundant modem firmware section from the user guide.

* Updated:

  * :ref:`ug_nrf52` guide by adding nRF Connect Device Manager as a tool for FOTA.
  * :ref:`ug_thingy53` guide by updating the section Samples and applications compatible with Thingy:53.

* Reorganized:

  * The contents of the :ref:`ug_app_dev` section:

    * Added:

      * New subpage :ref:`app_optimize` and moved the optimization sections under it.
      * New subpage "Using external components" and moved the sections for using external components or modules under it.

  * The contents of the :ref:`ug_radio_fem` section:

   * Added:

     * :ref:`ug_radio_fem_nrf21540_spi_gpio`.
     * :ref:`ug_radio_fem_direct_support`.
     * More information about supported protocols and hardware.
