.. _ncs_release_notes_180:

|NCS| v1.8.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
It includes the TF-M, MCUboot and the Zephyr RTOS open source projects, which are continuously integrated and re-distributed with the SDK.

The |NCS| is where you begin building low-power wireless applications with Nordic Semiconductor nRF52, nRF53, and nRF91 Series devices.
Wireless protocol stacks and libraries may be experimental if they are under development or based on verification and certification status.
Labels in Kconfig indicate experimental configurations and can generate warnings in build output if they are used in a project.
These configurations should not be used in production.
See the release notes and documentation for those protocols and libraries for further information.

Highlights
**********

* The :ref:`ble_rpc` library now has full API support to interface the Bluetooth® LE Host on another CPU core.
* The :ref:`central_and_peripheral_hrs` sample has been added to demonstrate how to use Bluetooth® with Central and Peripheral roles concurrently.
* The :ref:`nrf_modem` for nRF9160 has been updated to version 1.4.1 with improvements and support for Modem version 1.3.1.
* Wi-Fi coexistence with Bluetooth LE and LTE coexistence with Bluetooth LE have been added for the nRF52 family.
* Experimental configuration labels in Kconfig now generate warnings when experimental configurations are used.
  For more information, check the Experimental features section on the :ref:`zephyr:application` page in the Zephyr documentation.
