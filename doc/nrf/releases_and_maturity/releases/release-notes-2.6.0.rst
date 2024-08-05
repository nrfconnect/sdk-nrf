.. _ncs_release_notes_260:

|NCS| v2.6.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 3

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

Added the following features as supported:

* Bluetooth® LE:

  * Isochronous channels feature that was introduced in |NCS| 2.5.0 as experimental is now supported.

* Bluetooth Mesh:

  * New sensor API that makes it possible to accurately represent all encodable sensor values.
    The existing API is deprecated and will be removed in a future release.
  * Script for extracting the Bluetooth Mesh DFU metadata automatically when building an application, to simplify the DFU process.
  * The nRF52840 dongle is now supported on the samples :ref:`bluetooth_mesh_light_lc`, :ref:`bluetooth_mesh_light_dim`, and :ref:`ble_mesh_dfu_target`.

* Wi-Fi®:

  * nRF70 Series firmware patches can now optionally reside in external NVM memory instead of on-board, releasing up to 70 KB of space for the application running on the host device (nRF5340 or nRF52840).
  * Raw Wi-Fi packet transmission.
  * New samples: :ref:`wifi_raw_tx_packet_sample` and :ref:`wifi_throughput_sample`.
  * :ref:`wifi_zephyr_samples` can be built with Trusted Firmware-M (TF-M) for enhanced security.

* Matter:

  * Matter 1.2, bringing support for nine new device types such as refrigerators, robotic vacuums, air quality sensors, and more.
  * The :ref:`matter_bridge_app` application that was introduced in |NCS| 2.5.0 as experimental is now supported.
  * Matter over Thread now uses PSA Certified Secure Storage API to enable secure storage of keys and certificates.

* Power Management (nPM1300):

  * New :ref:`npm1300_one_button` sample that demonstrates how to support wake-up, shutdown, and user interactions through a single button connected to the nPM1300.
  * LDO and load-switches soft start configuration to limit voltage fluctuations, and PFM mode configuration for added user flexibility.

* Amazon Sidewalk:

  * Multi-link (autonomous link switching), which provides more flexibility and optimized control of radio links and message transfers based on application profiles.
  * On-device key generation, enhancing the security of IoT products at manufacturing.

* Cellular IoT:

  * Support for nRF9151 DK on the majority of :ref:`cellular IoT samples <cellular_samples>`.
  * The :ref:`serial_lte_modem` application can now be used to turn an nRF91 Series SiP into a standalone modem that can be used through Zephyr's cellular modem driver.

* Security:

  * Introduced the new :ref:`trusted_storage_readme` library which uses Hardware Unique Key to provide an implementation of PSA Certified Secure Storage API without the use of TF-M Platform Root of Trust (PRoT).

* Other:

  * New :ref:`networking_samples`: :ref:`net_coap_client_sample` and :ref:`http_server`.
    These samples can be built for either a cellular IoT target or a Wi-Fi target.

Added the following features as experimental:

* Wi-Fi:

  * Raw Wi-Fi reception in both Monitor and Promiscuous modes.
  * Software-enabled Access Point (SoftAP) with the primary use case being provisioning over Wi-Fi instead of the existing provisioning over Bluetooth LE solution.
  * New samples: :ref:`wifi_softap_sample` and :ref:`wifi_monitor_sample`.

* Amazon Sidewalk:

  * Downlink File Transfer over Bluetooth LE, providing Sidewalk bulk transfer mode and integration of AWS IoT FUOTA (Firmware Updates over-the-air) service.

* Thread:

  * Thread commissioning over authenticated TLS (TCAT) to address the needs of professional installation and commercial building scenarios, where scanning physical install codes is an inadequate solution.
* DFU:

  * Possibility to split the application image between internal and external memory.
    See the documentation on :ref:`qspi_xip` and :ref:`smp_svr_ext_xip`.

* nRF Cloud:

  * The :ref:`nrf_cloud_multi_service` sample now includes experimental support for the following:

    * Runtime installation of TLS certificates using the TLS Credentials Shell when built for :ref:`Wi-Fi connectivity <nrf_cloud_multi_service_building_wifi_conn>` on an nRF5340 DK with an nRF7002 EK.
    * Communication using either MQTT or CoAP over Wi-Fi.
    * Better out-of-box experience by using the new nRF Cloud Security Services :ref:`auto-onboarding feature <nrf_cloud_multi_service_provisioning_service>`.
      This makes it easier to connect a new device to nRF Cloud, including installation of security credentials.
  * The :ref:`nrf_cloud_rest_device_message` sample also includes experimental support for the new nRF Cloud Security Services :ref:`auto-onboarding <remote_prov_auto_onboard>` feature.

* Other:

  * Link Time Optimization that brings flash usage improvements.
    This has been tested on Matter over Wi-Fi template sample and it reduced flash by 73 KB (931 KB to 858 KB) for debug build and 64 KB (840 KB to 776 KB) on release build.

Improved:

* Bluetooth LE Audio:

  * The application nRF5340 Audio is now called :ref:`nrf53_audio_app` and it has been refactored, expanded, and improved, using Nordic Semiconductor's :ref:`softdevice_controller` instead of the previous dedicated LE Audio Controller subsystem.
    This application is still of experimental maturity.

* Matter:

  * Reduction of flash utilization for Matter over Thread template application:

    * nRF52840: Reduction of 48 KB (813 KB to 765 KB) for debug build and 50 KB (719 KB to 669 KB) for release build.
    * nRF5340: Reduction of 36 KB (732 KB to 696 KB) for debug build and 39 KB (637 KB to 598 KB) for release build.

Sign up for the `nRF Connect SDK v2.6.0 webinar`_ to learn more about the new features.

See :ref:`ncs_release_notes_260_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.6.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.6.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.6.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

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
     - `Changelog <Modem library changelog for v2.6.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.6.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.6.0`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v2.6.0`_ for the changes required or recommended when migrating your application from |NCS| v2.5.0 to |NCS| v2.6.0.

.. _ncs_release_notes_260_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE and tool support
====================

* Updated the supported operating system table on the :ref:`requirements` page.

Working with nRF91 Series
=========================

* Added:

  * New partition layout configuration options for Thingy:91.
    See :ref:`thingy91_partition_layout` for more details.
  * Developing with nRF9161 DK user guide.
  * A section on :ref:`tfm_enable_share_uart` in the developing with nRF9161 DK user guide.

* Updated:

  * The :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages so that instructions in the :ref:`nrf9160_gs_connecting_dk_to_cloud` and :ref:`thingy91_connect_to_cloud` sections, respectively, match the updated nRF Cloud workflow.
  * The :ref:`ug_nrf9160_gs` by replacing the Updating the DK firmware section with a new :ref:`nrf9160_gs_installing_software` section.
    This new section includes steps for using Quick Start, a new application in `nRF Connect for Desktop`_ that streamlines the getting started process with the nRF91 Series DKs.
  * :ref:`ug_nrf9160` user guide by separating the information about snippets into its own page, :ref:`ug_nrf91_snippet`.

Working with nRF70 Series
=========================

* Added:

  * The :ref:`ug_nrf70_fw_patch_update` section that describes how to perform a DFU for firmware patches using Wi-Fi in the :ref:`ug_nrf70_developing` user guide.
  * The :ref:`ug_nrf70_developing_fw_patch_ext_flash` and :ref:`ug_nrf70_stack partitioning` pages in the :ref:`ug_nrf70_developing` user guide.

* Updated:

  * The :ref:`nrf7002dk_nrf5340` page with a link to the `Wi-Fi Fundamentals course`_ in the `Nordic Developer Academy`_.
  * The Getting started with nRF70 Series user guide is split into three user guides, :ref:`ug_nrf7002_gs`, :ref:`ug_nrf7002ek_gs` and :ref:`ug_nrf7002eb_gs`.
  * The Operating with a resource constrained host user guide by renaming it to :ref:`nRF70_nRF5340_constrained_host`.
    Additionally, the information about stack configuration and performance is placed into its own separate page, :ref:`ug_wifi_stack_configuration`, under :ref:`ug_wifi`.

Working with nRF53 Series
=========================

* Added the :ref:`qspi_xip` user guide under :ref:`ug_nrf53`.

Working with RF front-end modules
=================================

* Added a clarification that the Simple GPIO implementation is intended for multiple front-end modules (not just one specific device) in the :ref:`ug_radio_fem_sw_support` section of the :ref:`ug_radio_fem` guide.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

