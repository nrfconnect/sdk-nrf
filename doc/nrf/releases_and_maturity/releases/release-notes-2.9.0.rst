.. _ncs_release_notes_290:

|NCS| v2.9.0 Release Notes
##########################

.. contents::
   :local:
   :depth: 2

|NCS| delivers reference software and supporting libraries for developing low-power wireless applications with Nordic Semiconductor products in the nRF52, nRF53, nRF54, nRF70, and nRF91 Series.
The SDK includes open source projects (TF-M, MCUboot, OpenThread, Matter, and the Zephyr RTOS), which are continuously integrated and redistributed with the SDK.

Release notes might refer to "experimental" support for features, which indicates that the feature is incomplete in functionality or verification, and can be expected to change in future releases.
To learn more, see :ref:`software_maturity`.

Highlights
**********

Added the following features as supported:

* nRF54L Series:

  * FMN extension available for members of the Apple MFi program and Apple Find My.
  * `nRF54L10`_ and `nRF54L05`_ SoCs support including emulated targets on the `nRF54L15 DK`_ including:

    * Protocol support on the nRF54L10 SoC for Bluetooth® LE, Amazon Sidewalk, :ref:`Thread 1.4 <thread_ug_supported_features_v14>`, and Matter over Thread.
    * Protocol support on the nRF54L05 SoC for Bluetooth LE and Thread 1.4.
    * All standard SoC peripherals.
    * :ref:`PSA Crypto APIs <ug_nrf54l_cryptography>` (hardware-accelerated) for cryptographic operations and key storage.

* Bluetooth:

  * :ref:`Parallel scanning and initiating connection <bt_scanning_while_connecting>`.
    It reduces the overall waiting time when there are several Bluetooth devices to be discovered and connected at the same time.

* Wi-Fi®:

  * WPA3 Enterprise security in the nRF Wi-Fi driver, using |NCS| legacy crypto APIs (non-PSA), running on an nRF5340 SoC as a host to an nRF70 Series Wi-Fi 6 companion IC.

* Matter 1.4 enhancements:

  * Long Idle Time (LIT) protocol extending battery life for Matter over Thread devices.
    This feature is showcased in the new :ref:`matter_smoke_co_alarm_sample` sample.
  * Check-in protocol ensuring reliable communication for low-power devices requiring LIT.
  * Energy management - Support for new device types, such as solar panels, batteries, heat pumps, and water heaters, as well as improvements to Electric Vehicle Supply Equipment (EVSE) and thermostats.
  * Occupancy sensing - Sensing features, such as radar vision and ambient sensing technologies.
  * Two new device types (mounted On/Off and dimmable load control) designed specifically for fixed in-wall smart home devices that deliver power to wired devices.

    For more information, see the `CSA press release for Matter 1.4`_.

Added the following features as experimental:

* Bluetooth:

  * :ref:`channel_sounding_ras_reflector` and :ref:`channel_sounding_ras_initiator` samples for the nRF54L Series devices.
    The samples showcase how to use the ranging service between two devices.
    They are primarily meant to demonstrate the data flow between initiator and reflector.
    The ranging algorithm in these samples is for illustrative purposes, and the resulting accuracy is not representative for Channel Sounding.

Sign up for the `nRF Connect SDK v2.9.0 webinar`_ to learn more about the new features.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.9.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.9.0`_.

Integration test results
************************

The integration test results for this tag can be found in the following external artifactory:

* `Twister test report for nRF Connect SDK v2.9.0`_
* `Hardware test report for nRF Connect SDK v2.9.0`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.9.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9151 SoC`_
* `Modem firmware compatibility matrix for the nRF9161 SoC`_
* `Modem firmware compatibility matrix for the nRF9160 SoC`_

