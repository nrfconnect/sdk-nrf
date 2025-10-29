.. _ncs_release_notes_3.0.0:

|NCS| v3.0.0 Release Notes
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

This major release of |NCS| introduces significant changes that can break backward compatibility for existing projects.
Refer to the :ref:`migration_3.0` for further information.

The SDK can be installed using any one of the following options:

* Using |VSC| and the :ref:`requirements_nrfvsc` (recommended).
* Using command line and :ref:`requirements_nrf_util`.

Starting with this release, the SDK installation using the Toolchain Manager app is not supported.

Added the following features as supported:

* Bluetooth®:

  * Channel Sounding Controller and Host will be Bluetooth qualified for the nRF54L Series.
  * The :ref:`channel_sounding_ras_initiator` sample now includes an IFFT algorithm for Phase-Based Ranging, providing a viable distance measurement result for basic ranging applications.
    Results from the illustrative MCPD algorithm from the |NCS| 2.9.0 release, which is shown alongside the IFFT result, are still not recommended for ranging.
  * Bluetooth controller and host qualified for the nRF54H20.

* Matter:

  * Matter 1.4.1: Matter commissioning using NFC tag can be officially certified now.
  * :ref:`matter_manufacturer_specific_sample` sample: Dedicated sample, containing documentation and a preview :ref:`PC app <ug_matter_gs_tools_matter_cluster_editor>`, facilitating creation and modification of manufacturer specific clusters.

* PMIC:

  * nPM2100 and nPM2100 EK:

    * Added support for nPM2100, which is a PMIC designed for primary (non-rechargeable) batteries in an extremely compact form factor.
      It has an ultra-efficient boost regulator, a dual purpose LDO/load switch, two GPIOs, an ADC, and other features.
    * :ref:`npm2100_fuel_gauge` sample, demonstrating how to calculate the state of charge of a supported primary cell battery using the nPM2100 and the :ref:`nrfxlib:nrf_fuel_gauge`.
    * :ref:`npm2100_one_button` sample, demonstrating how to support wake-up, shutdown, and user interactions through a single button connected to the nPM2100.

* nRF54L Series:

  * nRF54L10 and nRF54L05 are added as supported targets in the :ref:`nrf_desktop` application.
  * :ref:`mcuboot_image_compression` is now supported on nRF54L15 and nRF54L10.
  * nRF21540 GPIO support on nRF54L Series.

Added the following features as experimental:

* nRF54L Series:

  * Bootloader and Device Firmware Update (DFU):

    * Support for nRF Secure Immutable Bootloader as first stage immutable bootloader.
    * Support for encrypted DFU with ECIES x25519 encryption using MCUboot.

  * QSPI external memory interface provided by :ref:`sQSPI Soft Peripheral <sQSPI>`, which utilizes the nRF54L15 FLPR coprocessor.
  * Coprocessor High Performance Framework, a framework designed to facilitate the creation and integration of :ref:`software peripherals using the nRF54L15 FLPR coprocessor <coprocessors_index>`.

Improved:

* Wi-Fi®:

  * Up to 25 kB reduction in the RAM footprint of the Wi-Fi stack on nRF5340 and nRF54L15 hosts, for Wi-Fi applications with low throughput requirements.
  * Added support for the runtime certificate update for WPA Enterprise security.

* nRF54H20:

  * Significantly improved support for multiple hardware features.

* LE Audio:

  * The following LE Audio roles are now qualified.
    Refer to the ICS details in the product listing for a complete overview of which profiles, services and features are included in the qualification.
    The LE Audio profiles and services run on top of the qualified Nordic BLE Host and Controller.

    * Unicast Client Source (base stations).
    * Broadcast Source (broadcasters/Auracasters).
    * Unicast Server Source (microphones).

* :ref:`nrf_desktop`:

  * Support for Bluetooth LE legacy pairing is no longer enabled by default, because it is not secure.
  * Enabled Link Time Optimization (LTO) for images built by :ref:`Sysbuild (System build) <sysbuild>`.

Removed:

* Hardware model v1, which was deprecated in |NCS| 2.7.0, has now been removed.
  Existing projects must :ref:`transition to hardware model v2 <hw_model_v2>`.
* Multi-image builds functionality (parent-child images), which was deprecated in |NCS| v2.7.0 has now been removed.
  Existing projects must transition to :ref:`Sysbuild (System build) <sysbuild>`.
* Zigbee R22, which was deprecated in |NCS| 2.8.0, has now been removed.
  Support for `Zigbee R22`_ and `Zigbee R23`_ is available as an `nRF Connect SDK Add-on <nRF Connect SDK Add-ons_>`_.
* Asset Tracker v2 application is now removed.
  The application is replaced by `Asset Tracker Template`_, which will be available as an `nRF Connect SDK Add-on <nRF Connect SDK Add-ons_>`_.
* The application configurations for the nRF52810 Desktop Mouse board in :ref:`nrf_desktop` has been removed.
* Amazon Sidewalk has been removed from |NCS| and is now available as an `Add-on <Amazon Sidewalk documentation_>`_.

Sign up for the `nRF Connect SDK v3.0.0 webinar`_ to learn more about the new features.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.0.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.0.0`_.

Integration test results
************************

The integration test results for this tag can be found in the following external artifactory:

* `Twister test report for nRF Connect SDK v3.0.0`_
* `Hardware test report for nRF Connect SDK v3.0.0`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.0.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9151 SoC`_
* `Modem firmware compatibility matrix for the nRF9161 SoC`_
* `Modem firmware compatibility matrix for the nRF9160 SoC`_

