.. _ncs_release_notes_290-nrf54h20-rc1:

|NCS| v2.9.0-nRF54H20-rc1 Release Notes
#######################################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

The |NCS| v2.9.0-nrf54h20-rc1 is an nRF54H20-exclusive release tag, only supporting products based on the nRF54H20 SoC.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

Added the following features as supported:

* nRF54H20 Series:

  * Triggering firmware recovery by pressing a hardware button or calling the respective API.
    For more information, see :ref:`ug_nrf54h20_suit_recovery`.
  * Global Domain Frequency Scaling (GDFS), optimizing USB suspend power consumption.
  * Drivers adapted for the following peripherals:

    * UARTE120
    * PWM120
    * SPIM120
    * EXMIF

  * UICR validation support.
  * MSPI EXMIF support.

Added the following features as experimental:

* Bluetooth®:

  * :ref:`Parallel scanning and initiating connection <bt_scanning_while_connecting>`.
    It reduces the overall waiting time when there are several Bluetooth devices to be discovered and connected at the same time.

Improved the following features:

* Better BICR handling and generation.
* Enhanced power management features across various subsystems.

Implemented the following bug fixes:

* NCSDK-30802: Resolved an issue where the nRF54H20 device suddenly stopped transmitting ESB packets after nrfxlib 3.9.0.
* NCSDK-30161: Fixed an assertion during boot time caused by a combination of :kconfig:option:`CONFIG_ASSERT`, :kconfig:option:`CONFIG_SOC_NRF54H20_GPD`, and external flash.
* NCSDK-30117: Ensured that it is no longer possible to declare a MEM component pointing to a memory region not assigned to a particular core.
* NCSDK-29682: Added support for the ``cose-alg-sha-512`` algorithm in the SUIT module.

Limitations
***********