* Energy consumption of Matter over Thread devices that are using Thread's Sleepy End Device role has been improved.
* Apple HomeKit ADK v6.1 has been integrated and support for nRF5340 for Bluetooth LE and Thread HomeKit accessories has been added.
* Support for the `Gazell`_ protocol has been added for the nRF52 family, with related libraries and samples.
* The :ref:`caf_preview_sample` sample has been added to demonstrate how to use CAF components to build event-based applications and interface with some simple IO like LEDs and Buttons.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v1.8.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` for more information.

Supported modem firmware
************************

See `Modem firmware compatibility matrix`_ for an overview of which modem firmware versions have been tested with this version of the |NCS|.

Use the latest version of the nRF Programmer app of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Known issues
************

See `known issues for nRF Connect SDK v1.8.0`_ for the list of issues valid for this release.

Changelog
*********

The following sections provide detailed lists of changes by component.

Application development
=======================

* Integrated Partition Manager with the new build type configuration scheme and updated several applications accordingly.
  The new scheme infers the build type from ``CONF_FILE`` instead of using the CMake variable ``CMAKE_BUILD_TYPE``.
  See :ref:`gs_modifying_build_types` for details.
* Build system:

  * Added an option to control the inclusion of RPMsg samples on the nRF53 network core :kconfig:option:`NCS_INCLUDE_RPMSG_CHILD_IMAGE`.
  * Updated generation of the :file:`manifest.json` file in the :file:`dfu_application.zip` and :file:`dfu_mcuboot.zip` files to include nrf and zephyr revisions reported by the new build file :file:`zephyr.meta`.
  * Build warnings are now printed when experimental features are enabled (NCSDK-6336).
    Warnings can be disabled by disabling :kconfig:option:`CONFIG_WARN_EXPERIMENTAL`
  * Fixed the NCSIDB-581 bug where application signing and file conversion for Device Firmware Update (DFU) could fail in SEGGER Embedded Studio during a build.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples and `Libraries`_ for lists of changes for the protocol-related libraries.

Bluetooth LE Controller
-----------------------

These changes are valid for :ref:`nrfxlib:softdevice_controller`.

* Added:

  * Experimental support for Periodic Advertising.
  * Production support for a 3-wire Packet Traffic Arbitration (PTA) interface for external radio coexistence on the nRF52 Series.
    This interface is typically implemented in the Wi-Fi products.
  * Experimental support for a 1-wire PTA interface for external radio coexistence for the nRF52 Series.
    This interface is specific to Nordic Semiconductor's nRF91 Series.
    See ``nrfxlib:bluetooth_coex`` for more information.
  * Support for the Simple GPIO Front-End Module implementation on the nRF53 Series.

Bluetooth mesh
--------------

* Updated several samples and libraries.
  For details, see `Bluetooth mesh samples`_ and `Bluetooth libraries`, respectively.

Matter
------

* Updated the `Matter (Project CHIP)`_ fork in the |NCS| to a newer version.
* Added the :ref:`ug_matter_configuring_protocol` user guide.
* Added a new documentation section :ref:`ug_matter_configuring` that contains several configuration guides for Matter.

Zigbee
------

* Updated ZBOSS Zigbee stack to version ``v3.9.0.1+v4.1.0``.
  See the :ref:`nrfxlib:zboss_changelog` in the nrfxlib documentation for detailed information.
* Added new version of the :ref:`ug_zigbee_tools_ncp_host` (v2.0.0).
* Added :ref:`ug_zigee_qsg`.
* Removed experimental support for Green Power Combo Basic functionality.
* Changed the default logging level in Zigbee applications to ``INF`` from Zephyr's :ref:`zephyr:logging_api` default level, which is set to ``ERR`` by default.

Gazell
------

* Added support for nRF52 Series.
* Added documentation pages about Gazell protocol under :ref:`protocols`.
* Added :ref:`lib_gazell`.
* Added :ref:`gazell_samples`.

HomeKit
-------

* Added:

  * Production support for nRF5340 for Thread and Bluetooth LE HomeKit accessories.
  * Production support for the nRF21540 front-end module combined with nRF53 Series SoCs.
  * Development support for Weather Station application on Thingy:53 for HAP over Bluetooth LE.
  * Using LED for indicating the state of Thread connectivity.

* Updated:

  * Modified the structure of folders for examples and applications.
  * ADK version has been updated to ADK 6.1.

* Bug fixes:

  * Fixed a stability issue in the Stateless Programmable Switch application.
  * Fixed memory access issues.
  * Fixed an issue with setting advertising interval.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Content-type and encoding properties to outgoing Azure MQTT messages.
  * Support for A-GPS and P-GPS in Azure IoT Hub integration.
  * New overlay configuration files and split the :file:`prj.conf` file to several files that now bind specific features.
    To build with a specific feature, such as P-GPS, Memfault or communications with AWS IoT, you need to include the respective overlay configuration in the build command.
    This is documented in :ref:`asset_tracker_v2_description`.

* Updated:

  * Updated the application to start sending batch messages to the new bulk endpoint topic supported in nRF Cloud.
  * Updated the application to use nRF Cloud A-GPS directly without the A-GPS library. SUPL is no longer supported.
  * Updated the application to start sending neighbor cell measurement data to nRF Cloud.
  * Updated the application to start sending neighbor cell measurement data to Azure IoT Hub.

nrf9160: Asset Tracker
----------------------

* Added timestamps to environment sensor data when compiled with :kconfig:option:`CONFIG_USE_BME680_BSEC`
* Updated the application to clear the ephemeris and almanac flags from an A-GPS request when P-GPS is enabled.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added:

  * Non-secure configuration for building :ref:`nrf_machine_learning_app` with :ref:`zephyr:thingy53_nrf5340`.
  * Secure configuration for building :ref:`nrf_machine_learning_app` with :ref:`zephyr:nrf5340dk_nrf5340`.
  * Power manager to the :ref:`nrf_machine_learning_app` application.

* Updated:

  * Updated information about custom build types.
  * The application switched to using the configuration file scheme based on :file:`prj.conf` files.
    See :ref:`gs_modifying_build_types` for details.

nRF Desktop
-----------

* Added:

  * Added documentation for :ref:`nrf_desktop_usb_state_pm`.
  * Added :ref:`nrf_desktop_ble_state_pm`.
  * Added peer erase feature for the ``nrf52840dk_nrf52840`` build target.

* Removed:

  * Removed configuration files used for building the application with :kconfig:option:`CONFIG_BT_LL_SW_SPLIT` for various boards.
    The configuration files for boards that do not have room for the SoftDevice LL in flash or SRAM remain untouched.

* Updated:

  * Updated to use DTS overlays instead of Kconfig configuration files for setting up external flash memory.
  * Updated information about custom build types.
  * The application switched to using the configuration file scheme based on :file:`prj.conf` files.
    See :ref:`gs_modifying_build_types` for details.
  * Updated documentation for :ref:`nrf_desktop_usb_state`.
  * Updated documentation for :ref:`nrf_desktop_config_channel` and added more detailed protocol description.
  * Updated :ref:`nrf_desktop_config_channel` to respond with the disconnected status to explicitly inform the host tools that the given HID instance cannot be used to configure device.
  * Updated documentation with information about forwarding boot reports.
    See the documentation page of nRF Desktop's :ref:`nrf_desktop_hid_forward` for details.

* Bug fixes:

  * Fixed an issue that was causing the HID keyboard LEDs to remain turned on after host disconnection while no other hosts were connected.
  * Fixed an issue that was causing an assertion failure in the :ref:`nrf_desktop_hid_state` on the nRF Desktop peripheral device during the boot of the host device connected through USB.

nRF Pelion Client
-----------------

* Updated:

  * Updated to use DTS overlays instead of Kconfig configuration files for setting up external flash memory.
  * The application switched to using the configuration file scheme based on :file:`prj.conf` files.
    See :ref:`gs_modifying_build_types` for details.

Thingy:53: Matter weather station
---------------------------------

* Added:

  * Support for the Power Source cluster, used to expose information about the battery.
  * Support for the Identify cluster, which uses the built-in buzzer to help find the device.

* Updated:

  * Updated to use DTS overlays instead of Kconfig configuration files for setting up external flash memory.
  * The application switched to using the configuration file scheme based on :file:`prj.conf` files.
    See :ref:`gs_modifying_build_types` for details.

nRF Machine Learning
--------------------

* Updated:

  * Updated to use DTS overlays instead of Kconfig configuration files for setting up external flash memory.

nRF9160: Serial LTE modem
-------------------------

* Updated the ``#XFOTA`` command to accept an integer parameter to specify the PDN ID to be used for the download, instead of the APN name.
* Added new AT commands related to the General Purpose Input/Output (GPIO).
* Added the ``#XUUID`` command to read out the device UUID from the modem.
* Added to the ``XNRFCLOUD`` command the following features:

  * The possibility to send to and receive from nRF Cloud JSON messages in data mode.
  * The ability to read out the ``sec_tag`` and the UUID of the device.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added :ref:`central_and_peripheral_hrs` sample.
* Updated some samples to use DTS overlay instead of Kconfig for external flash.
* Updated some samples with support for :ref:`zephyr:thingy53_nrf5340` in non-secure configuration.
* Removed the ``pairing_confirm`` callback from the Bluetooth samples where it was incorrectly used, that is where pairing was accepted without user interaction.
* :ref:`direct_test_mode`:

  * Isolated the usage of Timer to the nRF52840 device in the workaround for Anomaly 172.
  * Replaced the busy wait mode with the idle mode to reduce RF noise coming from accesses to flash on devices that do not support instruction cache.

* :ref:`ble_llpm` sample - Added role selection.
* :ref:`peripheral_bms` sample - Modified the Testing section in the documentation.
* :ref:`peripheral_hids_mouse` and :ref:`central_uart` samples - These samples now come with the :ref:`ble_rpc_host` child image configuration overlay.
  The overlay shows how to configure an application running a serialized Bluetooth Host.
  These samples run out the box with the :ref:`ble_rpc` library.
* :ref:`peripheral_uart` sample - The sample is now the default one for the :ref:`ble_rpc` library.
  The sample runs out of the box with a serialized Bluetooth Host.

Bluetooth mesh samples
----------------------

* Added:

  * :ref:`bluetooth_ble_peripheral_lbs_coex`, demonstrating how to combine Bluetooth mesh and Bluetooth Low Energy features in a single application.
  * Support for :ref:`zephyr:nrf21540dk_nrf52840`.

* Updated:

  * :ref:`bluetooth_mesh_light` and :ref:`bluetooth_mesh_light_switch` with support for :ref:`zephyr:thingy53_nrf5340` in non-secure configuration.
  * :ref:`bluetooth_mesh_light_lc` and :ref:`bluetooth_mesh_sensor_server` with support for :ref:`zephyr:thingy53_nrf5340`.
  * Updated some samples to use DTS overlays instead of Kconfig configuration files for setting up external flash memory.

Gazell samples
--------------

* New section.
* Added:

  * :ref:`Gazell ACK Payload <gzll_ack_payload_host>`
  * :ref:`Gazell Dynamic Pairing <gzp_dynamic_pairing_host>`

HomeKit samples
---------------

* Added samples are using Apple HomeKit ADK v6.1.
* Updated the role of buttons in all samples due to the ADK update:

	* Button 1 - Clear pairing
	* Button 2 - Factory reset
	* Button 3 - Enter pairing mode
	* Button 4 - Application action

Matter samples
--------------

* Added a table that lists variants and extensions available out of the box for each Matter sample on the :ref:`matter_samples` page.
* :ref:`matter_lock_sample`:

  * Added multi-image Device Firmware Upgrade over Bluetooth LE support for the nRF5340 DK.
  * Added low-power build support.

* :ref:`matter_light_bulb_sample`:

  * Added multi-image Device Firmware Upgrade over Bluetooth LE support for the nRF5340 DK.

NFC samples
-----------

* Added the :ref:`record_launch_app` sample.

nRF9160 samples
---------------

* Added:

  * :ref:`nrf_cloud_rest_fota` sample, demonstrating how to perform FOTA updates with the nRF Cloud REST API.

* :ref:`https_client` sample:

  * Added a possibility to use TF-M and Zephyr Mbed TLS instead of using the offloaded TLS stack in modem.

* :ref:`lwm2m_client` sample:

  * Added support for Thingy:91.
  * Added more LwM2M objects.
  * LwM2M sensor objects now use the actual sensors available to the Thingy:91.
    If the nRF9160 DK is used, the object uses simulated sensors instead.
  * Added support for polling sensors and notifying the server if the measured changes are large enough.
  * Added support for full modem firmware update.
  * Increased the NB-IoT time (in seconds) before the registration timeout when the LwM2M Registration Update message is sent by the engine.

* Multicell location sample:

  * Modified to use runtime location service selection instead of compile-time configurations.

* :ref:`modem_shell_application` sample:

  * Added a new shell command ``rest`` for sending simple REST requests and receiving responses to them.
  * Added a new shell command ``location`` for using the Location library to retrieve device's location with different methods.
  * Updated the sample to use DTS overlays instead of Kconfig configuration files for setting up external flash memory.
  * Added support for nRF Cloud A-GPS and P-GPS.
    A-GPS support is enabled by default.
  * Added PPP-related updates:

    * Added IPv6 support.
    * Added LTE link MTU to be informed to PC.
    * Improved autostart of PPP.
    * Made changes for better performance.

* :ref:`gnss_sample` sample:

  * Added support for periodic fixes.
  * Added support for power saving.
  * Added support for low accuracy fixes.
  * Renamed.
    The previous name was "nRF9160: GPS with SUPL client library".
  * Added support for nRF Cloud A-GPS and P-GPS.
  * LTE now remains connected to the network all the time when assistance is enabled.
    With A-GPS, the sample can be configured to connect to network only when needed.

* nRF9160: A-GPS sample:

  * The sample has been removed.
    nRF Cloud A-GPS and P-GPS are demonstrated in the :ref:`gnss_sample` sample.

OpenThread samples
------------------

* Added:

  * Support for ``nrf5340dk_nrf5340_cpuapp_ns`` build target for :ref:`zephyr:nrf5340dk_nrf5340`.
    This allows to build the OpenThread samples with Trusted Firmware-M and the PSA crypto API support.
    This platform is experimental, so :ref:`nrfxlib:ot_libs` are not generated for it.

Zigbee samples
--------------

* Added:

   * :ref:`Zigbee shell <zigbee_shell_sample>` sample.

* Updated:

   * Fixed issue with cluster declaration in :ref:`Zigbee shell <zigbee_shell_sample>` sample and :ref:`Zigbee template <zigbee_template_sample>` sample.

Other samples
-------------

* Added the :ref:`hw_unique_key_usage` sample.
* Added the :ref:`caf_preview_sample` sample.
* :ref:`bootloader` sample:

  * Improved how hardware unique keys are handled.

    * Introduced :kconfig:option:`CONFIG_HW_UNIQUE_KEY_LOAD` with fewer dependencies than :kconfig:option:`CONFIG_HW_UNIQUE_KEY` solely for loading the key.
    * The bootloader now allows a single boot with no key present, to allow the app to write a key.
      After the first boot, the key must be present or the bootloader will not boot the app.

  * Bug fixes:

    * Fixed the NCSDK-10209 issue with alignment errors caused by changes in the size of partitions.
      The |NSIB| and MCUboot have been made more robust against such errors.

* :ref:`radio_test` sample:

  * Clarified units for numerical parameters in shell commands.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added the API documentation and conceptual documentation for the following drivers:

  * :ref:`sensor_sim`
  * :ref:`paw3212`
  * :ref:`pmw3360`

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* Added the :ref:`lib_hrs_client_readme` library.
* :ref:`ble_rpc` library:

  * Added support for the GATT API serialization.
  * Changed the configuration option that enables the library from the :kconfig:option:`CONFIG_BT_RPC` to the :kconfig:option:`CONFIG_BT_RPC_STACK`.

* :ref:`bms_readme` - Changed security permissions of the service's characteristics to require authentication.
* :ref:`hogp_readme` - Added a clarification about the report size in the documentation of one API function.
* :ref:`bt_mesh` library:

  * Aligned the Silvair EnOcean Proxy Server model implementation with rev 1.2 of the Silvair EnOcean Switch Mesh Proxy Server specification.
  * Fixed an issue where the Sensor Client API can be used as non-blocking by passing ``NULL`` to the arguments that are used to fill the response.

* :ref:`nus_client_readme` library:

  * Added context to functions :c:func:`bt_nus_client.received`, :c:func:`bt_nus_client.sent` and :c:func:`bt_nus_client.unsubscribed` to enable their use in a multi-client application.

Common Application Framework (CAF)
----------------------------------

* Added the following modules:

  * :ref:`caf_ble_state_pm`
  * :ref:`caf_buttons_pm_keep_alive`

* Updated:

  * :ref:`caf_power_manager`:

    * Added the state transition diagram on the documentation page.
    * The power management support in modules is now enabled by default when the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option is enabled.
    * Added a dependency on :kconfig:option:`CONFIG_PM_POLICY_APP`, which is required by the application that is using the :ref:`caf_power_manager` to link.

  * :ref:`caf_sensor_manager`:

    * Renamed from Sensor sampler.
      All references updated.
    * Extended the functionality of the module with passive and active power management.
    * Aligned initialization of the module with the documentation.
      The module now reports error state at init, only if all sensors fail to initialize.

* Also added the :ref:`caf_preview_sample` sample that demonstrates the use of CAF.

Gazell libraries
----------------

* New section.
* Added the following libraries:

  * :ref:`gzll_glue`
  * :ref:`gzp`

Modem libraries
---------------

* Added the following libraries:

  * :ref:`lib_location`
  * :ref:`lib_at_shell`

* :ref:`lte_lc_readme` library:

  * Changed the value of an invalid E-UTRAN cell ID from zero to UINT32_MAX for the LTE_LC_EVT_NEIGHBOR_CELL_MEAS event.
  * Added support for multiple LTE event handlers.
    Thus, deregistration is not possible by using ``lte_lc_register_handler(NULL)`` anymore and it is done by the :c:func:`lte_lc_deregister_handler` function in the API.
  * Added neighbor cell measurement search type parameter in :c:func:`lte_lc_neighbor_cell_measurement`.
  * Added timing advance measurement time to current cell data in :c:enum:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS` event.
  * Updated the library to use the :ref:`nrfxlib:nrf_modem_at` API and the :ref:`at_monitor_readme` library for AT commands.
  * Added support for periodic search configuration. API functions have been added to set, read and clear the configuration, and to request extra searches.

