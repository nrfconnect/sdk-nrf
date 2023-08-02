.. _ncs_release_notes_230:

|NCS| v2.3.0 Release Notes
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

Sign up for the `nRF Connect SDK v2.3.0 webinar`_ to learn more about the new features.

* Added :ref:`support <software_maturity>` for the following features:

  * nRF7002 Wi-Fi 6 companion IC and nRF7002 DK.
  * Bluetooth® Low Energy: Period Advertising Sync Transfer.
    This feature allows Periodic Advertising synchronization data to be transferred over an ACL connection, which gives power saving benefits for energy constraint devices.
    This is a key feature for the `Auracast™`_ Assistant.
  * The :ref:`lib_location` library now supports sending both Wi-Fi and cellular location data to nRF Cloud location services for improved location accuracy.
    This is demonstrated in :ref:`asset_tracker_v2`, :ref:`modem_shell_application`, and :ref:`lwm2m_client`.
  * Writing modem trace to external flash for later retrieval.
    The :ref:`modem_shell_application` sample now demonstrates how to store and upload modem trace to cloud.
  * New :ref:`Serial LTE Modem (SLM) Shell <slm_shell_sample>` sample:
    The sample demonstrates sending AT commands to the nRF9160 SiP from shell for nRF53 and nRF52 Series SoCs.
  * New :ref:`mqtt_sample` sample supporting Wi-Fi together with cellular connectivity (replacing the Simple MQTT sample).

* Added :ref:`experimental support <software_maturity>` for the following features:

  * Periodic Advertisement with Responses (PAwR) - Advertiser.
    This is a new feature introduced in Bluetooth v5.4 specification, enabling the bidirectional exchange of application data using connectionless communication.
    Learn more in our `Bluetooth 5.4 DevZone blog`_.
    The SoftDevice Controller continues to be Bluetooth v5.3 qualified.
  * Bluetooth LE Audio: Public Broadcast Announcement (PBA) and bidirectional streams to and from two headsets.
  * New :ref:`wifi_sr_coex_sample` sample.

* Improved:

  * The :ref:`matter_lock_sample` sample has been extended to support switching between Matter over Thread and Matter over Wi-Fi during application operation.
  * The :ref:`wifi_shell_sample` sample by adding the Power Save feature.