On the nRF54H20 SoC, the Device Firmware Update (DFU) procedure from external flash memory does not work with the new flash memory driver based on the MSPI EXMIF handling.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.9.0-nRF54H20**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.9.0-nRF54H20-rc1`_.

Integration test results
************************

The integration test results for this tag can be found in the following external Artifactory:

* `Twister test report for nRF Connect SDK v2.9.0-nRF54H20-rc1`_
* `Hardware test report for nRF Connect SDK v2.9.0-nRF54H20-rc1`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.9.0-nrf54h20-rc1.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.9.0-nRF54H20-rc1`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v2.9.0-nRF54H20-rc1`_ for the changes required or recommended when migrating your nRF54H Series application from |NCS| v2.8.0 to |NCS| v2.9.0-nrf54h20-rc1.

.. _ncs_release_notes_290-nRF54H20-rc1_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated the deprecation notes for `nRF Command Line Tools`_ added in the previous release.
  The notes now clearly state the tools will be archived, no updates will be made to the software, but it will still be available for download.

Board support
=============

* Updated various tests and samples to use Zephyr's :ref:`native simulator <zephyr:native_sim>` instead of Zephyr's :ref:`native POSIX <zephyr:native_posix>` for :ref:`running_unit_tests`.
  This mirrors the deprecation of ``native_posix`` in Zephyr.
  Support for ``native_posix`` will be removed in Zephyr with the v4.2 release.
  In the |NCS|, it will be removed once Zephyr v4.2 is upmerged to sdk-nrf.

Build and configuration system
==============================

* Fixed the issue in the ``nordic-bt-rpc`` snippet, where an invalid memory map was created for nRF54H20 devices, which resulted in a runtime failure.

Security
========

* Extended the ``west ncs-provision`` command so that different key lifetime policies can be selected.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
-------------

* Added support for scanning and initiating at the same time.
  This was introduced in |NCS| 2.7.0 as experimental.
  The :ref:`bt_scanning_while_connecting` sample showcases how you can use this feature to reduce the time to establish connections to many devices.

* Updated the Bluetooth LE SoftDevice Controller driver to make the :c:func:`hci_vs_sdc_llpm_mode_set` function return an error if Low Latency Packet Mode (LLPM) is not supported or not enabled in the Bluetooth LE Controller driver configuration (:kconfig:option:`CONFIG_BT_CTLR_SDC_LLPM`).

Matter
------

* Added:

  * Implementation of the ``Spake2pVerifier`` class for the PSA crypto backend.
    You can use this class to generate the Spake2+ verifier at runtime.
    To use this class, enable the Kconfig options :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC` and :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`.
  * The :ref:`ug_matter_device_watchdog_pause_mode` to the :ref:`ug_matter_device_watchdog` feature.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.4.0.0`` tag.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Enhanced Network Infrastructure with Home Routers and Access Points (HRAP).
    This provides requirements for devices such as home routers, modems, or access points to create a necessary infrastructure for Matter products.
  * Enhanced multi-admin that aims to simplify the smart home management from the user perspective.
    This term includes several different features and in this release only Fabric Synchronization was fully delivered.
    The Fabric Synchronization enables commissioning of devices from one fabric to another without requiring manual user actions, only user consent.
  * Dynamic SIT LIT switching support that allows the application to switch between these modes, as long as the requirements for these modes are met.
    You can enable this using the :kconfig:option:`CONFIG_CHIP_ICD_DSLS_SUPPORT` Kconfig option.
  * The Kconfig option :kconfig:option:`CONFIG_CHIP_ICD_SIT_SLOW_POLL_LIMIT` to limit the slow polling interval value for the device while it is in the SIT mode.
    You can use this to limit the slow poll interval for the ICD LIT device while it is temporarily working in the SIT mode.
  * New device types:

    * Water heater
    * Solar power
    * Battery storage
    * Heat pump
    * Mounted on/off control
    * Mounted dimmable load control

* Updated:

  * Thermostat cluster with support for scheduling and preset modes, like vacation, and home or away settings.
  * Electric Vehicle Supply Equipment (EVSE) with support for user-defined charging preferences, like specifying the time when the car will be charged.
  * Occupancy sensing cluster with features like radar, vision, and ambient sensing.
  * Intermittently Connected Devices feature with enhancements for the Long Idle Time (LIT) and Check-In protocol.
    With these enhancements, the state of this feature is changed from provisional to certifiable.

Thread
------

* Added Kconfig options for configuring the MLE child update timeout, child supervision interval, and child supervision check timeout.

Zigbee
------

* Updated:

  * :ref:`nrfxlib:zboss` to v3.11.6.0 and platform v5.1.7 (``v3.11.6.0+5.1.7``).
    They contain several fixes related to malfunctioning in a heavy traffic environment and more.
    For details, see :ref:`zboss_changelog`.
  * The :ref:`ZBOSS Network Co-processor Host <ug_zigbee_tools_ncp_host>` package to the new version v2.2.5.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Machine learning
----------------

* Updated the application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system for the :ref:`zephyr:nrf54h20dk_nrf54h20` board.

IPC radio firmware
------------------

* Updated the application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system in all devices that contain MRAM, such as the nRF54H Series devices.

Matter bridge
-------------

* Added:

  * Support for the ``UniqueID`` attribute in the Bridged Device Basic Information cluster.
  * Version 2 of the bridged device data scheme containing ``UniqueID``.
  * Kconfig options :ref:`CONFIG_BRIDGE_MIGRATE_PRE_2_7_0 <CONFIG_BRIDGE_MIGRATE_PRE_2_7_0>` and :ref:`CONFIG_BRIDGE_MIGRATE_VERSION_1 <CONFIG_BRIDGE_MIGRATE_VERSION_1>` to enable migration from older data schemes.

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_settings_loader` to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology.
    As a result, all :ref:`zephyr:nrf54h20dk_nrf54h20` configurations were migrated from the NVS settings backend to the ZMS settings backend.
  * :ref:`nrf_desktop_watchdog` by adding the :ref:`zephyr:nrf54h20dk_nrf54h20` release configuration.
  * Updated the configuration files of the :ref:`nrf_desktop_click_detector` (:file:`click_detector_def.h`) to allow them to be used even when Bluetooth LE peer control using a dedicated button (:ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>`) is disabled.
  * The DTS description for board targets with a different DTS overlay file for each build type to isolate the common configuration that is now defined in the :file:`app_common.dtsi` file.
    The :ref:`zephyr:nrf54h20dk_nrf54h20` board configuration has been updated.

  * The :ref:`nrf_desktop_ble_conn_params` with the following changes:

    * Fixed the Bluetooth LE connection parameters update loop (NCSDK-30261) that replicated if an nRF Desktop dongle without Low Latency Packet Mode (LLPM) support was connected to an nRF Desktop peripheral with LLPM support.
    * The module now waits until a triggered Bluetooth LE connection parameters update is completed before triggering subsequent updates for a given connection.
    * Improved the log to also display the information if USB is suspended.
      The information is needed to determine the requested connection parameters.
    * The module now uses non-zero Bluetooth LE peripheral latency while USB is suspended.
      This is done to prevent peripheral latency increase requests from :ref:`nrf_desktop_ble_latency` on peripheral's end.
    * The module reverts the USB suspended Bluetooth LE connection parameter update when USB cable is disconnected.

  * The :ref:`nrf_desktop_ble_scan` to always use a connection interval of 10 ms for peripherals without Low Latency Packet Mode (LLPM) support if a dongle supports LLPM and more than one Bluetooth LE connection.
    This is required to avoid Bluetooth Link Layer scheduling conflicts that could lead to HID report rate drop.

nRF SoC flash driver
--------------------

* Removed the ``imply`` for the partial erase feature of the nRF SoC flash driver (:kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE`) for the USB next stack (:ref:`CONFIG_DESKTOP_USB_STACK_NEXT <config_desktop_app_options>`).
  The partial erase feature was used as a workaround for device errors that might be reported by the Windows USB host in Device Manager if a USB cable is connected while erasing a secondary image slot in the background.
  The workaround is no longer needed after the nRF UDC driver was improved.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added:

  * The :ref:`channel_sounding_ras_reflector` sample demonstrating how to implement a Channel Sounding Reflector that exposes the Ranging Responder GATT Service.
  * The :ref:`channel_sounding_ras_initiator` sample demonstrating Channel Sounding by setting up a Channel Sounding Initiator that acts as a Ranging Requestor GATT Client.
    It includes a basic distance estimation to demonstrate IQ data handling.
    The accuracy is not representative for Channel Sounding and should be replaced if accuracy is important.
  * The :ref:`bt_peripheral_with_multiple_identities` sample demonstrating how to use a single physical device to create and manage multiple advertisers, making it appear as multiple distinct devices by assigning each a unique identity.
  * The :ref:`bt_scanning_while_connecting` sample demonstrating how to establish multiple connections faster using the :kconfig:option:`CONFIG_BT_SCAN_AND_INITIATE_IN_PARALLEL` Kconfig option.

