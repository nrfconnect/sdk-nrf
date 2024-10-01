.. _ncs_release_notes_270:

|NCS| v2.7.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 3

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

This release introduces significant, potentially breaking, changes to the SDK:

* The `previous method to define a board (hardware model)`_ is deprecated and being replaced by :ref:`a new method (hardware model v2) <zephyr:board_porting_guide>`.
* The previous method to define :ref:`multi-image builds (parent-child images) <ug_multi_image>` is deprecated and being replaced by :ref:`zephyr:sysbuild`.

All samples and applications in the SDK have been migrated.
Consult respective documentation as additional or changed parameters might be needed to build them successfully.
Applications that are outside of the SDK and use custom-defined boards should not be affected by these changes.
However, modifications might still be required as per the migration procedure described in `Migration guide for nRF Connect SDK v2.7.0`_.
nRF Connect for VS Code users migrating to the latest version of the SDK might be affected.

All samples and applications in the SDK are built with sysbuild by default.
Applications that are outside the SDK are not built with sysbuild by default.

The deprecated methods are scheduled for removal after the next release.
We recommend transitioning to the alternatives as soon as possible.
Consult migration guides for `Migrating to the current hardware model`_ and `Migrating from multi-image builds to sysbuild`_.
Exercise caution when migrating production environments to the latest SDK.

Added the following features as supported:

* Matter:

  * Matter 1.3, which enables energy reporting for Matter devices and support for water and energy management, electric vehicle charges, and new major appliances.
    Learn more about Matter 1.3 in the `Matter 1.3 CSA blog post`_.

* Wi-Fi®:

  * Wi-Fi and Thread coexistence.
  * Software-enabled Access Point (SoftAP) for provisioning that was introduced in |NCS| v2.6.0 as experimental is now supported.
  * Raw Wi-Fi reception in both Monitor and Promiscuous modes that was introduced in |NCS| v2.6.0 as experimental is now supported.
  * New samples: :ref:`softap_wifi_provision_sample`, :ref:`wifi_promiscuous_sample`, and :ref:`wifi_thread_coex_sample`.

* Other:

  * Google Find My Device that allows the creation of a locator device that is compatible with the Android `Find My Device app`_.
    This feature is showcased in the :ref:`fast_pair_locator_tag` sample.
  * Hardware model v2 (HWMv2), an improved extensible system for defining boards.
    This is the default boards definition system from this |NCS| release and onwards.
    See `Migrating to the current hardware model`_.
  * :ref:`zephyr:sysbuild`, an improved and extensible system for multi-image build, replacing :ref:`ug_multi_image` (parent/child images).
    See `Migrating from multi-image builds to sysbuild`_.
  * Samples and applications that use short-range radio and run on multi-core SoCs were migrated to use the :ref:`ipc_radio` as the default image for the network/radio core.
    Samples previously used for the network/radio core are no longer used in the default builds: :zephyr:code-sample:`bluetooth_hci_ipc`, :zephyr:code-sample:`nrf_ieee802154_rpmsg`, :ref:`multiprotocol-rpmsg-sample`, and :ref:`ble_rpc_host`.

Added the following features as experimental:

* Support for |NCS| v2.7.0 in the |nRFVSC| is experimental.
  The extension users that need v2.7.0 should `switch to the pre-release version of the extension`_.
  See the `IDE and tool support`_ section for more information.

* nRF54H and nRF54L Series:

  * Experimental support for next-generation devices.
    Hardware access is restricted to the participants in the limited sampling program.
    For more information, `contact our sales`_.

* Bluetooth® LE:

  * Path loss monitoring in the SoftDevice controller, which is part of LE Power Control.
    This feature does not yet have a host stack API, it requires access via HCI commands.

* Wi-Fi:

  * Random MAC address generation at boot time in the nRF Wi-Fi driver.
  * Support for the nRF70 device in CSP package.
  * Support for nRF54H with nRF70 in :ref:`wifi_shell_sample` and :ref:`wifi_scan_sample` samples.
  * Support for nRF54L with nRF70 in :ref:`wifi_shell_sample` and :ref:`wifi_scan_sample` samples.

* DECT NR+:

  * DECT NR+ PHY API which is showcased in the :ref:`nrf_modem_dect_phy_hello` sample.
    To get access to the DECT NR+ PHY modem firmware, please `contact our sales`_.

* nRF Cloud:

  * The nRF Cloud CoAP library now uses the SO_KEEPOPEN socket option available on modem firmware 2.0.1 and above.
    This option keeps the UDP socket open during temporary network outages, avoiding unnecessary handshakes and saving power and data.

Sign up for the `nRF Connect SDK v2.7.0 webinar`_ to learn more about the new features.

See :ref:`ncs_release_notes_270_changelog` for the complete list of changes.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.7.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.7.0`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.7.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9160 DK`_
* `Modem firmware compatibility matrix for the nRF9161 DK`_

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
     - `Changelog <Modem library changelog for v2.7.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.7.0_>`_

Known issues
************

For the list of issues valid for this release, navigate to `known issues page on the main branch`_ and select ``v2.7.0`` from the dropdown list.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v2.7.0`_ for the changes required or recommended when migrating your application from |NCS| v2.6.0 to |NCS| v2.7.0.

.. _ncs_release_notes_270_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Build and configuration system
==============================

* Added:

  * Documentation page about :ref:`companion components <companion_components>`, which are independent from the application and are included as separate firmware images.
  * Documentation section about the :ref:`file suffix feature from Zephyr <app_build_file_suffixes>` with related information in the :ref:`migration guide <migration_2.7_recommended>`.
  * Documentation section about :ref:`app_build_snippets`.
  * Documentation sections about :ref:`configuration_system_overview_sysbuild` and :ref:`sysbuild_enabled_ncs`.

* Updated:

  * All board targets for Zephyr's :ref:`Hardware Model v2 <zephyr:hw_model_v2>`, with additional information added on the :ref:`app_boards_names` page.
  * The use of :ref:`cmake_options` to specify the image when building with :ref:`configuration_system_overview_sysbuild`.
    If not specified, the option will be added to all images.
  * The format version of the :file:`dfu_application.zip` files generated by the |NCS| build system.

    Introducing Zephyr's :ref:`Hardware Model v2 <zephyr:hw_model_v2>` and building with :ref:`configuration_system_overview_sysbuild` for the |NCS| v2.7.0 release modified the content of the generated zip file.
    For example, binary file names and the board field were updated.

    To indicate the change in the zip format, the ``format-version`` property in the :file:`manifest.json` file included in the mentioned zip archive was updated from ``0`` to ``1``.

    Make sure that the used DFU host tools support the :file:`dfu_application.zip` file with the new format version (``1``).
    If the used DFU host tools do not support the new format version and you cannot update them, you can manually align the content of the zip archive generated with format version ``1`` to format version ``0``.

    Detailed steps are described in :ref:`migration_2.7`.

Working with nRF91 Series
=========================

* Updated:

  * The name and the structure of the section, with :ref:`ug_nrf91` as the landing page.
  * The :ref:`ug_nrf9160_gs` and :ref:`ug_thingy91_gsg` pages have been moved to the :ref:`gsg_guides` section.

Working with nRF70 Series
=========================

* Updated:

  * The name and the structure of the section, with :ref:`ug_nrf70` as the landing page.
  * The :ref:`ug_nrf7002_gs` guide has been moved to the :ref:`gsg_guides` section.

Working with nRF54H Series
==========================

* Added the :ref:`ug_nrf54h` section.

Working with nRF54L Series
==========================

* Added the :ref:`ug_nrf54l` section.
* Updated the default value for the Kconfig option :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` from ``500`` to ``250`` if :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is used.