See :ref:`ncs_release_notes_230_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.3.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.3.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the only officially supported IDE for |NCS| v2.3.0.

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
     - `Changelog <Modem library changelog for v2.3.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.3.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.3.0`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_230_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

* Removed the :file:`scripts/tool-version-minimum.txt` file because only one toolchain version is used for testing.
  The table in :ref:`gs_recommended_versions` has been updated accordingly to no longer specify minimum tool versions.

MCUboot
=======

* Updated:

  * MCUboot now uses the Secure RAM region on TrustZone-enabled devices.
    The Kconfig option :kconfig:option:`CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM` was added to allow MCUboot to use all the available RAM.

Application development
=======================

* Added a new user guide about :ref:`app_approtect`.

Build system
------------

* Removed:

  * Manifest file entry ``mbedtls-nrf`` (:makevar:`ZEPHYR_MBEDTLS_NRF_MODULE_DIR`) checked out at path :file:`mbedtls`.
  * Manifest file entry ``tfm-mcuboot`` (:makevar:`ZEPHYR_TFM_MCUBOOT_MODULE_DIR`) checked out at path :file:`modules/tee/tfm-mcuboot`.

* Updated:

  * Manifest file entry ``mbedtls`` (:makevar:`ZEPHYR_MBEDTLS_MODULE_DIR`) checked out at path :file:`modules/crypto/mbedtls` now points to |NCS|'s fork of Mbed TLS instead of Zephyr's fork.

Working with nRF52 Series
=========================

* :ref:`ug_nrf52_developing`:

  * Added Kconfig options :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` and :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` to configure FOTA updates over Bluetooth Low Energy in the default setup.
    The default setup uses MCUmgr libraries with the Bluetooth transport layer and requires the user to enable MCUboot bootloader.
    See details in the :ref:`FOTA updates <ug_nrf52_developing_ble_fota>` section of the :ref:`ug_nrf52_developing` guide.

Working with nRF53 Series
=========================

* :ref:`ug_nrf5340`:

  * Added Kconfig options :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` and :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` to configure FOTA updates over Bluetooth Low Energy in the default setup.
    The default setup uses MCUmgr libraries with the Bluetooth transport layer and requires the user to enable MCUboot bootloader.
    See details in the FOTA updates section of the :ref:`ug_nrf5340` guide.

* Developing with Thingy:53:

  * Added the :kconfig:option:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM` Kconfig option to configure USB CDC ACM to be used as logger's backend by default.
    See details in the :ref:`thingy53_app_usb` section of the Developing with Thingy:53 guide.
  * Provided support for Thingy:53 FOTA updates within the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` option.
    See details in the :ref:`thingy53_app_fota_smp` section of the Developing with Thingy:53 guide.
  * Enabled MCUboot bootloader in Thingy:53 board configuration by default.
    See details in the :ref:`thingy53_app_mcuboot_bootloader` section of the Developing with Thingy:53 guide.
  * Cleaned up Thingy:53 configuration files of samples and applications as a result of introducing simplifications.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

See `Bluetooth samples`_ for the list of changes in the Bluetooth samples.

Bluetooth mesh
--------------

* Updated the :ref:`bt_mesh_light_ctrl_srv_readme` model to make sure that the illuminance regulator starts running when a fresh value of the ambient LuxLevel is reported when the controller is enabled.

* Fixed an issue in the :ref:`bt_mesh_light_ctrl_srv_readme` model where multiple scene recall messages for the same scene did not repeatedly trigger the same scene recall.
  This prevents the interruption of an ongoing transition.

See `Bluetooth mesh samples`_ for the list of changes in the Bluetooth mesh samples.

Matter
------

* Added:

  * Support for switching between Matter over Thread and Matter over Wi-Fi.
    This feature is available for the :ref:`matter_lock_sample` sample programmed on ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002_ek`` shield attached, using the ``thread_wifi_switched`` build type.
    See :ref:`matter_lock_sample_wifi_thread_switching` in the sample documentation for more information.
  * Support for Wi-Fi Network Diagnostic Cluster (which counts the number of packets received and transmitted on the Wi-Fi interface).
  * Default support for nRF7002 revision B.
  * Specific QR code and onboarding information in the documentation for each :ref:`Matter sample <matter_samples>` and the :ref:`Matter weather station <matter_weather_station_app>`.
  * The Bluetooth LE advertising arbiter class that enables easier coexistence of application components that want to advertise their Bluetooth LE services.
  * Support for erasing settings partition during DFU over Bluetooth LE SMP for the Nordic nRF52 Series SoCs.
  * Mechanism to retry a failed Wi-Fi connection.
  * Support for ZAP tool under Windows.
  * Documentation about :ref:`switchable Matter over Thread and Matter over Wi-Fi <ug_matter_overview_architecture_integration_designs>` platform design.
  * Documentation about :ref:`ug_matter_gs_ecosystem_compatibility_testing`.
  * Documentation about :ref:`ug_matter_device_low_power_configuration`.
  * Documentation about :ref:`ug_matter_gs_transmission_power`.

* Updated:

  * The default heap implementation to use Zephyr's ``sys_heap`` (:kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP`) to better control the RAM usage of Matter applications.
  * :ref:`ug_matter_device_certification` page with a section about certification document templates.
  * :ref:`ug_matter_overview_commissioning` page with information about :ref:`ug_matter_network_topologies_commissioning_onboarding_formats`.
  * :ref:`ug_matter_hw_requirements` page with a section about :ref:`ug_matter_hw_requirements_layouts`.
  * Default retry intervals used by Matter Reliability Protocol for Matter over Thread to account for longer round-trip times in Thread networks with multiple intermediate nodes.
  * The Bluetooth LE connection timeout parameters and the update timeout parameters to make communication over Bluetooth LE more reliable.
  * Default transmission output power for Matter over Thread devices to the maximum available one for all targets:
    8 dBm for nRF52840, 3 dBm for nRF5340, 20 dBm for all devices with FEM enabled, and 0 dBm for sleepy devices.
  * :ref:`ug_matter_gs_adding_cluster` page with instructions on how to use ZAP tool binaries.
    Before this release, the ZAP tool had to be built from sources.
  * :ref:`ug_matter_hw_requirements` with updated memory requirement values valid for the |NCS| v2.3.0.

* Fixed:

  * An issue where the connection would time out when attaching to a Wi-Fi access point that requires Wi-Fi Protected Access 3 (WPA3).
  * An issue where the ``NetworkInterfaces`` attribute of General Diagnostics cluster would return EUI-64 instead of MAC Extended Address for Thread network interfaces.

* Removed support for Android CHIP Tool from the documentation and release artifacts.
  Moving forward, it is recommended to use the development tool CHIP Tool for Linux or macOS or mobile applications from publicly available Matter Ecosystems.