Use the latest version of the `Programmer app`_ of `nRF Connect for Desktop`_ to update the modem firmware.
See :ref:`nrf9160_gs_updating_fw_modem` for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v3.0.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.0.0_>`_


Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.0.0`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v3.0.0`_ for the changes required or recommended when migrating your application from |NCS| v2.9.0 to |NCS| v3.0.0.

.. _ncs_release_notes_300_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated:

  * The required `SEGGER J-Link`_ version to v8.18.
  * The :ref:`installation` page with the following:

    * The :ref:`installing_vsc` section with a list valid for both development environments.
      The list now includes nRF Util as an additional requirement for :ref:`west runner <programming_selecting_runner>`  for the |nRFVSC|, and the Windows-only requirement to install SEGGER USB Driver for J-Link for both development environments.
    * The command-line instructions now use the ``nrfutil sdk-manager`` command instead of the ``nrfutil toolchain-manager`` command.
      You can read more about the new command in the `nRF Util documentation <sdk-manager command_>`_.

  * Mentions of commands that use tools from the nRF Command Line Tools to use nRF Util.
    |nrf_CLT_deprecation_note|

* Removed the Toolchain Manager app section from the following pages:

  * :ref:`installation`
  * :ref:`updating`
  * :ref:`requirements`

  The app no longer provides the latest toolchain and |NCS| versions for installation.

Board support
=============

* Removed support for the nRF52810 Desktop Mouse board (``nrf52810dmouse/nrf52810``).

Build and configuration system
==============================

* Removed support for the deprecated multi-image builds (parent-child images) functionality.
  All |NCS| projects must now use :ref:`sysbuild`.
  See :ref:`child_parent_to_sysbuild_migration` for an overview of differences with parent-child image and how to migrate.
* Updated:

  * The default runner for the ``west flash`` command to `nRF Util`_ instead of ``nrfjprog`` that is part of the archived `nRF Command Line Tools`_.
    For more information, see the :ref:`required_build_system_mig_300` section and the :ref:`programming_selecting_runner` section on the programming page.

    .. note::

       For |NCS| 3.0.0, use the nrfutil-device v2.8.8.

  * Erasing the external memory when programming a new firmware image with the ``west flash`` series now always correctly honors the ``--erase`` flag (and its absence) both when using the ``nrfjprog`` and ``nrfutil`` backends.
    Before this release, the ``nrfjprog`` backend would always erase only the sectors of the external flash used by the new firmware, and the ``nrfutil`` backend would always erase the whole external flash.
  * ``west ncs-provision`` command was ported onto newest nrfutil device provisioning command.
    User must update ``nrfutil-device`` to v2.8.8 for this |NCS| release.
  * The ``CONFIG_NRF53_MULTI_IMAGE_UPDATE`` Kconfig option no longer depends on external flash (NCSIDB-1232).
  * The static partition manager file for network core images can now be set (NCSIDB-1442).
  * QSPI XIP support has been extended to include building with TF-M on the nRF5340 device.
    An instance of TF-M can now be part of the internal NVM application image.

Bootloader and DFU
==================

* Added experimental support |NSIB| for the nRF54L15 SoC.
  On nRF54L SoCs, NSIB uses KMU for authentication key storage.
  It supports keys revocation scheme and it can be protected using immutable-boot region SoC's hardware feature.
* Updated by improving DFU timing performance on the nRF54L SoC by applying optimal RRAMC buffering.

Developing with nRF70 Series
============================

* Added:

  * Support for the nRF7002-EB II (PCA63571) with the nRF54 Series DKs as detailed in :ref:`ug_nrf7002eb2_gs`.
  * A new section :ref:`ug_nrf70_wifi_enterprise_mode` in the :ref:`ug_nrf70_wifi_advanced_security_modes` page.

* Deprecated support for the nRF7002 EB (PCA63561) with the nRF54 Series DKs.
* Removed support for storing the nRF70 firmware patches in external flash without the :ref:`partition_manager`, as mentioned in :ref:`ug_nrf70_developing_fw_patch_ext_flash`.

Developing with nRF54L Series
=============================

* Added:

  * HMAC SHA-256 with a 128-bit key type to KMU, as detailed in the :ref:`ug_nrf54l_crypto_kmu_supported_key_types` documentation section.
  * A workaround for nRF54L15 Errata 30.
    Use ``CONFIG_CLOCK_CONTROL_NRF_HF_CALIBRATION=y`` to explicitly activate the workaround.
    A consequence of activating the workaround might be increased power consumption due to periodic CPU wake-up, so use it only if errata conditions are met.
    The workaround is already included in the MPSL component, so the solution is applicable only if your application does not use MPSL.

Developing with nRF54H Series
=============================

* Added a new documentation page ``ug_nrf54h20_suit_signing`` under ``ug_nrf54h20_suit_dfu``.
* Removed the note on installing SEGGER USB Driver for J-Link on Windows from the :ref:`ug_nrf54h20_gs` page and moved its contents to the `nRF Util prerequisites`_ documentation.
  The Windows-only requirement to install the SEGGER USB Driver for J-Link is now mentioned in the :ref:`installing_vsc` section on the :ref:`installation` page.

Developing with Front-End Modules
=================================

* Added support for the following:

  * :ref:`nRF21540 Front-End Module in GPIO mode <ug_radio_fem_nrf21540_gpio>` for the nRF54L Series devices.

* Fixed an issue for the nRF21540 Front-End Module (for GPIO and GPIO+SPI modes) when spurious emission occurred due to late activation of the ``TX_EN`` pin.
  The ``PDN`` pin is now activated earlier by the call to the :c:func:`mpsl_fem_enable` function.
  The ``TX_EN`` pin is now activated 15 µs earlier during the ramp-up of the radio.

Developing with PMICs
=====================

* Added the :ref:`ug_npm2100_developing` documentation.

Security
========

* Added:

  * Support for HKDF-Expand and HKDF-Extract in CRACEN.
  * Support for HashEdDSA (ed25519ph) to CRACEN.
  * TF-M now supports Attestation service on nRF54L15
  * The following documentation pages:

    * :ref:`ug_tfm_architecture`.
    * :ref:`ug_psa_certified_api_overview`.
    * :ref:`ug_tfm_supported_services`.

* Updated:

  * The Oberon PSA core to version 1.3.4 that introduces support for the following:

    * PSA static key slots with the option :kconfig:option:`CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOTS`.
    * NIST SP 800-108 conformant CMAC and HMAC based key derivation using Oberon PSA driver.

      For more information regarding the Oberon PSA core v1.3.4 update, see the relevant changelog entry in the `Oberon PSA core changelog`_.

    * The :ref:`app_approtect` page with nRF Util commands that replaced the nrfjprog commands.
      This is part of the ongoing work of archiving `nRF Command Line Tools`_ and replacing them with nRF Util.
    * The :ref:`app_boards_spe_nspe` documentation page from the :ref:`ug_app_dev` section has been moved under :ref:`ug_tfm_index`.

* Removed the Running applications with Trusted Firmware-M page.
  Its contents have been moved into the following new pages:

  * :ref:`ug_tfm_index`
  * :ref:`ug_tfm_building`
  * :ref:`ug_tfm_logging`
  * :ref:`ug_tfm_services`
  * :ref:`ug_tfm_provisioning`

Protocols
=========

Bluetooth LE
------------

* Updated the Bluetooth LE SoftDevice Controller driver to make the :c:func:`hci_vs_sdc_llpm_mode_set` function return an error if Low Latency Packet Mode (LLPM) is not supported or not enabled in the Bluetooth LE Controller driver configuration (:kconfig:option:`CONFIG_BT_CTLR_SDC_LLPM`).

* Fixed:

  * An issue where a flash operation executed on the system workqueue might result in ``-ETIMEDOUT``, if there is an active Bluetooth LE connection.
  * An issue where Bluetooth applications built with the ``nordic-bt-rpc`` snippet (in the :ref:`ble_rpc` configuration) did not work on the nRF54H20 devices due to incorrect memory mapping.

* Removed the ``HCI_LE_Read_Local_P-256_Public_Key`` and ``HCI_LE_Generate_DHKey`` commands emulation from the HCI driver.

Bluetooth Mesh
--------------

* Added:

  * The key importer functionality (:kconfig:option:`CONFIG_BT_MESH_KEY_IMPORTER`).
  * A note to the :ref:`dfu_over_ble` page about a need to disable the application settings erase option in the `nRF Connect for Mobile`_ and `nRF Connect Device Manager`_ mobile applications when performing P2P FOTA over Bluetooth Low Energy.
  * Added mesh-specific documentation regarding trusted storage.

* Updated the default value for the :kconfig:option:`CONFIG_MBEDTLS_HEAP_SIZE` Kconfig option if :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_CONF` is selected.
  The :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_CONF` Kconfig option increases the number of keys used by the mesh stack and the value of the :kconfig:option:`CONFIG_MBEDTLS_HEAP_SIZE` Kconfig option needs to be increased accordingly.
* Deprecated the :kconfig:option:`CONFIG_BT_MESH_USES_TINYCYPT` Kconfig option.
  It is not recommended to use this Kconfig option for future designs.
  For platforms that support TF-M, the :kconfig:option:`CONFIG_BT_MESH_USES_TFM_PSA` Kconfig option is used by default.
  For platforms that do not support TF-M, the :kconfig:option:`CONFIG_BT_MESH_USES_MBEDTLS_PSA` Kconfig option is used by default.
* Removed experimental flags for TF-M PSA and Mbed TLS PSA.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * Loading of radio trims and a fix of a hardware errata for the nRF54H20 SoC to improve the RF performance.
  * Workaround for the hardware errata HMPAN-216 for the nRF54H20 SoC.

Matter
------

* Added:

  * A new documentation page :ref:`ug_matter_group_communication` in the :ref:`ug_matter_intro_overview`.
  * A new page on :ref:`ug_matter_creating_custom_cluster`.
  * A description for the new :ref:`ug_matter_gs_tools_matter_west_commands_append` within the :ref:`ug_matter_gs_tools_matter_west_commands` page.
  * New arguments to the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool_gui` to provide a custom cache directory and add new clusters to Matter Data Model.
  * :ref:`ug_matter_debug_snippet`.
  * Storing Matter key materials in the :ref:`matter_platforms_security_kmu`.
  * A new section :ref:`ug_matter_device_low_power_calibration_period` in the :ref:`ug_matter_device_low_power_configuration` page.
  * A new section :ref:`ug_matter_gs_tools_opp` in the :ref:`ug_matter_gs_tools` page.
  * A new overview page for :ref:`ug_matter_gs_tools_matter_cluster_editor`.
  * Released the first preview version of the Matter Cluster Editor app.
    The app allows you to create and edit Matter Cluster files or create an extension to the existing one.
    The app is available in release artifacts.

* Updated:

  * By disabling the :ref:`mpsl` before performing a factory reset to speed up the process.
  * The :ref:`ug_matter_device_low_power_configuration` page to mention the `nWP049 - Matter over Thread: Power consumption and battery life`_ and `Online Power Profiler for Matter over Thread`_ as useful resources in optimizing the power consumption of a Matter device.
  * The general documentation on secure storage by moving it to the :ref:`secure_storage_in_ncs` page and :ref:`trusted_storage_readme` library documentation.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``5fd234d4f14e1225533eaea85854f160bbd0fd55`` commit from the ``v1.4-branch``.