* Added:

  * An application behavior that selects the Bluetooth LE only library when the ``CONFIG_SIDEWALK_SUBGHZ_SUPPORT`` option is disabled.
    The library has a limited set of Sidewalk features and does not include multi-link.
  * The sensor monitoring support for Thingy:53 with Sidewalk over Bluetooth LE.
  * Amazon Sidewalk libraries v1.16.2, that include the following:

     * Autonomous link switching (multi-link) - Enables applications to configure a policy for optimizing radio links usage based on the application profiles.
       A device with only one or all link types (Bluetooth LE, LoRa, FSK radio) can benefit from this feature.
     * On-device certificate generation - Enhances the security of IoT products at manufacturing through the on-device key generation and creation of the manufacturing page.
     * Downlink file transfer over Bluetooth LE (experimental) - The Sidewalk Bulk Data Transfer mode and Integration of the AWS IoT FUOTA service allow sending files (up to 1 MB) to a fleet of IoT devices from the AWS IoT FUOTA task.
       The current implementation covers a basic scenario that shows the transferred data in the logs with minimal callback implementation.
       There is no Device Firmware Update (DFU) integration.

  * `Amazon Sidewalk Sample IoT App`_ to the codebase (:file:`sidewalk/tools`) as an original tool for provisioning.

* Updated the entire samples model to include a common, configurable codebase.

Bluetooth LE
------------

* Added host extensions for Nordic Semiconductor-only vendor-specific command APIs.
  Implementation and integration of the host APIs can be found in the :file:`host_extensions.h` header file.

Bluetooth Mesh
--------------

* Added:

  * A script for extracting the Bluetooth Mesh DFU metadata automatically when building an application.
    The script is enabled by the :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` Kconfig option.
  * A separate command ``west build -t ble_mesh_dfu_metadata`` to print Bluetooth Mesh DFU metadata that is already generated to a terminal.

* Updated:

  * :ref:`bt_mesh_dm_srv_readme` and :ref:`bt_mesh_dm_cli_readme` model IDs and opcodes to avoid conflict with Simple OnOff Server and Client models.
  * :ref:`bt_mesh_sensors_readme` to use an updated API with sensor values represented by :c:struct:`bt_mesh_sensor_value` instead of :c:struct:`sensor_value`.
    This makes it possible to accurately represent all encodable sensor values.
    The old APIs based on the :c:struct:`sensor_value` type are deprecated, but are still available for backward compatibility and can be enabled for use by setting the :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE` Kconfig option.
  * The :ref:`bt_mesh_ug_reserved_ids` page with model ID and opcodes for the new :ref:`bt_mesh_le_pair_resp_readme` model.
  * :ref:`bt_mesh_light_ctrl_readme` APIs to match the new sensor APIs.
  * The Kconfig option :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_CONF` to no longer select the :kconfig:option:`CONFIG_BT_MESH_MODEL_EXTENSIONS` option.
  * The :ref:`ug_bt_mesh_configuring` page with the following information:

    * The recommendation on how to configure persistent storage to increase performance.
    * The required configuration for the network core.
  * The :ref:`bluetooth_mesh_dfu_eval_md` section with details about the automated metadata generation.

* Fixed an issue when the Time Server model rewrites TTL to zero forever during the unsolicited Time Status publication.

Matter
------

* For devices that use Matter over Thread, the default cryptography backend is now Arm PSA Crypto API instead of Mbed TLS, which was used in earlier versions.
  You can still build all examples with deprecated Mbed TLS support by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``, but you must build the Thread libraries from sources.
  To :ref:`inherit Thread certification <ug_matter_device_certification_reqs_dependent>` from Nordic Semiconductor, you must use the PSA Crypto API backend.

* Added:

  * The Kconfig option :kconfig:option:`CONFIG_CHIP_ENABLE_READ_CLIENT` for disabling or enabling :ref:`ug_matter_configuring_read_client`.
  * Support for PSA Crypto API for devices that use Matter over Thread.
    It is enabled by default and can be disabled by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``.
  * :file:`VERSION` file implementation to manage versioning for DFU over SMP as well as Matter OTA.
    Backward compatibility is maintained for users who use the :file:`prj.conf` file for versioning.
  * Migration of the Device Attestation Certificate (DAC) private key from the factory data set to the PSA ITS secure storage.

    The DAC private key can be removed from the factory data set after the migration.
    You can enable this experimental functionality by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_MIGRATE_DAC_PRIV_KEY` Kconfig option to ``y``.
  * Redefinition of thermostat sample measurement process, deleted ``CONFIG_THERMOSTAT_EXTERNAL_SENSOR``.
    By default, the thermostat sample generates simulated temperature measurements.
    The generated measurements simulate local temperature changes.
    Additionally, you can enable periodic outdoor temperature measurements by binding the thermostat with an external temperature sensor device.

  * Migration of the Node Operational Key Pair (NOK) from the generic Matter persistent storage to the PSA ITS secure storage.
    All existing NOKs for all Matter fabrics will be migrated to the PSA ITS secure storage at boot.
    After the migration, generic Matter persistent storage entries in the settings storage will be removed and are no longer available.
    To enable operational keys migration, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS` Kconfig option to ``y``.

    In |NCS| Matter samples, the default reaction to migration failure is a factory reset of the device.
    To change the default reaction, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE` Kconfig option to ``n``.
  * Experimental support for building Matter samples and applications with Link Time Optimization (LTO).
    To enable it, set the :kconfig:option:`CONFIG_LTO` and :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION` Kconfig options to ``y``.
  * Documentation page about :ref:`ug_matter_gs_matter_api`.

* Updated:

  * The :ref:`ug_matter_device_low_power_configuration` page with the information about Intermittently Connected Devices (ICD) configuration.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.2.0.1`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

   * Support for the Intermittently Connected Devices (ICD) Management cluster.
   * The Default Kconfig values and developing aspects section to the :doc:`matter:nrfconnect_factory_data_configuration` page.
     The section contains useful developer tricks and device configurations.
   * The Kconfig options :kconfig:option:`CONFIG_CHIP_ICD_IDLE_MODE_DURATION`, :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_DURATION`, and :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC` to manage ICD configuration.
   * New device types:

     * Refrigerator
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

  * The following Kconfig options have been renamed:

   * ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` to :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT`.
   * ``CONFIG_CHIP_SED_IDLE_INTERVAL`` to :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`.
   * ``CONFIG_CHIP_SED_ACTIVE_INTERVAL`` to :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL`.
   * ``CONFIG_CHIP_SED_ACTIVE_THRESHOLD`` to :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD`.

   * zcbor 0.7.0 to 0.8.1.

   * The :kconfig:option:`CONFIG_PRINTK_SYNC` Kconfig option has been disabled to avoid potential interrupts blockage in Matter applications that can violate time-sensitive components, like :ref:`nrf_802154`.

* The Factory Data Guide regarding the changes for the SPAKE2+ verifier, and generation of the Unique ID for Rotating Device ID.
* The factory data default usage has been changed:

  * The SPAKE2+ verifier is now generated by default with each build, and it will change if any of the related Kconfig options are modified.
    This resolves the :ref:`known issue <known_issues>` related to the SPAKE2+ verifier not regenerating (KRKNWK-18315).
  * The Test Certification Declaration can now be generated independently from the generation of the DAC and PAI certificates.
  * The Unique ID for Rotating Device ID can now only be generated if the :kconfig:option:`CONFIG_CHIP_ROTATING_DEVICE_ID` Kconfig option is set to ``y``.

Thread
------

* Updated:

  * The default cryptography backend for Thread is now Arm PSA Crypto API instead of Mbed TLS, which was used in earlier versions.
    You can still build all examples with deprecated Mbed TLS support by setting the :kconfig:option:`CONFIG_OPENTHREAD_NRF_SECURITY_CHOICE` Kconfig option to ``y``, but you must build the Thread libraries from sources.
    To :ref:`inherit Thread certification <ug_thread_cert_inheritance_without_modifications>` from Nordic Semiconductor, you must use the PSA Crypto API backend.

  * nRF5340 SoC targets that do not include :ref:`Trusted Firmware-M <ug_tfm>` now use Hardware Unique Key (HUK, see the :ref:`lib_hw_unique_key` library) for PSA Internal Trusted Storage (ITS).

See `Thread samples`_ for the list of changes in the Thread samples.

Zigbee
------

* Updated:

  * :ref:`nrfxlib:zboss` to v3.11.3.0 and platform v5.1.4 (``v3.11.3.0+5.1.4``).
    They contain fixes for security vulnerabilities and other bugs.
    For details, see :ref:`zboss_changelog`.
  * :ref:`ZBOSS Network Co-processor Host <ug_zigbee_tools_ncp_host>` package to the new version v2.2.2.

* Removed the precompiled development variant of ZBOSS libraries.
* Fixed a bus fault issue at reset when using :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` in some samples configuration (KRKNWK-18572).