Working with nRF53 Series
=========================

* Added the :ref:`features_nrf53` page.

* Updated:

  * The name and the structure of the section, with :ref:`ug_nrf53` as the landing page.
  * The default value for the Kconfig option :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` from ``500`` to ``250`` if :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC` is used.
  * The nrfjprog commands in :ref:`ug_nrf5340` with commands from `nRF Util`_.
  * The :ref:`ug_nrf5340_gs` guide has been moved to the :ref:`gsg_guides` section.

Working with nRF52 Series
=========================

* Updated:

  * The name and the structure of the section, with :ref:`ug_nrf52` as the landing page.
  * The :ref:`ug_nrf52_gs` guide has been moved to the :ref:`gsg_guides` section.

Working with PMIC
=================

* Updated the name and the structure of the section, with :ref:`ug_pmic` as the landing page.

Working with RF front-end modules
=================================

* Updated the name and the structure of the section, with :ref:`ug_radio_fem` as the landing page.

Security
========

* Added information about the default Kconfig option setting for :ref:`enabling access port protection mechanism in the nRF Connect SDK <app_approtect_ncs>`.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

* Added:

  * Experimental support for the Bluetooth LE and sub-GHz protocols on the :ref:`zephyr:nrf54l15pdk_nrf54l15`.
  * Support for the DFU FUOTA.

* Updated:

  * Moved the LED configuration for the state notifier to DTS.
  * Improved the responsiveness of the SID Device Under Test (DUT) shell.

* Removed the support for child image builds (only sysbuild is supported).

Bluetooth Mesh
--------------

* Updated:

  * The Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` to no longer depend on the Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA`.
  * The Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_CLI` to no longer enable the Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` by default.
    The Kconfig option :kconfig:option:`CONFIG_BT_MESH_DFU_METADATA_ON_BUILD` can still be manually enabled.
  * The JSON file, added to :file:`dfu_application.zip` during the automatic DFU metadata generation, to now contain a field for the ``core_type`` used when encoding the metadata.

DECT NR+
--------

* Added a new :ref:`ug_dect` user guide under :ref:`protocols`.

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
  * The number of available packet buffers in the Matter stack has been increased to avoid packet allocation issues.
  * The :ref:`ug_matter_gs_adding_cluster` user guide to use west commands.

* Removed the :file:`Kconfig.mcuboot.defaults`, :file:`Kconfig.hci_ipc.defaults`, and :file:`Kconfig.multiprotocol_rpmsg.defaults` Kconfig files that stored the default configuration for the child images.
  This was done because of the :ref:`configuration_system_overview_sysbuild` integration and the child images deprecation.
  The configurations are now applied using the configuration files located in the sample's or application's directory.
  For information on how to migrate an application from the previous to the current approach, see the :ref:`migration guide <migration_2.7>`.

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

* Updated the network commissioning to provide more information related to the used networking technologies.
  For Wi-Fi devices, they can now report which Wi-Fi band they support and they have to perform Wi-Fi directed scanning.
  For Thread devices, the Network Commissioning cluster now includes Thread version and supported Thread features attributes.

Thread
------

* Added:

  * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` in the :ref:`ot_cli_sample` and :ref:`ot_coprocessor_sample` samples.
  * New :ref:`feature set <thread_ug_feature_sets>` option :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_RCP`.

Zigbee
------

* Updated:

  * :ref:`nrfxlib:zboss` to v3.11.4.0 and platform v5.1.5 (``v3.11.4.0+5.1.5``).
    They contain fixes for infinite boot loop due to ZBOSS NVRAM corruption and other bugs.
    For details, see :ref:`zboss_changelog`.
  * :ref:`ZBOSS Network Co-processor Host <ug_zigbee_tools_ncp_host>` package to the new version v2.2.3.

* Fixed:

  * An issue with Zigbee FOTA updates failing after a previous attempt was interrupted.
  * The RSSI level value reported to the MAC layer in the Zigbee stack.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and the :ref:`zephyr:nrf54l15pdk_nrf54l15`.
  * Fast switching between radio states for the nRF54H20 SoC.
  * Fast radio channel switching for the nRF54H20 SoC.

Wi-Fi
-----

* Added:

  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`zephyr:nrf54l15pdk_nrf54l15` boards with :ref:`nRF7002 EB <ug_nrf7002eb_gs>`.
  * General enhancements in low-power mode including watchdog based recovery.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Applications that used :zephyr:code-sample:`bluetooth_hci_ipc`, :zephyr:code-sample:`nrf_ieee802154_rpmsg`, or :ref:`multiprotocol-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.

Asset Tracker v2
----------------

* Added support for Thingy:91 X.

* Updated:

  * The MQTT topic name for A-GNSS requests is changed to ``agnss`` for AWS and Azure backends.
  * GNSS heading is only sent to the cloud when it is considered accurate enough.

Serial LTE modem
----------------

* Added:

  * Support for Thingy:91 X.
  * New behavior for when a connection is closed unexpectedly while SLM is in data mode.
    SLM now sends the :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` string when this happens.
  * Sending of GNSS data to carrier library when the library is enabled.
  * New :kconfig:option:`CONFIG_SLM_CARRIER_AUTO_STARTUP` Kconfig option to enable automatic startup of the carrier library on device boot.
  * New custom carrier library commands: ``AT#XCARRIER="app_data_create"``, ``AT#XCARRIER="dereg"``, ``AT#XCARRIER="regup"`` and ``AT#XCARRIERCFG="auto_register"``.