See `Matter samples`_ for the list of changes in the Matter samples.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``1.0.0.2`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * The initial implementation of Matter's cryptographic operations based on PSA crypto API.
  * An alternative factory reset implementation that erases the entire non-volatile storage flash partition.

* Updated Basic cluster by renaming it to Basic Information cluster to match the specification.

Thread
------

* Added:

  * Support for setting the default Thread output power using the :kconfig:option:`OPENTHREAD_DEFAULT_TX_POWER` Kconfig option.
  * A Thread :ref:`power consumption data <thread_power_consumption>` page.

See `Thread samples`_ for the list of changes in the Thread samples.

Zigbee
------

* Updated Zigbee Network Co-processor Host package to the new version v2.2.1.

* Fixed an issue where buffer would not be freed at the ZC after a secure rejoin of a ZED.

Enhanced ShockBurst (ESB)
-------------------------

* Added support for front-end modules.
  The ESB module requires linking the :ref:`MPSL library <nrfxlib:mpsl_lib>`.

* Updated:

  * Number of PPI/DPPI channels used from three to six.
  * Events 6 and 7 from the EGU0 instance by assigning them to the ESB module.
  * The type parameter of the function :c:func:`esb_set_tx_power` to ``int8_t``.

Wi-Fi
-----

* Added:

  * New sample :ref:`wifi_sr_coex_sample` demonstrating Wi-Fi Bluetooth LE coexistence.
  * :ref:`ug_wifi` document.
  * :ref:`lib_wifi_credentials` library to store credentials.
  * :ref:`wifi_mgmt_ext` library to provide an ``autoconnect`` command based on Wi-Fi credentials.

* Updated:

  * Wi-Fi coexistence is no longer enabled by default.
    It must be enabled explicitly in Kconfig using :kconfig:option:`CONFIG_MPSL_CX`.
    On the nRF5340, this option must be selected for both the application core and the network core images.

* Removed the support for nRF7002 revision A.

See `Wi-Fi samples`_ for the list of changes in the Wi-Fi samples.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF9160: Asset Tracker v2
-------------------------

* Added:

  * Wi-Fi support for nRF9160 DK + nRF7002 EK configuration.
  * A section about :ref:`Custom transport <asset_tracker_v2_ext_transport>` in the :ref:`asset_tracker_v2_debug_module` documentation.

* Updated:

  * Due to the :ref:`lib_location` library updates related to combined cellular and Wi-Fi positioning, the following events and functions have been added replacing old ones:

    * :c:enum:`LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY` replaces ``LOCATION_MODULE_EVT_NEIGHBOR_CELLS_DATA_READY`` and ``LOCATION_MODULE_EVT_WIFI_ACCESS_POINTS_DATA_READY``
    * :c:enum:`DATA_EVT_CLOUD_LOCATION_DATA_SEND` replaces ``DATA_EVT_NEIGHBOR_CELLS_DATA_SEND`` and ``DATA_EVT_WIFI_ACCESS_POINTS_DATA_SEND``
    * :c:func:`cloud_codec_encode_cloud_location` function replaces ``cloud_codec_encode_neighbor_cells`` and ``cloud_codec_encode_wifi_access_points``
    * :c:func:`cloud_wrap_cloud_location_send` function replaces ``cloud_wrap_neighbor_cells_send`` and ``cloud_wrap_wifi_access_points_send``

  * Replaced deprecated LwM2M API calls with calls to new functions.
  * Removed static modem data handling from the application's nRF Cloud codec.
    Enabled the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS` configuration option to send static modem data.

nRF9160: Serial LTE modem
-------------------------

* Added:

  * RFC1350 TFTP client, currently supporting only *READ REQUEST*.
  * AT command ``#XSHUTDOWN`` to put nRF9160 SiP to System Off mode.
  * Support for nRF Cloud C2D appId ``MODEM`` and ``DEVICE``.
  * Support for the :ref:`liblwm2m_carrier_readme` library.

* Updated:

  * The response for the ``#XDFUGET`` command, using unsolicited notification to report download progress.
  * The response for the ``#XDFUSIZE`` command, adding a CRC32 checksum of the downloaded image.
  * The ``#XSLMVER`` command to report the versions of both the |NCS| and the modem library.

nRF5340 Audio
-------------

* Added:

  * Support for the nRF21540 front-end module.
  * Possibility to create a Public Broadcast Announcement (PBA) needed for Auracast.
  * Encryption for BISes.
  * Support for bidirectional streams to or from two headsets (True Wireless Stereo).
  * Support for interleaved packing.

* Updated:

  * Controller from version 3310 to 3330, with the following major changes:

    * Changes to accommodate BIS + ACL combinations.
    * Improvements to support creating CIS connections in any order.
    * Basic support for interleaved broadcasts.

  * The power module has been refactored to use the upstream Zephyr INA23X sensor driver.
  * BIS headsets can now switch between two broadcast sources (two hardcoded broadcast names).
  * :ref:`nrf53_audio_app_ui` and :ref:`nrf53_audio_app_testing_steps_cis` sections in the application documentation with information about using **VOL** buttons to switch headset channels.
  * :ref:`nrf53_audio_app_requirements` section in the application documentation by moving the information about the nRF5340 Audio DK to `Nordic Semiconductor Infocenter`_, under `nRF5340 Audio DK Hardware`_.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added a Kconfig option :kconfig:option:`CONFIG_APP_SENSOR_SLEEP_TO` to set the sensor idling timeout before suspending the sensor.

* Removed the usage of ``ml_runner_signin_event`` from the application.

nRF Desktop
-----------