Gazell
------

* Added:

  * Kconfig options :kconfig:option:`CONFIG_GAZELL_PAIRING_USER_CONFIG_ENABLE` and :kconfig:option:`CONFIG_GAZELL_PAIRING_USER_CONFIG_FILE`.
    The options allow to use a user-specific file as the Gazell pairing configuration header to override the pairing configuration.
* Fixed the clear system address and host ID in RAM when :c:func:`gzp_erase_pairing_data` is called.

Wi-Fi
-----

* Added:

  * The :kconfig:option:`CONFIG_NRF_WIFI_FW_PATCH_DFU` Kconfig option that enables DFU support for nRF70 Series devices.
    This allows firmware patches for signed images to be sent to multi-image targets over Wi-Fi.
  * Support for the CSP package.
  * Modification to the application of edge backoff.
    Previously, it was subtracted from the regulatory power limit.
    Now, backoff is subtracted from the data rate-dependent power ceiling.

* Updated:

  * WPA supplicant now reserves libc heap memory rather than using leftover RAM.
    This does not affect the overall memory used, but now the RAM footprint as reported by the build shows higher usage.

  * The Wi-Fi interface is renamed from ``wlan0`` to ``nordic_wlan0`` and is available as ``zephyr_wifi`` in the DTS.
    Use the DT APIs to get the handle to the Wi-Fi interface.

HomeKit
-------

HomeKit is now removed, as announced in the :ref:`ncs_release_notes_250`.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

* The Matter bridge application is now supported instead of being experimental.
* Added the :ref:`ipc_radio` application.

Asset Tracker v2
----------------

* Added:

  * Support for the nRF9151 development kit.
  * The :ref:`CONFIG_DATA_SAMPLE_WIFI_DEFAULT <CONFIG_DATA_SAMPLE_WIFI_DEFAULT>` Kconfig option to configure whether Wi-Fi APs are included in sample requests by default.
  * The :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_FOTA` and :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI` Kconfig options.
    The application no longer sends a device shadow update; this is now handled by the :ref:`lib_nrf_cloud` library.
  * Support for ADXL367 accelerometer.

* Updated:

  * The following power optimizations to the LwM2M configuration overlay:

    * Enable DTLS Connection Identifier.
    * Perform the LwM2M update once an hour and request for a similar update interval of the periodic tracking area from the LTE network.
    * Request the same active time as the QUEUE mode polling time.
    * Enable eDRX with shortest possible interval and a short paging window.
    * Enable tickless mode in the LwM2M engine.
    * Enable LTE Release Assist Indicator.
    * The application documentation by separating the Application modules to its own page.

* Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

Connectivity bridge
-------------------

* Updated the handling of the :file:`README.txt` file to support multiple boards.
  The :file:`README.txt` file now contains also version information.

Serial LTE modem
----------------

* Added:

  * Support for the CMUX protocol in order to multiplex multiple data streams through a single serial link.
    The ``#XCMUX`` AT command is added to set up CMUX.
  * Support for PPP in order to use the external MCU's own IP stack instead of offloaded sockets used through AT commands.
    It can be used in conjunction with CMUX to use a single UART for both AT data and PPP.
    The ``#XPPP`` AT command is added to manage the PPP link.
  * ``#XMQTTCFG`` AT command to configure the MQTT client before connecting to the broker.
  * The :ref:`CONFIG_SLM_AUTO_CONNECT <CONFIG_SLM_AUTO_CONNECT>` Kconfig option to support automatic LTE connection at start-up or reset.
  * The :ref:`CONFIG_SLM_CUSTOMER_VERSION <CONFIG_SLM_CUSTOMER_VERSION>` Kconfig option for customers to define their own version string after customization.
  * The optional ``path`` parameter to the ``#XCARRIEREVT`` AT notification.
  * ``#XCARRIERCFG`` AT command to configure the LwM2M carrier library using the LwM2M carrier settings (see the :kconfig:option:`CONFIG_LWM2M_CARRIER_SETTINGS` Kconfig option).
  * Support for Zephyr's cellular modem driver, which allows a Zephyr application running on an external MCU to seamlessly use Zephyr's IP stack instead of AT commands for connectivity.
    See :ref:`slm_as_zephyr_modem` for more information.

* Updated:

  * The ``CONFIG_SLM_WAKEUP_PIN`` Kconfig option has been renamed to :ref:`CONFIG_SLM_POWER_PIN <CONFIG_SLM_POWER_PIN>`.
    In addition to its already existing functionality, it can now be used to power off the SiP.
  * ``#XMQTTCON`` AT command to exclude MQTT client ID from the parameter list.
  * ``#XSLMVER`` AT command to report :ref:`CONFIG_SLM_CUSTOMER_VERSION <CONFIG_SLM_CUSTOMER_VERSION>` if it is defined.
  * The ``#XTCPCLI``, ``#XUDPCLI``, and ``#XHTTPCCON`` AT commands with options for the following purposes:

    * Set the ``PEER_VERIFY`` socket option.
      Set to ``TLS_PEER_VERIFY_REQUIRED`` by default.
    * Set the ``TLS_HOSTNAME`` socket option to ``NULL`` to disable the hostname verification.

  * You can now build the application on nRF9160 DK boards with revisions older than 0.14.0.
  * ``#XCMNG`` AT command to store credentials in Zephyr settings storage.
    The command is activated with the :file:`overlay-native_tls.conf` overlay file.
  * The documentation for the ``#XCARRIER`` and ``#XCARRIERCFG`` commands by adding more detailed information.

* Removed the Kconfig options ``CONFIG_SLM_CUSTOMIZED`` and ``CONFIG_SLM_SOCKET_RX_MAX``.

nRF5340 Audio applications
--------------------------

* Added:

  * Support for Filter Accept List (enabled by default).
  * Metadata used in Auracast, such as ``active_state`` and ``parental_rating``.
  * Support for converting the audio sample rate between 48 kHz, 24 kHz, and 16 kHz.

* Updated:

  * LE Audio controller for nRF5340 from v3424 to v18929.
  * The default controller has been changed from the LE Audio controller for nRF5340 library to the :ref:`ug_ble_controller_softdevice`.
    See the :ref:`nRF5340 Audio applications <nrf5340_audio_migration_notes>` section in the :ref:`migration_2.6` for information about how this affects your application.
  * Sending of the ISO data, which is now done in a single file :file:`bt_le_audio_tx`.
  * Application structure, which is now split into four separate, generic applications with separate :file:`main.c` files.
  * Advertising process by reverting back to slower advertising after a short burst (1.28 s) of directed advertising.
  * Scan process for broadcasters by adding ID as a searchable parameter.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated:

  * The MCUboot and HCI RPMsg child images debug configurations to disable the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option.
    Disabling this Kconfig option improves the debugging experience.
  * The MCUboot and HCI RPMsg child images release configurations to explicitly enable the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option.
    Enabling this Kconfig option improves the reliability of the firmware.

nRF Desktop
-----------

* Added:

  * The :ref:`CONFIG_DESKTOP_HID_STATE_SUBSCRIBER_COUNT <config_desktop_app_options>` Kconfig option to the :ref:`nrf_desktop_hid_state`.
    The option allows to configure a maximum number of simultaneously supported HID data subscribers.
    By default, the value of this Kconfig option is set to ``1``.
    Make sure to align the value in your application configuration.
    For example, to allow subscribing for HID reports simultaneously from the :ref:`nrf_desktop_hids` and :ref:`nrf_desktop_usb_state` (a single USB HID instance), you must set the value of this Kconfig option to ``2``.
  * The pin control (:kconfig:option:`CONFIG_PINCTRL`) dependency to the :ref:`nrf_desktop_wheel`.
    The dependency simplifies accessing **A** and **B** QDEC pins in the wheel module's implementation.
    The pin control is selected by the QDEC driver (:kconfig:option:`CONFIG_QDEC_NRFX`).