* Updated:

  * AT command parsing to utilize the :ref:`at_cmd_custom_readme` library.
  * The ``AT#XCARRIER="app_data_set"`` and ``AT#XCARRIER="log_data"`` commands to accept hexadecimal strings as input parameters.
  * The ``#XCARRIEREVT: 12`` unsolicited notification to indicate the type of event and to use hexadecimal data format.
  * The format of the ``#XCARRIEREVT: 7`` and ``#XCARRIEREVT: 20`` notifications.

* Removed:

  * Mention of Termite and Teraterm terminal emulators from the documentation.
    The recommended approach is to use one of the emulators listed on the :ref:`test_and_optimize` page.
  * Sending GNSS UI service info to nRF Cloud.
    This is no longer required by the cloud.
  * The ``AT#XCARRIERCFG="server_enable"`` command.

Connectivity Bridge
-------------------

* Added support for Thingy:91 X.
* Updated to make the Bluetooth LE feature work for Thingy:91 X by using the load switch.

nRF5340 Audio
-------------

* Added:

  * CAP initiator for the :ref:`unicast client <nrf53_audio_unicast_client_app>`, including Coordinated Set Identification Profile (CSIP).
  * Support for any context type, not just media.
  * Rejection of connection if :ref:`unicast client <nrf53_audio_unicast_client_app>` or :ref:`broadcast source <nrf53_audio_broadcast_source_app>` (or both) tries to use an unsupported sample rate.
  * Debug prints of discovered endpoints.
  * Support for multiple :ref:`unicast servers <nrf53_audio_unicast_server_app>` in :ref:`unicast client <nrf53_audio_unicast_client_app>`, regardless of location.

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
  * PAC record creation in :ref:`unicast server <nrf53_audio_unicast_server_app>` so that it does not expose source records if only the sink direction is supported.
  * Presentation delay calculation so that it is railed between min and max values of the :ref:`unicast server <nrf53_audio_unicast_server_app>`.