The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Enhanced Setup Flow that allows the standard Matter commissioning process to enable display and acknowledgment of device makers' legal terms and conditions before the device setup.
  * Onboarding Payload in NFC tags.
  * Large messages over TCP.
  * New ``kFactoryReset`` event that is posted during a factory reset.
    The application can register a handler and perform additional cleanup

Thread
------

* Added:

  * Support for storing the Thread key materials in the :ref:`ug_ot_thread_security_kmu`.
  * The :ref:`ug_ot_thread_security` user guide describing the security features of the OpenThread implementation in the |NCS|.

Zigbee
------

* Removed all Zigbee resources.
  They are now available as separate `Zigbee R22`_ and `Zigbee R23`_ add-on repositories.

Wi-Fi
-----

* Updated:

  * Throughputs for Wi-Fi usage profiles.
  * The Wi-Fi credential shell, by renaming it from ``wifi_cred`` to ``wifi cred``.
  * The :ref:`ug_wifi_regulatory_certification` documentation by moving it to the :ref:`ug_wifi` protocol page.

Applications
============

* Added the new :ref:`hpf_mspi_example` application.
* Removed the Asset Tracker v2 application.
  For the development of asset tracking applications, refer to the `Asset Tracker Template <Asset Tracker Template_>`_.

  The factory-programmed Asset Tracker v2 firmware is still available to program the nRF91xx DKs using the `Programmer app`_, `Quick Start app`_, and the `Cellular Monitor app`_.

* Renamed the SDP GPIO application to :ref:`hpf_gpio_example`.

IPC radio firmware
------------------

* Updated:

  * The application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system in all devices that contain MRAM, such as the nRF54H Series devices.
  * The documentation of applications and samples that use the IPC radio firmware as a :ref:`companion component <companion_components>` to mention its usage when built with :ref:`configuration_system_overview_sysbuild`.

* Fixed a performance issue where the :ref:`ipc_radio` application could drop HCI packets in case of high data traffic.

Machine learning
----------------

* Updated:

  * The application to enable the :ref:`Zephyr Memory Storage (ZMS) <zephyr:zms_api>` file system for the :zephyr:board:`nrf54h20dk` board target.
  * The Edge Impulse URI configuration to use the new model location.

Matter bridge
-------------

* Updated by enabling Link Time Optimization (LTO) by default for the ``release`` configuration.
* Removed support for the nRF54H20 devices.

nRF5340 Audio
-------------

* Added more information on new :ref:`DNs and QDIDs <nrf5340_audio_dns_and_qdids>`.

* Updated:

  * The documentation for :ref:`nrf53_audio_app_building` with cross-links and additional information.
  * The :file:`buildprog.py` script is an app-specific script for building and programming multiple kits and cores with various audio application configurations.
    The script will be deprecated in a future release.
    The audio applications will gradually shift to using only standard tools for building and programming development kits.
  * The :ref:`nRF5340 Audio application\'s <nrf53_audio_app>` :ref:`script for building and programming <nrf53_audio_app_building_script>` now builds into a directory for each transport, device type, core, and version combination.
  * The build system to use overlay files for each of the four applications instead of using :file:`Kconfig.default`.
  * The :file:`buildprog.py` script to demand argument ``--transport`` to set either ``unicast`` or ``broadcast``.

* Fixed:

  * The static random address for the broadcast source and unicast server.
  * The time sync issue that occasionally caused a 1 ms difference between the Left/Right headset.

nRF Desktop
-----------

* Added:

  * System power management for the :zephyr:board:`nrf54h20dk` board target on the application and radio cores.
  * Application configurations for the nRF54L05 and nRF54L10 SoCs (emulated on the nRF54L15 DK).
    The configurations are supported through ``nrf54l15dk/nrf54l10/cpuapp`` and ``nrf54l15dk/nrf54l05/cpuapp`` board targets.
    For details, see the :ref:`nrf_desktop_board_configuration`.
  * The ``dongle_small`` configuration for the nRF52833 DK.
    The configuration enables logs and mimics the dongle configuration used for small SoCs.
  * Requirement for zero latency in Zephyr's :ref:`zephyr:pm-system` while USB is active (:ref:`CONFIG_DESKTOP_USB_PM_REQ_NO_PM_LATENCY <config_desktop_app_options>` Kconfig option of the :ref:`nrf_desktop_usb_state_pm`).
    The feature is enabled by default if Zephyr power management (:kconfig:option:`CONFIG_PM`) is enabled.
    It prevents entering power states that introduce wakeup latency and ensures high performance.
  * Static Partition Manager memory maps for single-image configurations (without bootloader and separate radio/network core image).
    In the |NCS|, the Partition Manager is enabled by default for single-image sysbuild builds.
    The static memory map ensures control over settings partition placement and size.
    The introduced static memory maps might not be consistent with the ``storage_partition`` defined by the board-level DTS configuration.
  * Support for GATT long (reliable) writes (:kconfig:option:`CONFIG_BT_ATT_PREPARE_COUNT`) to Fast Pair and Works With ChromeBook (WWCB) configurations.
    This allows performing :ref:`fwupd <nrf_desktop_fwupd>` DFU image upload over Bluetooth LE with GATT clients that do not perform MTU exchange (for example, ChromeOS using the Floss Bluetooth stack).
  * The ``dongle`` and ``release_dongle`` application configurations for the nRF54H20 DK (``nrf54h20dk/nrf54h20/cpuapp``).
    The configurations act as a HID dongle.