Use the latest version of the `Programmer app`_ of `nRF Connect for Desktop`_ to update the modem firmware.
See the `Programming nRF91 Series DK firmware` page for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v2.9.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.9.0_>`_


Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.9.0`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v2.9.0`_ for the changes required or recommended when migrating your application from |NCS| v2.8.0 to |NCS| v2.9.0.

.. _ncs_release_notes_290_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Added support for the Nordic Thingy:91 X to the `Quick Start app`_.
  The list on the :ref:`gsg_guides` page is updated accordingly.

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

Bootloaders and DFU
===================

* Updated the allowed offset for :ref:`doc_fw_info` in the :ref:`bootloader`.
  It can now be placed at offset ``0x600``.
  However, you cannot use it for any applications with |NSIB| compiled before this change.

Developing with nRF91 Series
============================

* Moved the Thingy:91 and Thingy:91 X guides to new sections, :ref:`thingy91_ug_intro` and :ref:`ug_thingy91x` respectively, under :ref:`ug_app_dev`.

Developing with nRF54L Series
=============================

* Added:

  * The :ref:`nRF54l_signing_app_with_flpr_payload` page that includes instructions for building separate applications, merging them, and signing them for MCUboot.
  * The :ref:`ug_nrf54l_developing_basics_kmu` page explaining basic concepts and recommendations.

Developing with nRF53 Series
============================

* Moved the Thingy:53 to a new section, :ref:`ug_thingy53`, under :ref:`ug_app_dev`.

Developing with Front-End Modules
=================================

* Deprecated the explicit use of ``-DSHIELD=nrf21540ek_fwd`` for boards with ``nrf5340/cpuapp`` qualifiers when the nRF21540 EK shield is used.
  The build system uses an appropriate overlay file for each core, relying only on the ``-DSHIELD=nrf21540ek`` parameter.

Security
========

* Extended the ``west ncs-provision`` command so that different key lifetime policies can be selected.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

* Added support for the ``nrf54l15dk/nrf54l10/cpuapp`` board target.

Bluetooth LE
-------------

* Added support for scanning and initiating at the same time.
  This was introduced in |NCS| 2.7.0 as experimental.
  The :ref:`bt_scanning_while_connecting` sample showcases how you can use this feature to reduce the time to establish connections to many devices.

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

  * ZBOSS Zigbee stack to v3.11.6.0 and platform v5.1.7 (``v3.11.6.0+5.1.7``).
    They contain several fixes related to malfunctioning in a heavy traffic environment and more.
    For details, see ZBOSS changelog.
  * The ZBOSS Network Co-processor Host package to the new version v2.2.5.

Wi-Fi
-----

* Updated the :ref:`wifi_regulatory_channel_rules` for some countries in the :ref:`ug_nrf70_developing_regulatory_support` documentation.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Asset Tracker v2
----------------

* Updated the Wi-Fi configurations to reduce the RAM usage by about 25 kB for an nRF91 Series DK and 12 kB for the Thingy:91 X.

Connectivity bridge
-------------------

* Updated the handling of USB CDC ACM baud rate requests to make sure the baud rate is set correctly when the host requests a change.
  This fixes an issue when using GNU screen with the Thingy:91 X.

Matter bridge
-------------

* Added:

  * Support for the ``UniqueID`` attribute in the Bridged Device Basic Information cluster.
  * Version 2 of the bridged device data scheme containing ``UniqueID``.
  * Kconfig options :ref:`CONFIG_BRIDGE_MIGRATE_PRE_2_7_0 <CONFIG_BRIDGE_MIGRATE_PRE_2_7_0>` and :ref:`CONFIG_BRIDGE_MIGRATE_VERSION_1 <CONFIG_BRIDGE_MIGRATE_VERSION_1>` to enable migration from older data schemes.

nRF5340 Audio
-------------

* Updated:

  * The documentation for :ref:`nrf53_audio_app_building` with cross-links and additional information, based on user feedback.
  * The calculation in ``audio_datapath.num_blks_in_fifo`` to consider wrapping.

nRF Desktop
-----------

* Updated:

  * The :ref:`nrf_desktop_settings_loader` to make the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` the default settings backend for all board targets that use the MRAM technology.
    As a result, all :zephyr:board:`nrf54h20dk` configurations were migrated from the NVS settings backend to the ZMS settings backend.
  * :ref:`nrf_desktop_watchdog` by adding the :zephyr:board:`nrf54h20dk` release configuration.
  * The configuration files of the :ref:`nrf_desktop_click_detector` (:file:`click_detector_def.h`) to allow using them also when Bluetooth LE peer control using a dedicated button (:ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>`) is disabled.
  * The DTS description for board targets with a different DTS overlay file for each build type to isolate the common configuration that is now defined in the :file:`app_common.dtsi` file.
    The following board configurations have been updated:

    * :zephyr:board:`nrf52840dk`
    * :zephyr:board:`nrf52840dongle`
    * :zephyr:board:`nrf54l15dk`
    * :zephyr:board:`nrf54h20dk`

  * MCUboot bootloader configurations to enable the following Kconfig options:

    * :kconfig:option:`CONFIG_FPROTECT` - Used to protect the bootloader partition against memory corruption.
    * :kconfig:option:`CONFIG_HW_STACK_PROTECTION` - Used to protect against stack overflows.

    The :kconfig:option:`CONFIG_HW_STACK_PROTECTION` Kconfig option and its dependency (the :kconfig:option:`CONFIG_ARM_MPU` Kconfig option) might be disabled in case of targets with limited memory.

  * MCUboot bootloader configuration for the MCUboot SMP build type and the nRF52840 Gaming Mouse target to enable the :kconfig:option:`CONFIG_ARM_MPU` Kconfig option that is required to enable hardware stack protection (:kconfig:option:`CONFIG_HW_STACK_PROTECTION`).

  * The nRF54L15 DK's configurations (``nrf54l15dk/nrf54l15/cpuapp``) to enable hardware cryptography for the MCUboot bootloader.
    The application image is verified using a pure ED25519 signature.
    The public key that MCUboot uses for validating the application image is securely stored in the hardware Key Management Unit (KMU).

    The change increases the MCUboot partition size (modifies the Partition Manager configuration) and changes the MCUboot image signing algorithm.
    Because of that, the nRF Desktop application images built for an nRF54L15 DK from the |NCS| v2.9.0 are not compatible with the MCUboot bootloader built from previous releases.
    It is highly recommended to use hardware cryptography for the nRF54L SoC Series for improved security.
    See the :ref:`nrf_desktop_bootloader` page for more details.

  * The :ref:`nrf_desktop_ble_conn_params` to:

    * Fix the Bluetooth LE connection parameters update loop (NCSDK-30261) that replicated if an nRF Desktop dongle without Low Latency Packet Mode (LLPM) support was connected to an nRF Desktop peripheral with LLPM support.
    * Wait until a triggered Bluetooth LE connection parameters update is completed before triggering subsequent updates for a given connection.
    * Improve the log to also display the information if USB is suspended.
      The information is needed to determine the requested connection parameters.
    * Use non-zero Bluetooth LE peripheral latency while USB is suspended.
      This is done to prevent peripheral latency increase requests from :ref:`nrf_desktop_ble_latency` on peripheral's end.
    * Revert the USB suspended Bluetooth LE connection parameter update when USB cable is disconnected.

  * The :ref:`nrf_desktop_ble_scan` to always use a connection interval of 10 ms for peripherals without Low Latency Packet Mode (LLPM) support if a dongle supports LLPM and more than one Bluetooth LE connection.
    This is required to avoid Bluetooth Link Layer scheduling conflicts that could lead to HID report rate drop.

* Removed imply for partial erase feature of the nRF SoC flash driver (:kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE`) for the USB next stack (:ref:`CONFIG_DESKTOP_USB_STACK_NEXT <config_desktop_app_options>`).
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
  * Support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in the following samples:

    * :ref:`direct_test_mode`
    * :ref:`peripheral_hids_mouse`
    * :ref:`peripheral_lbs`
    * :ref:`power_profiling`
    * :ref:`peripheral_uart`

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

  * Fixed the data sending (OCT-3251).
    Data is now sent on all BISes when generated by the application (no SD card inserted).

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated the MCUboot bootloader configuration for the :zephyr:board:`nrf54l15dk` board target to enable the :kconfig:option:`CONFIG_FPROTECT` Kconfig option that is used to protect the bootloader partition against memory corruption.