* :ref:`nrf_modem_lib_readme` library:

  * Added a possibility to create native sockets when nRF91 socket offloading is enabled.

* :ref:`pdn_readme` library:

  * Added an optional ``family`` parameter to :c:func:`pdn_activate`, which is used to report when the IP family of a PDN changes after activation.
  * Aligned the return values of :c:func:`pdn_init` to return negative errnos on error.
  * Added logging on modem errors.
  * Changed the return values on modem errors to ``-ENOEXEC`` to avoid conflicts with return of other positive values.

* A-GPS library:

  * The A-GPS library has been deprecated in favor of using the :ref:`lib_nrf_cloud_agps` library directly.

Libraries for networking
------------------------

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for Firmware Update object to use :ref:`lib_fota_download` library for downloading firmware images.
  * Added support for full modem firmware update.

* Multicell location library:

  * Updated to only request neighbor cell measurements when connected and to only copy neighbor cell measurements if they exist.
  * Added support for Polte location service.
  * Removed device ID from the ``multicell_location_get`` function parameter list. nRF Cloud and HERE did not use it
    Skyhook will now set modem UUID as its device ID.
  * Selection of location service changed from compile-time to runtime configuration.
  * Added support for MQTT transport for nRF Cloud service.

* :ref:`lib_nrf_cloud` library:

  * Removed the ``CONFIG_NRF_CLOUD`` Kconfig option.
  * Removed GNSS socket API support from A-GPS and P-GPS.
  * Added support for sending data to a new bulk endpoint topic that is supported in nRF Cloud.
    A message published to the bulk topic is typically a combination of multiple messages.
  * Changed REST API for A-GPS to use GNSS interface structure instead of GPS driver structure.
    Also changed from GPS driver ``GPS_AGPS_`` request types to ``NRF_CLOUD_AGPS_`` request types.
  * Added function :c:func:`nrf_cloud_jwt_generate` that generates a JWT using the library's configured values.
  * Added handling of MQTT ping failures and MQTT input failures.
  * Updated the :c:func:`nrf_cloud_configured_client_id_get` function to use :c:func:`nrf_modem_at_cmd` instead of the deprecated :c:func:`at_cmd_write`.
  * Added state checks to functions :c:func:`nrf_cloud_agps_request`, :c:func:`nrf_cloud_cell_pos_request`, :c:func:`nrf_cloud_pgps_request`, and :c:func:`json_send_to_cloud`.
    These functions should be called only after the device has connected to the nRF Cloud ``d2c`` topic.