* Updated:

  * RTT (:kconfig:option:`CONFIG_USE_SEGGER_RTT`) is disabled in the MCUboot configuration of the nRF52840 DK (`mcuboot_smp` file suffix).
    Using RTT for logs in both the application and the bootloader leads to crashes.
    The MCUboot bootloader provides logs over UART.
  * The :ref:`nrf_desktop_failsafe` to use the Zephyr :ref:`zephyr:hwinfo_api` driver for getting and clearing the reset reason information (see the :c:func:`hwinfo_get_reset_cause` and :c:func:`hwinfo_clear_reset_cause` functions).
    The Zephyr :ref:`zephyr:hwinfo_api` driver replaces the dependency on the nrfx reset reason helper (see the :c:func:`nrfx_reset_reason_get` and :c:func:`nrfx_reset_reason_clear` functions).
  * The ``release`` configuration for the :zephyr:board:`nrf54h20dk` board target to enable the :ref:`nrf_desktop_failsafe` (see the :ref:`CONFIG_DESKTOP_FAILSAFE_ENABLE <config_desktop_app_options>` Kconfig option).
  * By enabling Link Time Optimization (:kconfig:option:`CONFIG_LTO` and :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION`) by default for an nRF Desktop application image.
    LTO was also explicitly enabled in configurations of other images built by sysbuild (bootloader, network core image).
  * Application configurations for nRF54L05, nRF54L10, and nRF54L15 SoCs to use Fast Pair PSA cryptography (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).
    Using PSA cryptography improves security and reduces memory footprint.
    Also, increased the size of the Bluetooth receiving thread stack (:kconfig:option:`CONFIG_BT_RX_STACK_SIZE`) to prevent stack overflows.
  * Application configurations for the nRF52820 SoC to reduce memory footprint:

    * Disabled Bluetooth long workqueue (:kconfig:option:`CONFIG_BT_LONG_WQ`).
    * Limited the number of key slots in the PSA Crypto core to 10 (:kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`).

  * Application configurations for HID peripherals by increasing the following thread stack sizes to prevent stack overflows during the :c:func:`settings_load` operation:

    * The system workqueue thread stack (:kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`).
    * The CAF settings loader thread stack (:kconfig:option:`CONFIG_CAF_SETTINGS_LOADER_THREAD_STACK_SIZE`).

    This change results from the Bluetooth subsystem transition to the PSA cryptographic API.
    The GATT database hash calculation now requires a larger stack size.

  * Support for Bluetooth LE legacy pairing is no longer enabled by default, because it is not secure.
    Using Bluetooth LE legacy pairing introduces, among others, a risk of passive eavesdropping.
    Supporting Bluetooth LE legacy pairing makes devices vulnerable to downgrade attacks.
    The :kconfig:option:`CONFIG_BT_SMP_SC_PAIR_ONLY` Kconfig option is enabled by default in Zephyr.
    If you still need to support the Bluetooth LE legacy pairing, you need to disable the option in the configuration.
  * :ref:`nrf_desktop_hid_state` and :ref:`nrf_desktop_fn_keys` to use :c:func:`bsearch` implementation from the C library.
    This simplifies maintenance and allows you to use Picolibc (:kconfig:option:`CONFIG_PICOLIBC`).
  * The IPC radio image configurations of the nRF5340 DK to use Picolibc (:kconfig:option:`CONFIG_PICOLIBC`).
    This aligns the configurations with the IPC radio image configurations of the nRF54H20 DK.
    Picolibc is used by default in Zephyr.
  * The nRF Desktop application image configurations to use Picolibc (:kconfig:option:`CONFIG_PICOLIBC`) by default.
    Using the minimal libc implementation (:kconfig:option:`CONFIG_MINIMAL_LIBC`) no longer decreases the memory footprint of the application image for most of the configurations.
  * By enabling :ref:`nrf_desktop_usb_state_sof_synchronization` (:ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option) by default on the nRF54H Series SoC (:kconfig:option:`CONFIG_SOC_SERIES_NRF54HX`).
    The negative impact of USB polling jitter is more visible in case of USB High-Speed.
  * The Fast Pair sysbuild configurations to align the application with the sysbuild Kconfig changes for controlling the Fast Pair provisioning process.
    The Nordic device models intended for demonstration purposes are now supplied by default in the nRF Desktop Fast Pair configurations.
  * The :ref:`nrf_desktop_dvfs` to no longer consume the :c:struct:`ble_peer_conn_params_event` event.
    This allows to propagate the event to further listeners of the same or lower priority.
    This prevents an issue where :ref:`nrf_desktop_ble_latency` is not informed about the connection parameter update (it might cause missing connection latency updates).
  * The Low Latency Packet Mode (LLPM) dependency in the :ref:`nrf_desktop_ble_conn_params`.
    The module relies on the :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM` Kconfig option.
    This allows using the module also when the Bluetooth LE controller is not part of the main application.
  * By enabling the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT <config_desktop_app_options>` Kconfig option for the nRF54H20 DK.
    The option mitigates HID report rate drops during DFU image transfer through the nRF Desktop dongle.
  * By explicitly enabling the :kconfig:option:`CONFIG_BT_CTLR_ASSERT_HANDLER` Kconfig option in IPC radio image configurations of the nRF54H20 DK.
    This is done to use an assertion handler defined by the IPC radio image.
  * By disabling the UDC DWC2 DMA support (:kconfig:option:`CONFIG_UDC_DWC2_DMA`) on the nRF54H20 DK.
    The DMA support is experimental, and disabling the feature improves USB HID stability.
    Since nRF Desktop uses only small HID reports (report size is smaller than 64 bytes), the DMA does not improve performance.
  * The nRF Desktop device names to remove the``52`` infix, because the nRF Desktop application supports other SoC Series also.
    This change breaks backwards compatibility.
    Peripherals using firmware from the |NCS| v3.0.0 (or newer) will not pair with dongles using firmware from an older |NCS| release and the other way around.
    The HID configurator script has also been aligned to the new naming scheme.

* Removed:

  * Application configurations for the nRF52810 Desktop Mouse board (``nrf52810dmouse/nrf52810``).
    The board is no longer supported in the |NCS|.

Serial LTE modem
----------------

* Added:

  * A new page :ref:`slm_as_linux_modem`.
  * An overlay file :file:`overlay-memfault.conf` to enable Memfault.
    See :ref:`mod_memfault` for more information about Memfault features in |NCS|.

* Updated the application to use the :ref:`lib_downloader` library instead of the deprecated Download client library.

Thingy:53: Matter weather station
---------------------------------

* Updated by enabling Link Time Optimization (LTO) by default for the ``release`` configuration.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added

  * Support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in the following samples:

    * :ref:`central_and_peripheral_hrs`
    * :ref:`central_bas`
    * :ref:`bluetooth_central_hids`
    * :ref:`bluetooth_central_hr_coded`
    * :ref:`bluetooth_central_dfu_smp`
    * :ref:`central_uart`
    * :ref:`multiple_adv_sets`
    * :ref:`peripheral_bms`
    * :ref:`peripheral_cgms`
    * :ref:`peripheral_cts_client`
    * :ref:`peripheral_gatt_dm`
    * :ref:`peripheral_hids_keyboard`
    * :ref:`peripheral_hr_coded`
    * :ref:`peripheral_mds`
    * :ref:`peripheral_nfc_pairing`
    * :ref:`peripheral_rscs`
    * :ref:`peripheral_status`
    * :ref:`shell_bt_nus`
    * :ref:`ble_throughput`

  * The Advertising Coding Selection feature to the following samples:

    * :ref:`bluetooth_central_hr_coded`
    * :ref:`peripheral_hr_coded`

* Updated:

  * The configurations of the non-secure ``nrf5340dk/nrf5340/cpuapp/ns`` board target in the following samples to properly use the TF-M profile instead of the predefined minimal TF-M profile:

    * :ref:`bluetooth_central_hids`
    * :ref:`peripheral_hids_keyboard`
    * :ref:`peripheral_hids_mouse`

    This change results from the Bluetooth subsystem transition to the PSA cryptographic standard.
    The Bluetooth stack can now use the PSA crypto API in the non-secure domain as all necessary TF-M partitions are configured properly.

  * The configurations of the following samples by increasing the main thread stack size (:kconfig:option:`CONFIG_MAIN_STACK_SIZE`) to prevent stack overflows:

    * :ref:`bluetooth_central_hids`
    * :ref:`peripheral_hids_keyboard`
    * :ref:`peripheral_hids_mouse`

    This change results from the Bluetooth subsystem transition to the PSA cryptographic API.

  * The following samples to use LE Secure Connection pairing (:kconfig:option:`CONFIG_BT_SMP_SC_PAIR_ONLY`):

    * :ref:`peripheral_gatt_dm`
    * :ref:`peripheral_mds`
    * :ref:`peripheral_cts_client`

* :ref:`direct_test_mode` sample:

  * Added:

    * Loading of radio trims and a fix of a hardware errata for the nRF54H20 SoC to improve the RF performance.
    * Workaround for the hardware errata HMPAN-216 for the nRF54H20 SoC.

* :ref:`central_uart` sample:

  * Added reconnection to bonded devices based on their address.

* :ref:`peripheral_hids_keyboard` sample:

  * Fixed the issue with the :kconfig:option:`CONFIG_NFC_OOB_PAIRING` Kconfig option that is defined at the sample level and could not be enabled due to the unmet dependency on the :kconfig:option:`CONFIG_HAS_HW_NRF_NFCT` Kconfig option.
    The issue is resolved by enabling the ``nfct`` node in the sample devicetree configuration, which sets the :kconfig:option:`CONFIG_HAS_HW_NRF_NFCT` Kconfig option to the expected value.

* :ref:`power_profiling` sample:

  * Added the :kconfig:option:`CONFIG_BT_POWER_PROFILING_NFC_DISABLED` Kconfig option to reduce power consumption by disabling the NFC.

* :ref:`peripheral_uart` sample:

  * Removed support for the nRF52805, nRF52810, and nRF52811 devices.

Bluetooth Mesh samples
----------------------

* Added:

  * Support for nRF54L10 in the following samples:

    * :ref:`bluetooth_mesh_sensor_client`
    * :ref:`bluetooth_mesh_sensor_server`
    * :ref:`bluetooth_ble_peripheral_lbs_coex`
    * :ref:`bt_mesh_chat`
    * :ref:`bluetooth_mesh_light_switch`
    * :ref:`bluetooth_mesh_silvair_enocean`
    * :ref:`bluetooth_mesh_light_dim`
    * :ref:`bluetooth_mesh_light`
    * :ref:`ble_mesh_dfu_target`
    * :ref:`bluetooth_mesh_light_lc`
    * :ref:`ble_mesh_dfu_distributor`

  * Support for nRF54L05 in the following samples:

    * :ref:`bluetooth_mesh_sensor_client`
    * :ref:`bluetooth_mesh_sensor_server`
    * :ref:`bluetooth_ble_peripheral_lbs_coex`
    * :ref:`bt_mesh_chat`
    * :ref:`bluetooth_mesh_light_switch`
    * :ref:`bluetooth_mesh_silvair_enocean`
    * :ref:`bluetooth_mesh_light_dim`
    * :ref:`bluetooth_mesh_light`
    * :ref:`bluetooth_mesh_light_lc`

* Updated:

  * The board configuration files for nRF54L15, nRF54L10, and nRF54L05 by increasing the values of :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` and :kconfig:option:`CONFIG_BT_RX_STACK_SIZE` Kconfig options to prevent stack overflows when :kconfig:option:`CONFIG_BT_MESH_USES_MBEDTLS_PSA` is selected.
  * The following samples to include the value of the :kconfig:option:`CONFIG_BT_COMPANY_ID` option in the Firmware ID:

    * :ref:`ble_mesh_dfu_distributor`
    * :ref:`ble_mesh_dfu_target`

  * :ref:`bluetooth_mesh_light_lc` sample by disabling the friend feature when the sample is compiled for the :zephyr:board:`nrf52dk` board target to increase the amount of RAM available for the application.

Bluetooth Fast Pair samples
---------------------------

* Added experimental support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in all Fast Pair samples.

* Updated:

  * The non-secure target (``nrf5340dk/nrf5340/cpuapp/ns`` and ``thingy53/nrf5340/cpuapp/ns``) configurations of all Fast Pair samples to use configurable TF-M profile instead of the predefined minimal TF-M profile.
    This change results from the Bluetooth subsystem transition to the PSA cryptographic standard.
    The Bluetooth stack can now use the PSA crypto API in the non-secure domain as all necessary TF-M partitions are configured properly.
  * The configuration of all Fast Pair samples by increasing the following thread stack sizes to prevent stack overflows:

    * The system workqueue thread stack (:kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`).
    * The Bluetooth receiving thread stack (:kconfig:option:`CONFIG_BT_RX_STACK_SIZE`).

    This change results from the Bluetooth subsystem transition to the PSA cryptographic API.
  * The sysbuild configurations in samples to align them with the sysbuild Kconfig changes for controlling the Fast Pair provisioning process.

* Removed a separate workqueue for connection TX notify processing (:kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ`) from configurations.
  The MPSL flash synchronization issue (NCSDK-29354 in the :ref:`known_issues`) is fixed.
  The workaround is no longer needed.

* :ref:`fast_pair_locator_tag` sample:

  * Added:

    * Experimental support for the :zephyr:board:`nrf54h20dk` board target.
    * Support for firmware update intents on the Android platform.
      The configuration of the default device model in the Google Nearby Console has been updated to properly support this feature.
      See the sample documentation for more information.
      Integrated the new connection authentication callback from the FMDN module and the Device Information Service (DIS) to support firmware version read operation over the Firmware Revision characteristic.
      Updated the sample documentation with a new section :ref:`android_notifications_fastpair` that contains the description of this feature and a :ref:`new testing procedure <fast_pair_locator_tag_testing_fw_update_notifications>` that demonstrates how this feature works.
      For further details on the Android intent feature for firmware updates, see the :ref:`ug_bt_fast_pair_provisioning_register_firmware_update_intent` section of the Fast Pair integration guide.

  * Updated:

    * The partition layout for the ``nrf5340dk/nrf5340/cpuapp/ns`` and ``thingy53/nrf5340/cpuapp/ns`` board targets to accommodate the partitions needed due to a change in the TF-M profile configuration.
    * The ``debug`` (default) configuration of the main image to enable the Link Time Optimization (LTO) with the :kconfig:option:`CONFIG_LTO` Kconfig option.
      This change ensures consistency with the sample release configuration that has the LTO feature enabled by default.
    * The ``nrf54l15dk/nrf54l15/cpuapp`` board target configuration to enable hardware cryptography for the MCUboot bootloader.
      The application image is verified using a pure ED25519 signature and the public key used by MCUboot for validating the application image is securely stored in the Key Management Unit (KMU) hardware peripheral.
      Support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets, which is added to this sample in this release iteration, also includes the same MCUboot bootloader configuration with the hardware cryptography enabled.

      The change modifies the memory partition layout for the ``nrf54l15dk/nrf54l15/cpuapp`` board target and changes the MCUboot image signing algorithm.
      Because of that, the application images built for the ``nrf54l15dk/nrf54l15/cpuapp`` board target from this |NCS| release are not compatible with the MCUboot bootloader built from previous releases.
      It is highly recommended to use hardware cryptography for the nRF54L Series SoC for improved security.
    * The configurations for board targets with the MCUboot bootloader support to use a non-default signature key file (the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` Kconfig option).
      The application uses a unique signature key file for each board target, which is defined at the same directory level as the target sysbuild configuration file.
      This modification changes the key set that is used by the MCUboot DFU solution.
      Because of that, the application images from this |NCS| release are not compatible with the MCUboot bootloader built from previous releases.
    * The MCUboot DFU signature type to the Elliptic curve digital signatures with curve P-256 (ECDSA P256 - the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256` Kconfig option) for the ``nrf52840dk/nrf52840`` board target.
      This is done to use Cryptocell 310 for image signature verification.
      This change breaks the backwards compatibility, as performing DFU from an old signature type to a new one is impossible.

Cellular samples
----------------

* Updated the following samples to use the :ref:`lib_downloader` library instead of the Download client library:

  * :ref:`http_application_update_sample`
  * :ref:`http_modem_delta_update_sample`
  * :ref:`http_modem_full_update_sample`
  * :ref:`location_sample`
  * :ref:`lwm2m_carrier`
  * :ref:`lwm2m_client`
  * :ref:`modem_shell_application`
  * :ref:`nrf_cloud_multi_service`
  * :ref:`nrf_cloud_rest_fota`

* :ref:`modem_shell_application` sample:

  * Added support for setting and getting socket options using the ``sock option set`` and ``sock option get`` commands.
  * Removed the ``CONFIG_MOSH_LINK`` Kconfig option.
    The link control functionality is now always enabled and cannot be disabled.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed:

    * Wrong header naming in :file:`provisioning_support.h` file, which was causing build errors when :file:`sample_reboot.h` was included in other source files.
    * An issue with an uninitialized variable in the :c:func:`handle_at_cmd_requests` function.
    * An issue with a very small :kconfig:option:`CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE` Kconfig value in the :file:`overlay-coap_nrf_provisioning.conf` file.
    * Slow Wi-Fi connectivity startup by selecting ``TFM_SFN`` instead of ``TFM_IPC``.
    * The size of TLS credentials buffer for Wi-Fi connectivity to allow installing both AWS and CoAP CA certificates.
    * Build issues with Wi-Fi configuration using CoAP.

* :ref:`lte_sensor_gateway` sample:

  * Fixed an issue with devicetree configuration after HCI updates in `sdk-zephyr`_.

* :ref:`pdn_sample` sample:

  * Added dynamic PDN information.

Cryptography samples
--------------------

* :ref:`crypto_tls` sample:

  * Added support for the TLS v1.3.

Edge Impulse samples
--------------------

* Added support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in all Edge Impulse samples.

Enhanced ShockBurst samples
---------------------------

* Added support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in all ESB samples.

Matter samples
--------------

* Added :ref:`matter_manufacturer_specific_sample` sample that demonstrates an implementation of custom manufacturer-specific clusters used by the application layer.

* :ref:`matter_template_sample` sample:

  * Updated:

    * The documentation with instructions on how to build the sample on the nRF54L15 DK with support for Matter OTA DFU and DFU over Bluetooth SMP, and using internal RRAM only.
    * Link Time Optimization (LTO) to be enabled by default for the ``release`` configuration and ``nrf7002dk/nrf5340/cpuapp`` board target.

  * Removed support for nRF54H20 devices.

* :ref:`matter_lock_sample` sample:

  * Updated the API of ``AppTask``, ``BoltLockManager``, and ``AccessManager`` to provide additional information for the ``LockOperation`` event.
  * Removed support for nRF54H20 devices.

Networking samples
------------------

* Added support for nRF7002-EB II with ``nrf54l15dk/nrf54l15/cpuapp`` board target in the following samples:

  * :ref:`aws_iot`
  * :ref:`download_sample`
  * :ref:`net_coap_client_sample`
  * :ref:`https_client`
  * :ref:`mqtt_sample`
  * :ref:`udp_sample`


* Updated:

  * The :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option value to ``1280`` for all networking samples that had it set to a lower value.
    This is a requirement from Zephyr and removes a build warning.
  * The following samples to use the :ref:`lib_downloader` library instead of the Download client library:

    * :ref:`aws_iot`
    * :ref:`azure_iot_hub`
    * :ref:`download_sample`

NFC samples
-----------

* Added support for the ``nrf54l15dk/nrf54l05/cpuapp`` and ``nrf54l15dk/nrf54l10/cpuapp`` board targets in the following samples:

  * :ref:`record_text`
  * :ref:`record_launch_app`
  * :ref:`nfc_shell`
  * :ref:`nrf-nfc-system-off-sample`
  * :ref:`nfc_tnep_tag`
  * :ref:`writable_ndef_msg`

nRF5340 samples
---------------

* Removed the nRF5340: Multiprotocol RPMsg sample.
  Use the :ref:`ipc_radio` application instead.
* :ref:`smp_svr_ext_xip` sample:

  * Added the nRF5340 non-secure (TF-M) target.

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added:

    * Loading of radio trims and a fix of a hardware errata for the nRF54H20 SoC to improve the RF performance.
    * Workaround for the hardware errata HMPAN-216 for the nRF54H20 SoC.

PMIC samples
------------

* Added:

  * The :ref:`npm2100_one_button` sample that demonstrates how to support wake-up, shutdown, and user interactions through a single button connected to the nPM2100 PMIC.
  * The :ref:`npm2100_fuel_gauge` sample that demonstrates how to calculate the battery state of charge of primary cell batteries using the :ref:`nrfxlib:nrf_fuel_gauge`.

* :ref:`nPM1300: Fuel gauge <npm13xx_fuel_gauge>` sample:

  * Updated to accommodate API changes in nRF Fuel Gauge library v1.0.0.

SDFW samples
------------

* Removed the SDFW: Service Framework Client sample as all services demonstrated by the sample have been removed.

SUIT samples
------------

* Added the ``nrf54h_suit_ab_sample`` sample that demonstrates how to perform A/B updates using SUIT manifests.

* ``nrf54h_suit_sample`` sample:

  * Updated:

    * The memory maps to cover the entire available MRAM memory.
    * The memory maps to place recovery firmware on lower addresses than the main firmware.
    * By enabling secure entropy source in all main Bluetooth-enabled sample variants (except recovery firmware).
    * By extending the manifests to process the ``suit-payload-fetch`` sequence of the Nordic top update candidate.
    * By extending the manifests with build-time checks for consistency between MPI and envelope signing configuration.
    * By migrating to the new JEDEC SPI-NOR flash driver that supports octal SPI transfer mode.


Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_psa_template` sample:

  * Added support for the following attestation token fields:

    * Profile definition
    * PSA certificate reference (optional), configured using the :kconfig:option:`SB_CONFIG_TFM_OTP_PSA_CERTIFICATE_REFERENCE` sysbuild Kconfig option
    * Verification service URL (optional), configured using the :kconfig:option:`CONFIG_TFM_ATTEST_VERIFICATION_SERVICE_URL` Kconfig option

* :ref:`tfm_secure_peripheral_partition` sample:

  * Updated documentation with information about how to access other TF-M partitions from the secure partition.

Thread samples
--------------

* :ref:`ot_cli_sample` sample:

  * Removed support for the nRF54H20 DK.

Zigbee samples
--------------

* Removed all Zigbee samples.
  They are now available as separate `Zigbee R22`_ and `Zigbee R23`_ add-on repositories.

Wi-Fi samples
-------------

* Added support for nRF7002-EB II with nRF54 Series devices in the following samples:

  * :ref:`wifi_station_sample`
  * :ref:`wifi_scan_sample`
  * :ref:`wifi_shell_sample`
  * :ref:`wifi_radio_test_sd`
  * :ref:`wifi_radio_test`
  * :ref:`wifi_softap_sample`
  * :ref:`wifi_twt_sample`
  * :ref:`wifi_offloaded_raw_tx_packet_sample`
  * :ref:`wifi_raw_tx_packet_sample`
  * :ref:`ble_wifi_provision`

* :ref:`wifi_station_sample` sample:

  * Added an :file:`overlay-zperf.conf` overlay file for :ref:`performance benchmarking and memory footprint analysis <wifi_sta_performance_testing_memory_footprint>`.

* :ref:`wifi_radiotest_samples`:

  * Added:

    * The :ref:`wifi_radio_test_sd` sample to demonstrate the Wi-Fi and Bluetooth LE radio test running on the application core.
    * The ``wifi_radio_test get_voltage`` command to read battery voltage.

  * Updated:

    * The :ref:`wifi_radio_test` sample is now moved to :file:`samples/wifi/radio_test/multi_domain`.

* :ref:`wifi_shell_sample` sample:

  * Updated by modifying support for storing the nRF70 firmware patches in external flash using the :ref:`partition_manager`.

* :ref:`wifi_wfa_qt_app_sample`:

  * Added a new section :ref:`wifi_qt_configuration_settings`.

Other samples
-------------

* Added :ref:`app_jwt_sample` sample that demonstrates how the application core can generate a signed JWT.

* :ref:`coremark_sample` sample:

  * Added:

    * Support for the nRF54L05 and nRF54L10 SoCs (emulated on nRF54L15 DK).
    * FLPR core support for the :zephyr:board:`nrf54l15dk` and :zephyr:board:`nrf54h20dk` board targets.

  * Removed the following compiler options that were set in the :kconfig:option:`CONFIG_COMPILER_OPT` Kconfig option:

    * ``-fno-pie``
    * ``-fno-pic``
    * ``-ffunction-sections``
    * ``-fdata-sections``

    These options are enabled by default in Zephyr and do not need to be set with the dedicated Kconfig option.

* :ref:`caf_sensor_manager_sample` sample:

  * Added low power configuration for the :zephyr:board:`nrf54h20dk` board target.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added a ``flash_ipuc`` that allows to manage SUIT IPUC memory through the Zephyr flash API.

Wi-Fi drivers
-------------

* Added:

  * Advanced debug shell for reading and writing registers and memory of the nRF70 Series chip.
    The debug shell can be enabled using the :kconfig:option:`CONFIG_NRF70_DEBUG_SHELL` Kconfig option.
  * A new shell command ``nrf70 util rpu_stats_mem`` to retrieve the RPU statistics even when the RPU processors are not functional (for example, if the LMAC processor has crashed).

* Updated the ``wifi_util`` shell command, which is now renamed to ``nrf70 util``.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated:

    * The library to v3.7.0.
      See the :ref:`liblwm2m_carrier_changelog` for detailed information.
    * The glue to use the :ref:`lib_downloader` library instead of the deprecated Download client library.

Bluetooth libraries and services
--------------------------------

* Added the :ref:`cs_de_readme` library.

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * A restriction on the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER` Kconfig option in the Find My Device Network (FMDN) extension configuration.
      You must set this Kconfig option now to ``0`` at minimum as the Fast Pair specification requires that the conducted Bluetooth transmit power for FMDN advertisements must not be lower than 0 dBm.
    * A new information callback, :c:member:`bt_fast_pair_fmdn_info_cb.conn_authenticated`, to the FMDN extension API.
      In the FMDN context, this change is required to support firmware update intents on the Android platform.
      For further details on the Android intent feature for firmware updates, see the :ref:`ug_bt_fast_pair_provisioning_register_firmware_update_intent` section in the Fast Pair integration guide.
    * A workaround for the issue where the FMDN clock value might not be correctly set after the system reboot for nRF54L Series devices.
      For details, see the NCSDK-32268 known issue in the :ref:`known_issues` page.
    * A new function :c:func:`bt_fast_pair_fmdn_is_provisioned` for the FMDN extension API.
      This function can be used to synchronously check the current FMDN provisioning state.
      For more details, see the :ref:`ug_bt_fast_pair_gatt_service_fmdn_info_callbacks_provisioning_state` section in the Fast Pair integration guide.

  * Updated:

    * The :c:func:`bt_fast_pair_info_cb_register` API to allow registration of multiple callbacks.
    * The Fast Pair sysbuild Kconfig options.
      The :kconfig:option:`SB_CONFIG_BT_FAST_PAIR` Kconfig option is replaced with the :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_MODEL_ID` and :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY`.
    * The method of supplying the Fast Pair Model ID and Anti-Spoofing Private Key to generate the Fast Pair provisioning data HEX file.
      The ``FP_MODEL_ID`` and ``FP_ANTI_SPOOFING_KEY`` CMake variables are replaced by the corresponding :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_MODEL_ID` and :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY` Kconfig options.
    * The automatically generated ``bt_fast_pair`` partition definition (in the :file:`subsys/partition_manager/pm.yml.bt_fast_pair` file) to work correctly when building with TF-M.
    * The behavior of the :c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed` callback.
      The callback no longer reports the initial provisioning state after the Fast Pair subsystem is enabled with the :c:func:`bt_fast_pair_enable` function call.
      See the :ref:`migration guide <migration_3.0_recommended>` for mandatory changes and the :ref:`ug_bt_fast_pair_gatt_service_fmdn_info_callbacks_provisioning_state` section in the Fast Pair integration guide for the description on how to track the FMDN provisioning state with the new approach.

  * Removed the sysbuild control over the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option that is defined in the main (default) image.
    Sysbuild no longer sets the value of this Kconfig option.

* :ref:`wifi_prov_readme` library:

  * Added a new section :ref:`wifi_provisioning_protocol`.

* :ref:`bt_mesh`:

  * Fixed an issue in the :ref:`bt_mesh_light_ctrl_srv_readme` model to automatically resume the Lightness Controller after recalling a scene (NCSDK-30033 known issue).

Common Application Framework
----------------------------

* :ref:`caf_buttons`:

  * Added:

    * The possibility of using more GPIOs.
      Earlier, only **GPIO0** and **GPIO1** devices were supported.
      Now, the generic solution supports all GPIOs available in the DTS.
    * The :c:struct:`power_off_event` handling to prevent entering system off state with GPIO interrupt disabled.
      Entering system off state with GPIO interrupt disabled would make CAF Buttons unable to trigger wakeup from the system off state on button press.

* :ref:`caf_power_manager`:

  * Updated:

    * The :kconfig:option:`CONFIG_CAF_POWER_MANAGER` Kconfig option to imply the device power management (:kconfig:option:`CONFIG_PM_DEVICE`) instead of selecting it.
      The device power management is not required by the module.
    * The :kconfig:option:`CONFIG_CAF_POWER_MANAGER` Kconfig option to imply device runtime power management (:kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`) for the nRF54H Series SoC (:kconfig:option:`CONFIG_SOC_SERIES_NRF54HX`).
      The feature can be used to reduce the power consumption of device drivers.
      Enabling the device runtime power management also prevents using system-managed device power management (:kconfig:option:`CONFIG_PM_DEVICE_SYSTEM_MANAGED`) by default.
      The system-managed device power management does not work properly with some drivers (for example, nrfx UARTE) and should be avoided.
    * The module implementation to use :c:func:`sys_poweroff` API instead of :c:func:`pm_state_force` API to enter the system off state.
    * The module implementation to integrate the newly introduced :c:struct:`power_off_event`.
      The event is used to inform application modules that system power off (:c:func:`sys_poweroff`) is about to happen.