* Added:

  * An application log indicating that the value of a configuration option has been updated in the :ref:`nrf_desktop_motion`.
  * Application-specific Kconfig options :ref:`CONFIG_DESKTOP_LOG <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>` to simplify the debug configurations for the Logging and Shell subsystems.
    See the debug configuration section of the :ref:`nrf_desktop` application for more details.
  * Application-specific Kconfig options that define common HID device identification values (product name, manufacturer name, Vendor ID, and Product ID).
    The identification values are used both by USB and the Bluetooth LE GATT Device Information Service.
    See the :ref:`nrf_desktop_hid_device_identifiers` documentation for details.
  * The :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO <config_desktop_app_options>` Kconfig option.
    It can be used to indicate the dongle peer identity with a dedicated event.
  * Synchronization between the Resolvable Private Address (RPA) rotation and the advertising data update in the Fast Pair configurations using the :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA` Kconfig option.
  * Application-specific Kconfig options that can be used to enable the :ref:`lib_caf` modules and to automatically tailor the default configuration to the nRF Desktop use case.
    Each used Common Application Framework module is handled by a corresponding application-specific option with a modified prefix.
    For example, :ref:`CONFIG_DESKTOP_SETTINGS_LOADER <config_desktop_app_options>` is used to automatically enable the :kconfig:option:`CONFIG_CAF_SETTINGS_LOADER` Kconfig option and to align the default configuration.
  * Prompts to Kconfig options that enable :ref:`nrf_desktop_hids`, :ref:`nrf_desktop_bas`, and :ref:`nrf_desktop_dev_descr`.
    An application-specific option (:ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>`) implies the Kconfig options that enable the mentioned modules together with other features that are needed for the Bluetooth HID peripheral role.
    The option is enabled by default if the nRF Desktop Bluetooth support (:ref:`CONFIG_DESKTOP_BT <config_desktop_app_options>`) is enabled.

* Updated:

  * The logging mechanism by implementing the following adjustments to avoid flooding logs:

    * Set the max compiled-in log level to ``warning`` for the Non-Volatile Storage (:kconfig:option:`CONFIG_NVS_LOG_LEVEL`).
    * Lowered log level to ``debug`` for the ``Identity x created`` log in the :ref:`nrf_desktop_ble_bond`.

  * The default values of the :kconfig:option:`CONFIG_BT_GATT_CHRC_POOL_SIZE` and :kconfig:option:`CONFIG_BT_GATT_UUID16_POOL_SIZE` Kconfig options are tailored to the nRF Desktop application requirements.
  * The :ref:`nrf_desktop_fast_pair_app` to remove the Fast Pair advertising payload for the dongle peer.
  * The default values of Bluetooth device name (:kconfig:option:`CONFIG_BT_DEVICE_NAME`) and Bluetooth device appearance (:kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE`) are set to rely on the nRF Desktop product name or the nRF Desktop device role and type combination.
  * The default value of the Bluetooth appearance (:kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE)` for nRF Desktop dongle is set to ``keyboard``.
    This improves the consistency with the used HID boot interface.
  * USB remote wakeup (:kconfig:option:`CONFIG_USB_DEVICE_REMOTE_WAKEUP`) is disabled in MCUboot bootloader configurations.
    The functionality is not used by the bootloader.
  * :ref:`nrf_desktop_hids` registers the GATT HID Service before Bluetooth LE is enabled.
    This is done to avoid submitting works related to Service Changed indication and GATT database hash calculation before the system settings are loaded from non-volatile memory.
  * The configuration of application modules.
    The modules automatically enable required libraries and align the related default configuration with the application use case.
    Configuration of the following application modules was simplified:

    * :ref:`nrf_desktop_hid_forward`
    * :ref:`nrf_desktop_hids`
    * :ref:`nrf_desktop_watchdog`

    See the documentation of the mentioned modules and their Kconfig configuration files for details.

* Removed:

  * Fast Pair discoverable advertising payload on Resolvable Private Address (RPA) rotation during discoverable advertising session.
  * Separate configurations enabling :ref:`zephyr:shell_api` (:file:`prj_shell.conf`).
    Shell support can be enabled for a given configuration with a single Kconfig option (:ref:`CONFIG_DESKTOP_SHELL <config_desktop_app_options>`).

Samples
=======

This section provides detailed lists of changes by :ref:`sample <sample>`, including protocol-related samples.
For lists of protocol-specific changes, see `Protocols`_.

Bluetooth samples
-----------------

* Added the :ref:`peripheral_status` sample.

* :ref:`peripheral_uart` sample:

  * Fixed a possible memory leak in the :c:func:`uart_init` function.

* :ref:`peripheral_hids_keyboard` sample:

  * Fixed a possible out-of-bounds memory access issue in the :c:func:`hid_kbd_state_key_set` and :c:func:`hid_kbd_state_key_clear` functions.

* :ref:`ble_nrf_dm` sample:

  * Added support for high-precision distance estimate using more compute-intensive algorithms.
  * Updated:

    * Documentation by adding :ref:`energy consumption <ble_nrf_dm_power>` information.
    * Documentation by adding a section about :ref:`distance offset calibration <ble_nrf_dm_calibr>`.
    * Configuration of the GPIO pins used by the DM module using the devicetree.

* :ref:`peripheral_nfc_pairing` and :ref:`central_nfc_pairing` samples:

  * Fixed OOB pairing between these samples.

* :ref:`direct_test_mode` sample:

  * Fixed an issue where the antenna switching was not functional on the nRF5340 DK with the nRF21540 EK shield.

* :ref:`peripheral_hids_mouse` sample:

  * Fixed building the sample with the :kconfig:option:`CONFIG_BT_HIDS_SECURITY_ENABLED` Kconfig option disabled.

Bluetooth mesh samples
----------------------

* Enabled the :kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE` Kconfig option in the following samples:

  * :ref:`bluetooth_ble_peripheral_lbs_coex`
  * :ref:`bt_mesh_chat`
  * :ref:`bluetooth_mesh_light`
  * :ref:`bluetooth_mesh_sensor_client`
  * :ref:`bluetooth_mesh_sensor_server`
  * :ref:`bluetooth_mesh_light_lc`

* :ref:`bluetooth_mesh_light_lc` sample:

  * Updated:

    * The specification-defined illuminance regulator (:kconfig:option:`CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC`) now selects the :kconfig:option:`CONFIG_FPU` option by default.
      Therefore, enabling it explicitly in the project file is no longer required.