* Updated:

  * The :ref:`nrf_desktop_dfu` to use :ref:`partition_manager` definitions for determining currently booted image slot at build time.
    The other image slot is used to store an application update image.
  * The :ref:`nrf_desktop_dfu_mcumgr` to use MCUmgr SMP command status callbacks (the :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` Kconfig option) instead of MCUmgr image and OS management callbacks.
  * The dependencies of the :ref:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK <config_desktop_app_options>` Kconfig option.
    The option can be enabled even when the Bluetooth controller is not enabled as part of the application that uses :ref:`nrf_desktop_ble_latency`.
  * The :ref:`nrf_desktop_bootloader` and :ref:`nrf_desktop_bootloader_background_dfu` sections in the nRF Desktop documentation to explicitly mention the supported DFU configurations.
  * The documentation describing the :ref:`nrf_desktop_memory_layout` configuration to simplify the process of getting started with the application.
  * Changed the term *flash memory* to *non-volatile memory* for generalization purposes.
  * The :ref:`nrf_desktop_watchdog` to use ``watchdog0`` DTS alias instead of ``wdt`` DTS node label.
    Using the alias makes the configuration of the module more flexible.
  * Introduced information about priority, pipeline depth and maximum number of HID reports to :c:struct:`hid_report_subscriber_event`.
  * The :ref:`nrf_desktop_hid_state` uses :c:struct:`hid_report_subscriber_event` to handle connecting and disconnecting HID data subscribers.
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
  * The default value of the newly introduced :kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME_PAIRING_MODE_ONLY` Kconfig option.
    The option is disabled by default.
    The Bluetooth device name is provided in the scan response also outside of pairing mode for backwards compatibility.
  * The default value of newly introduced :kconfig:option:`CONFIG_CAF_BLE_ADV_POWER_DOWN_ON_DISCONNECTION_REASON_0X15` Kconfig option.
    The option is enabled by default.
    Force power down on disconnection with reason ``0x15`` (Remote Device Terminated due to Power Off) is triggered to avoid waking up HID host until user input is detected.
  * The :ref:`nrf_desktop_wheel` configuration to allow using **GPIO1** for scroll wheel.
  * The :ref:`application documentation <nrf_desktop>` by splitting it into several pages.

Thingy:53: Matter weather station
---------------------------------

* Updated by changing the deployment of configuration files to align with other Matter applications.
* Removed instantiation of ``OTATestEventTriggerDelegate``, which was usable only for Matter Test Event purposes.

Matter bridge
-------------

* Added:

  * Support for groupcast communication to the On/Off Light device implementation.
  * Support for controlling the OnOff Light simulated data provider by using shell commands.
  * Support for Matter Generic Switch bridged device type.
  * Support for On/Off Light Switch bridged device type.
  * Support for bindings to the On/Off Light Switch device implementation.
  * Guide about extending the application by adding support for a new Matter device type, a new Bluetooth LE, service or a new protocol.
  * Support for Bluetooth LE Security Manager Protocol that allows to establish secure session with bridged Bluetooth LE devices.
  * Callback to indicate the current state of Bluetooth LE Connectivity Manager.
    The current state is indicated by **LED 2**.
  * Support for the nRF5340 DK with the nRF7002 EK attached.
  * Support for Wi-Fi firmware patch upgrades on external memory.
    This is only available for the nRF5340 DK with the nRF7002 EK.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added the :ref:`ble_event_trigger` sample for the proprietary Event Trigger feature.
* :ref:`ble_throughput` sample:

  * Updated by enabling encryption in the sample.
    The measured throughput is calculated over the encrypted data, which is how most of the Bluetooth products use this protocol.

* :ref:`direct_test_mode` sample:

  * Added the configuration option to disable the Direction Finding feature.

* :ref:`central_uart` sample:

  * Updated by correcting the behavior when building with the :ref:`ble_rpc` library.

* :ref:`bluetooth_central_dfu_smp` sample:

  * Updated to adapt to API changes in zcbor 0.8.x.

Bluetooth Mesh samples
----------------------

* :ref:`ble_mesh_dfu_distributor` sample:

  * Added support for pairing with display capability and the :ref:`bt_mesh_le_pair_resp_readme`.
  * Fixed an issue where the shell interface was not accessible over UART because UART was used as a transport for the MCUmgr SMP protocol.
    Shell is now accessible over RTT.

* :ref:`ble_mesh_dfu_target` sample:

  * Added support for the :ref:`zephyr:nrf52840dongle_nrf52840`.

* :ref:`bluetooth_mesh_light_dim` sample:

  * Added support for the :ref:`zephyr:nrf52840dongle_nrf52840`.
  * Fixed an issue where Bluetooth could not be initialized due to a misconfiguration between the Bluetooth host and the Bluetooth LE Controller when building with :ref:`zephyr:sysbuild` for the :ref:`zephyr:nrf5340dk_nrf5340` and :ref:`zephyr:thingy53_nrf5340` boards.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added support for the :ref:`zephyr:nrf52840dongle_nrf52840`.
  * Fixed an issue where Bluetooth could not be initialized due to a misconfiguration between the Bluetooth host and the Bluetooth LE Controller when building with :ref:`zephyr:sysbuild` for the :ref:`zephyr:nrf5340dk_nrf5340` and :ref:`zephyr:thingy53_nrf5340` boards.

* :ref:`bluetooth_mesh_light` sample:

  * Fixed an issue where Bluetooth could not be initialized due to a misconfiguration between the Bluetooth host and the Bluetooth LE Controller when building with :ref:`zephyr:sysbuild` for the :ref:`zephyr:nrf5340dk_nrf5340` and :ref:`zephyr:thingy53_nrf5340` boards.

* :ref:`bluetooth_mesh_light_switch` sample:

  * Fixed an issue where Bluetooth could not be initialized due to a misconfiguration between the Bluetooth host and the Bluetooth LE Controller when building with :ref:`zephyr:sysbuild` for the :ref:`zephyr:nrf5340dk_nrf5340` and :ref:`zephyr:thingy53_nrf5340` boards.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Fixed an issue where Bluetooth could not be initialized due to a misconfiguration between the Bluetooth host and the Bluetooth LE Controller when building with :ref:`zephyr:sysbuild` for the :ref:`zephyr:nrf5340dk_nrf5340` and :ref:`zephyr:thingy53_nrf5340` boards.

* :ref:`bluetooth_mesh_silvair_enocean` sample:

  * Fixed an issue where Bluetooth could not be initialized due to a misconfiguration between the Bluetooth host and the Bluetooth LE Controller when building with :ref:`zephyr:sysbuild` for the :ref:`zephyr:nrf5340dk_nrf5340` board.

Cellular samples
----------------

* Added support for the nRF9151 DK in all cellular samples except for the following samples:

  * :ref:`lte_sensor_gateway`
  * :ref:`smp_svr`
  * :ref:`slm_shell_sample`

* :ref:`ciphersuites` sample:

  * Updated:

    * The format of the :file:`.pem` file to the PEM format.
    * The sample to automatically convert the :file:`.pem` file to HEX format so it can be included.

* :ref:`location_sample` sample:

  * Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for full modem FOTA.
    * Printing of the last reset reason when the sample starts.
    * Support for printing the sample version information using the ``version`` command.
    * Support for counting pulses from a GPIO pin using the ``gpio_count`` command.
    * Support for changing shell UART baudrate using the ``uart baudrate`` command.
    * Support for DNS query using the ``sock getaddrinfo`` command.
    * Support for PDN CID 0 in the ``-I`` argument for the ``sock connect`` command.
    * Support for listing interface addresses using the ``link ifaddrs`` command.
    * Support for the ``MSG_WAITACK`` send flag using the ``sock send`` command.
    * Support for connect with ``SO_KEEPOPEN`` using the ``sock connect`` command.

  * Updated:

    * The MQTT and CoAP overlays to enable the Kconfig option :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI`.
      The sample no longer sends a device shadow update for MQTT and CoAP builds; this is now handled by the :ref:`lib_nrf_cloud` library.
    * To use the new :c:struct:`nrf_cloud_location_config` structure when calling the :c:func:`nrf_cloud_location_request` function.
    * The ``connect`` subcommand to use the :c:func:`connect` function on non-secure datagram sockets.
      This sets the peer address for the non-secure datagram socket.
      This fixes a bug where using the ``connect`` subcommand and then using the ``rai_no_data`` option with the ``rai`` subcommand on a non-secure datagram socket would lead to an error.
      The ``rai_no_data`` option requires the socket to be connected and have a peer address set.
      This bug is caused by the non-secure datagram socket not being connected when using the ``connect`` subcommand.
    * The ``send`` subcommand to use the :c:func:`send` function for non-secure datagram sockets that are connected and have a peer address set.
    * The ``modem_trace`` command has been moved to :ref:`nrf_modem_lib_readme`.
      See :ref:`modem_trace_shell_command` for information about modem trace commands.
    * The sample to allow TLS/DTLS security tag values up to ``4294967295``.

  * Removed:

    * The nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.
    * The ``modem_trace send memfault`` shell command.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * A generic processing example for application-specific shadow data.
    * Configuration and overlay files to enable MCUboot to use the external flash on the nRF1961 DK.
    * The :kconfig:option:`CONFIG_COAP_ALWAYS_CONFIRM` Kconfig option to select CON or NON CoAP transfers for functions that previously used NON transfers only.
    * Support for the :ref:`lib_nrf_provisioning` library.
    * Two overlay files :file:`overlay-http_nrf_provisioning.conf` and :file:`overlay-coap_nrf_provisioning.conf` to enable the :ref:`lib_nrf_provisioning` library with HTTP and CoAP connectivity, respectively.
      Both overlays specify UUID-style device IDs (not 'nrf-\ *IMEI*\ '-style) for compatibility with nRF Cloud auto-onboarding.
    * An overlay file :file:`overlay_nrf7002ek_wifi_coap_no_lte.conf` for experimental CoAP over Wi-Fi support.

  * Updated:

    * The sample now explicitly uses the :c:func:`conn_mgr_all_if_connect` function to start network connectivity, instead of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START` and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT` Kconfig options.
    * The sample to use the FOTA support functions in the :file:`nrf_cloud_fota_poll.c` and :file:`nrf_cloud_fota_common.c` files.
    * The sample now enables the Kconfig options :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_FOTA` and :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI`.
      It no longer sends a device status update on initial connection; this is now handled by the :ref:`lib_nrf_cloud` library.
    * Increased the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` and :kconfig:option:`CONFIG_AT_MONITOR_HEAP_SIZE` Kconfig options to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.
    * The sample reboot logic is now in a dedicated file so that it can be used in multiple locations.
    * The Wi-Fi connectivity overlay now uses the PSA Protected Storage backend of the :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` instead of the volatile backend.
    * The Wi-Fi connectivity overlay now enables the :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>` for run-time credential installation.
    * The DNS resolution timeout for Wi-Fi connectivity builds has been increased.

  * Fixed:

    * The sample now waits for a successful connection before printing ``Connected to nRF Cloud!``.
    * Building for the Thingy:91.
    * The PSM Requested Active Time is now reduced from 1 minute to 20 seconds.
      The old value was too long for PSM to activate.

  * Removed the nRF7002 EK devicetree overlay file :file:`nrf91xxdk_with_nrf7002ek.overlay`, because UART1 is disabled through the shield configuration.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added credential check before connecting to the network.
  * Updated:

    * The sample now uses the functions in the :file:`nrf_cloud_fota_poll.c` and :file:`nrf_cloud_fota_common.c` files.
    * The :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` Kconfig option value has been increased to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.

* :ref:`nrf_cloud_rest_cell_pos_sample` sample:

  * Added:

    * Credential check before connecting to the network.
    * Use of the :c:struct:`nrf_cloud_location_config` structure to modify the ground fix results.

  * Updated by increasing the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` Kconfig option value to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.

* :ref:`nrf_provisioning_sample` sample:

  * Added event handling for events from device mode callback.

* :ref:`gnss_sample` sample:

  * Added the configuration overlay file :file:`overlay-supl.conf` for building the sample with SUPL assistance support.

* :ref:`udp` sample:

  * Added the :ref:`CONFIG_UDP_DATA_UPLOAD_ITERATIONS <CONFIG_UDP_DATA_UPLOAD_ITERATIONS>` Kconfig option for configuring the number of data transmissions to the server.

* :ref:`lwm2m_carrier` sample:

  * Updated:

    * The format of the :file:`.pem` files to the PEM format.
    * The sample to automatically convert the :file:`.pem` files to HEX format so they can be included.

* :ref:`lwm2m_client` sample:

  * Added:

    * A workaround for ground fix location assistance queries in AVSystem Coiote by using the fixed Connectivity Monitor object version.
      This is enabled in the :file:`overlay-assist-cell.conf` configuration overlay.
    * Release Assistance Indication (RAI) feature.
      This helps to save power by releasing the network connection faster on a network that supports it.

  * Updated:

    * The eDRX cycle to 5.12 s for both LTE-M and NB-IoT.
    * The periodic TAU (RPTAU) to 12 hours.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Updated:

    * The :file:`overlay-nrf_provisioning.conf` overlay to specify UUID-style device IDs for compatibility with nRF Cloud auto-onboarding.
    * The values of :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` and :kconfig:option:`CONFIG_AT_MONITOR_HEAP_SIZE` Kconfig options have been increased to 2048 bytes since nRF Cloud credentials are sometimes longer than 1024 bytes.

Cryptography samples
--------------------

* Added the :ref:`crypto_pbkdf2` sample.
* Updated:

  * All crypto samples to use ``psa_key_id_t`` instead of ``psa_key_handle_t``.
    These concepts have been merged and ``psa_key_handle_t`` is removed from the PSA API specification.

Debug samples
-------------

* :ref:`memfault_sample` sample:

  * Added support for the nRF9151 development kit.

Matter samples
--------------

* Unified common code for buttons, LEDs, and events in all Matter samples:

  * Created the task executor module that is responsible for posting and dispatching tasks.
  * Moved common methods for managing buttons and LEDs that are located on the DK to the board module.
  * Divided events to application and system events.
  * Defined common LED and button constants in the dedicated board configuration files.
  * Created the Kconfig file for the Matter common directory.
  * Created a CMake file in the Matter common directory to centralize the sourcing of all common software module source code.

* Disabled the following features:

  * :ref:`ug_matter_configuring_read_client` in most Matter samples using the new :kconfig:option:`CONFIG_CHIP_ENABLE_READ_CLIENT` Kconfig option.
  * WPA supplicant advanced features in all Matter samples using the ``CONFIG_WPA_SUPP_ADVANCED_FEATURES`` Kconfig option.
    This saves roughly 25 KB of flash memory for firmware images with Wi-Fi support.

* Added ``matter_shell`` shell commands set to gather the current information about the NVS settings backend, such as current usage, free space, and peak usage value.
  You can enable them by setting the :kconfig:option:`NCS_SAMPLE_MATTER_SETTINGS_SHELL` Kconfig option to ``y``.
  To read more, see the :ref:`ug_matter_configuring_settings_shell` section.

* :ref:`matter_light_bulb_sample` sample:

  * Added support for `AWS IoT Core`_.

* :ref:`matter_template_sample` sample:

  * Added support for DFU over Bluetooth LE SMP.
    The functionality is disabled by default.
    To enable it, set the :kconfig:option:`CONFIG_CHIP_DFU_OVER_BT_SMP` Kconfig option to ``y``.

* :ref:`matter_lock_sample` sample:

  * Added support for Wi-Fi firmware patch upgrade on external memory, only for the combination of the nRF5340 DK with the nRF7002 EK.
  * Updated the design of the :ref:`matter_lock_sample_wifi_thread_switching` feature so that support for both Matter over Thread and Matter over Wi-Fi is included in a single firmware image.
  * Fixed an issue that prevented nRF Toolbox for iOS in version 5.0.9 from controlling the sample using :ref:`nus_service_readme`.

Networking samples
------------------

* Added:

  * A new sample :ref:`http_server`.
  * Support for the nRF9151 development kit.

* Updated the networking samples to build using the non-secure target when building for the nRF7002DK.
  The :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_SMALL` profile type is used for Trusted Firmware-M (TF-M) to optimize its memory footprint.

* :ref:`net_coap_client_sample` sample:

  * Added support for Wi-Fi and LTE connectivity through the connection manager API.

  * Updated:

    * The sample is moved from the :file:`cellular/coap_client` folder to :file:`net/coap_client`.
      The documentation is now found in the :ref:`networking_samples` section.
    * The sample to use the :ref:`coap_client_interface` library.

* :ref:`https_client` sample:

  * Updated:

    * The :file:`.pem` certificate for example.com.
    * The format of the :file:`.pem` file to the PEM format.
    * The sample to automatically convert the :file:`.pem` file to HEX format so it can be included.
    * The sample to gracefully bring down the network interfaces.
    * Renamed :file:`overlay-pdn_ipv4.conf` to :file:`overlay-pdn-nrf91-ipv4.conf` and :file:`overlay-tfm_mbedtls.conf` to :file:`overlay-tfm-nrf91.conf`.

* :ref:`download_sample` sample:

  * Updated:

    * The format of the :file:`.pem` file to the PEM format.
    * The sample to automatically convert the :file:`.pem` file to HEX format so it can be included.
    * The sample to gracefully bring down the network interfaces.


nRF5340 samples
---------------

* Added the :ref:`smp_svr_ext_xip` sample that demonstrates how to split an application that uses internal flash and Quad Serial Peripheral Interface (QSPI) flash with the Simple Management Protocol (SMP) server.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Updated:

    * The ``start_tx_modulated_carrier`` command, when used without an additional parameter, does not enable the radio end interrupt.
    * Corrected the way of setting the TX power with FEM.

PMIC samples
------------

* Added :ref:`npm1300_one_button` sample that demonstrates how to support wake-up, shutdown, and user interactions through a single button connected to the nPM1300.

* :ref:`npm1300_fuel_gauge` sample:

  * Updated to accommodate API changes in the :ref:`nrfxlib:nrf_fuel_gauge`.

Thread samples
--------------

* Added experimental support for Thread Over Authenticated TLS.
* Updated the building method to use :ref:`zephyr:snippets` for predefined configuration.

* Removed:

  * In the :ref:`thread_ug_feature_sets` provided as part of the |NCS|, the following features have been removed from the FTD and MTD variants:

    * ``DHCP6_CLIENT``
    * ``JOINER``
    * ``SNTP_CLIENT``
    * ``LINK_METRICS_INITIATOR``

    All mentioned features are still available in the master variant.

Wi-Fi samples
-------------

* Added:

  * The :ref:`wifi_throughput_sample` sample that demonstrates how to measure the network throughput of a Nordic Semiconductor Wi-Fi enabled platform under the different Wi-Fi stack configuration profiles.
  * The :ref:`wifi_softap_sample` sample that demonstrates how to start a nRF70 Series device in :term:`Software-enabled Access Point (SoftAP or SAP)` mode.
  * The :ref:`wifi_raw_tx_packet_sample` sample that demonstrates how to transmit raw TX packets.
  * The :ref:`wifi_monitor_sample` sample that demonstrates how to start a nRF70 Series device in :term:`Monitor` mode.

* :ref:`wifi_shell_sample` sample:

  * Updated the sample by adding the following extensions to the Wi-Fi command line:

    * ``raw_tx`` extension.
      It adds the subcommands to configure and sends raw TX packets.

    * ``promiscuous_set`` extension.
      It adds the subcommand to configure Promiscuous mode.

* :ref:`wifi_ble_coex_sample` sample:

  * Updated the sample folder name from ``sr_coex`` to ``ble_coex`` to accurately represent the functionality of the sample.

* :ref:`wifi_radio_test` sample:

  * Updated:

    * The sample now has added support for runtime configuration of antenna gain and edge backoff parameters.
    * Antenna gain and edge backoff values are not applied if regulatory is bypassed.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* Added the following features to the nRF70 Series devices:

  * TX injection feature.
  * Monitor feature.
  * Promiscuous mode.

*  Updated:

  * OS agnostic code is moved to |NCS| (``sdk-nrfxlib``) repository.

    * Low-level API documentation is now available on the ``Wi-Fi driver API``.

  * The Wi-Fi interface is now renamed and registered as a devicetree instance.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v3.4.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * The :c:func:`bt_fast_pair_enable`, :c:func:`bt_fast_pair_disable`, :c:func:`bt_fast_pair_is_ready` functions to handle the Fast Pair subsystem readiness.
      In order to use the Fast Pair service, the Fast Pair subsystem must be enabled with the :c:func:`bt_fast_pair_enable` function.
    * The :file:`include/bluetooth/services/fast_pair/uuid.h` header file that contains UUID definitions for the Fast Pair service and its characteristics.

  * Updated:

    * The :ref:`bt_fast_pair_readme` library documentation has been improved to include the description of the missing Kconfig options.
    * The requirements regarding the thread context of Fast Pair API calls have been strengthened.
      More functions are required to be called in the cooperative thread context.
    * The :c:func:`bt_fast_pair_info_cb_register` function is now not allowed to be called when the Fast Pair is enabled.
      The :c:func:`bt_fast_pair_info_cb_register` function can only be called before enabling the Fast Pair with the :c:func:`bt_fast_pair_enable` function.
    * The :file:`include/bluetooth/services/fast_pair.h` header file is moved to the :file:`ncs/nrf/include/bluetooth/services/fast_pair` directory to have a common place for Fast Pair headers.

* :ref:`bt_mesh` library:

  * Added the :ref:`bt_mesh_le_pair_resp_readme` model to allow passing a passkey used in LE pairing over a mesh network.

* :ref:`nrf_bt_scan_readme`:

  * Added the :c:func:`bt_scan_update_connect_if_match` function to update the autoconnect flag after a filter match.

* :ref:`bt_le_adv_prov_readme`:

  * Updated the default behavior of the Bluetooth device name provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`).
    By default, the device name is provided only in the pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`).
    You can disable the newly introduced Kconfig option (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME_PAIRING_MODE_ONLY`) to provide the device name also when the device is not in the pairing mode.