DFU libraries
-------------

* Added:

  * Support for ``manifest-controlled variables`` that allow to control manifest logic based on previous evaluations as well as store integer values inside the SUIT non-volatile memory region.
  * Support for ``In-place Updateable Components (IPUC)`` that allow to cross memory permission boundaries to update inactive memory regions from the main application.

* :ref:`lib_fmfu_fdev`:

  * Regenerated the zcbor-generated code files using v0.9.0.

* ``subsys_suit``:

   * Added:

     * Support for manifest-controlled variables, that allow to control manifest logic based on previous evaluations as well as store integer values inside the SUIT non-volatile memory region.
     * Support for in-place updateable components (IPUC) that allows to cross memory permission boundaries to update inactive memory regions from the main application.
     * Support for IPUC in SUIT manifests that makes possible the following:

       * To fetch payloads directly into an IPUC.
       * To declare an IPUC as DFU cache area.
       * To trigger Nordic firmware updates from an IPUC-based DFU cache area.

     * Support for IPUC in DFU protocols that makes possible the following:

       * To write into IPUC using SMP image command group.
       * To write into IPUC using SUIT SMP cache raw upload commands.
       * To write into IPUC using SUIT ``dfu_target<lib_dfu_target_suit_style_update>`` library.

     * Possibilities for the following:

       * To copy binaries into radio local RAM memory from SUIT radio manifests.
       * To specify the minimal Nordic top manifest version in the :file:`VERSION` file.
       * To block independent updates of Nordic manifests using the :kconfig:option:`CONFIG_SUIT_NORDIC_TOP_INDEPENDENT_UPDATE_FORBIDDEN` Kconfig option.

   * Updated by moving the MPI configuration from local Kconfig options to sysbuild.