* :ref:`lib_nrf_cloud_agps` library:

  * Removed GNSS socket API support.
  * Updated to always request ephemerides and almanacs.
    The application is now responsible for clearing the flags if P-GPS is enabled.

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed an issue with :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE` to ensure predictions are properly stored.
  * Fixed error handling associated with :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE`.
  * Added :c:func:`nrf_cloud_pgps_request_reset`, so P-GPS application request handler can indicate failure to process the request.
    This ensures the P-GPS library tries the request again.
  * Added :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_SOCKET_RETRIES`.
  * Changed :c:func:`nrf_cloud_pgps_init` to limit allowable :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` to an even number, and limited :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` to this value minus 2.
  * Updated the signature of :c:func:`npgps_download_start` to accept an integer parameter specifying the PDN ID, which replaces the parameter used to specify the APN.

* :ref:`lib_nrf_cloud_rest` library:

  * Added functions :c:func:`nrf_cloud_rest_shadow_state_update` and :c:func:`nrf_cloud_rest_shadow_service_info_update`.
    They enable device shadow updates using REST.

* :ref:`lib_rest_client` library:

  * Added REST client library for sending REST requests and receiving their responses.

* :ref:`lib_aws_iot` library:

  * Added handling of MQTT ping failures and MQTT input failures.

* :ref:`lib_azure_iot_hub` library:

  * Added handling of MQTT ping failures and MQTT input failures.
  * Updated the API version used in MQTT connection to Azure IoT Hub to 2020-09-30.
  * Added the :c:func:`azure_iot_hub_dps_reset` function for resetting the DPS information.
  * Added a note about the credentials and their location.

* :ref:`lib_download_client` library:

  * Removed the ``apn`` field in the ``download_client_cfg`` configuration structure.

* :ref:`lib_fota_download` library:

  * Updated the signature of :c:func:`fota_download_start_with_image_type` to accept an integer parameter specifying the PDN ID, which replaces the parameter used to specify the APN.

* :ref:`lib_nrf_cloud_cell_pos` library:

  * Added callback parameter to :c:func:`nrf_cloud_cell_pos_request` to handle response data from the cloud.

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v0.21.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Libraries for NFC
-----------------

* Added the :ref:`nfc_launch_app` library.

Trusted Firmware-M libraries
----------------------------

* Added support for non-secure storage.
  This feature enables non-secure applications to use the Zephyr Settings API to save and load persistent data.

Other libraries
---------------

* New libraries:

  * Added API documentation and :ref:`conceptual documentation page <wave_gen>` for the wave generator library.
  * Added documentation for the :ref:`app_event_manager_profiler_tracer` module.
  * Added documentation for :ref:`lib_fatal_error`.

* :ref:`app_event_manager` library:

  * Added a weak function to allow overriding the allocation in Event Manager.
  * Increased the number of supported Event Manager events.
  * Moved the Event Manager features responsible for profiling events into the :ref:`app_event_manager_profiler_tracer` module.
  * Added a sample showing the use of the nRF Profiler for Event Manager events.

* :ref:`ei_wrapper` library:

  * Expanded API to provide information about input data sampling frequency, every label used by the machine learning model, and results associated with every label.
  * Removed FPU dependency.
    The FPU is implied to speed up calculations.

* :ref:`fprotect_readme` library:

  * Added a new function ``fprotect_is_protected()`` for devices with the ACL peripheral.

* :ref:`lib_hw_unique_key` library:

  * Made the checking for ``hw_unique_key_write_random()`` stricter.
    This change will trigger panic if any key is unwritten after writing random keys.
  * Refactored the ``HUK_HAS_*`` macros to be defined or undefined instead of ``1`` or ``0``.
  * Added a new sample :ref:`hw_unique_key_usage` showing how to use a hardware unique key to derive an encryption key.
    The sample can be run with or without TF-M.
  * Fixed ``hw_unique_key_is_written()`` which would previously trigger a fault under certain circumstances.

* :ref:`nrf_profiler` library:

  * Updated Python scripts to use multiple processes that communicate over pipes.
  * Increased the number of supported nRF Profiler events.
  * Added a special nRF Profiler event for indicating a situation where the nRF Profiler's data buffer has overflowed and some events have been dropped, which causes the device to stop sending events.

* Secure Partition Manager (SPM):

  * Fixed the NCSDK-5156 issue with the size calculation for the non-secure callable region, which prevented users from adding a large number of custom secure services.
  * All EGU peripherals, instead of just EGU1 and EGU2, are now configurable to be non-secure and are configured as non-secure by default.
  * Fixed a bug where the fp context could be overwritten by other threads if both threads are using Non-secure-callable functions (secure services).

* :ref:`mod_memfault`:

  * Added PSM and eDRX configuration metrics that are collected when :kconfig:option:`MEMFAULT_NCS_LTE_METRICS` is enabled.

* :ref:`lib_date_time` library:

  * The library now stores the received date-time information as Zephyr and modem time.
    Also modem XTIME notifications are used as time source.
    Added the :kconfig:option:`CONFIG_DATE_TIME_AUTO_UPDATE` option to trigger a time update when device has connected to LTE.

Libraries for Zigbee
--------------------

* :ref:`lib_zigbee_shell`:

  * Added ZCL commands.
  * Added :ref:`BDB command for printing install codes <bdb_ic_list>`.
  * Improved logging.
  * Made several minor fixes and improvements.

* :ref:`lib_zigbee_osif`:

  * Improved logging.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nfxlib documentation <nrfxlib:README>` for additional information.