* Removed the LE Audio controller for nRF5340 library.
  The only supported controller for LE Audio is :ref:`ug_ble_controller_softdevice`.
  This enables use of standard tools for building, configuring, and DFU.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Added:

  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20`.
  * Support for :ref:`configuration_system_overview_sysbuild`.

* Updated:

  * The ``ml_runner`` application module to allow running a machine learning model without anomaly support.
  * The :ref:`application documentation <nrf_machine_learning_app>` by splitting it into several pages.

nRF Desktop
-----------

* Added:

  * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target.

    The PDK can act as a sample mouse or keyboard.
    It supports the Bluetooth LE HID data transport and uses SoftDevice Link Layer with Low Latency Packet Mode (LLPM) enabled.
    The PDK uses MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and supports firmware updates using the :ref:`nrf_desktop_dfu`.
  * Support for the nRF54H20 DK with the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

    The DK can act as a sample mouse.
    It supports the Bluetooth LE and USB HID data transports.
    The Bluetooth LE transport uses the SoftDevice Link Layer with Low Latency Packet Mode (LLPM) enabled.

    The USB transport uses new USB stack "USB-next" (:kconfig:option:`CONFIG_USB_DEVICE_STACK_NEXT`).
    It allows using USB High-Speed with HID report rate up to 8000 Hz.

    The nRF54H20 DK supports firmware update using Software Updates for Internet of Things (SUIT).
    The update image transfer can be handled either by the :ref:`nrf_desktop_dfu` or :ref:`nrf_desktop_smp`.
  * Integrated :ref:`zephyr:sysbuild`.
    The application defines the Sysbuild configuration file per board and build type.
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
  * Dependencies and documentation of the real-time QoS information printouts (:ref:`CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE <config_desktop_app_options>`) in :ref:`nrf_desktop_ble_qos`.
    The dependencies ensure that USB legacy stack integration in nRF Desktop is enabled (:ref:`CONFIG_DESKTOP_USB_STACK_LEGACY <config_desktop_app_options>`) and that selected USB CDC ACM instance is enabled and specified in DTS.
  * Enabled :kconfig:option:`CONFIG_BT_ATT_SENT_CB_AFTER_TX` Kconfig option in nRF Desktop HID peripheral configurations.
    The option enables a temporary solution allowing to control flow of GATT notifications with HID reports.

    This is needed to ensure low latency of provided HID data.
    The feature is not available in Zephyr RTOS (it is specific to the ``sdk-zephyr`` fork).
    Enabling this option might require increasing :kconfig:option:`CONFIG_BT_CONN_TX_MAX` in configuration, because ATT would use additional TX contexts.
    See Kconfig help for details.
  * Configurations for boards that use either nRF52810 or nRF52820 SoC enable the :kconfig:option:`CONFIG_BT_RECV_WORKQ_SYS` Kconfig option to reuse system workqueue for Bluetooth RX.
    This is needed to decrease RAM usage.

Thingy:53: Matter weather station
---------------------------------

* Updated:

  * The :ref:`diagnostic log module<ug_matter_configuration_diagnostic_logs>` is now enabled by default.
  * The :makevar:`OVERLAY_CONFIG` variable has been deprecated in the context of supporting factory data.
    See the :ref:`matter_weather_station_app_build_configuration_overlays` section for information on how to configure factory data support.

Matter Bridge
-------------

* Added:

  * The :ref:`CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES <CONFIG_BRIDGE_BT_MAX_SCANNED_DEVICES>` Kconfig option to set the maximum number of scanned Bluetooth LE devices.
  * The :ref:`CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS <CONFIG_BRIDGE_BT_SCAN_TIMEOUT_MS>` Kconfig option to set the scan timeout.

* Updated the implementation of the persistent storage to leverage ``NonSecure``-prefixed methods from the common Persistent Storage module.
* Changed data structure of information stored in the persistent storage to use fewer settings keys.
  The new structure uses approximately 40% of the memory used by the old structure, and provides a new field to store user-specific data.
  Backward compatibility is kept by using an internal dedicated method that automatically detects the older data format and performs data migration to the new representation.

IPC radio firmware
------------------

* Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

* Added:

  * New categories of samples: :ref:`dect_samples` and :ref:`suit_samples`.
  * Steps for nRF54 devices across all supported samples to reflect the new button and LED numbering on the nRF54H20 DK and the nRF54L15 PDK.

Bluetooth samples
-----------------

* Added:

  * The :ref:`bluetooth_iso_combined_bis_cis` sample showcasing forwarding isochronous data from CIS to BIS.
  * The :ref:`bluetooth_isochronous_time_synchronization` sample showcasing time-synchronized processing of isochronous data.
  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board in the following samples:

    * :ref:`central_bas` sample
    * :ref:`bluetooth_central_hr_coded` sample
    * :ref:`multiple_adv_sets` sample
    * :ref:`peripheral_ams_client` sample
    * :ref:`peripheral_ancs_client` sample
    * :ref:`peripheral_bms` sample
    * :ref:`peripheral_cgms` sample
    * :ref:`peripheral_cts_client` sample
    * :ref:`peripheral_gatt_dm` sample
    * :ref:`peripheral_hr_coded` sample
    * :ref:`peripheral_nfc_pairing` sample
    * :ref:`peripheral_rscs` sample
    * :ref:`peripheral_status` sample
    * :ref:`shell_bt_nus` sample

  * Support for both the :ref:`zephyr:nrf54h20dk_nrf54h20` and the :ref:`zephyr:nrf54l15pdk_nrf54l15` boards in the following samples:

    * :ref:`peripheral_lbs` sample
    * :ref:`bluetooth_central_hids` sample
    * :ref:`peripheral_hids_mouse` sample
    * :ref:`peripheral_hids_keyboard` sample
    * :ref:`central_and_peripheral_hrs` sample
    * :ref:`direct_test_mode` sample
    * :ref:`peripheral_uart` sample
    * :ref:`central_uart` sample

* Updated the Bluetooth samples that used the :zephyr:code-sample:`bluetooth_hci_ipc` radio core firmware so that now they use the :ref:`ipc_radio`.

* :ref:`direct_test_mode` sample:

  * Added support for :ref:`configuration_system_overview_sysbuild`.

Bluetooth Fast Pair samples
---------------------------

* Added :ref:`fast_pair_locator_tag` sample to demonstrate support for the locator tag use case and the Find My Device Network (FMDN) extension.
  The new sample supports the debug build configuration (the default option) and the release build configuration (available with the ``release`` file suffix).

* :ref:`fast_pair_input_device` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

  * Updated:

    * The sample is moved to the :ref:`bt_fast_pair_samples` category.
    * Replaced the :zephyr:code-sample:`bluetooth_hci_ipc` radio core firmware with the :ref:`ipc_radio`.

  * Removed unnecessary :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig configuration.

Bluetooth Mesh samples
----------------------

* Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board in the following samples:

  * :ref:`bluetooth_mesh_sensor_client` sample
  * :ref:`bluetooth_mesh_sensor_server` sample
  * :ref:`bluetooth_ble_peripheral_lbs_coex` sample
  * :ref:`bt_mesh_chat` sample
  * :ref:`bluetooth_mesh_light_switch` sample
  * :ref:`bluetooth_mesh_silvair_enocean` sample
  * :ref:`bluetooth_mesh_light_dim` sample
  * :ref:`bluetooth_mesh_light` sample
  * :ref:`ble_mesh_dfu_target` sample
  * :ref:`bluetooth_mesh_light_lc` sample
  * :ref:`ble_mesh_dfu_distributor` sample

* Updated the Bluetooth Mesh samples that used the :zephyr:code-sample:`bluetooth_hci_ipc` radio core firmware so that they now use the :ref:`ipc_radio`.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added motion sensing, time since motion sensed, and people count occupancy sensor types.

* :ref:`bluetooth_mesh_sensor_server` sample:

  * Added motion sensing, time since motion sensed, and people count occupancy sensor types.

  * Updated:

    * Actions of **Button 1** and **Button 2**.
      They are swapped to align with the elements order.
    * Log messages to be more informative.

* :ref:`bluetooth_mesh_light` sample:

  * Added support for DFU over Bluetooth Low Energy for the :ref:`nRF5340 DK <ug_nrf5340>` board.

* :ref:`ble_mesh_dfu_target` sample:

  * Added:

    * A note on how to compile the sample with new Composition Data.
    * Point-to-point DFU support with overlay file :file:`overlay-ptp_dfu.conf`.

* :ref:`bluetooth_mesh_light_lc` sample:

  * Added a section about the :ref:`occupancy mode <bluetooth_mesh_light_lc_occupancy_mode>`.

Cellular samples
----------------

* :ref:`at_client_sample` sample:

  * Added support for Thingy:91 X.

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

  * Added the :ref:`CONFIG_REST_CELL_SEND_DEVICE_STATUS <CONFIG_REST_CELL_SEND_DEVICE_STATUS>` Kconfig option to control sending device status on initial connection.

  * Removed the button press interface for enabling the device location card on the nRF Cloud website.
    The card is now automatically displayed.

* :ref:`modem_shell_application` sample:

  * Removed sending GNSS UI service info to nRF Cloud.
    This is no longer required by the cloud.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * The ability to control the state of the test counter using the config section in the device shadow.
    * Handling of L4 disconnect where CoAP connection is paused and the socket is kept open, then resumed when L4 reconnects.
    * Checking in CoAP FOTA and shadow polling threads to improve recovery from communication failures.
    * Sysbuild overlays for Wi-Fi and external-flash builds.

  * Fixed:

    * An issue that prevented network connectivity when using Wi-Fi scanning with the nRF91 Series devices.
    * An issue that caused the CoAP shadow polling thread to run too often if no data was received.

* :ref:`udp` sample:

  * Updated the sample to use the ``SO_RAI`` socket option with values ``RAI_LAST`` and ``RAI_ONGOING`` instead of the deprecated socket options ``SO_RAI_LAST`` and ``SO_RAI_ONGOING``.

Cryptography samples
--------------------

* Added:

    * :ref:`crypto_spake2p` sample.
    * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board for all crypto samples.
    * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board in all crypto samples, except :ref:`crypto_persistent_key` and :ref:`crypto_tls`.
    * Support for the :ref:`zephyr:nrf9151dk_nrf9151` board for all crypto samples.
    * Support for the :ref:`nRF9161 DK <ug_nrf9161>` board for the :ref:`crypto_test`.

Common samples
--------------

* Added a description about :file:`samples/common` and their purpose in the :ref:`samples` and :ref:`building_advanced` pages (:ref:`common_sample_components`).

Debug samples
-------------

* :ref:`memfault_sample` sample:

  * Added support for Thingy:91 X.

DECT NR+ samples
----------------

* Added the :ref:`nrf_modem_dect_phy_hello` sample.


Enhanced ShockBurst samples
---------------------------

* :ref:`esb_prx` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`zephyr:nrf54l15pdk_nrf54l15` boards.

Keys samples
------------

* Added support for the :ref:`zephyr:nrf9151dk_nrf9151` and the :ref:`zephyr:nrf9161dk_nrf9161` boards for all keys samples.

Matter samples
--------------

* Added:

  * Test event triggers to all Matter samples and enabled them by default.
    By utilizing the test event triggers, you can simulate various operational conditions and responses in your Matter device without the need for external setup.
    To get started with using test event triggers in your Matter samples and to understand the capabilities of this feature, refer to the :ref:`ug_matter_test_event_triggers` page.

  * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` with the ``nrf54l15pdk/nrf54l15/cpuapp`` board target to the following Matter samples:

    * :ref:`matter_template_sample` sample.
    * :ref:`matter_light_bulb_sample` sample.
    * :ref:`matter_light_switch_sample` sample.
    * :ref:`matter_thermostat_sample` sample.
    * :ref:`matter_window_covering_sample` sample.
    * :ref:`matter_lock_sample` sample.

    DFU over Matter OTA and Bluetooth LE SMP are supported in all samples but require an external flash.

  * Support for Matter over Thread on the :ref:`zephyr:nrf54h20dk_nrf54h20` with the ``nrf54h20dk/nrf54h20/cpuapp`` board target to the following Matter samples:

    * :ref:`matter_lock_sample` sample.
    * :ref:`matter_template_sample` sample.

    DFU, factory data, and PSA Crypto API are not currently supported for the nRF54H20 DK.

  * Default support of :ref:`ug_matter_device_watchdog` for ``release`` build type of all samples.
    The default watchdog timeout value is set to 10 minutes.
    The watchdog feeding interval time is set to 5 minutes.