Modem libraries
---------------

* Deprecated the AT parameters library.

* :ref:`pdn_readme` library:

  * Deprecated the :c:func:`pdn_dynamic_params_get` function.
    Use the new function :c:func:`pdn_dynamic_info_get` instead.

* :ref:`lte_lc_readme` library:

  * Added sending of ``LTE_LC_EVT_NEIGHBOR_CELL_MEAS`` event with ``current_cell`` set to ``LTE_LC_CELL_EUTRAN_ID_INVALID`` in case an error occurs while parsing the ``%NCELLMEAS`` notification.
  * Fixed handling of ``%NCELLMEAS`` notification with status 2 (measurement interrupted) and no cells.

* :ref:`modem_key_mgmt` library:

  * Added:

    * The :c:func:`modem_key_mgmt_digest` function that would retrieve the SHA1 digest of a credential from the modem.
    * The :c:func:`modem_key_mgmt_list` function that would retrieve the security tag and type of every credential stored in the modem.

  * Fixed:

    * An issue with the :c:func:`modem_key_mgmt_clear` function where it returned ``-ENOENT`` when the credential was cleared.
    * A race condition in several functions where ``+CMEE`` error notifications could be disabled by one function before the other one got a chance to run its command.
    * An issue with the :c:func:`modem_key_mgmt_clear` function where ``+CMEE`` error notifications were not restored to their original state if the ``AT%CMNG`` AT command failed.
    * The :c:func:`modem_key_mgmt_clear` function to lock the shared scratch buffer.