Modem library
+++++++++++++

* Updated:

  * Updated :ref:`nrf_modem` to version 1.4.1.
    See the :ref:`nrfxlib:nrf_modem_changelog` for detailed information.
  * nrf_errno values have been aligned with the errno values of newlibc C library.
  * The :ref:`Modem API <nrf_modem_api>` (:file:`nrf_modem.h`) has been updated to return negative errno values on error.
  * The :ref:`Full Modem DFU API <nrf_modem_bootloader_api>` (:file:`nrf_modem_full_dfu.h`) has been updated to return negative errno values on error.
  * The :ref:`GNSS API <nrf_modem_gnss_api>` (:file:`nrf_modem_gnss.h`) has been updated to return negative errno values on error.

* Removed:

  * The GNSS socket has been removed.
  * The PDN socket has been removed.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

Partition Manager
-----------------

* Partition manager information is no longer appended to the ``rom_report`` target.
  To inspect the current partition manager configuration please use the ``partition_manager_report`` target.
* Updated the ``share_size`` functionality to let a partition share size with a partition in another region.
* Added a new directive, ``align_next``, which controls the alignment of the next partition.
  See the Partition Manager documentation for more information.

DFU target
----------

* Fixed an issue where the offset to the last erased page was set incorrectly one page ahead whenever the flash write ended just after a page boundary.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``680ed07``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed support for Low Power in Zephyr's single-thread mode.
  See ``MCUBOOT_CPU_IDLE()`` macro.
