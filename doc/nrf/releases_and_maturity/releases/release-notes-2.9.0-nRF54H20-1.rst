.. _ncs_release_notes_290-nrf54h20-1:

|NCS| v2.9.0-nRF54H20-1 Release Notes
#####################################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

The |NCS| v2.9.0-nRF54H20-1 is an nRF54H20-exclusive release tag, only supporting products based on the nRF54H20 SoC.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

Added the following features as supported:

* nRF54H20 Series:

  * Triggering firmware recovery by pressing a hardware button or calling the respective API.
    For more information, see ``ug_nrf54h20_suit_recovery``.
  * Global Domain Frequency Scaling (GDFS), optimizing USB suspend power consumption.
  * The support for a new ZMS backend for Settings.
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

Improved:

* BICR handling and generation.
* Power management features across various subsystems.

Fixed:

* NCSDK-30802: An issue where the nRF54H20 device suddenly stopped transmitting ESB packets after nrfx 3.9.0.
* NCSDK-30161: An assertion during boot time caused by a combination of :kconfig:option:`CONFIG_ASSERT`, :kconfig:option:`CONFIG_SOC_NRF54H20_GPD`, and external flash.
* NCSDK-30117: An issue where a MEM component could be declared pointing to a memory region not assigned to a specific core.
* NCSDK-29682: Added support for the ``cose-alg-sha-512`` algorithm in the SUIT module.

Limitations
***********