* :ref:`nrf_modem_lib_readme`:

  * Updated the :ref:`nrf_modem_lib_lte_net_if` to automatically set the actual link :term:`Maximum Transmission Unit (MTU)` on the network interface when PDN connectivity is gained.
  * Fixed a bug where various subsystems would be erroneously initialized during a failed initialization of the library.

* :ref:`lib_location` library:

  * Removed support for HERE location services.

* :ref:`lib_at_host` library:

  * Fixed a bug where AT responses would erroneously be written to the logging UART instead of being written to the chosen ``ncs,at-host-uart`` UART device when the :kconfig:option:`CONFIG_LOG_BACKEND_UART` Kconfig option was set.

* :ref:`modem_info_readme` library:

  * Added:

    * The :c:enum:`modem_info_data_type` type for representing LTE link information data types.
    * The :c:func:`modem_info_data_type_get` function for requesting the data type of the current modem information type.

  * Deprecated the :c:func:`modem_info_type_get` function in favor of the :c:func:`modem_info_data_type_get` function.

* :ref:`lib_modem_slm` library:

  * Updated:

    * By renaming the ``CONFIG_MODEM_SLM_WAKEUP_PIN`` and ``CONFIG_MODEM_SLM_WAKEUP_TIME`` Kconfig options to :kconfig:option:`CONFIG_MODEM_SLM_POWER_PIN` and :kconfig:option:`CONFIG_MODEM_SLM_POWER_PIN_TIME`, respectively.
    * By renaming the ``modem_slm_wake_up`` function to :c:func:`modem_slm_power_pin_toggle`.

Multiprotocol Service Layer libraries
-------------------------------------

* Added:

  * Integration with the nrf2 clock control driver for the nRF54H20 SoC.
  * Integration with Zephyr's system power management for the nRF54H20 SoC.
  * Global domain HSFLL120 320MHz frequency request if MPSL is enabled.
    The high frequency in global domain is required to ensure that fetching instructions from L2-cache and MRAM is as fast as possible.
    It is needed for the radio protocols to operate correctly.
  * MRAM always-on request for scheduled radio events.
    It is needed to avoid MRAM wake-up latency for radio protocols.

Libraries for networking
------------------------

* Added:

  * The :ref:`lib_downloader` library.
  * A backend for the :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` that stores the credentials in the modem, see :kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_NRF_MODEM`.

* Deprecated the Download client library.
  See the :ref:`migration guide <migration_3.0_recommended>` for recommended changes.