nRF9160 samples
---------------

* Added:

  * The :ref:`mqtt_sample` sample that supports Wi-Fi and LTE connectivity.
  * The :ref:`nidd_sample` sample that demonstrates how to use Non-IP Data Delivery (NIDD).
  * The :ref:`slm_shell_sample` sample for nRF52 and nRF53 Series devices to send AT commands to nRF9160 SiP from shell.

* :ref:`modem_shell_application` sample:

  * Added:

    * External location service handling to test :ref:`lib_location` library functionality commonly used by applications.
      The :ref:`lib_nrf_cloud` library is used with MQTT for location requests to the cloud.
    * New command ``th pipeline`` for executing several MoSh commands sequentially in one thread.
    * New command ``sleep`` for introducing wait periods between commands when using ``th pipeline``.
    * New command ``heap`` for printing kernel and system heap usage statistics.

  * Updated:

    * Timeout command-line arguments for the ``location get`` command changed from integers in milliseconds to floating-point values in seconds.
    * Replaced deprecated LwM2M API calls with calls to new functions.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added the usage of GCI search option if running modem firmware v1.3.4.
  * Updated the sample to wait for RRC idle mode before requesting neighbor cell measurements.

* :ref:`lwm2m_client` sample:

  * Added:

    * Support for nRF7002 EK shield and Wi-Fi based location.
    * Location events and event handlers.

  * Updated:

    * The sensor module has been simplified.
      It does not use application events, filtering, or configurable periods anymore.
    * Replaced deprecated LwM2M API calls with calls to new functions.
    * Enabled LwM2M queue mode and updated documentation accordingly.
    * Moved configuration options from the :file:`overlay-queue.conf` file to the default configuration file :file:`prj.conf`.
    * Removed the :file:`overlay-queue.conf` file.
    * Enabled the :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS` Kconfig option.

* :ref:`http_application_update_sample` sample:

  * Added support for the :ref:`liblwm2m_carrier_readme` library.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * MCUboot child image files to properly access external flash on newer nRF9160 DK versions.
    * An :file:`overlay_mcuboot_ext_flash.conf` file to enable MCUboot to use external flash.
    * Sending an alert to the cloud on boot and when a temperature limit is exceeded.

  * Updated the sample to use a partition in external flash for full modem FOTA updates.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added sending an alert to nRF Cloud on boot.

* Removed:

  * Multicell location sample because of the deprecation of the Multicell location library.
    Relevant functionality is available through the :ref:`lib_location` library.
  * nRF9160: Simple MQTT sample.
    This is now replaced by a new :ref:`mqtt_sample` sample that supports Wi-Fi and LTE connectivity.

* :ref:`nrf_cloud_rest_fota` sample:

  * Updated:

    * Device status information, including FOTA enablement, is now sent to nRF Cloud when the device connects.
    * Removed user prompt and button press handling for FOTA enablement.
    * The sample now uses a partition in external flash for full modem FOTA updates.

* :ref:`azure_fota_sample` sample:

  * Updated the sample to use the logging subsystem for console output.

* :ref:`azure_iot_hub` sample:

  * Updated the sample to use the logging subsystem for console output.

* :ref:`aws_iot` sample:

  * Updated the sample to use the logging subsystem for console output.

Thread samples
--------------

* Updated the overlay structure:

  * The :file:`overlay-rtt.conf` file was removed from all samples.
  * The :file:`overlay-log.conf` file now uses RTT backend by default.
  * Logs removed from default configuration (moved to :file:`overlay-logging.conf`).
  * Asserts removed from default configuration (moved to :file:`overlay-debug.conf`).

* :ref:`ot_cli_sample`:

  * Added the :file:`overlay-low_power.conf` and :file:`low_power.overlay` files to facilitate power consumption measurements.

Matter samples
--------------

* Enabled Matter shell commands for all build types except ``release`` in all Matter samples.
* Removed FEM-related Kconfig options from all samples.
  Now, the transmission output power for Matter over Thread can be set using the :kconfig:option:`OPENTHREAD_DEFAULT_TX_POWER` Kconfig option.

* :ref:`matter_lock_sample` sample:

  * Added:

    * ``thread_wifi_switched`` build type that enables switching between Thread and Wi-Fi network support in the field.
      See :ref:`matter_lock_sample_wifi_thread_switching` in the sample documentation for more information.
    * Wi-Fi low power configuration using Wi-Fi's :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.

  * The sample is now positively verified against "Works with Google" certification tests.

* :ref:`matter_light_switch_sample`:

  * Added Wi-Fi low power configuration using Wi-Fi's :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.

* :ref:`matter_light_bulb_sample`:

  * The sample is now positively verified against "Works with Google" certification tests.
  * Tested compatibility with the following ecosystems:

    * Google Home ecosystem for both Matter over Thread and Matter over Wi-Fi solutions.
      Tested with Google Nest Hub 2nd generation (software version: 47.9.4.447810048; Chromecast firmware version: 1.56.324896, and Google Home mobile application v2.63.1.12).
    * Apple Home ecosystem for both Matter over Thread and Matter over Wi-Fi solutions.
      Tested with Apple HomePod mini and Apple iPhone (iOS v16.3).
    * Samsung SmartThings ecosystem for Matter over Thread solution.
      Tested with Aeotec Smart Home Hub and SmartThings mobile application (v1.7.97.22).
    * Amazon Alexa ecosystem for both Matter over Thread and Matter over Wi-Fi solutions.
      Tested with Amazon Echo Dot and Amazon Alexa mobile application (v2.2.495949.0).

NFC samples
-----------

* Fixed an issue where NFC samples that use the NFC Reader feature returned false error code with value ``1`` during the NFC T4T operation.

Wi-Fi samples
-------------

* Added the :ref:`mqtt_sample` sample that supports Wi-Fi and LTE connectivity.
* Added the :ref:`wifi_sr_coex_sample` sample demonstrating Wi-Fi Bluetooth LE coexistence.

* Updated:

  * The :ref:`wifi_shell_sample` sample now uses the :ref:`lib_wifi_credentials` and :ref:`wifi_mgmt_ext` libraries.
  * The :ref:`wifi_provisioning` sample now uses the :ref:`lib_wifi_credentials` and :ref:`wifi_prov_readme` libraries.

* Removed nRF7002 revision A support.

Other samples
-------------

* :ref:`esb_prx_ptx` sample:

  * Added support for front-end modules and :ref:`zephyr:nrf21540dk_nrf52840`.

* :ref:`radio_test` sample:

  * Added support for the nRF7002 DK.
  * Updated the documentation to clarify that this sample is dedicated for the short-range radio (Bluetooth LE, IEEE 802.15.4, and proprietary modes).
  * Fixed sample building with support for the Skyworks front-end module.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* :ref:`pmw3360`:

  * Updated by reducing log verbosity.

* :ref:`paw3212`:

  * Updated by reducing log verbosity.

* :ref:`lib_bh1749`:

  * Fixed an issue where the driver would attempt to use APIs before the sensor was ready, which in turn could make the application hang.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Removed the dependency on the :ref:`lte_lc_readme` library.

Bluetooth libraries and services
--------------------------------

* Added the :ref:`nsms_readme` library.
* Added the :ref:`wifi_prov_readme` library.

* :ref:`mds_readme` library:

  * Fixed URI generation in the :c:func:`data_uri_read` function.

* :ref:`ble_rpc` library:

  * Fixed a possible memory leak in the :c:func:`bt_gatt_indicate_rpc_handler` function.

* :ref:`bt_le_adv_prov_readme` library:

  * Added the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_STOP_DISCOVERABLE_ON_RPA_ROTATION` Kconfig option to drop the Fast Pair advertising payload on RPA rotation.

  * Updated:

    * The :c:struct:`bt_le_adv_prov_adv_state` structure has been extended to include new fields.
      The new :c:member:`bt_le_adv_prov_adv_state.rpa_rotated` field is used to notify registered providers about Resolvable Private Address (RPA) rotation.
      The new :c:member:`bt_le_adv_prov_adv_state.new_adv_session` field is used to notify registered providers that the new advertising session is about to start.
    * The :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_BATTERY_DATA_MODE` Kconfig option (default value) no longer includes Fast Pair battery data in the Fast Pair advertising payload by default.

* :ref:`bt_fast_pair_readme` service:

  * Added the :c:func:`bt_fast_pair_factory_reset` function to clear the Fast Pair storage.

Bootloader libraries
--------------------

* :ref:`doc_bl_storage` library:

  * Updated:

    * The monotonic counter functions can now return errors.
    * The :c:func:`get_monotonic_version` function is split into functions :c:func:`get_monotonic_version` and :c:func:`get_monotonic_slot`.
    * The monotonic counter functions now have a counter description parameter to be able to distinguish between different counters.

* :ref:`doc_bl_validation` library:

  * Updated the :c:func:`get_monotonic_version` function so that it can now return an error.

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OFF` Kconfig option to set the modem trace level to off by default.
    * The flash trace backend that enables the application to store trace data to flash for later retrieval.

  * Updated:

    * It is now possible to poll Modem library and Zephyr sockets at the same time using the :c:func:`poll` function.
      This includes special sockets such as event sockets created using the :c:func:`eventfd` function.
    * The minimal value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE` Kconfig option to meet the requirements of modem firmware 1.3.4.
    * The :c:func:`nrf_modem_lib_diag_stats_get` function now returns an error if called when the :ref:`nrf_modem_lib_readme` library has not been initialized.
    * The trace backend interface to be exposed to the :ref:`modem_trace_module` using the :c:struct:`nrf_modem_lib_trace_backend` struct.
    * The :ref:`modem_trace_module` to support backends that store the trace data for later retrieval.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT` Kconfig option, enabling the RTT trace backend, now requires the :kconfig:option:`CONFIG_USE_SEGGER_RTT` Kconfig option to be enabled.