* :ref:`fast_pair_input_device` sample:

  * Added support for the :zephyr:board:`nrf54h20dk` board target.

Cellular samples
----------------

* Updated the :kconfig:option:`CONFIG_NRF_CLOUD_CHECK_CREDENTIALS` Kconfig option to be optional and enabled by default for the following samples:

  * :ref:`nrf_cloud_rest_cell_location`
  * :ref:`nrf_cloud_rest_device_message`
  * :ref:`nrf_cloud_rest_fota`

* :ref:`location_sample` sample:

  * Updated:

    * The Thingy:91 X build to support Wi-Fi by default without overlays.
    * The Wi-Fi configurations to reduce the RAM usage by about 25 kB.

* :ref:`modem_shell_application` sample:

  * Updated the Wi-Fi configurations to reduce the RAM usage by about 25 kB.

* :ref:`nrf_cloud_multi_service` sample:

  * Updated the Wi-Fi configurations to reduce the RAM usage by about 12 kB for an nRF91 Series DK and 25 kB for the Thingy:91 X.

DECT NR+ samples
----------------

* :ref:`dect_shell_application` sample:

  * Added:

    * The ``dect mac`` command.
      A brief MAC-level code sample on top of DECT PHY interface with new commands to create a periodic cluster beacon, scan for it, associate or disassociate a PT/client, and send data to a FT/beacon random access window.
      This is not a full MAC implementation and not fully compliant with DECT NR+ MAC specification (`ETSI TS 103 636-4`_).
    * The ``startup_cmd`` command.
      This command is used to store shell commands to be run sequentially after bootup.
    * Band 4 support for nRF9151 with modem firmware v1.0.2.

  * Updated:

    * The ``dect rssi_scan`` command with busy/possible/free subslot count-based RSSI scan.
    * The ``dect rx`` command to provide the possibility to iterate all channels and to enable RX filter.

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
  To enable it, set the :ref:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP<CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP>` Kconfig option to ``y``.

* :ref:`matter_template_sample` sample:

  * Added support for the ``nrf54l15dk/nrf54l10/cpuapp`` board target.
  * Updated the internal configuration for the :zephyr:board:`nrf54l15dk` target to use the DFU image compression and provide more memory space for the application.

* :ref:`matter_smoke_co_alarm_sample` sample:

  * Added:

    * Support for ICD dynamic SIT LIT switching (DSLS).
    * Support for the ``nrf54l15dk/nrf54l10/cpuapp`` board target.

Peripheral samples
------------------

* Added support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in the :ref:`radio_test` sample.

Protocol serialization samples
------------------------------

* Updated GPIO pins on the nRF54L15 DK used for communication between the client and server over UART.
  One of the previously selected pins was also used to drive an LED, which may have disrupted the UART communication.

Thread samples
--------------

* Removed support for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target for all samples.

* :ref:`ot_cli_sample` sample:

  * Added support for the ``nrf54l15dk/nrf54l10/cpuapp`` board target.

* :ref:`ot_coprocessor_sample` sample:

  * Added support for the ``nrf54l15dk/nrf54l10/cpuapp`` and ``nrf54l15dk/nrf54l05/cpuapp`` board targets.

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

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* Added the :ref:`nrf70_wifi_tx_power_calculation` section to the :ref:`nrf70_wifi` page.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated the :ref:`req_appln_limitations` page to clarify carrier-specific requirements.
    Added overlay files and documentation to Serial LTE modem application and :ref:`lwm2m_carrier` sample to guide in the correct usage of LwM2M carrier library for SoftBank and LG U+.

Bluetooth libraries and services
--------------------------------

* Added the :ref:`rreq_readme` and :ref:`rrsp_readme` libraries.

* :ref:`hogp_readme` library:

  * Updated the :c:func:`bt_hogp_rep_read` function to forward the GATT read error code through the registered user callback.
    This ensures that API user is aware of the error.

* :ref:`bt_fast_pair_readme` library:

  * Added support in build system for devices that do not support the :ref:`partition_manager`.
    The :zephyr:board:`nrf54h20dk` board target is the only example of such a device.

Modem libraries
---------------

* :ref:`modem_key_mgmt` library:

  * Added the CME error code 527 - invalid content.
  * Updated to handle generic CME errors from all ``AT%CMNG`` commands.

nRF RPC libraries
-----------------

* Added the :ref:`nrf_rpc_dev_info` library for obtaining information about a device connected through the :ref:`nrfxlib:nrf_rpc`.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`nrf_desktop_config_channel_script` Python script:

  * Added support for pure ED25519 signature (used by nRF54L-based devices that enable MCUboot hardware cryptography).
    This requires using ``imgtool`` supporting pure ED25519 signature that can be installed from ``sdk-mcuboot`` repository.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``e890df7ab975da181a9f3fb3abc470bf935625ab``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added an option that allows to select the number of KMU key slots (also known as generations) to use when verifying an image.
  See MCUboot's Kconfig option :kconfig:option:`CONFIG_BOOT_SIGNATURE_KMU_SLOTS`.

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

* Updated the structure and contents of the :ref:`gpio_pin_config` page with more detailed information.

* Fixed an issue on the :ref:`install_ncs` page where an incorrect directory path was provided for Linux and macOS at the end of the :ref:`cloning_the_repositories_win` section.