* Switched USB CDC ACM serial recovery I/O device configuration from Kconfig to Devicetree with the compatible ``"zephyr,cdc-acm-uart"`` property.
* Switched UART serial recovery I/O device configuration from Kconfig to Devicetree using Zephyr's ``zephyr,console`` property of the chosen node.
* Fixed a deadlock issue with cryptolib selectors in Kconfig.
* Fixed an issue with the serial recovery skipping on nRF5340.
* Added cleanup of UARTE devices before chainloading the application.
  This allows the application to initialize the devices correctly and fixes potential missing output of the application's log.

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``657deb65``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes for this release.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each NCS release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``3f82656``, plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 3f82656 ^v2.6.0-rc1-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^3f82656

The current |NCS| main branch is based on the Zephyr v2.7 development branch.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, ``bbd19d92f6d58ef79c98793fe0dfb2979db6336d``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Support for Administrator Commissioning Cluster, which allows enabling or disabling the commissioning window on a Matter device.
    This is required by the Matter multi-admin functionality.
  * Support for Power Source Cluster, which exposes information about the power source of a Matter device, including the battery level.
  * Initial support for Thread Sleepy End Devices.

Documentation
=============

In addition to documentation related to the changes listed above, the following documentation has been updated:

* General changes:

  * Renamed occurrences of ``master`` branch to ``main`` to reflect the changes in the `nrfconnect GitHub organization`_.
  * Updated documentation to use the acronym GNSS instead of GPS when not talking explicitly about the GPS system.
  * Modified section names on this page.
    Now the section names better match the |NCS| code and documentation structure.