* Updated:

  * Configurations of the following Bluetooth samples to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology:

      * :ref:`bluetooth_central_hids`
      * :ref:`peripheral_hids_keyboard`
      * :ref:`peripheral_hids_mouse`
      * :ref:`central_and_peripheral_hrs`
      * :ref:`central_bas`
      * :ref:`central_nfc_pairing`
      * :ref:`central_uart`
      * :ref:`peripheral_bms`
      * :ref:`peripheral_cgms`
      * :ref:`peripheral_cts_client`
      * :ref:`peripheral_lbs`
      * :ref:`peripheral_mds`
      * :ref:`peripheral_nfc_pairing`
      * :ref:`power_profiling`
      * :ref:`peripheral_rscs`
      * :ref:`peripheral_status`
      * :ref:`peripheral_uart`
      * :ref:`ble_rpc_host`

    As a result, all :ref:`zephyr:nrf54h20dk_nrf54h20` configurations of the affected samples were migrated from the NVS settings backend to the ZMS settings backend.
  * Testing steps in the :ref:`peripheral_hids_mouse` to provide the build configuration that is compatible with the `Bluetooth Low Energy app`_ testing tool.

* :ref:`power_profiling` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board target.

* :ref:`nrf_auraconfig` sample:

  * Fixed an issue with data transmission (OCT-3251).
    Data is now sent on all BISes when generated by the application (no SD card inserted).

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_input_device` sample:

  * Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board target.

Edge Impulse samples
--------------------

* Added support for the :ref:`zephyr:nrf54h20dk_nrf54h20` board target in the following samples:

  * :ref:`ei_data_forwarder_sample`
  * :ref:`ei_wrapper_sample`

Matter samples
--------------

* Updated:

  * All Matter samples that support low-power mode to enable the :ref:`lib_ram_pwrdn` feature.
    It is enabled by default for the release configuration of the following samples:

    * :ref:`matter_lock_sample`
    * :ref:`matter_light_switch_sample`
    * :ref:`matter_smoke_co_alarm_sample`
    * :ref:`matter_window_covering_sample`

  * All Matter samples to enable the ZMS file subsystem in all devices that contain MRAM, such as the nRF54H Series devices.

* Disabled pausing Matter watchdog while CPU is in idle state in all Matter samples.
  To enable it, set the :ref:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP<CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP>` Kconfig option to ``y``.