* :ref:`lib_location` library:

  * The Multicell location library is now :ref:`deprecated <api_deprecation>`.
    Relevant functionality from the library is moved to this library.
    The following features were not moved:

      * Definition of HTTPS port for HERE service, that is :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT`.
      * HERE v1 API.
      * nRF Cloud CA certificate handling.

  * Added:

    * Support for the application to send the Wi-Fi access point list to the cloud.
    * :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL` Kconfig option that replaces the following configurations:

      * ``CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL``
      * ``CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL``
      * ``CONFIG_LOCATION_METHOD_CELLULAR_EXTERNAL``

      The new configuration also handles Wi-Fi positioning.
    * Several new Kconfig options for default location request configurations, including default method priority configuration.
      These new Kconfig options are applied when the :c:func:`location_config_defaults_set` function is called.

  * Updated:

    * Neighbor cell measurements and Wi-Fi scan results are combined into a single cloud request.
      This also means that cellular and Wi-Fi positioning are combined into a single cloud positioning method if they are one after the other in the method list of the location request.
      Because of this, some parts of the API are replaced with new ones as follows:

      * Event :c:enum:`LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST` replaces old events ``LOCATION_EVT_CELLULAR_EXT_REQUEST`` and ``LOCATION_EVT_WIFI_EXT_REQUEST`` that are removed.
      * Function :c:func:`location_cloud_location_ext_result_set` replaces old functions ``location_cellular_ext_result_set`` and ``location_wifi_ext_result_set`` that are removed.
      * Member variable :c:var:`cloud_location_request` replaces old members ``cellular_request`` and ``wifi_request`` that are removed in :c:struct:`location_event_data`.
      * :kconfig:option:`CONFIG_LOCATION_SERVICE_CLOUD_RECV_BUF_SIZE` replaces ``CONFIG_LOCATION_METHOD_CELLULAR_RECV_BUF_SIZE`` and ``CONFIG_LOCATION_METHOD_WIFI_REST_RECV_BUF_SIZE``.
    * GNSS assistance data need handling by improving it.
    * The GNSS filtered ephemerides mechanism.
      These are no longer used when the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED_RUNTIME` Kconfig option is enabled.
    * Renamed:

      * ``enum location_cellular_ext_result`` to ``enum location_ext_result``, because Wi-Fi will use the same enumeration.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_NRF_CLOUD`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_API_KEY`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_API_KEY`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_HOSTNAME`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_HOSTNAME`.
      * ``CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_TLS_SEC_TAG`` to :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG`.

  * Fixed an issue causing the A-GPS data download to be delayed until the RRC connection release.

* Added the :ref:`lib_modem_slm` library.
  This library is meant for the external MCU to work with nRF9160 SiP through the :ref:`serial_lte_modem` application.

* Multicell location library is :ref:`deprecated <api_deprecation>` and will be removed in one of the future releases.

* :ref:`modem_info_readme` library:

  * Added the :c:func:`modem_info_get_hw_version` function to obtain the hardware version string using the ``AT%HWVERSION`` command.

* :ref:`lte_lc_readme` library:

  * Fixed an issue where cell update events could be sent without the cell information from the modem actually being updated.

Libraries for networking
------------------------

* Added:

  * The :ref:`lib_mqtt_helper` library that simplifies Zephyr MQTT API and socket handling.
  * The :ref:`lib_nrf_cloud_alert` library for sending notifications of critical device events to nRF Cloud, using either REST or MQTT connections.

* :ref:`lib_azure_iot_hub` library:

  * Pulled out the :file:`azure_iot_hub_mqtt.c` file that is now implemented by a new library :ref:`lib_mqtt_helper`.

* Multicell location library:

  * This library is now deprecated and relevant functionality is available through the :ref:`lib_location` library.

* :ref:`lib_fota_download` library:

  * Fixed:

    * An issue where the :c:func:`download_client_callback` function was continuing to read the offset value even if :c:func:`dfu_target_offset_get` returned an error.
    * An issue where the cleanup of the downloading state was not happening when an error event was raised.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * Support for full modem FOTA updates using a partition in external flash.
    * Automatic sending of the device status information to nRF Cloud when the device connects if the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS` Kconfig option is enabled.
      Network information is included if the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_NETWORK` Kconfig option is enabled.
      SIM card information is included if the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_SIM` Kconfig option is enabled.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_DEVICE_STATUS_ENCODE_VOLTAGE` Kconfig option, which controls if device voltage is included when device status data is encoded.
    * Possibility to include an application version string in the :c:struct:`nrf_cloud_init_param` struct.

  * Updated:

    * Handling of the MQTT disconnect event.
      It is now handled by the FOTA module, allowing for updates to be completed while disconnected and reported properly when reconnected.
    * Encoding of the GCI search results.
      These are now encoded in location requests.
    * Encoding of the neighbor cell's time difference value.
      It is now encoded in location requests.

  * Fixed:

    * An issue where the same buffer was incorrectly shared between caching a P-GPS prediction and loading a new one, when external flash was used.
    * An issue where external flash only worked if the P-GPS partition was located at address 0.

* :ref:`lib_lwm2m_location_assistance` library:

  * Added:

    * Support for Wi-Fi based location through LwM2M.
    * API for scanning Wi-Fi access points.

  * Removed location events and event handlers.

* :ref:`lib_nrf_cloud_rest` library:

  * Added the :c:func:`nrf_cloud_rest_device_status_message_send` function to send the device status information as an nRF Cloud device message.

Libraries for NFC
-----------------

* Added:

  * The possibility of moving an NFC callback to a thread context.
  * Support for zero-latency interrupts for NFC.

* Updated by aligning the :file:`ncs/nrf/subsys/nfc/lib/platform.c` file with new library implementation.

* :ref:`nfc_ndef_ch_rec_parser_readme` library:

  * Fixed a bug where the AC Record Parser was not functional and returned invalid results.

Other libraries
---------------

* Added the :ref:`lib_pcm_mix` library.
  This was previously a component of the :ref:`nrf53_audio_app` application, now moved to :file:`lib/pcm_mix`.

* :ref:`lib_contin_array` library:

  * Updated by separating the library from the :ref:`nrf53_audio_app` application and moving it to :file:`lib/contin_array`.
    Updated code and documentation accordingly.

* :ref:`lib_pcm_stream_channel_modifier` library:

  * Updated by separating the library from the :ref:`nrf53_audio_app` application and moving it to :file:`lib/pcm_stream_channel_modifier`.
    Updated code and documentation accordingly.

* :ref:`lib_data_fifo` library:

  * Updated by separating the library from the :ref:`nrf53_audio_app` application and moving it to :file:`lib/data_fifo`.
    Updated code and documentation accordingly.

* :ref:`QoS` library:

  * Updated by removing the ``QOS_MESSAGE_TYPES_REGISTER`` macro.

* Secure Partition Manager (SPM):

  * Removed Secure Partition Manager (SPM) and the Kconfig option ``CONFIG_SPM``.
    It is replaced by the :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` as the supported trusted execution solution.