* :ref:`ncs_introduction`:

  * Added a section describing how licenses work in |NCS|.
  * Added a section describing the Git tool.
  * Expanded the existing section about the west tool.

* :ref:`gs_installing` - Added a note in the Install the GNU Arm Embedded Toolchain section about TF-M sample incompatibility issue related to GNU Arm Embedded Toolchain versions *9-2020-q2-update* and *10-2020-q4-major*.
  This was listed earlier as a known issue.
* :ref:`gs_programming`:

  * Updated the page with a note about Windows path length limitations.
    This was listed earlier as a known issue.
  * Updated the Building with SEGGER Embedded Studio section with a warning about a "no input files" error.

* :ref:`gs_updating` - Added a section about updating SEGGER Embedded Studio packages.
* :ref:`glossary` - Added new terms related to :ref:`ug_matter` and :ref:`ug_zigbee`.
* :ref:`ug_nrf52` - Added a section describing Bluetooth mesh under the Supported protocols section.
* :ref:`ug_nrf5340`:

  * Added a note about varying folder names of the network core child image when programming with nrfjprog.
  * Updated the multi-image build section of :ref:`ug_nrf5340` to better match the programming procedure.
  * Updated the :ref:`logging_cpunet` section with information about different virtual COM ports for the nRF5340 DK v1.0.0 and v2.0.0.
  * Added a section for Bluetooth mesh and its samples for application core under the section Protocols and use cases.