DFU libraries
-------------

* :ref:`lib_dfu_target` library:

  * Updated:

    * The :kconfig:option:`CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION` Kconfig option to be automatically enabled when ``nordic,pm-ext-flash`` is chosen in the devicetree.
      See :ref:`partition_manager` for details.
    * Adapted to API changes in zcbor 0.8.x.

* :ref:`lib_fmfu_fdev` library:

  * Updated by regenerating cbor code with zcbor 0.8.1 and adapting to API changes in zcbor 0.8.x.

Debug libraries
---------------

* :ref:`mod_memfault` library:

  * Added more default LTE metrics, such as band, operator, RSRP, and kilobytes sent and received.
  * Updated the default metric names to follow the standard |NCS| variable name convention.

Modem libraries
---------------

* :ref:`lib_location` library:

  * Added:

    * The :c:enumerator:`LOCATION_EVT_STARTED` event to indicate that a location request has been started, and the :c:enumerator:`LOCATION_EVT_FALLBACK` event to indicate that a fallback from one location method to another has occurred.
      These are for metrics collection purposes and sent only if the :kconfig:option:`CONFIG_LOCATION_DATA_DETAILS` Kconfig option is set.
    * Support for multiple event handlers.
    * Additional location data details into the :c:struct:`location_data_details` structure hierarchy.
    * The :c:enumerator:`LOCATION_METHOD_WIFI_CELLULAR` method that cannot be added into the location configuration passed to the :c:func:`location_request` function, but may occur within the :c:struct:`location_event_data` structure.

  * Updated:

    * The use of neighbor cell measurements for cellular positioning.
      Previously, 1-2 searches were performed and now 1-3 will be done depending on the requested number of cells and the number of found cells.
      Also, only GCI cells are counted towards the requested number of cells, and normal neighbors are ignored from this perspective.
    * Cellular positioning to not use GCI search when the device is in RRC connected mode, because the modem cannot search for GCI cells in that mode.
    * GNSS is not started at all if the device does not enter RRC idle mode within two minutes.