* Updated:

  * Matter samples that used :zephyr:code-sample:`nrf_ieee802154_rpmsg` or :ref:`multiprotocol-rpmsg-sample` radio core firmware, now use the :ref:`ipc_radio`.
  * Enabled the Bluetooth LE Extended Announcement feature for all samples, and increased advertising timeout from 15 minutes to 1 hour.

* Removed:

  * The :file:`configuration` directory that contained the Partition Manager configuration file.
    It has been replaced with :file:`pm_static_<BOARD>` Partition Manager configuration files for all required target boards in the samples' directories.
  * The :file:`prj_no_dfu.conf` file.
  * Support for the ``no_dfu`` build type for the nRF5340 DK, the nRF52840 DK, and the nRF7002 DK.

* :ref:`matter_lock_sample` sample:

  * Added:

    * Support for emulation of the nRF7001 Wi-Fi companion IC on the nRF7002 DK.
    * Door lock access manager module.
      The module is used to implement support for refined handling and persistent storage of PIN codes.
    * The :ref:`matter_lock_scheduled_timed_access` feature.

* :ref:`matter_template_sample` sample:

  * Added experimental support for DFU using the internal ROM only.
    This option support requires the :file:`pm_static_nrf54l15pdk_nrf54l15_cpuapp_internal.yml` pm static file and currently works with the ``release`` build type only.

Multicore samples
-----------------

* Removed the "Multicore Hello World application" sample in favor of :zephyr:code-sample:`sysbuild_hello_world` that has equivalent functionality.
  This also removes the Multicore samples category from the :ref:`samples` page.

Networking samples
------------------

* Updated the networking samples to support import of certificates in valid PEM formats.
* Removed QEMU x86 emulation support and added support for the :ref:`native simulator <zephyr:native_sim>` board.

* :ref:`mqtt_sample` sample:

  * Added support for Thingy:91 X.

* :ref:`http_server` sample:

  * Added ``DNS_SD_REGISTER_TCP_SERVICE`` so that mDNS services can locate and address the server using its host name.

  * Updated:

    * The value of the :kconfig:option:`CONFIG_POSIX_MAX_FDS` Kconfig option is set to ``25`` to get the Transport Layer Security (TLS) working.
    * The default value of the :ref:`CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX <CONFIG_HTTP_SERVER_SAMPLE_CLIENTS_MAX>` Kconfig option is set to ``1``.

NFC samples
-----------

* Added:

  * Support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board in the :ref:`nrf-nfc-system-off-sample` sample.
  * Support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`zephyr:nrf54l15pdk_nrf54l15` boards in the following samples:

    * :ref:`record_launch_app` sample
    * :ref:`record_text` sample
    * :ref:`nfc_shell` sample
    * :ref:`nfc_tnep_tag` sample
    * :ref:`writable_ndef_msg` sample

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` and :ref:`zephyr:nrf54l15pdk_nrf54l15` boards.

  * The CLI command ``fem tx_power_control <tx_power_control>`` replaces ``fem tx_gain <tx_gain>`` .
    This change applies to the sample built with the :ref:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC <CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC>` set to ``n``.

* :ref:`802154_phy_test` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`802154_sniffer` sample:

  * The sample no longer exposes two USB CDC ACM endpoints on the nRF52840 Dongle.

SUIT samples
------------

* Added experimental support using the Software Updates for Internet of Things (SUIT):

   * :ref:`nrf54h_suit_sample` sample - For DFUs using SMP over Bluetooth LE and UART.

   * :ref:`suit_flash_companion` sample - For enabling access to external flash memory in DFUs.

   * :ref:`suit_recovery` sample - For recovering the device firmware if the original firmware is damaged.

Trusted Firmware-M (TF-M) samples
---------------------------------

* Added support for the :ref:`zephyr:nrf9151dk_nrf9151` and the :ref:`zephyr:nrf9161dk_nrf9161` boards for all TF-M samples, except for the :ref:`provisioning_image_net_core` sample.

Thread samples
--------------