* :ref:`matter_smoke_co_alarm_sample` sample:

  * Added support for ICD dynamic SIT LIT switching (DSLS).

SUIT samples
------------

* Updated the :ref:`suit_recovery` by adding support for triggering firmware recovery by pressing a hardware button or calling a dedicated API.
  For more information, see :ref:`ug_nrf54h20_suit_recovery`.

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Updated:

    * Configuration for the :ref:`zephyr:nrf54h20dk_nrf54h20` board to support multi-domain logging using the ARM Coresight STM.
    * The logging format in the standard logging mode to align it with the format used in the multi-domain logging mode.
    * Support for alternative configurations to use the :ref:`file suffix feature from Zephyr <app_build_file_suffixes>`.
      The following file suffixes are supported as alternative configurations:

      * ``flash_and_run``
      * ``heap_memory``
      * ``static_memory``
      * ``multiple_threads``

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* Added the :ref:`rreq_readme` and :ref:`rrsp_readme` libraries.

* :ref:`hogp_readme` library:

  * Updated the :c:func:`bt_hogp_rep_read` function to forward the GATT read error code through the registered user callback.
    This ensures that API user is aware of the error.

* :ref:`bt_fast_pair_readme` library:

  * Added support in build system for devices that do not support the :ref:`partition_manager`.
    The :ref:`zephyr:nrf54h20dk_nrf54h20` board target is the only example of such a device.
  * Updated the :c:func:`bt_fast_pair_info_cb_register` API to allow registration of multiple callbacks.

nRF RPC libraries
-----------------

* Added the :ref:`nrf_rpc_dev_info` library for obtaining information about a device connected through the :ref:`nrfxlib:nrf_rpc`.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Added instructions on how to provision the Fast Pair data onto devices without the :ref:`partition_manager` support, specifically for the :ref:`zephyr:nrf54h20dk_nrf54h20`.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``beb733919d8d64a778a11bd5e7d5cbe5ae27b8ee``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline beb733919d ^ea02b93eea

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^beb733919d

The current |NCS| main branch is based on revision ``beb733919d`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Documentation
=============

* Added:

  * The :ref:`matter_samples_config` page that documents Kconfig options and snippets shared by Matter samples and applications.
  * A page about :ref:`add_new_driver`.
  * A page for the :ref:`sdp_gpio` application.

* Updated:

  * The :ref:`ug_nrf54h20_gs` page.
  * The :ref:`ug_nrf54h20_custom_pcb` page.
  * The :ref:`abi_compatibility` page.
  * The structure and contents of the :ref:`gpio_pin_config` page with more detailed information.

* Fixed an issue on the :ref:`install_ncs` page where an incorrect directory path was provided for Linux and macOS at the end of the :ref:`cloning_the_repositories_win` section.