* :ref:`app_event_manager`:

  * Updated the way section names are created for event subscribers.
    This allows you to use any event naming scheme.
    For more information, see the :ref:`NCSIDB-925 <ncsidb_925>` issue description on the :ref:`known_issues` page.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_smp`:

  * Updated by aligning the module with the recent MCUmgr API, following the :ref:`zephyr:mcumgr_callbacks` migration guide in the Zephyr documentation.
    The module now requires the Kconfig options :kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` and :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK` to be enabled.

* :ref:`caf_ble_adv`:

  * Added the :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA` Kconfig option.
    The option synchronizes Resolvable Private Address (RPA) rotation with the advertising data update in the Bluetooth Privacy mode.
    Added dependent Kconfig options :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT` and :kconfig:option:`CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT_RAND`.
    They are used to specify the rotation period and its randomization factor.

* :ref:`caf_overview_events`:

  * Added a macro intended to set the size of events member enums to 32 bits when the Event Manager Proxy is enabled.
    Applied this macro to all affected CAF events.

  * Updated by improving inter-core compatibility.

* :ref:`caf_sensor_data_aggregator`:

  * Updated:

    * :c:struct:`sensor_data_aggregator_event` now uses the :c:struct:`sensor_value` struct data buffer and carries a number of sensor values in a single sample, which is sufficient to describe data layout.

    * The way buffers are declared is updated when the instance is created.
      Now, the memory-region devicetree property works independently for each instance and does not require the specific instance name.

* :ref:`caf_sensor_manager`:

  * Updated by cleaning up :file:`sensor_event.h` and :file:`sensor_manager.h` files.
    Moved unrelated declarations to a separate :file:`caf_sensor_common.h` file.

DFU libraries
-------------

* :ref:`lib_dfu_target` library:

  * Added:

    * The :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION` Kconfig option to support the ``FMFU_STORAGE`` partition in external flash.
    * The :c:func:`dfu_target_full_modem_fdev_get` function that gets the configured flash device information.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom`:

  * Updated the output contents.
    The output now contains source repository and version information for each file.

* :ref:`partition_manager`:

  * Added:

    * The :file:`ncs/nrf/subsys/partition_manager/pm.yml.fmfu` file.
    * Support for the full modem FOTA update (FMFU) partition: ``FMFU_STORAGE``.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``cfec947e0f8be686d02c73104a3b1ad0b5dcf1e6``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added an option to prevent inclusion of the default nRF5340 network core DFU image hook, which allows a custom implementation by users if the :kconfig:option:`CONFIG_BOOT_IMAGE_ACCESS_HOOK_NRF5340` Kconfig option is disabled (enabled by default).
  CMake can be used to add additional hook files.
  See :file:`modules/mcuboot/hooks/CMakeLists.txt` for an example of how to achieve this.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``e1e06d05fa8d1b6ac1b0dffb1712e94e308861f8``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline e1e06d05fa ^cd16a8388f

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^e1e06d05fa

The current |NCS| main branch is based on revision ``e1e06d05fa`` of Zephyr.

Documentation
=============

* Added:

  * A page about :ref:`ug_matter_device_security` in the Matter protocol section.
  * Template for the :ref:`Integration <integration_template>` user guides.
  * A page on :ref:`ug_avsystem`.
  * The :ref:`ug_nrf70_developing` user guide.
  * A page on :ref:`ug_nrf70_features`.
  * Template for :ref:`Applications <application>`.
  * The :ref:`ug_nrf5340_gs` guide.

* Updated:

  * The :ref:`software_maturity` page with details about Wi-Fi feature support.
  * The :ref:`app_power_opt` user guide by adding sections about power saving features and PSM usage.
  * The :ref:`ug_thingy53` guide by aligning with current simplified configuration.
  * The :ref:`ug_nrf52_developing` guide by aligning with current simplified FOTA configuration.
  * The :file:`doc/nrf` folder by significantly changing the folder structure.
    See `commit #d55314`_ for details.