* Added new :ref:`feature set <thread_ug_feature_sets>` Kconfig option :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_RCP`.

* Updated the Thread samples that used :zephyr:code-sample:`nrf_ieee802154_rpmsg` or :ref:`multiprotocol-rpmsg-sample` radio core firmware, so that they now use the :ref:`ipc_radio`.

* :ref:`ot_cli_sample` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.

* :ref:`ot_coprocessor_sample` sample:

  * Added support for the :ref:`zephyr:nrf54l15pdk_nrf54l15` board.
  * Changed the default :ref:`feature set <thread_ug_feature_sets>` from Master to RCP.

Zigbee samples
--------------

* Updated the Zigbee samples that used :zephyr:code-sample:`nrf_ieee802154_rpmsg` or :ref:`multiprotocol-rpmsg-sample` radio core firmware, so that they now use the :ref:`ipc_radio`.

Wi-Fi samples
-------------

* Added:

  * The :ref:`softap_wifi_provision_sample` sample that demonstrates how to use the :ref:`lib_softap_wifi_provision` library to provision credentials to a Nordic Semiconductor Wi-Fi device.
  * The :ref:`wifi_thread_coex_sample` sample that demonstrates Wi-Fi and Thread coexistence.
  * The :ref:`wifi_promiscuous_sample` sample that demonstrates how to set Promiscuous mode, establish a connection to an Access Point (AP), analyze incoming Wi-Fi packets, and print packet statistics.

* Updated the Wi-Fi samples that used :zephyr:code-sample:`bluetooth_hci_ipc` or :zephyr:code-sample:`nrf_ieee802154_rpmsg` radio core firmware, so that they now use the :ref:`ipc_radio`.

* :ref:`wifi_softap_sample` sample:

  * Updated to use the :ref:`lib_wifi_ready` library to manage the Wi-Fi use.

* :ref:`wifi_shell_sample` sample:

  * Added:

    * ``Auto-Security-Personal`` mode to the ``connect`` command.
    * Support for the ``WPA-PSK`` security mode to the ``wifi_mgmt_ext`` library.

  * Updated the ``connect`` command to provide better control over connection parameters.

* :ref:`wifi_station_sample` sample:

  * Updated to use the :ref:`lib_wifi_ready` library to manage the Wi-Fi use.

Other samples
-------------

* Added the :ref:`coremark_sample` sample that demonstrates how to easily measure a performance of the supported SoCs by running the Embedded Microprocessor Benchmark Consortium (EEMBC) CoreMark benchmark.

* :ref:`bootloader` sample:

  * Added support for the :ref:`zephyr:nrf9151dk_nrf9151` and the :ref:`zephyr:nrf9161dk_nrf9161` boards.
  * Updated the key revocation handling process to remove a security weakness found in the previous design.
    It is recommended to switch to the improved revocation handling in the newly manufactured devices.

* :ref:`ipc_service_sample` sample:

  * Removed support for the `OpenAMP`_ library backend on the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* Added support for random MAC address generation at boot using the :kconfig:option:`CONFIG_WIFI_RANDOM_MAC_ADDRESS` Kconfig option.
* Removed support for setting RTS threshold through the ``wifi_util`` command.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v3.5.1.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

.. _lib_bt_ll_acs_nrf53_readme:

* Removed the LE Audio controller for nRF5340 library, which was deprecated in the :ref:`v2.6.0 release <ncs_release_notes_260>`.
  As mentioned in the :ref:`migration_2.6`, make sure to transition to Nordic Semiconductor's standard :ref:`ug_ble_controller_softdevice` (:ref:`softdevice_controller_iso`).

Bluetooth libraries and services
--------------------------------

* :ref:`bt_mesh` library:

  * Updated:

    * The :ref:`bt_mesh_light_ctrl_srv_readme` model documentation to explicitly mention the Occupancy On event.
    * The :ref:`bt_mesh_light_ctrl_srv_readme` to instantly change the current or the target lightness level if the state machine is in or transitioning to one of the Run, Prolong, or Standby states when receiving a Light LC Property Set message (as a result of calling the function :c:func:`bt_mesh_light_ctrl_cli_prop_set`), modifying the relevant state's level.

* :ref:`ble_rpc` library:

  * Reworked the :ref:`ble_rpc` page to be more informative and aligned with the library template.

* :ref:`bt_enocean_readme` library:

  * Fixed an issue where the sensor data of a certain length was incorrectly parsed as switch commissioning.

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * Support for the FMDN extension (:kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN`).
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_AK_BACKEND` Kconfig option to select the Account Key storage backend and introduced the new minimal backend for Account Key storage.
      The new backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_AK_BACKEND_MINIMAL`) is dedicated for the locator tag use case and supports the Owner Account Key functionality required by the FMDN extension.
      The old backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_AK_BACKEND_STANDARD`) is used in the input device use case.
    * The Beacon Clock storage unit used with the FMDN extension.
    * The Ephemeral Identity Key (EIK) storage unit used with the FMDN extension.
    * Support for AES-ECB-256, SECP160R1, and SECP256R1 cryptographic operations to the Oberon crypto backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON`) that are used in the FMDN extension.
    * Experimental support for a new cryptographical backend that relies on the PSA crypto APIs (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_REQ_PAIRING` Kconfig option to provide the possibility to skip the Bluetooth pairing and bonding phase during the Fast Pair procedure.
      This option must be disabled in the Fast Pair locator tag use case due to its requirements.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID` Kconfig option to provide the possibility to remove the Model ID characteristic from the Fast Pair service.
      This option must be disabled in the Fast Pair locator tag use case due to its requirements.
    * A UUID definition of the FMDN Beacon Actions characteristic to the Fast Pair UUID header (:file:`bluetooth/services/fast_pair/uuid.h`).
    * The experimental status (:kconfig:option:`CONFIG_EXPERIMENTAL`) to the :kconfig:option:`CONFIG_BT_FAST_PAIR_PN` Kconfig option, which is used to enable Fast Pair Personalized Name extension.

  * Removed the experimental status (:kconfig:option:`CONFIG_EXPERIMENTAL`) from the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option, which is the main Fast Pair configuration option.

Debug libraries
---------------

* :ref:`mod_memfault` library:

  * Fixed an issue where the library resets the LTE connectivity statistics after each read.
    This could lead to an accumulated error over time because of the byte counter resolution.
    The connectivity statistics are now only reset when the library is initialized and will be cumulative after that.

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_CHUNK_SZ` Kconfig option to process traces in chunks.
      This can improve the availability of trace memory, and thus reduce the chances of losing traces.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_LTE_NET_IF` Kconfig option for sending modem faults to the :ref:`nrf_modem_lib_lte_net_if` when it is enabled.
    * The :kconfig:option:`CONFIG_NRF_MODEM_LIB_FAULT_THREAD_STACK_SIZE` Kconfig option to allow the application to set the modem fault thread stack size.

  * Fixed an issue with the CFUN hooks when the Modem library is initialized during ``SYS_INIT`` at kernel level and makes calls to the :ref:`nrf_modem_at` before the application level initialization is done.

  * Deprecated the Kconfig option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ZEPHYR`.

  * Removed the deprecated Kconfig options ``CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ASYNC`` and ``CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_SYNC``.

* :ref:`nrf_modem_lib_lte_net_if` library:

  * Added the dependency to the :kconfig:option:`CONFIG_NET_CONNECTION_MANAGER` Kconfig option.

  * Updated the required stack size of the :kconfig:option:`CONFIG_NET_CONNECTION_MANAGER_MONITOR_STACK_SIZE` Kconfig option by increasing it to ``1024``.

  * Removed the requirement of IPv4 and IPv6 support for all applications.
    IPv4 and IPv6 support can be enabled using the :kconfig:option:`CONFIG_NET_IPV4` and :kconfig:option:`CONFIG_NET_IPV6` Kconfig options, respectively.


* :ref:`lib_location` library:

  * Added:

    * Convenience function to get :c:struct:`location_data_details` from the :c:struct:`location_event_data`.
    * Location data details for event :c:enum:`LOCATION_EVT_RESULT_UNKNOWN`.
    * Sending GNSS coordinates to nRF Cloud when the :kconfig:option:`CONFIG_LOCATION_SERVICE_NRF_CLOUD_GNSS_POS_SEND` Kconfig option is set.

* :ref:`pdn_readme` library:

  * Updated the ``dns4_pri``, ``dns4_sec``, and ``ipv4_mtu`` parameters of the :c:func:`pdn_dynamic_params_get` function to be optional.
    If the MTU is not reported by the SIM card, the ``ipv4_mtu`` parameter is set to zero.

* :ref:`lte_lc_readme` library:

  * Added the :kconfig:option:`CONFIG_LTE_PLMN_SELECTION_OPTIMIZATION` to enable PLMN selection optimization on network search.
    This option is enabled by default.
    For more information, see the ``AT%FEACONF`` command documentation in the `nRF91x1 AT Commands Reference Guide`_.

  * Removed ``AT%XRAI`` related deprecated functions ``lte_lc_rai_param_set()`` and ``lte_lc_rai_req()``, and Kconfig option ``CONFIG_LTE_RAI_REQ_VALUE``.
    The application uses the Kconfig option :kconfig:option:`CONFIG_LTE_RAI_REQ` and ``SO_RAI`` socket option instead.

Libraries for networking
------------------------

* Added:

   * The :ref:`lib_softap_wifi_provision` library.
   * The :ref:`lib_wifi_ready` library.

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
    * The :c:func:`nrf_cloud_coap_shadow_get` function to include a parameter to specify the content format of the returned payload.

  * Deprecated:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_SEND_SERVICE_INFO_UI` and its related UI Kconfig options.
    * The :c:struct:`nrf_cloud_svc_info_ui` structure contained in the :c:struct:`nrf_cloud_svc_info` structure.
      nRF Cloud no longer uses the UI section in the shadow.


* :ref:`lib_mqtt_helper` library:

  * Updated:

    * Changed the library to read certificates as standard PEM format.
      Previously, the certificates had to be manually converted to string format before compiling the application.
    * Replaced the ``CONFIG_MQTT_HELPER_CERTIFICATES_FILE`` Kconfig option with :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`.
      The new option specifies the folder where the certificates are stored.

* :ref:`lib_nrf_provisioning` library:

  * Added the :c:func:`nrf_provisioning_set_interval` function to set the interval between provisioning attempts.

* :ref:`lib_nrf_cloud_coap` library:

  * Added:

    * The :c:func:`nrf_cloud_coap_shadow_desired_update` function to allow devices to reject invalid shadow deltas.
    * Support for IPv6 connections.
    * The ``SO_KEEPOPEN`` socket option to keep the socket open even during PDN disconnect and reconnect.
    * The experimental Kconfig option :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` that enables downloading FOTA and P-GPS data using CoAP instead of HTTP.

  * Updated to request proprietary PSM mode for ``SOC_NRF9151_LACA`` and ``SOC_NRF9131_LACA`` in addition to ``SOC_NRF9161_LACA``.

  * Removed the ``CONFIG_NRF_CLOUD_COAP_GF_CONF`` Kconfig option.

* :ref:`lib_lwm2m_client_utils` library:

  * Added support for the ``SO_KEEPOPEN`` socket option to keep the socket open even during PDN disconnect and reconnect.

  * Fixed an issue where the Location Area Code was not updated when the Connection Monitor object version 1.3 was enabled.

  * Deprecated:

    * The following initialization functions as these modules are now initialized automatically on boot:

      * :c:func:`lwm2m_init_location`
      * :c:func:`lwm2m_init_device`
      * :c:func:`lwm2m_init_cellular_connectivity_object`
      * :c:func:`lwm2m_init_connmon`

    * :c:func:`lwm2m_init_firmware` is deprecated in favour of :c:func:`lwm2m_init_firmware_cb` that allows application to set a callback to receive FOTA events.

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed a NULL pointer issue that could occur when there were some valid predictions in flash but not the one required at the current time.

* :ref:`lib_download_client` library:

  * Removed the deprecated ``download_client_connect`` function.

* :ref:`lib_fota_download` library:

  * Added:

    * The function :c:func:`fota_download_b1_file_parse` to parse a bootloader update file path.
    * Experimental support for performing FOTA updates using an external download client with the Kconfig option :kconfig:option:`CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL` and functions :c:func:`fota_download_external_start` and :c:func:`fota_download_external_evt_handle`.

Security libraries
------------------

* :ref:`trusted_storage_readme` library:

  * Added the Kconfig option :kconfig:option:`CONFIG_TRUSTED_STORAGE_STORAGE_BACKEND_CUSTOM` that enables use of custom storage backend.

SUIT DFU
--------

* Added:

  * Experimental support for :ref:`SUIT DFU <ug_nrf54h20_suit_dfu>` for nRF54H20.
    SUIT is now the only way to boot local domains of the nRF54H20 SoC.
  * Experimental support for DFU from external flash for nRF54H20.
  * Experimental support for recovery image for application and radio cores for nRF54H20.

Other libraries
---------------

* Added:

  * The :ref:`dult_readme` library with experimental maturity level.
  * The :ref:`lib_uart_async_adapter` library.

* :ref:`app_event_manager` library:

  * Added the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_REBOOT_ON_EVENT_ALLOC_FAIL` Kconfig option.
    The option allows to select between system reboot or kernel panic on event allocation failure for default event allocator.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* Added:

  * The :file:`thingy91x_dfu.py` script in the :file:`scripts/west_commands` folder.
    The script adds the west commands ``west thingy91x-dfu`` and ``west thingy91x-reset`` for convenient use of the serial recovery functionality.
  * Support for format version ``1`` of the :file:`dfu_application.zip` file to :ref:`nrf_desktop_config_channel_script` Python script.
    From |NCS| v2.7.0 onwards, the script supports both format version ``0`` and ``1`` of the zip archive.
    Older versions of the script do not support format version ``1`` of the zip archive.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Updated the :ref:`ug_bt_fast_pair` guide to document integration steps for the FMDN extension, the locator tag use case, and aligned the page with the sysbuild migration.

CoreMark integration
--------------------

* Added the :ref:`ug_coremark` page.

DULT integration
----------------

* Added the :ref:`ug_dult` guide.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Skipping of the MCUboot downgrade prevention for updating the *S0*/*S1* image (MCUboot self update).

    This applies to both the software semantic version based and security counter based downgrade prevention.
    Thanks to this change, the MCUboot's downgrade prevention does not prevent updating of the *S0*/*S1* image while using a swap algorithm.
    *S0*/*S1* image version management is under the control of the :ref:`bootloader` anyway.

 * Example DTS configuration for building MCUboot on the nRF54L15 PDK with external flash support.

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

Trusted Firmware-M
==================

* Added:

  * Support for PSA PAKE APIs from the PSA Crypto API specification 1.2.
  * Support for PBKDF2 algorithms as of |NCS| v2.6.0.

Documentation
=============

* Added:

  * :ref:`ug_dect` user guide under :ref:`protocols`.
  * The :ref:`ug_coremark` page.
  * The :ref:`dult_readme` page.
  * The :ref:`ug_dult` guide.
  * :ref:`Software Updates for Internet of Things (SUIT) documentation <ug_nrf54h20_suit_dfu>` in :ref:`ug_nrf54h`:

    The following conceptual guides:

      * :ref:`ug_nrf54h20_suit_intro` - An overview of the SUIT procedure.
      * :ref:`ug_nrf54h20_suit_manifest_overview` - An overview of the role and importance of the SUIT manifest.
      * :ref:`ug_nrf54h20_suit_components` - An explanation of SUIT components, found within the manifest(s).
      * :ref:`ug_nrf54h20_suit_hierarchical_manifests` - An explanation of the SUIT manifest topology that Nordic Semiconductor has implemented for the nRF54H20 SoC.
      * :ref:`ug_nrf54h20_suit_compare_other_dfu` - A comparison of SUIT with other bootloader and DFU procedures supported in the |NCS|.

    The following user guides:

      * :ref:`ug_nrf54h20_suit_customize_dfu` - Describing how to customize the SUIT DFU procedure (and a quick-start guide version :ref:`ug_nrf54h20_suit_customize_dfu_qsg`).
      * :ref:`ug_nrf54h20_suit_fetch` - Describing how to reconfigure an application that uses the push model to a fetch model-based upgrade.
      * :ref:`ug_nrf54h20_suit_external_memory` - Describing how to enable external flash memory when using SUIT.

  * :ref:`ug_wifi_mem_req_raw_mode` under :ref:`ug_wifi_advanced_mode`.
  * The :ref:`bt_mesh_models_common_blocking_api_rule` section to the :ref:`bt_mesh_models_overview` page.
  * Steps for nRF54 devices across all supported samples to reflect the new button and LED numbering on the nRF54H20 DK and the nRF54L15 PDK.
  * The :ref:`test_framework` section for gathering information about unit tests, with a new page about :ref:`running_unit_tests`.
  * List of :ref:`debugging_tools` on the :ref:`debugging` page.
  * Recommendation for the use of a :file:`VERSION` file for :ref:`ug_fw_update_image_versions_mcuboot` in the :ref:`ug_fw_update_image_versions` user guide.

* Updated:

  * The :ref:`cmake_options` section on the :ref:`configuring_cmake` page with the list of most common CMake options and more information about how to provide them.
  * The table listing the :ref:`boards included in sdk-zephyr <app_boards_names_zephyr>` with the nRF54L15 PDK and nRF54H20 DK boards.
  * The :ref:`ug_wifi_overview` page by separating the information about Wi-Fi certification into its own :ref:`ug_wifi_certification` page under :ref:`ug_wifi`.
  * The :ref:`ug_bt_mesh_configuring` page with an example of possible entries in the Settings NVS name cache.
  * The :ref:`lib_security` page to include all security-related libraries.
  * The trusted storage support table in the :ref:`trusted_storage_in_ncs` section by adding nRF52833 and replacing nRF9160 with nRF91 Series.
  * The :ref:`ug_nrf52_developing` and :ref:`ug_nrf5340` by adding notes about how to perform FOTA updates with samples using random HCI identities, some specifically relevant when using the iOS app.
  * Improved the :ref:`ug_radio_fem` user guide to be up-to-date and more informative.
  * The :ref:`bt_fast_pair_readme` page to document support for the FMDN extension and aligned the page with the sysbuild migration.
  * The :ref:`ug_bt_fast_pair` guide to document integration steps for the FMDN extension, the locator tag use case, and aligned the page with the sysbuild migration.
  * The :ref:`software_maturity` page to add the new Ecosystem category.
    In the new category, defined software maturity levels for Google Fast Pair use cases and features supported in the |NCS|.

* Fixed:

  * Replaced the occurrences of the outdated :makevar:`OVERLAY_CONFIG` with the currently used :makevar:`EXTRA_CONF_FILE`.