* :ref:`lte_lc_readme` library:

  * Added the :c:func:`lte_lc_psm_param_set_seconds` function and Kconfig options :kconfig:option:`CONFIG_LTE_PSM_REQ_FORMAT`, :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU_SECONDS`, and :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT_SECONDS` to enable setting of PSM parameters in seconds instead of using bit field strings.

  * Updated:

    * The default network mode to :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` from :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_GPS`.
    * The default LTE mode preference to :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE_LTE_M_PLMN_PRIO` from :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE_AUTO`.
    * The ``CONFIG_LTE_NETWORK_USE_FALLBACK`` Kconfig option is deprecated.
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
    * The :c:macro:`SO_RAI` socket option for Release Assistance Indication (RAI).
      The socket option uses values ``RAI_NO_DATA``, ``RAI_LAST``, ``RAI_ONE_RESP``, ``RAI_ONGOING``, or ``RAI_WAIT_MORE`` to specify the desired indication.
      This socket option substitutes the deprecated RAI (``SO_RAI_*``) socket options.

  * Updated:

    * The following socket options have been deprecated:

       * :c:macro:`SO_RAI_NO_DATA`
       * :c:macro:`SO_RAI_LAST`
       * :c:macro:`SO_RAI_ONE_RESP`
       * :c:macro:`SO_RAI_ONGOING`
       * :c:macro:`SO_RAI_WAIT_MORE`

      Use the :c:macro:`SO_RAI` socket option instead.

    * The library by renaming ``lte_connectivity`` module to ``lte_net_if``.
      All related Kconfig options have been renamed accordingly.
    * The default value of the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_START`, :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_CONNECT`, and :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF_AUTO_DOWN` Kconfig options from enabled to disabled.
    * The modem trace shell command implementation is moved from :ref:`modem_shell_application` sample into :ref:`nrf_modem_lib_readme` to be used by any application with the :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHELL_TRACE` Kconfig option enabled.

  * Fixed:

    * The ``lte_net_if`` module now handles the :c:enumerator:`~pdn_event.PDN_EVENT_NETWORK_DETACH` PDN event.
      Not handling this caused permanent connection loss and error message (``ipv4_addr_add, error: -19``) in some situations when reconnecting.
    * Threads sleeping in the :c:func:`nrf_modem_os_timedwait` function with context ``0`` are now woken by all calls to the :c:func:`nrf_modem_os_event_notify` function.

  * Removed:

    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_SYS_INIT``.
    * The deprecated Kconfig option ``CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO_OVERRIDE``.
    * The ``NRF_MODEM_LIB_NET_IF_DOWN`` flag support in the ``lte_net_if`` network interface driver.