* Updated the following libraries to use the :ref:`lib_downloader` library instead of the Download client library:

  * :ref:`lib_nrf_cloud`
  * :ref:`lib_aws_fota`
  * :ref:`lib_azure_fota`
  * :ref:`lib_fota_download`

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed the warning due to missing ``https`` download protocol.

* :ref:`lib_fota_download` library:

  * Added error codes related to unsupported protocol, DFU failures, and invalid configuration.

* :ref:`lib_nrf_cloud` library:

  * Added the :kconfig:option:`CONFIG_NRF_CLOUD` Kconfig option to prevent unintended inclusion of nRF Cloud Kconfig variables in non-nRF Cloud projects.
  * Updated to use the :ref:`lib_app_jwt` library to generate JWT tokens.

Other libraries
---------------

* Added new library :ref:`lib_app_jwt` library.

* :ref:`mod_dm` library:

  * Updated the default timeslot duration to avoid an overstay assert when the ranging failed.

* Removed the unused SDFW services ``echo_service``, ``reset_evt_service``, and ``sdfw_update_service``.

Libraries for Zigbee
--------------------

* Removed Zigbee libraries.
  They are now available as separate `Zigbee R22`_ and `Zigbee R23`_ add-on repositories.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`nrf_desktop_config_channel_script`:

  * Removed HID device type mapping for Development Kits.
    A Development Kit may use various HID roles (depending on configuration).
    Assigning a fixed type for each board might be misleading.
    HID device type is still defined for boards that are always configured as the same HID device type.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Added:

  * Instructions on how to provision the Fast Pair data onto devices without the :ref:`partition_manager` support, specifically for the :zephyr:board:`nrf54h20dk`.
  * Information on how to support the firmware update intent feature on the Android platform.
    Expanded the documentation for the Fast Pair devices with the FMDN extension, which requires additional steps to support this feature.

* Updated:

  * The :ref:`ug_bt_fast_pair_provisioning_register_hex_generation` section that describes how to generate the hex file with the Fast Pair provisioning data.
  * The :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section to align it with recent changes in the sysbuild configuration for Fast Pair.
  * The :ref:`ug_bt_fast_pair_gatt_service_fmdn_info_callbacks_provisioning_state` section with changes to the FMDN API elements that are used for tracking of the FMDN provisioning state.

Memfault integration
--------------------

* Added a new feature to automatically post coredumps to Memfault when network connectivity is available.
  To enable this feature, set the :kconfig:option:`CONFIG_MEMFAULT_NCS_POST_COREDUMP_ON_NETWORK_CONNECTED` Kconfig option to ``y``.
  Only supported for nRF91 Series devices.

* Added a new feature to automatically capture and upload modem traces to Memfault with coredumps upon a crash.
  To enable this feature, set the :kconfig:option:`CONFIG_MEMFAULT_NCS_POST_MODEM_TRACE_ON_COREDUMP` Kconfig option to ``y``.
  Only supported for nRF91 Series devices.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

* Added :ref:`soft_peripherals`.
* Removed the Zboss documentation.
  It is now available in separate `Zigbee R22`_ and `Zigbee R23`_ add-on repositories (depending on the device you are working with).

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``1b2fc096d9a683a7481b13749d01ca8fa78e7afd``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed an issue where an unusable secondary slot was cleared three times instead of once during cleanup.
* Added keys revocation scheme support for nRF54L SoCs, see ``CONFIG_BOOT_KEYS_REVOCATION`` MCUboot Kconfig option.
* Improved time performance of firmware update by usage of best RRAM write operation buffering on nRF54L SoCs.
* Introduced improved swap algorithm: swap-using-offset, see ``CONFIG_BOOT_SWAP_USING_OFFSET`` MCUboot Kconfig option.
* Image compression support has been brought to production quality.
* Experimental support has been added for encrypted DFU with ECIES x25519 encryption using MCUboot for nRF54L SoCs.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``fdeb7350171279d4637c536fcceaad3fbb775392``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline fdeb735017 ^beb733919d

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^fdeb735017

The current |NCS| main branch is based on revision ``fdeb735017`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

The new ZMS settings backend is not backward compatible with the old version.

Using the new backend, you can now enable some performance optimizations using the following Kconfig options:

* :kconfig:option:`CONFIG_SETTINGS_ZMS_LL_CACHE` - Used for caching the linked list nodes related to Settings Key/Value entries.
* :kconfig:option:`CONFIG_SETTINGS_ZMS_LL_CACHE_SIZE` - Specifies the size of the linked list cache (each entry occupies 8 Bytes of RAM).
* :kconfig:option:`CONFIG_SETTINGS_ZMS_NO_LL_DELETE` - Disables deleting the linked list nodes when deleting a Settings Key.
  Use this option only when the application is always using the same Settings Keys.
  When the application uses random Keys, enabling this option could lead to incrementing the linked list nodes without corresponding Keys and cause excessive delays to loading of the Keys.
  Use this option only to accelerate the delete operation for a fixed set of Settings elements.
* :kconfig:option:`CONFIG_SETTINGS_ZMS_LOAD_SUBTREE_PATH` - First loads the subtree path passed in the argument, then continues to load all the Keys in the same subtree if the handler returns a zero value.
* Using the :c:func:`settings_load_one` function to retrieve the value of a known Key is recommended with ZMS as it improves the read performance a lot.
* To get the value length of a Settings entry, it is recommended to use the :c:func:`settings_get_val_len` function.

Documentation
=============

* Added:

  * Extensive documentation on :ref:`Developing with coprocessors <coprocessors_index>`.
    It covers topics related to :ref:`High-Performance Framework (HPF) <hpf_index>`, detailing how to create and integrate software peripherals using coprocessors.
  * New page :ref:`ug_custom_board`.
    This page includes the following subpages:

    * :ref:`defining_custom_board` - Previously located under :ref:`app_boards`.
    * :ref:`programming_custom_board` - New subpage.

  * New page :ref:`thingy53_precompiled` under :ref:`ug_thingy53`.
    This page includes some of the information previously located on the standalone page for Getting started with Nordic Thingy:53.
  * New page :ref:`add_new_led_example` under :ref:`configuring_devicetree`.
    This page includes information previously located in the |nRFVSC| documentation.
  * The :ref:`cellular_psm` page under the :ref:`ug_lte` documentation, and the documentation is now split into subpages.
  * The :ref:`ug_bootloader_main_config` page under the :ref:`app_dfu` documentation.

* Updated:

  * The :ref:`create_application` page with the :ref:`creating_add_on_index` section.
  * The :ref:`ug_nrf91` documentation to use `nRF Util`_ instead of nrfjprog.
  * The :ref:`dm-revisions` section of the :ref:`dm_code_base` page with information about the preview release tag, which replaces the development tag.
  * The :ref:`ug_bt_mesh_configuring` page with the security toolbox section and the key importer functionality.
  * The :ref:`ug_nrf7002_gs` documentation to use `nRF Util`_ instead of nrfjprog.

* Removed:

  * The entire Zigbee protocol, application, and samples documentation.
    It is now available as separate `Zigbee R22`_ and `Zigbee R23`_ add-on repositories.
  * The standalone page for getting started with Nordic Thingy:53.
    The contents of this page have been moved to the :ref:`thingy53_precompiled` page and to the `Programming Nordic Thingy:53 <Programming Nordic Thingy53_>`_ section in the Programmer app documentation.
  * The standalone page for getting started with Nordic Thingy:91.
    The contents of this page are covered by the `Cellular IoT Fundamentals course`_ in the `Nordic Developer Academy`_.
    The part about connecting the prototyping platform to nRF Cloud is now a standalone :ref:`thingy91_connect_to_cloud` page in the :ref:`thingy91_ug_intro` section.
  * The standalone page for getting started with the nRF9160 DK.
    This page has been replaced by the `Quick Start app`_ that supports the nRF9160 DK.
    The content about connecting the DK to nRF Cloud is now a standalone :ref:`nrf9160_gs_connecting_dk_to_cloud` page in the :ref:`ug_nrf9160` section.
  * The guide about migrating from Secure Partition Manager to Trusted Firmware-M (Secure Partition manager was removed in the |NCS| v2.1.0 release).
    If you still need to migrate, see the `information in the nRF Connect SDK v2.0.0 documentation <Migrating from Secure Partition Manager to Trusted Firmware-M_>`_.