On the nRF54H20 SoC, the Device Firmware Update (DFU) procedure from external flash memory does not work with the new flash memory driver based on the MSPI EXMIF handling.
You must use the :file:`spi_dw.c` driver in such case.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.9.0-nRF54H20-1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.9.0-nRF54H20-1`_.

Integration test results
************************

The integration test results for this tag can be found in the following external locations:

* `Twister test report for nRF Connect SDK v2.9.0-nRF54H20-1`_
* `Hardware test report for nRF Connect SDK v2.9.0-nRF54H20-1`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.9.0-nRF54H20-1.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.9.0-nRF54H20-1`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v2.9.0-nRF54H20-1`_ for the changes required or recommended when migrating your nRF54H Series application from |NCS| v2.8.0 to |NCS| v2.9.0-nRF54H20-1.

.. _ncs_release_notes_290-nrf54h20-1_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated the deprecation notes for `nRF Command Line Tools`_ added in the previous release.
  The notes now clearly state the tools will be archived, no updates will be made to the software, but it will still be available for download.

Board support
=============

* Updated various tests and samples to use Zephyr's :zephyr:board:`native simulator <native_sim>` instead of Zephyr's native POSIX for :ref:`running_unit_tests`.
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

* Fixed an issue where Bluetooth applications built with the ``nordic-ble-rpc`` snippet (in the :ref:`ble_rpc` configuration) did not work on the nRF54H20 devices due to incorrect memory mapping.

Matter
------

* Added:

  * Implementation of the ``Spake2pVerifier`` class for the PSA crypto backend.
    You can use this class to generate the Spake2+ verifier at runtime.
    To use this class, enable the Kconfig options :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC` and :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`.
  * The :ref:`ug_matter_device_watchdog_pause_mode` to the :ref:`ug_matter_device_watchdog` feature.

Enhanced ShockBurst (ESB)
-------------------------

* Added support for loading radio trims.
* Fixed:

  * An hardware erratum for the nRF54H20 SoC to improve RF performance.
  * An issue where the nRF54H20 device suddenly stopped transmitting ESB packets after nrfx 3.9.0.

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

  * ZBOSS Zigbee stack to v3.11.6.0 and platform v5.1.7 (``v3.11.6.0+5.1.7``).
    They contain several fixes related to malfunctioning in a heavy traffic environment and more.
    For details, see the ZBOSS changelog.
  * The ZBOSS Network Co-processor Host package to the new version v2.2.5.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Machine learning
----------------

* Updated the application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system for the :zephyr:board:`nrf54h20dk` board.

IPC radio firmware
------------------

* Updated the application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system in all devices that contain MRAM, such as the nRF54H Series devices.

Matter bridge
-------------

* Added:

  * Support for the ``UniqueID`` attribute in the Bridged Device Basic Information cluster.
  * Version 2 of the bridged device data scheme containing ``UniqueID``.
  * Kconfig options :option:`CONFIG_BRIDGE_MIGRATE_PRE_2_7_0` and :option:`CONFIG_BRIDGE_MIGRATE_VERSION_1` to enable migration from older data schemes.

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_settings_loader` to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology.
    As a result, all :zephyr:board:`nrf54h20dk` configurations were migrated from the NVS settings backend to the ZMS settings backend.
  * :ref:`nrf_desktop_watchdog` by adding the :zephyr:board:`nrf54h20dk` release configuration.
  * The configuration files of the :ref:`nrf_desktop_click_detector` (:file:`click_detector_def.h`) to allow them to be used even when Bluetooth LE peer control using a dedicated button (:ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>`) is disabled.
  * The DTS description for board targets with a different DTS overlay file for each build type to isolate the common configuration that is now defined in the :file:`app_common.dtsi` file.
    The :zephyr:board:`nrf54h20dk` board configuration has been updated.
  * The :ref:`nrf_desktop_failsafe` to use the Zephyr :ref:`zephyr:hwinfo_api` driver for getting and clearing the reset reason information (see the :c:func:`hwinfo_get_reset_cause` and :c:func:`hwinfo_clear_reset_cause` functions).
    The Zephyr :ref:`zephyr:hwinfo_api` driver replaces the dependency on the nrfx reset reason helper (see the :c:func:`nrfx_reset_reason_get` and :c:func:`nrfx_reset_reason_clear` functions).

  * The release configuration for the :zephyr:board:`nrf54h20dk` board target to enable the :ref:`nrf_desktop_failsafe` (see the :ref:`CONFIG_DESKTOP_FAILSAFE_ENABLE <config_desktop_app_options>` Kconfig option).

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

  * :ref:`direct_test_mode`:

    * Added support for loading radio trims.
    * Fixed a hardware erratum for the nRF54H20 SoC to improve RF performance.

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

    As a result, all :zephyr:board:`nrf54h20dk` configurations of the affected samples were migrated from the NVS settings backend to the ZMS settings backend.
  * Testing steps in the :ref:`peripheral_hids_mouse` to provide the build configuration that is compatible with the `Bluetooth Low Energy app`_ testing tool.

* :ref:`power_profiling` sample:

  * Added support for the :zephyr:board:`nrf54h20dk` board target.

* :ref:`nrf_auraconfig` sample:

  * Fixed an issue with data transmission (OCT-3251).
    Data is now sent on all BISes when generated by the application (no SD card inserted).

Peripheral samples
------------------

* :ref:`radio_test`:

  * Added support for loading radio trims.
  * Fixed a hardware erratum for the nRF54H20 SoC to improve RF performance.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_input_device` sample:

  * Added support for the :zephyr:board:`nrf54h20dk` board target.

* :ref:`fast_pair_locator_tag` sample:

  * Added support for the :zephyr:board:`nrf54h20dk` board target.

Edge Impulse samples
--------------------

* Added support for the :zephyr:board:`nrf54h20dk` board target in the following samples:

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
  To enable it, set the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP` Kconfig option to ``y``.

* :ref:`matter_smoke_co_alarm_sample` sample:

  * Added support for ICD dynamic SIT LIT switching (DSLS).

SUIT samples
------------

* Updated the ``suit_recovery`` by adding support for triggering firmware recovery by pressing a hardware button or calling a dedicated API.
  For more information, see ``ug_nrf54h20_suit_recovery``.

Other samples
-------------

* :ref:`coremark_sample` sample:

  * Updated:

    * Configuration for the :zephyr:board:`nrf54h20dk` board to support multi-domain logging using the ARM Coresight STM.
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

  * Added support in the build system for devices that do not support the :ref:`partition_manager`.
    The :zephyr:board:`nrf54h20dk` board target is the only example of such a device.

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

* Added instructions on how to provision the Fast Pair data onto devices without the :ref:`partition_manager` support, specifically for the :zephyr:board:`nrf54h20dk`.

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

ZMS backend
-----------

* Added the support for a new ZMS backend for Settings in |NCS|:

  * The following Kconfig options for the *ZMS backend for Settings* are not available in the |NCS| v2.9.0-nRF54H20-1:

    * ``CONFIG_SETTINGS_ZMS_NAME_CACHE``
    * ``CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE``
    * ``CONFIG_ZMS_LOOKUP_CACHE_FOR_SETTINGS``

  * The ZMS settings backend now defaults to using the entire available storage partition.
    See :ref:`migration_2.9.0-nRF54H20-1`.

Documentation
=============

* Added:

  * The :ref:`matter_samples_config` page that documents Kconfig options and snippets shared by Matter samples and applications.
  * A page about :ref:`add_new_driver`.
  * A page for the :ref:`sdp_gpio` application.
  * The :ref:`ug_nrf54h20_keys` page.

* Updated:

  * The :ref:`ug_nrf54h20_gs` page.
  * The :ref:`ug_nrf54h20_custom_pcb` page.
  * The :ref:`abi_compatibility` page.
  * The :ref:`zms_memory_storage` page to document its use on the nRF54H20 SoC.
  * The structure and contents of the :ref:`gpio_pin_config` page with more detailed information.

* Fixed an issue on the :ref:`install_ncs` page where an incorrect directory path was provided for Linux and macOS at the end of the :ref:`cloning_the_repositories_win` section.