* :ref:`lib_modem_slm`:

    * Updated the library by making the used GPIO to be configurable using devicetree.

* :ref:`pdn_readme` library:

   * Added the :c:func:`pdn_dynamic_params_get` function to retrieve dynamic parameters of an active PDN connection.
   * Updated the library to add PDP auto configuration to the :c:enumerator:`LTE_LC_FUNC_MODE_POWER_OFF` event.
   * Fixed:

     * A potential issue where the library tries to free the PDN context twice, causing the application to crash.
     * A potential crash when an AT notification arrives during :c:func:`pdn_ctx_create`.
       The callback was only initialized after adding it to the list.

  * Removed the dependency on the :ref:`lte_lc_readme` library.

* :ref:`lib_at_host` library:

  * Added the :kconfig:option:`CONFIG_AT_HOST_STACK_SIZE` Kconfig option.
    This option allows the stack size of the AT host workqueue thread to be adjusted.

* :ref:`modem_key_mgmt` library:

  * Fixed a potential race condition, where two threads might corrupt a shared response buffer.

* :ref:`lib_modem_attest_token` library:

  * Updated to adapt to API changes in zcbor 0.8.x.

Libraries for networking
------------------------

* :ref:`lib_aws_iot` library:

  * Added library tests.
  * Updated the library to use the :ref:`lib_mqtt_helper` library.
    This simplifies the handling of the MQTT stack.

* :ref:`lib_azure_iot_hub` library:

  * Added support for the newly added :file:`cert_tool.py` Python script.
    This is a script to generate EC private keys, create Certificate Signing Request (CSR), create root Certificate Authority (CA), subordinate CA certificates, and sign CSRs.
  * Updated the documentation on IoT Hub configuration and credentials storage.
    It now contains more details on how to use the Azure CLI to set up an IoT Hub.
    The documentation on credential provisioning has also been updated, both for nRF91 Series devices and nRF70 Series devices.

* :ref:`lib_download_client` library:

  * Added the ``family`` parameter to the :c:struct:`download_client_cfg` structure.
    This is used to optimize the download sequence when the device only supports IPv4 or IPv6.

  * Updated:

    * IPv6 support changed from compile time to runtime, and is enabled by default.
    * IPv6 to IPv4 fallback is done when both DNS request and TCP/TLS connect fails.
    * HTTP downloads forward data fragments to a callback only when the buffer is full.

  * Removed the ``CONFIG_DOWNLOAD_CLIENT_IPV6`` Kconfig option.

* :ref:`lib_nrf_cloud_coap` library:

  * Added:

    * Automatic selection of proprietary PSM mode when building for the ``SOC_NRF9161_LACA``.
    * Support for bulk transfers to the :c:func:`nrf_cloud_coap_json_message_send` function.
    * Support for raw transfers to the :c:func:`nrf_cloud_coap_bytes_send` function.
    * Optional support for ground fix configuration flags.
    * Support for non-modem build.
    * Support for credentials provisioning.

  * Updated:

    * The :c:func:`nrf_cloud_coap_shadow_delta_process` function to include a parameter for application-specific shadow data.
    * The :c:func:`nrf_cloud_coap_shadow_delta_process` function to process default shadow data added by nRF Cloud, which is not used by CoAP.
    * The CDDL file for AGNSS to align with cloud changes and regenerated the AGNSS encoder accordingly.
    * To allow QZSS constellation assistance requests from AGNSS.
    * The following functions to accept a ``confirmable`` parameter:

      * :c:func:`nrf_cloud_coap_bytes_send`
      * :c:func:`nrf_cloud_coap_obj_send`
      * :c:func:`nrf_cloud_coap_sensor_send`
      * :c:func:`nrf_cloud_coap_message_send`
      * :c:func:`nrf_cloud_coap_json_message_send`
      * :c:func:`nrf_cloud_coap_location_send`

      This parameter determines whether CoAP CON or NON messages are used.

    * The cbor code has been regenerated with zcbor 0.8.1 and adapted to API changes in zcbor 0.8.x.

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
    * An :c:struct:`nrf_cloud_location_config` structure for specifying the desired behavior of an nRF Cloud ground fix request.
    * Support for custom JWT creation by selecting the :kconfig:option:`CONFIG_NRF_CLOUD_JWT_SOURCE_CUSTOM` Kconfig option.
    * Support for specific credentials provisioning using the following Kconfig options:

      * :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CA_CERT`
      * :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CLIENT_CERT`
      * :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_PRV_KEY`

    * Support for the :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` by selecting the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_MGMT_TLS_CRED` Kconfig option.
      This is applicable to the :kconfig:option:`CONFIG_NRF_CLOUD_CHECK_CREDENTIALS` and :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig options.
    * The :file:`nrf_cloud_fota_poll.c` file to consolidate the FOTA polling code from the :ref:`nrf_cloud_multi_service` and :ref:`nrf_cloud_rest_fota` samples.
    * The :file:`nrf_cloud_fota_poll.h` file.

  * Updated:

    * The :c:func:`nrf_cloud_obj_object_add` function to reset the added object on success.
    * Custom shadow data is now passed to the application during shadow update events.
    * The AGNSS handling to use the AGNSS app ID string and corresponding MQTT topic instead of the older AGPS app ID string and topic.
    * The :c:func:`nrf_cloud_obj_location_request_create` and :c:func:`nrf_cloud_location_request` functions to accept the :c:struct:`nrf_cloud_location_config` structure in place of the ``bool request_loc`` parameter.

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed:

    * A bug in the prediction set update when the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` Kconfig option was set to non-zero value.
    * A bug in handling errors when downloading P-GPS data that prevented retries until after a reboot.
    * A bug in handling errors when downloading P-GPS data does not begin due to active FOTA download.

* :ref:`lib_nrf_provisioning` library:

  * Added the :kconfig:option:`CONFIG_NRF_PROVISIONING_PRINT_ATTESTATION_TOKEN` option to enable printing the attestation token when the device is not yet claimed.
  * Updated:

    * Renamed nRF Device provisioning library to :ref:`lib_nrf_provisioning` library.
    * The device mode callback sends an event when the provisioning state changes.
    * The cbor code has been regenerated with zcbor 0.8.1 and adapted to API changes in zcbor 0.8.x.

  * Fixed file descriptor handling by setting the :c:struct:`coap_client` structure's ``fd`` field to ``-1`` when closing the socket.

* :ref:`lib_mqtt_helper` library:

  * Added support for using a password when connecting to a broker.

* :ref:`lib_lwm2m_client_utils` library:

  * Updated:

    * The Release Assistance Indication (RAI) support to follow socket state changes from LwM2M engine and modify RAI values based on the state.
    * The library requests a periodic TAU (RPTAU) to match the client lifetime.

* :ref:`lib_lwm2m_location_assistance` library:

  * Updated the Ground Fix object to copy received coordinates to the LwM2M Location object.

* :ref:`lib_fota_download` library:

  * Added:

    * The functions :c:func:`fota_download` and :c:func:`fota_download_any` that can accept a security tag list and security tag count as arguments instead of a single security tag.
    * :c:enumerator:`FOTA_DOWNLOAD_ERROR_CAUSE_CONNECT_FAILED` as a potential error cause in  :c:enumerator:`FOTA_DOWNLOAD_EVT_ERROR` events.

* :ref:`lib_nrf_cloud_rest` library:

  * Updated the :c:struct:`nrf_cloud_rest_location_request` structure to accept a pointer to a :c:struct:`nrf_cloud_location_config` structure in place of the single ``disable_response`` flag.

* :ref:`lib_wifi_credentials` library:

  * Updated the PSA backend to use the PSA Internal Trusted Storage (ITS) for storing Wi-Fi credentials instead of the Protected Storage.
    This has been changed because the PSA ITS is a better fit for storing assets like credentials.
    When switching storage, the credentials need to be migrated manually, or the existing credentials are lost.
  * Removed the ``CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_UID_OFFSET`` Kconfig option.
    Use the :kconfig:option:`CONFIG_WIFI_CREDENTIALS_BACKEND_PSA_OFFSET` Kconfig option instead.

Libraries for NFC
-----------------

* Fixed an issue with handling zero size data (when receiving empty I-blocks from poller) in the :file:`platform_internal_thread` file.

Security libraries
------------------

* Added the :ref:`trusted_storage_readme` library.

* :ref:`nrf_security` library:

  * Updated:

    * The library no longer enables RSA keys by default, which reduces the code size by 30 kB for those that are not using RSA keys.
      The RSA key size must be explicitly enabled to avoid breaking the configuration when using the RSA keys, for example, by setting the :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048` Kconfig option if 2048-bit RSA keys are required.
    * The PSA config is now validated by the :file:`ncs/nrf/ext/oberon/psa/core/library/check_crypto_config.h` file.
      Users with invalid configurations must update their PSA configuration according to the error messages that the :file:`check_crypto_config.h` file provides.

Other libraries
---------------

* :ref:`lib_adp536x` library:

  * Fixed the issue where the adp536x driver was included in the immutable bootloader on Thingy:91 when the :kconfig:option:`CONFIG_SECURE_BOOT` Kconfig option was enabled.

* :ref:`lib_date_time` library:

  * Added the :c:func:`date_time_now_local` function to the API.

* :ref:`dk_buttons_and_leds_readme` library:

  * Added an experimental no interrupts mode for button handling.
    To enable this mode, use the :kconfig:option:`CONFIG_DK_LIBRARY_BUTTON_NO_ISR` Kconfig option.
    You can use this mode as a workaround to avoid using the GPIO interrupts.
    However, it increases the power consumption.

Common Application Framework (CAF)
----------------------------------

* :ref:`caf_ble_state`:

  * Added :c:member:`ble_peer_event.reason` to inform about the reason code related to state of the Bluetooth LE peer.
    The field is used to propagate information about an error code related to a connection establishment failure and disconnection reason.
  * Updated the dependencies of the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` Kconfig option.
    You can enable the option even when the Bluetooth controller is not enabled as part of the application that uses :ref:`caf_ble_state`.

* :ref:`caf_ble_adv`:

  * Added the :kconfig:option:`CONFIG_CAF_BLE_ADV_POWER_DOWN_ON_DISCONNECTION_REASON_0X15` Kconfig option.
    You can use this option to force system power down when a bonded peer disconnects with reason ``0x15`` (Remote Device Terminated due to Power Off).

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* Added the :file:`cert_tool.py` script.
  This is a script to generate EC private keys, create CSRs, create root CA and subordinate CA certificates and sign CSRs.

* :ref:`nrf_desktop_config_channel_script`:

  * Separated functions that are specific to handling the :file:`dfu_application.zip` file format.
    The ZIP format is used for update images in the |NCS|.
    The change simplifies integrating new update image file formats.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``11ecbf639d826c084973beed709a63d51d9b684e``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * The ``CONFIG_XIP_SPLIT_IMAGE`` Kconfig option that enables build system support for relocating part of the application to external memory using hardware QSPI XIP feature and MCUboot third image on nRF5340 SoC.
  * MCUboot procedure that performs clean up of content in all the secondary slots, which contain valid header but cannot be assigned to any of supported primary images.
    This behavior is desired when the configuration (``CONFIG_MCUBOOT_CLEANUP_UNUSABLE_SECONDARY``) allows to use one secondary slot for collecting image for multiple primary slots.

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

zcbor
=====

zcbor has been updated from 0.7.0 to 0.8.1.
For more information see the `zcbor 0.8.0 release notes`_ and the `zcbor 0.8.1 release notes`_.

* Added:

  * Support for unordered maps.
  * Performance improvements.
  * Naming improvements for generated code.
  * Bugfixes.

Trusted Firmware-M
==================

* Updated:

  * The minimal TF-M build profile no longer silences TF-M logs by default.

    .. note::
       This can be a breaking change if the UART instance used by TF-M is already in use, for example, by modem trace with a UART backend.

  * The TF-M version is updated to TF-M v2.0.0.

Mbed TLS
========

* Updated the Mbed TLS version to v3.5.2.


Documentation
=============

* Added:

  * :ref:`contributions_ncs` page in a new :ref:`contributions` section that also includes the development model pages, previously listed under :ref:`releases_and_maturity`.
  * :ref:`ug_lte` user guide under :ref:`protocols`.
  * Gazell and NFC sections in the :ref:`app_power_opt_recommendations` user guide.
  * Following new pages under :ref:`ug_wifi`:

    * :ref:`ug_nrf70_developing_debugging`
    * :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission`
    * :ref:`nRF70_soft_ap_mode`
    * :ref:`ug_wifi_mem_req_softap_mode`
    * :ref:`ug_nrf70_developing_raw_ieee_80211_packet_reception`
    * :ref:`ug_nrf70_developing_promiscuous_packet_reception`

  * :ref:`lib_security` where libraries :ref:`trusted_storage_readme` and :ref:`nrf_security` are added.

* Updated:

  * The :ref:`installation` section by replacing two separate pages about installing the |NCS| with just one (:ref:`install_ncs`).
  * The :ref:`requirements` page with new sections about :ref:`requirements_clt` and :ref:`toolchain_management_tools`.
  * The :ref:`configuration_and_build` section and added structural changes:

    * :ref:`app_build_system` gathers conceptual information about the build and configuration system previously listed on several other pages.
      The :ref:`app_build_additions` section on this page now provides more information about :ref:`app_build_additions_build_types` specific to the |NCS|.
    * :ref:`app_boards` is now a section and its contents have been moved to several subpages.
    * New :ref:`configuring_devicetree` subsection now groups guides related to configuration of hardware using the devicetree language.
    * Removed the page Modifying an application and distributed the contents into the following new pages:

      * :ref:`configuring_cmake`
      * :ref:`configuring_kconfig`
    * Added a new page :ref:`building_advanced`.
    * New reference page :ref:`app_build_output_files` gathers information previously listed on several other pages.
    * :ref:`app_dfu` and :ref:`app_bootloaders` are now separate sections, with the DFU section summarizing the available DFU methods in a table.

  * The :ref:`test_and_optimize` section by separating information about debugging into its own :ref:`gs_debugging` page.
    The basic information about the default serial port settings and the different connection methods and terminals is now on the main :ref:`test_and_optimize` page.

  * Integration steps in the :ref:`ug_bt_fast_pair` guide.
    Reorganized extension-specific content into dedicated subsections.
  * The :ref:`dev-model` section with the :ref:`documentation` pages, previously listed separately.
  * The :ref:`glossary` page by moving it back to the main hierarchy level.
  * The :ref:`ug_wifi` page:

    * Moved :ref:`ug_nrf70_developing_powersave`, :ref:`ug_nrf70_developing_regulatory_support`, and :ref:`ug_nrf70_developing_scan_operation` pages, which were previously listed under :ref:`ug_nrf70_developing`.
    * The Memory requirements for Wi-Fi applications page is split into two sections :ref:`ug_wifi_mem_req_sta_mode` and :ref:`ug_wifi_mem_req_scan_mode`, based on different supported modes.
    * The :ref:`ug_nrf70_powersave` user guide by separating information about power profiling into its own :ref:`ug_nrf70_developing_power_profiling` page under :ref:`ug_nrf70_developing`.
      Additionally, updated the :ref:`ug_nrf70_developing_powersave_power_save_mode` section.
  * The :ref:`security` page with information about the trusted storage.

* Removed the Welcome to the |NCS| page.
  This page is replaced with existing :ref:`ncs_introduction` page.
