.. _ncs_release_notes_320:

|NCS| v3.2.0 Release Notes
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

* Matter:

  * Integration of `Matter 1.5.0 <CSA press release for Matter 1.5_>`_ with dedicated sample for closures:

    * Better closures - Supporting different motion types (sliding, rotating, opening), configurations (single/dual panels, nested mechanisms), and improved safety features (through precise position reporting).
    * Soil sensors - Introducing soil sensors that enable Matter-based valves and irrigation systems to optimize water use for plants.
    * Energy management - Providing real-time data on energy prices, carbon data, and advanced smart metering, thereby improving tracking, enabling grid coordination, and supporting EV charging features.

  * Introduced the `Matter Quick Start app`_ as part of `nRF Connect for Desktop`_.
    This tool allows you to set up and configure Matter accessory devices and evaluate Matter samples without installing the |NCS| and setting up the development environment.

* Bluetooth®:

  * Support for Bluetooth Core version 6.2.
  * LE Shorter Connection Intervals feature and sample.
  * LE Frame Space Update feature.
  * Central-only and Peripheral-only library variants for the nRF54H Series devices.
  * See the changelogs of :ref:`MPSL <nrfxlib:mpsl_changelog>` and :ref:`Softdevice Controller <nrfxlib:softdevice_controller_changelog>` for the full list of improvements and changes.

* Bluetooth Mesh:

  * Added the :ref:`bluetooth_mesh_sensor_client` sample.
  * Updated several Bluetooth Mesh samples to support NLC.
    Updated sample names in the documentation to reflect the upgrade.

* Wi-Fi®:

  * `Wi-Fi Alliance Enterprise Certification for WPA2 and WPA3 on nRF7002 DK`_.

* Cellular and Non-Terrestrial Networks (NTN):

  * Added a new library for :ref:`Non-Terrestrial Network (NTN) related helpers <lib_ntn>`.
  * The `nRF9151 SMA DK <nRF9151 SMA DK product page_>`_, using the same board build configurations as the regular nRF9151 DK.
  * Merged the PDN library and the :ref:`lte_lc_readme` library to ensure compatibility with the upcoming version of modem firmware that will bring experimental support for NTN.

* nRF Desktop:

  * Support for the nRF54H20 DK that is based on the new IronSide SE architecture.

Added the following features as experimental:

* Matter:

  * Matter over Wi-Fi with the nRF54LM20A SoC combined with the `nRF7002 Expansion Board II`_ (nRF7002 EBII).

* Wi-Fi:

  * nRF70 Series Wi-Fi connectivity stack on the nRF54LM20A SoC, using the nRF7002 EBII.
  * Wi-Fi Direct operation mode on the nRF7002 DK, with support for Wi-Fi Direct added to the :ref:`wifi_wfa_qt_app_sample`.

* nRF54L Series:

  * This is the first release of |NCS| that brings experimental support for the `nRF54LV10 DK`_ and the `nRF54LV10A SoC <nRF54LV10A System-on-Chip_>`_.
  * Protocols with experimental support include:

    * Bluetooth LE, including Channel Sounding
    * 2.4 GHz proprietary

  * All standard SoC peripherals are supported.
  * Device Firmware Update (DFU) and bootloader support.
  * :ref:`PSA Crypto APIs (HW accelerated) <ug_nrf54l_cryptography>` for cryptographic operations and key storage.
  * Trusted Firmware-M providing secure processing environment.
  * Out-of-the-box support across many standard SDK samples.

* Device Firmware Update (DFU) and Firmware Over-the-Air (FOTA):

  * Single slot DFU support for the nRF54H20 SoC and the nRF54L Series.
  * Encrypted DFU support using ECIES on the nRF54L15, nRF54LM20, and nRF54LV10 SoCs.
  * Support for DFU using external flash on the nRF54H20 SoC.

Improved:

* System-Off mode can now be used in devices running TF-M.
* Bluetooth:

  * The Quality of Service (QoS) channel survey feature now supports incremental channel surveying, allowing scheduling with smaller or shorter timeslots.
  * :ref:`MPSL <nrfxlib:mpsl_changelog>` and :ref:`Softdevice Controller <nrfxlib:softdevice_controller_changelog>` :

    * The Scheduler can now manage events that are scheduled less than 100 microseconds apart.
    * Timeslot events can now run for longer than 128 seconds.

  * Reduced code size for nRF52 and nRF53 Series devices.

Deprecated:

* The *Matter over Wi-Fi* support on the nRF5340 SoC combined with nRF7002.
  It is recommended to switch to using the nRF54LM20A SoC (in combination with the nRF7002 EBII), which provides a significantly larger amount of available non-volatile memory for Matter over Wi-Fi applications.

Removed:

* The Serial LTE Modem application.
  The application is replaced by the `Serial Modem`_, which is available as an `nRF Connect Add-on <nRF Connect SDK Add-ons_>`_.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.2.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.2.0`_.

Integration test results
************************

The integration test results for this tag can be found in the following external artifactory:

* `Twister test report for nRF Connect SDK v3.2.0`_
* `Hardware test report for nRF Connect SDK v3.2.0`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.2.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9151 SoC`_
* `Modem firmware compatibility matrix for the nRF9160 SoC`_

Use the latest version of the `Programmer app`_ of `nRF Connect for Desktop`_ to update the modem firmware.
See `Programming nRF91 Series DK firmware`_ for instructions.

Modem-related libraries and versions
====================================

.. list-table:: Modem-related libraries and versions
   :widths: 15 10
   :header-rows: 1

   * - Library name
     - Version information
   * - Modem library
     - `Changelog <Modem library changelog for v3.2.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.2.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.2.0`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v3.2.0`_ for the changes required or recommended when migrating your application from |NCS| v3.1.0 to |NCS| v3.2.0.

.. _ncs_release_notes_320_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Added macOS 26 support (Tier 3) to the table listing :ref:`supported operating systems for proprietary tools <additional_nordic_sw_tools_os_support>`.
* Updated:

  * The required `SEGGER J-Link`_ version to v8.76.
  * Steps on the :ref:`install_ncs` page for installing the |NCS| and toolchain together.
    With this change, the separate steps to install the toolchain and the SDK have been merged into a single step.
  * The table listing :ref:`supported operating systems for proprietary tools <additional_nordic_sw_tools_os_support>`: Windows 11 is now Tier 1, Windows 10 is now Tier 3, macOS 15 is now Tier 1, and macOS 13 is now Tier 3.
  * The table listing :ref:`supported operating systems for firmware <supported_OS>` from Windows 10 to Windows 11.

Board support
=============

* Added support for the nRF7002-EB II Wi-Fi shield for use with the nRF54LM20 DK board target.

Bootloaders and DFU
===================

* Added:

  * Support for extra images in DFU packages (multi-image binary and ZIP).
    This allows applications to extend the built-in DFU functionality with additional firmware images beyond those natively supported by the |NCS|, for example, firmware for external devices.
    See :ref:`lib_dfu_extra` for details.
  * An option to restore progress after a power failure when using DFU multi-image with MCUboot.

* Updated:

  * The NSIB monotonic counter configuration by moving it to sysbuild, where it is now configured using the :kconfig:option:`SB_CONFIG_SECURE_BOOT_MONOTONIC_COUNTER` and :kconfig:option:`SB_CONFIG_SECURE_BOOT_NUM_VER_COUNTER_SLOTS` Kconfig options.
  * The default MCUmgr and Bluetooth LE buffer sizes used for MCUmgr OTA DFU over Bluetooth (:kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` and :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP`).
    The new, smaller default sizes reduce RAM usage, but might also reduce DFU image upload speed.

Developing with nRF91 Series
============================

* Added support for the `nRF9151 SMA development kit <nRF9151 SMA DK product page_>`_.
* Moved the sections about updating the firmware using the Programmer app to the `Programming nRF91 Series DK firmware`_ tools page.

Developing with nRF54L Series
=============================

* Added:

  * The :ref:`ug_nrf54l_otp_map` page, showing the OTP memory allocation for nRF54L Series devices.
  * The :ref:`ug_nrf54l_pinmap` page, describing the pin mapping process for nRF54L Series devices.

Developing with nRF54H Series
=============================

* Updated:

  * The location of the merged binaries in direct-xip mode from :file:`build/zephyr` to :file:`build`.
  * The :ref:`ug_nrf54h20_architecture_lifecycle` page with an expanded introduction.
  * The :ref:`ug_nrf54h20_gs` page to update the getting started instructions for the nRF54H20 DK.
  * The :ref:`ug_nrf54h20_custom_pcb` documentation to clarify how to configure the BICR for a custom PCB based on the nRF54H20 SoC.
  * The :ref:`ug_nrf54h20_pm_optimization` document to include a section describing logical domains, power domains, locality, deep-sleep policy, and latency optimization.
  * The :ref:`ug_nrf54h20_keys` page to remove outdated information pertaining to SUIT-based SDFW.
  * The :ref:`ug_nrf54h20_ironside` page to add information about the counter service and the SPU MRAMC feature configuration.

Developing with Thingy:91
=========================

* Updated the title of the page about updating the Thingy:91 firmware using the Cellular Monitor app to :ref:`thingy91_update_firmware`.

* Removed the page about updating the Thingy:91 firmware using the Programmer app and Cellular Monitor app.
  Contents are now available in the app documentation on the `Programming Nordic Thingy prototyping platforms`_ and `Programming Nordic Thingy:91 firmware`_ pages, respectively.
  The :ref:`thingy91_partition_layout` section has been moved to the :ref:`thingy91_update_firmware` page.

Security
========

* Added:

  * CRACEN and nrf_oberon driver support for nRF54LM20 and nRF54LV10.
    For the list of supported features and limitations, see the :ref:`ug_crypto_supported_features` page.
  * Support for disabling Internal Trusted Storage (ITS) on nRF54L series devices when using :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO` with Trusted Firmware-M (TF-M) through the :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` Kconfig option.
  * Support for AES in counter mode and CBC mode using CRACEN for the :zephyr:board:`nrf54lm20dk`.
  * Support for the PureEdDSA and HashEdDSA for Ed448 curve in the :ref:`CRACEN driver <crypto_drivers_cracen>`.
  * Support for the SHAKE256 hash function with 64-byte and 114-byte digests in the :ref:`CRACEN driver <crypto_drivers_cracen>`.
  * Support for the :ref:`ug_tfm_services_system_off` to the TF-M implementation of the TF-M platform service.
  * Experimental support for TF-M on the nRF54LV10A SoC.
  * Experimental support for NSIB and MCUboot on the nRF54LV10A SoC.
  * Experimental support for KMU on the nRF54LV10A SoC.
  * Experimental support for compression and encryption on the nRF54LV10A SoC.

* Updated:

  * The :ref:`security_index` page with a table that lists the versions of security components implemented in the |NCS|.
  * The Trusted storage in the |NCS| page is now renamed to :ref:`secure_storage_in_ncs` and has been updated with new information about the secure storage configuration in the |NCS|.
  * The :ref:`ug_crypto_supported_features` page with the missing entries for the HMAC key type (:kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HMAC`).
  * The :ref:`ug_nrf54l_crypto_kmu_supported_key_types` section specific for the nRF54L Series devices to list the supported algorithms for each key type.
  * The :ref:`ug_nrf54l_developing_provision_kmu` page with more detailed information about requirements for KMU provisioning and steps for provisioning KMU for development and production.
    This also led to some updates to the :ref:`ug_nrf54l_crypto_kmu_storing_keys` section on the :ref:`ug_nrf54l_cryptography` page.

* Removed:

  * Curve types smaller than 224 bytes from the :ref:`CRACEN driver <crypto_drivers_cracen>`.
  * Some curve types are no longer supported.
    For details, see the :ref:`ug_crypto_supported_features` page.

Mbed TLS
--------

* Updated to version 3.6.5.

Trusted Firmware-M
==================

* Updated:

  * The TF-M version to 2.2.0.
  * Documentation to clarify the support for TF-M on devices emulated using the nRF54L15 DK:

    * nRF54L05 does not support TF-M.
    * nRF54L10 supports TF-M experimentally.

* Removed several documentation pages from the :ref:`tfm_wrapper` section not relevant to the understanding of the TF-M integration in the |NCS|.
  The section now includes only pages that provide background information about TF-M design that are relevant for the |NCS|.

Protocols
=========

Bluetooth LE
------------

* Added:

  * The :c:func:`bt_nrf_conn_set_ltk` API.
    This API allows you to set a custom Long Term Key (LTK) for a connection.
    You can use it when two devices have a shared proprietary method for obtaining an LTK.
  * Support for the Frame Space Update feature.
    This feature allows the time between connection events to be negotiated, allowing for shorter or longer spacing to improve throughput for various applications.
  * Support for the Shorter Connection Intervals (SCI) feature.
    This feature introduces a range of connection intervals below 7.5 ms, providing faster device responsiveness for high-performance HID devices, real-time HMI systems, and sensors.
    SCI extends the connection interval range to span from 375 μs to 4.0 s, and defines the resolution to be a multiple of 125 μs.
    The minimum supported connection interval depends on the current device and controller capabilities.
    For more information, see the :ref:`softdevice_controller` documentation.

Bluetooth Mesh
--------------

* Updated the NLC profile configuration system:

  * Introduced individual profile configuration options for better user control.
  * Deprecated the :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_CONF` and :kconfig:option:`CONFIG_BT_MESH_NLC_PERF_DEFAULT` Kconfig options.
    Existing configurations continue to work but you should migrate to individual profile options.

* Updated the LE Pairing Responder model:

  * Deprecated the :kconfig:option:`CONFIG_BT_FIXED_PASSKEY` Kconfig option in favor of the new and supported :kconfig:option:`CONFIG_BT_APP_PASSKEY` option.

Enhanced ShockBurst (ESB)
-------------------------

* Added:

  * The :ref:`esb_monitor_mode` feature.
  * Experimental support for the nRF54LV10A SoC in the following samples:

    * :ref:`esb_prx`
    * :ref:`esb_ptx`

  * A workaround for the hardware errata HMPAN-229 for the nRF54H20 SoC.

* Updated:

  * Workaround handling for the hardware erratas.
  * The implementation of the hardware errata HMPAN-103 for the nRF54H20 SoC.

Matter
------

* Added:

  * The `Matter Quick Start app`_ v1.0.0 as part of nRF Connect for Desktop.
  * Documentation for leveraging Matter Compliant Platform certification through the Derived Matter Product (DMP) process.
    See :ref:`ug_matter_platform_and_dmp`.

* Updated to use the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option as the C library instead of :kconfig:option:`CONFIG_NEWLIB_LIBC`, in compliance with Zephyr requirements.

* Deprecated the Matter over Wi-Fi samples that are using nRF5340 SoC (nRF7002 DK and nRF5340 DK with the nRF7002 EK shield attached).
  This is mainly due to the very limited non-volatile memory space left for application code.
  As an alternative, it is recommended to use the nRF54LM20A SoC in combination with the nRF7002-EB II shield, which provides a significantly greater amount of available non-volatile memory for Matter over Wi-Fi applications.

* Removed the ``CONFIG_CHIP_SPI_NOR`` and ``CONFIG_CHIP_QSPI_NOR`` Kconfig options.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.5.0.0`` tag.

* Added:

  * Support for the following new device types:

    * Irrigation System
    * Soil Sensor
    * Closure
    * Closure Panel
    * Closure Controller
    * Meter Reference Point
    * Electrical Energy Tariff
    * Electrical Meter
    * Electrical Utility Meter
    * Camera
    * Floodlight Camera
    * Video Doorbell
    * Snapshot Camera
    * Chime
    * Camera Controller
    * Doorbell
    * Intercom
    * Audio Doorbell

  * Full support for operation over TCP transport to Data Transport, which enables more efficient and reliable transmission of large messages.
  * The new code-driven approach for the Matter Data Model and Cluster configuration handling.
    This approach assumes gradually replacing the configuration based on the ZAP files and the ZAP-generated code, and handling the configuration in the source code.
    For example, to enable a specific cluster or its attribute, the new model requires calling a dedicated delegate and registering the cluster in a source code.
    The code-driven approach is not yet fully implemented for all the available clusters, but the coverage will be increasing and it is used for the newly created clusters.
    The new model is meant to be backward compatible with the previous configuration based on the ZAP files and the ZAP-generated code, until the code-driven approach is fully implemented for all the available clusters.
    See the `Migration guide for nRF Connect SDK v3.2.0`_ for more information.
  * The :ref:`ug_matter_gs_tools_matter_west_commands_sync` to synchronize the ZAP and :file:`zcl.json` files after updating the ZAP tool version.
  * The check to all :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool` that verify whether ZAP tool sandbox permissions are correctly set.
    In case of detecting incorrect permissions, the command prompts the user to accept automatically updating the permissions to required ones.

* Updated the :ref:`ug_matter_gs_tools_matter_west_commands_append` to accept ``--clusters`` argument instead of ``new_clusters`` argument.

* Removed dependencies on Nordic DK-specific configurations in Matter configurations.
  See the `Migration guide for nRF Connect SDK v3.2.0`_ for more information.

Thread
------

* Updated:

  * The :ref:`thread_sed_ssed` documentation to clarify the impact of the SSED configuration on the device's power consumption and provide a guide for :ref:`thread_ssed_fine_tuning` of SSED devices.
  * The platform configuration to use the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option as the C library instead of :kconfig:option:`CONFIG_NEWLIB_LIBC`, in compliance with Zephyr requirements.

Wi-Fi®
------

* Added:

  * Support for Wi-Fi Direct (P2P) mode, see :ref:`ug_wifi_direct` for details.
  * Support for WPA3-SAE using the Oberon PSA PAKE implementation, see :ref:`ug_nrf70_wifi_advanced_security_modes` for details.

Applications
============

* Removed the Serial LTE modem application.
  Instead, use `Serial Modem`_, an |NCS| add-on application.

Matter bridge
-------------

* Added:

  * Support for the nRF54LM20 DK working with both Thread and Wi-Fi protocol variants.
    For the Wi-Fi protocol variant, the nRF54LM20 DK works with the nRF7002-EB II shield attached.
  * The ``matter_bridge list`` command to show a list of all bridged devices and their endpoints.

* Updated:

  * The application to store a portion of the application code related to the nRF70 Series Wi-Fi firmware in the external flash memory by default.
    This change breaks the DFU between the previous |NCS| versions and the upcoming release.
    To fix this, you need to disable storing the Wi-Fi firmware patch in external memory.
    See the `migration guide <Migration guide for nRF Connect SDK v3.2.0_>`_ for more information.
  * By moving code from :file:`samples/matter/common/src/bridge` to :file:`applications/matter_bridge/src/core` and :file:`applications/matter_bridge/src/ble` directories.
  * The Identify cluster implementation in the application to use the code-driven approach instead of the zap-driven approach.
  * The default number of Bluetooth Low Energy connections that can be selected using the Kconfig configuration from ``10`` to ``8`` for the Matter bridge over Thread configuration.

nRF5340 Audio
-------------

* Added:

  * A way to store servers in RAM on the unicast client (gateway) side.
    The storage does a compare on server address to match multiple servers in a unicast group.
    This means that if another device appears with the same address, it is treated as the same server.
  * Experimental support for stereo in :ref:`unicast server application<nrf53_audio_unicast_server_app_configuration_stereo>`.
  * The :ref:`Audio application API documentation <audio_api>` page.
  * The :ref:`nrf53_audio_app_config_audio_app_options` page.
  * The API documentation in the header files listed on the :ref:`audio_api` page.
  * Ability to connect by address as a unicast client.

* Updated:

  * The unicast client (gateway) application has been rewritten to support N channels.
  * The unicast client (gateway) application now checks if a server has a resolvable address.
    If this has not been resolved, the discovery process starts in the identity resolved callback.
  * The power measurements to be disabled by default in the default debug versions of the applications.
    To enable power measurements, see :ref:`nrf53_audio_app_configuration_power_measurements`.
  * The audio application targeting the :zephyr:board:`nrf5340dk` to use pins **P1.5** to **P1.9** for the I2S interface instead of **P0.13** to **P0.17**.
    This change avoids conflicts with the onboard peripherals on the nRF5340 DK.
  * The documentation pages with information about the :ref:`SD card playback module <nrf53_audio_app_overview_architecture_sd_card_playback>` and :ref:`how to enable it <nrf53_audio_app_configuration_sd_card_playback>`.
  * The API documentation in the header files listed on the :ref:`audio_api` page.

* Removed the LC3 QDID from the :ref:`nrf53_audio_feature_support` page.
  The QDID is now listed in the `nRF5340 Bluetooth DNs and QDIDs Compatibility Matrix`_.

nRF Desktop
-----------

* Added:

  * Support for the ``nrf54h20dk/nrf54h20/cpuapp`` board target configurations, aligned with the IronSide SE architecture.
    These configurations use the MCUboot bootloader in direct-XIP mode and a merged image slot that combines both the application and radio core images.
    They provide the same feature set as the SUIT-based configuration released in |NCS| v3.0.0.
    Support for the ``nrf54h20dk/nrf54h20/cpuapp`` board target in the nRF Desktop application has been removed in |NCS| v3.1.0, because the nRF54H20 configurations relied on the SUIT solution, also removed in the same release.
    For guidance on how to migrate an nRF Desktop application from SUIT to IronSide SE, see the nRF Desktop section in the `Migration guide for nRF Connect SDK v3.2.0`_.
  * Support for the DTS-based memory layout in the :ref:`nrf_desktop_dfu` when using the MCUboot bootloader.
  * Experimental support of the ``ram_load`` and ``release_ram_load`` configuration variants for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.
    These variants use the MCUboot bootloader in its experimental RAM load mode and support firmware updates using the :ref:`nrf_desktop_dfu`.
    Bootloader configurations in this mode retain the same security features as direct-xip variants, including hardware cryptography, signature type, and public key storage.

    The MCUboot RAM load mode is used to improve the USB HID report rate by executing the application code from the RAM.

    For more details on the MCUboot RAM load mode, see the MCUboot :ref:`nrf_desktop_configuring_mcuboot_bootloader_ram_load` section in the nRF Desktop documentation.
  * Experimental support for the MCUboot RAM load mode in the :ref:`nrf_desktop_dfu`.

* Updated:

  * The memory layouts for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to make more space for the application code.
    This change in the partition map of every nRF54LM20 configuration is a breaking change and cannot be performed using DFU.
    As a result, the DFU procedure fails if you attempt to upgrade the application firmware based on one of the |NCS| v3.1 releases.
  * The application and MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
    The application image signature is verified with the CRACEN hardware peripheral.
  * The MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the KMU-based key storage.
    The public key used by MCUboot for validating the application image is securely stored in the KMU hardware peripheral.
    To simplify the programming procedure, the application is configured to use the automatic KMU provisioning.
    The KMU provisioning is performed by the west runner as a part of the ``west flash`` command when the ``--erase`` or ``--recover`` flag is used.
  * Application configurations to avoid using the deprecated Kconfig options :option:`CONFIG_DESKTOP_HID_REPORT_EXPIRATION` and :option:`CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE`.
    The configurations rely on Kconfig options specific to HID providers instead.
    The HID keypress queue sizes for HID consumer control (:option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL_EVENT_QUEUE_SIZE`) and HID system control (:option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_EVENT_QUEUE_SIZE`) reports have been decreased to ``10``.
  * Application configurations integrating the USB legacy stack (:option:`CONFIG_DESKTOP_USB_STACK_LEGACY`) to suppress build warnings related to deprecated APIs of the USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
    The configurations enable the :kconfig:option:`CONFIG_DEPRECATION_TEST` Kconfig option to suppress the deprecation warnings.
    The USB legacy stack is still used by default.
  * MCUboot configurations that support serial recovery over USB CDC ACM to enable the :kconfig:option:`CONFIG_DEPRECATION_TEST` Kconfig option to suppress deprecation warnings.
    The implementation of serial recovery over USB CDC ACM still uses the deprecated APIs of the USB legacy stack (:kconfig:option:`CONFIG_USB_DEVICE_STACK`).
  * Configurations of the ``nrf52840dongle/nrf52840`` board target to align them after Zephyr introduced the ``bare`` board variant.
    The application did not switch to the ``bare`` board variant to keep backwards compatibility.
  * HID transports (:ref:`nrf_desktop_hids`, :ref:`nrf_desktop_usb_state`) to use the early :c:struct:`hid_report_event` subscription (:c:macro:`APP_EVENT_SUBSCRIBE_EARLY`).
    This update improves the reception speed of HID input reports in HID transports.
  * The :ref:`nrf_desktop_motion` implementations to align internal state names for consistency.
  * The :ref:`nrf_desktop_motion` implementation that generates simulated motion.
    Improved the Zephyr shell (:kconfig:option:`CONFIG_SHELL`) integration to prevent potential race conditions related to using preemptive execution context for shell commands.
  * The :c:struct:`motion_event` to include information if the sensor is still active or goes to idle state waiting for user activity (:c:member:`motion_event.active`).
    The newly added field is filled by all :ref:`nrf_desktop_motion` implementations.
    The :ref:`nrf_desktop_hid_provider_mouse` uses the newly added field to improve the synchronization of motion sensor sampling.
    After the motion sensor sampling is triggered, the provider waits for the result before submitting a subsequent HID mouse input report.
  * The default value of the :kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL_NORMAL_PRIORITY_TIMEOUT_US` Kconfig option to ``0``.
    This is done to start using high MPSL timeslot priority quicker and speed up non-volatile memory operations.
  * The number of preemptive priorities (:kconfig:option:`CONFIG_NUM_PREEMPT_PRIORITIES`).
    The Kconfig option has been increased to ``15`` (the default value in Zephyr).
    The priority of ``10`` is used by default for preemptive contexts (for example, :kconfig:option:`CONFIG_BT_GATT_DM_WORKQ_PRIO` and :kconfig:option:`CONFIG_BT_LONG_WQ_PRIO`).
    The previously used Kconfig option value of ``11`` leads to using the same priority for the mentioned preemptive contexts as the lowest available application thread priority (used for example, by the log processing thread).
  * Application image configurations to explicitly specify the LED driver used by the :ref:`nrf_desktop_leds` (:kconfig:option:`CONFIG_CAF_LEDS_GPIO` or :kconfig:option:`CONFIG_CAF_LEDS_PWM`).
    Also, disabled unused LED drivers enabled by default to reduce memory footprint.
  * The :ref:`nrf_desktop_hid_forward` to allow using the module when configuration channel support (:option:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE`) is disabled in the application configuration.
  * The :ref:`nrf_desktop_keys_state` to rely on runtime assertions, ensuring that the initialization function is called only once and other APIs of the utility are called after the utility has been initialized.
    This is done to align the utility with other application utilities.
  * The :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr` to handle the ``nrf54h20dk/nrf54h20/cpuapp`` board target with the IronSide SE architecture and the MCUboot bootloader.
  * The :ref:`nrf_desktop_hid_state`:

    * Now allows for the delayed registration of HID report providers.
      Previously, subscribing to a HID input report before the respective provider had been registered triggered an assertion failure.
    * The :ref:`nrf_desktop_hid_state_pm` now skips submitting the :c:struct:`keep_alive_event` if the :c:enum:`POWER_MANAGER_LEVEL_ALIVE` power level is enforced by any application module through the :c:struct:`power_manager_restrict_event`.
      This is done to improve performance.
    * The documentation for the module and default HID report providers now has a simplified getting started guide for updating HID input reports used by the application or introducing support for a new HID input report.
    * Fixed issues related to selective HID report subscription.
      This resolves the `known issue <known issues for nRF Connect SDK v3.2.0_>`_ NCSDK-35718.

* Removed:

  * SUIT support in the :ref:`nrf_desktop_dfu` and the :ref:`nrf_desktop_dfu_mcumgr`.
  * The ``CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_MCUBOOT_DIRECT_XIP`` Kconfig option from the :ref:`nrf_desktop_dfu` and the ``CONFIG_DESKTOP_DFU_MCUMGR_MCUBOOT_DIRECT_XIP`` Kconfig option from the :ref:`nrf_desktop_dfu_mcumgr`.
    These options indicate only that MCUboot has been running in the direct-xip mode within each application module.
    They are replaced by direct use of the :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP` Kconfig option provided by the MCUboot component.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Updated:

  * The application to change the default libc from the :ref:`zephyr:c_library_newlib` to the :ref:`zephyr:c_library_picolibc` to align with the |NCS| and Zephyr.
  * By changing the number of preemptive priorities (:kconfig:option:`CONFIG_NUM_PREEMPT_PRIORITIES`) from ``11`` to default value ``15`` from Zephyr.
    The priority of ``10`` is used by default for some preemptive contexts (for example, :kconfig:option:`CONFIG_BT_LONG_WQ_PRIO`).
    The previously used Kconfig option value of ``11`` led to using the lowest available application thread priority for the mentioned preemptive contexts, which at the same time is used, for example, by the log processing thread.

* Removed support for the ``thingy53/nrf5340/cpuapp/ns`` board target.

Thingy:53: Matter weather station
---------------------------------

* Updated the application to use the code-driven approach for the Identify cluster implementation instead of the zap-driven approach.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added:

  * The :ref:`ble_shorter_conn_intervals` sample to demonstrate how to use the Bluetooth Shorter Connection Intervals feature.
  * The :ref:`bluetooth_path_loss_monitoring` sample.
  * The :ref:`samples_test_app` application to demonstrate how to use the Bluetooth LE Test GATT Server and test Bluetooth LE functionality in peripheral samples.
  * The :ref:`bluetooth_automated_power_control` sample.
  * Experimental support for the nRF54LV10A SoC in the following Bluetooth samples:

    * :ref:`central_and_peripheral_hrs`
    * :ref:`central_bas`
    * :ref:`bluetooth_central_hids`
    * :ref:`bluetooth_central_hr_coded`
    * :ref:`bluetooth_central_dfu_smp`
    * :ref:`central_uart`
    * :ref:`channel_sounding_ras_initiator`
    * :ref:`channel_sounding_ras_reflector`
    * :ref:`bluetooth_conn_time_synchronization`
    * :ref:`direct_test_mode`
    * :ref:`direction_finding_connectionless_tx`
    * :ref:`direction_finding_peripheral`
    * :ref:`ble_event_trigger`
    * :ref:`bluetooth_iso_combined_bis_cis`
    * :ref:`bluetooth_isochronous_time_synchronization`
    * :ref:`ble_llpm`
    * :ref:`multiple_adv_sets`
    * :ref:`peripheral_bms`
    * :ref:`peripheral_cgms`
    * :ref:`peripheral_cts_client`
    * :ref:`peripheral_gatt_dm`
    * :ref:`peripheral_hids_keyboard`
    * :ref:`peripheral_hids_mouse`
    * :ref:`peripheral_hr_coded`
    * :ref:`peripheral_lbs`
    * :ref:`peripheral_mds`
    * :ref:`power_profiling`
    * :ref:`peripheral_rscs`
    * :ref:`peripheral_status`
    * :ref:`peripheral_uart`
    * :ref:`bt_peripheral_with_multiple_identities`
    * :ref:`bluetooth_radio_coex_1wire_sample`
    * :ref:`ble_radio_notification_conn_cb`
    * :ref:`bt_scanning_while_connecting`
    * :ref:`shell_bt_nus`
    * :ref:`ble_shorter_conn_intervals`
    * :ref:`ble_subrating`
    * :ref:`ble_throughput`

* Updated:

  * The network core image applications for the following samples from the :zephyr:code-sample:`bluetooth_hci_ipc` sample to the :ref:`ipc_radio` application for multicore builds:

    * :ref:`bluetooth_conn_time_synchronization`
    * :ref:`bluetooth_iso_combined_bis_cis`
    * :ref:`bluetooth_isochronous_time_synchronization`
    * :ref:`bt_scanning_while_connecting`
    * :ref:`channel_sounding_ras_initiator`
    * :ref:`channel_sounding_ras_reflector`

    The :ref:`ipc_radio` application is commonly used for multicore builds in other |NCS| samples and projects.
    Hence, this is to align with the common practice.

  * Disabled legacy pairing in the following samples:

     * :ref:`central_nfc_pairing`
     * :ref:`power_profiling`

    Support for legacy pairing remains exclusively for the :ref:`peripheral_nfc_pairing` sample to retain compatibility with older Android devices.


* Removed support for the ``thingy53/nrf5340/cpuapp/ns`` board target from the following samples:

   * :ref:`peripheral_lbs`
   * :ref:`peripheral_status`
   * :ref:`peripheral_uart`

* :ref:`direct_test_mode` sample:

  * Updated by simplifying the 2-wire UART polling.
    This is done by replacing the hardware timer with the ``k_sleep()`` function.

Bluetooth Mesh samples
----------------------

* Added:

  * Support for external flash settings for the ``nrf52840dk/nrf52840``, ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54l15dk/nrf54l05/cpuapp`` board targets in all Bluetooth Mesh samples.
  * Support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target in all Bluetooth Mesh samples.

* :ref:`ble_mesh_dfu_distributor` sample:

  * Added:

    * Support for external flash memory for the ``nrf52840dk/nrf52840`` and the ``nrf54l15dk/nrf54l15/cpuapp`` as the secondary partition for the DFU process.
    * Support for external flash settings for the ``nrf52840dk/nrf52840`` board targets.

  * Updated:

    * The sample to use the :kconfig:option:`CONFIG_BT_APP_PASSKEY` option instead of the deprecated :kconfig:option:`CONFIG_BT_FIXED_PASSKEY` option.
    * The :makevar:`FILE_SUFFIX` make variable to use more descriptive suffixes for external flash configurations.
      The new suffixes are ``_dfu_ext_flash`` for external flash DFU storage and ``_ext_flash_settings`` for external flash settings storage.

* :ref:`ble_mesh_dfu_target` sample:

  * Added:

    * Support for external flash memory for the ``nrf52840dk/nrf52840`` and the ``nrf54l15dk/nrf54l15/cpuapp`` as the secondary partition for the DFU process.
    * Support for external flash settings for the ``nrf52840dk/nrf52840`` board targets.

  * Updated the :makevar:`FILE_SUFFIX` make variable to use more descriptive suffixes for external flash configurations.
    The new suffixes are ``_dfu_ext_flash`` for external flash DFU storage and ``_ext_flash_settings`` for external flash settings storage.

* :ref:`bluetooth_mesh_sensor_client` sample:

  * Added polling toggle to **Button 1** (**Button 0** on nRF54 DKs) to start or stop the periodic Sensor Get loop, ensuring the functionality is available on all supported devices including single-button hardware.

  * Updated:

    * To demonstrate the Bluetooth :ref:`ug_bt_mesh_nlc` HVAC Integration profile.
    * The following Mesh samples to use individual NLC profile configurations instead of the deprecated options:

      * :ref:`bluetooth_mesh_light_dim`
      * :ref:`bluetooth_mesh_light_lc`
      * :ref:`bluetooth_mesh_sensor_server`
      * :ref:`bluetooth_mesh_sensor_client`

    * Button functions.
      Assignments are shifted down one index to accommodate the new polling toggle.
      The descriptor action has been removed from button actions but is still available through mesh shell commands.

  * Removed support for the ``nrf52dk/nrf52832``, since it does not have enough RAM space after NLC support has been added.

* :ref:`bluetooth_mesh_sensor_server`:

  * Updated:

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

* :ref:`bluetooth_mesh_light_dim`:

  * Updated:

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

* :ref:`bluetooth_mesh_light_lc`:

  * Updated:

    * To use individual NLC profile configurations instead of the deprecated options.
    * The sample name to indicate NLC support.

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Updated:

    * The memory layout for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to make more space for the application code.
      This change in the nRF54LM20 partition map is a breaking change and cannot be performed using DFU.
      As a result, the DFU procedure fails if you attempt to upgrade the sample firmware based on one of the |NCS| v3.1 releases.
    * The application and MCUboot configurations for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
      The Fast Pair subsystem still uses the Oberon software library.
      The application image signature is verified with the CRACEN hardware peripheral.
    * The MCUboot configuration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the KMU-based key storage.
      The public key used by MCUboot for validating the application image is securely stored in the KMU hardware peripheral.
      To simplify the programming procedure, the samples are configured to use the automatic KMU provisioning.
      The KMU provisioning is performed by the west runner as a part of the ``west flash`` command when the ``--erase`` or ``--recover`` flag is used.

* :ref:`fast_pair_input_device` sample:

  * Updated the application configuration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target to use the CRACEN hardware crypto driver instead of the Oberon software crypto driver.
    The Fast Pair subsystem still uses the Oberon software library.

Cellular samples
----------------

* Added:

  * The :ref:`nrf_cloud_coap_cell_location` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for nRF Cloud's cellular location service.
  * The :ref:`nrf_cloud_coap_fota_sample` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for FOTA updates.
  * The :ref:`nrf_cloud_coap_device_message` sample to demonstrate how to use the `nRF Cloud CoAP API`_ for device messages.
  * The :ref:`nrf_cloud_mqtt_device_message` sample to demonstrate how to use the `nRF Cloud MQTT API`_ for device messages.
  * The :ref:`nrf_cloud_mqtt_fota` sample to demonstrate how to use the `nRF Cloud MQTT API`_ for FOTA updates.
  * The :ref:`nrf_cloud_mqtt_cell_location` sample to demonstrate how to use the `nRF Cloud MQTT API`_ for nRF Cloud's cellular location service.

* Updated the following samples to use the new ``SEC_TAG_TLS_INVALID`` definition:

  * :ref:`modem_shell_application`
  * :ref:`http_application_update_sample`
  * :ref:`http_modem_delta_update_sample`
  * :ref:`http_modem_full_update_sample`

* Removed:

  * The SLM Shell sample.
    Use the sample from `Serial Modem`_ instead.
  * The deprecated LTE Sensor Gateway sample.

* :ref:`nrf_cloud_rest_cell_location` sample:

  * Added runtime setting of the log level for the nRF Cloud logging feature.

* :ref:`modem_shell_application` sample:

  * Added:

    * Support for environment evaluation using the ``link enveval`` command.
    * Support for NTN NB-IoT to the ``link sysmode`` and ``link edrx`` commands.
    * Support for Non-Terrestrial Network (NTN) helper functionality using the ``ntn`` command.

  * Updated the PDN connection management to use the PDN functionality in the :ref:`lte_lc_readme` library instead of the :ref:`pdn_readme` library.

* :ref:`pdn_sample` sample:

  * Updated the PDN functionality to use the PDN management in the :ref:`lte_lc_readme` library instead of the :ref:`pdn_readme` library.

* :ref:`gnss_sample` sample:

  * Added TLS support for connection to the SUPL server.

* :ref:`nrf_cloud_multi_service` sample:

  * Fixed an issue where sporadically the application would get stuck waiting for the device to connect to the Internet, caused by an incorrect :ref:`Connection Manager <zephyr:conn_mgr_overview>` initialization.
  * Deprecated the sample.

Cryptography samples
--------------------

* Added:

  * Support for the ``nrf54lv10dk/nrf54lv10a/cpuapp`` and ``nrf54lv10dk/nrf54lv10a/cpuapp/ns`` board targets to all samples (except :ref:`crypto_test`).
  * Support for the ``nrf54h20dk/nrf54h20/cpuapp`` board target to the :ref:`crypto_persistent_key` sample, demonstrating use of Internal Trusted Storage (ITS) on the nRF54H20 DK.
  * Support for the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board target in all supported cryptography samples.
  * Support for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target in the following samples:

    * :ref:`crypto_aes_ctr`
    * :ref:`crypto_aes_cbc`

  * The :ref:`crypto_kmu_cracen_usage` sample.

* Updated documentation of all samples for style consistency.
  Lists of cryptographic features used by each sample and sample output in the testing section have been added.

* :ref:`crypto_tls` sample:

  * Updated sample-specific Kconfig configuration structure and documentation.

DECT NR+ samples
----------------

* :ref:`dect_shell_application` sample:

  * Added the ``dect perf`` command client - Listen Before Talk (LBT) support with configurable LBT period and busy threshold.

  * Updated:

    * PCC and PDC printings improved to show SNR and RSSI-2 values with actual dB/dBm resolutions.
    * ``dect perf`` command - Improved operation schedulings to avoid scheduling conflicts and fix to TX the results in server side.
    * ``dect ping`` command - Improved operation schedulings to avoid scheduling conflicts.

DFU samples
-----------

* Added:

  * The :ref:`dfu_multi_image_sample` sample to demonstrate how to use the :ref:`lib_dfu_target` library.
  * The :ref:`ab_sample` sample to demonstrate how to implement the A/B firmware update strategy using :ref:`MCUboot <mcuboot_index_ncs>`.
  * The :ref:`fw_loader_ble_mcumgr` sample that provides a minimal configuration for firmware loading using SMP over Bluetooth LE.
    This sample is intended as a starting point for developing custom firmware loader applications that work with the MCUboot bootloader.
  * The :ref:`single_slot_sample` sample to demonstrate how to maximize the available space for the application with MCUboot using firmware loader mode (single-slot layout).
  * The :ref:`mcuboot_with_encryption` sample demonstrating how to build MCUboot with image encryption enabled.

Enhanced ShockBurst samples
---------------------------

* Added:

  * The :ref:`esb_monitor` sample to demonstrate how to use the :ref:`ug_esb` protocol in Monitor mode.
  * Experimental support for the nRF54LV10A SoC in the following samples:

    * :ref:`esb_prx`
    * :ref:`esb_ptx`

|ISE| samples
--------------

* Added the :ref:`secondary_boot_sample` sample that demonstrates how to build and boot a secondary application image on the nRF54H20 DK.

Matter samples
--------------

* Added:

  * The :ref:`matter_temperature_sensor_sample` sample that demonstrates how to implement and test a Matter temperature sensor device.
  * The :ref:`matter_contact_sensor_sample` sample that demonstrates how to implement and test a Matter contact sensor device.
  * The :ref:`matter_closure_sample` sample that demonstrates how to implement and test a Matter closure device.
  * The ``matter_custom_board`` toggle paragraph in the Matter advanced configuration section of all Matter samples that demonstrates how add and configure a custom board.
  * Support for the Matter over Wi-Fi on the nRF54LM20 DK with the nRF7002-EB II shield attached to all Matter over Wi-Fi samples.
  * Enabled deprecated warnings for all Matter over Wi-Fi samples that are using nRF5340 SoC.

* Updated:

  * All Matter over Wi-Fi samples and applications to store a portion of the application code related to the nRF70 Series Wi-Fi firmware in the external flash memory by default.
    This change breaks the DFU between the previous |NCS| versions and the |NCS| v3.2.0.
    To fix this, you need to disable storing the Wi-Fi firmware patch in external memory.
    See the `migration guide <Migration guide for nRF Connect SDK v3.2.0_>`_ for more information.
  * All Matter samples that support low-power mode to use the :ref:`lib_ram_pwrdn` feature with the nRF54LM20 DK.
    This change resulted in decreasing the sleep current consumption by more than 2 µA.
  * All Matter samples to use the code-driven approach for the Identify cluster implementation instead of the zap-driven approach.

* :ref:`matter_lock_sample` sample:

   * Added a callback for the auto-relock feature.
     This resolves the `known issue <known issues for nRF Connect SDK v3.2.0_>`_ KRKNWK-20691.
   * Updated the NUS service to use the :kconfig:option:`CONFIG_BT_APP_PASSKEY` Kconfig option instead of the deprecated :kconfig:option:`CONFIG_BT_FIXED_PASSKEY` Kconfig option.

Networking samples
------------------

* Added support for the nRF7002-EB II with the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target in the following samples:

  * :ref:`aws_iot`
  * :ref:`net_coap_client_sample`
  * :ref:`download_sample`
  * :ref:`http_server`
  * :ref:`https_client`
  * :ref:`mqtt_sample`
  * :ref:`udp_sample`

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_hello_world` sample:

  * Added:

    * Support for the ``nrf54lv10dk/nrf54lv10a/cpuapp/ns`` board target.
    * Support for the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board target.

* :ref:`tfm_secure_peripheral_partition`

  * Added support for the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` board target.

Wi-Fi samples
-------------

* Removed support for the nRF7002-EB II with the ``nrf54h20dk/nrf54h20/cpuapp`` board target from the following samples:

  * :ref:`wifi_station_sample`
  * :ref:`wifi_scan_sample`
  * :ref:`wifi_shell_sample`
  * :ref:`wifi_radio_test`
  * :ref:`ble_wifi_provision`
  * :ref:`wifi_provisioning_internal_sample`

Other samples
-------------

* Added the :ref:`idle_relocated_tcm_sample` sample to demonstrate how to relocate the firmware to the TCM memory at boot time.
  The sample also uses the ``radio_loader`` sample image (located in :file:`nrf/samples/nrf54h20/radio_loader`), which cannot be tested as a standalone sample, to relocate the firmware from the MRAM to the TCM memory at boot time.

* :ref:`nrf_profiler_sample` sample:

  * Added a new testing step demonstrating how to calculate event propagation statistics.
    Also added the related test preset for the :file:`calc_stats.py` script (:file:`nrf/scripts/nrf_profiler/stats_nordic_presets/nrf_profiler.json`).

* :ref:`app_event_manager_profiling_tracer_sample` sample:

  * Added a new testing step demonstrating how to calculate event propagation statistics.
    Also added the related test preset for the :file:`calc_stats.py` script (:file:`nrf/scripts/nrf_profiler/stats_nordic_presets/app_event_manager_profiler_tracer.json`).

* :ref:`event_manager_proxy_sample` sample:

  * Added experimental support for the nRF54LV10A SoC.

* :ref:`ipc_service_sample` sample:

  * Added experimental support for the nRF54LV10A SoC.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

nrfx
----

* nrfx 4.0 has been released.
  For details on nrfx API changes, see the `nrfx 4.0 migration note`_.
  For details on Zephyr changes, see the `migration guide <Migration guide for nRF Connect SDK v3.2.0_>`_.

Flash drivers
-------------

* Added a Kconfig option to configure timeout for normal priority MPSL request (:kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL_NORMAL_PRIORITY_TIMEOUT_US`) in MPSL flash synchronization driver (:file:`nrf/drivers/mpsl/flash_sync/flash_sync_mpsl.c`).
  After the timeout specified by this Kconfig option, a higher timeslot priority is used to increase the priority of the flash operation.
  The default timeout has been reduced from 30 milliseconds to 10 milliseconds to speed up non-volatile memory operations.


Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

* :ref:`nrf_security_readme` library:

   * The ``CONFIG_CRACEN_PROVISION_PROT_RAM_INV_DATA`` Kconfig option has been renamed to :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated the glue layer to manage PDN connections using the PDN management functionality in the :ref:`lte_lc_readme` library when the :kconfig:option:`CONFIG_LTE_LC_PDN_MODULE` Kconfig option is enabled, or direct AT commands otherwise.

  * Removed the dependency on the deprecated :ref:`pdn_readme` library.

Bluetooth libraries and services
--------------------------------

* :ref:`hids_readme` library:

  * Updated the report length of the HID boot mouse to ``3``.
    The :c:func:`bt_hids_boot_mouse_inp_rep_send` function only allows to provide the state of the buttons and mouse movement (for both X and Y axes).
    No additional data can be provided by the application.

Security libraries
------------------

* :ref:`trusted_storage_readme` library:

  * Updated the API documentation to be based on headers in :file:`subsys/trusted_storage/include/psa` instead of :file:`include/`.
  * Removed the :file:`internal_trusted_storage.h` and :file:`protected_storage.h` files from the :file:`include/` folder as these files had duplicates in the :file:`subsys/trusted_storage/include/psa` folder.

Modem libraries
---------------

* Added the :ref:`lib_ntn` library to provide helper functionality for Non-Terrestrial Network (NTN) usage.

* Removed:

  * The AT command parser library.
    Use the :ref:`at_parser_readme` library instead.
  * The AT parameters library.
  * The Modem SLM library.
    Use the library from `Serial Modem`_ instead.

* :ref:`lte_lc_readme`:

  * Added:

    * Support for environment evaluation.
    * Support for NTN NB-IoT system mode.
    * eDRX support for NTN NB-IoT.
    * Support for new modem events :c:enumerator:`LTE_LC_MODEM_EVT_RF_CAL_NOT_DONE`, :c:enumerator:`LTE_LC_MODEM_EVT_INVALID_BAND_CONF`, and :c:enumerator:`LTE_LC_MODEM_EVT_DETECTED_COUNTRY`.
    * Description of new features supported by mfw_nrf91x1 and mfw_nrf9151-ntn in receive only functional mode.
    * Sending of the ``LTE_LC_EVT_PSM_UPDATE`` event with ``tau`` and ``active_time`` set to ``-1`` when registration status is ``LTE_LC_NW_REG_NOT_REGISTERED``.
    * New registration statuses and functional modes for the ``mfw_nrf9151-ntn`` modem firmware.
    * Support for PDP context and PDN connection management.
      The functionality is available when the :kconfig:option:`CONFIG_LTE_LC_PDN_MODULE` Kconfig option is enabled.
    * Support for also disabling the default modules that are enabled by default.
      This is useful when the application only needs a subset of the functionality provided by the library and to reduce the size of the application image.
      To disable a module, set the corresponding ``CONFIG_LTE_LC_<MODULE_NAME>_MODULE`` Kconfig option to ``n``.

  * Updated:

    * The type of the :c:member:`lte_lc_evt.modem_evt` field to :c:struct:`lte_lc_modem_evt`.
    * Replaced modem events ``LTE_LC_MODEM_EVT_CE_LEVEL_0``, ``LTE_LC_MODEM_EVT_CE_LEVEL_1``, ``LTE_LC_MODEM_EVT_CE_LEVEL_2`` and ``LTE_LC_MODEM_EVT_CE_LEVEL_3`` with the :c:enumerator:`LTE_LC_MODEM_EVT_CE_LEVEL` modem event.
    * The order of the ``LTE_LC_MODEM_EVT_SEARCH_DONE`` modem event, and registration and cell related events.
      See the `migration guide <Migration guide for nRF Connect SDK v3.2.0_>`_ for more information.

  * Fixed an issue where band lock, RAI notification subscription, and DNS fallback address are lost when the modem has been put into :c:enumerator:`LTE_LC_FUNC_MODE_POWER_OFF` functional mode.

* :ref:`nrf_modem_lib_readme`:

  * Added the :c:func:`nrf_modem_lib_trace_peek_at` function to the :c:struct:`nrf_modem_lib_trace_backend` interface to peek trace data at a byte offset without consuming it.
    Support for this API has been added to the flash trace backend.

  * Updated the PDN functionality to use the PDN management in the :ref:`lte_lc_readme` library instead of the :ref:`pdn_readme` library.

  * Removed the deprecated ``CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ZEPHYR`` Kconfig option.

* :ref:`at_parser_readme`:

  * Fixed an issue where an unquoted string parameter in the middle of a response would not be parsed correctly.

* :ref:`pdn_readme`:

  * Deprecated the library.
    Use the PDN management functionality in the :ref:`lte_lc_readme` library instead.

Libraries for networking
------------------------

* Added missing brackets that caused C++ compilation to fail in the following libraries:

  * :ref:`lib_nrf_cloud_pgps`
  * :ref:`lib_nrf_cloud_fota`

* Updated the following libraries to use the new ``SEC_TAG_TLS_INVALID`` definition for checking whether a security tag is valid:

  * :ref:`lib_aws_fota`
  * :ref:`lib_fota_download`
  * :ref:`lib_ftp_client`

* Removed the Download client library.
  Use the :ref:`lib_downloader` library instead.

* :ref:`lib_nrf_provisioning` library:

  * Added a blocking call to wait for a functional-mode change, relocating the logic from the app into the library.

  * Updated:

    * Made internal scheduling optional.
      Applications can now trigger provisioning manually using the :kconfig:option:`CONFIG_NRF_PROVISIONING_SCHEDULED` Kconfig option.
    * Moved root CA provisioning to the modem initialization callback to avoid blocking and ensure correct state during startup.
    * Expanded the event handler to report additional provisioning events, including failures.
    * Made the event handler callback mandatory to ensure the application is notified of failures and to prevent silent errors.
    * Unified the device-mode and modem-mode callbacks into a single handler for cleaner integration.
    * Updated the documentation and sample code to reflect the changes.

  * Fixed multiple bugs and enhanced error handling.

* :ref:`lib_nrf_cloud_rest` library:

  * Deprecated the library.
    Use the :ref:`lib_nrf_cloud_coap` library instead.

* :ref:`lib_nrf_cloud_fota` library:

  * Fixed occasional message truncation notifying that the download is complete.

* :ref:`lib_nrf_cloud_log` library:

  * Updated by adding a missing CONFIG prefix.

* :ref:`lib_nrf_cloud` library:

  * Added the :c:func:`nrf_cloud_obj_location_request_create_timestamped` function to make location requests for past cellular or Wi-Fi scans.
  * Updated:

    * By refactoring the folder structure of the library to separate the different backend implementations.
    * Handling of ports, which led to confusing log messages with byte-inversed values.

* :ref:`lib_downloader` library:

  * Fixed an issue where HTTP download would hang if the application had not set the socket receive timeout and data flow from the server stopped.
    The HTTP transport now sets the socket receive timeout to 30 seconds by default.

Other libraries
---------------

* :ref:`nrf_profiler` library:

  * Updated the documentation by separating out the :ref:`nrf_profiler_script` documentation.

* :ref:`lib_ram_pwrdn` library:

  * Added support for the nRF54LM20A SoC.

Scripts
=======

* Added:

  * The :ref:`esb_sniffer_scripts` scripts for the :ref:`esb_monitor` sample.
  * The documentation page for the :ref:`nrf_profiler_script`.
    The page also describes the script for calculating statistics (:file:`calc_stats.py`).

* Removed:

  * The SUIT support from the :ref:`nrf_desktop_config_channel_script`.
    The :ref:`nrf_desktop_config_channel_script` from older |NCS| versions can still be used to perform the SUIT DFU operation.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Removed the Fast Pair TinyCrypt cryptographic backend (``CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT``), because the TinyCrypt library support has been removed from Zephyr.
  You can use either the Fast Pair Oberon cryptographic backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_OBERON`) or the Fast Pair PSA cryptographic backend (:kconfig:option:`CONFIG_BT_FAST_PAIR_CRYPTO_PSA`).

Memfault integration
--------------------

* Added a metric tracking the unused stack space of the Bluetooth Long workqueue thread, when the :kconfig:option:`CONFIG_MEMFAULT_NCS_BT_METRICS` Kconfig option is enabled.
  The new metric is named ``ncs_bt_lw_wq_unused_stack``.

* Updated:

  * The ``CONFIG_MEMFAULT_DEVICE_INFO_CUSTOM`` Kconfig option has been renamed to :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_INFO_CUSTOM`.
  * The :kconfig:option:`CONFIG_MEMFAULT_NCS_FW_VERSION` Kconfig option now has a default value set from a :file:`VERSION` file, if present in the application root directory.
    Previously, this option had no default value.
  * Simplified the options for ``CONFIG_MEMFAULT_NCS_DEVICE_ID_*``, which sets the Memfault Device Serial. The default is now :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID`, which uses the :ref:`lib_hw_id` library to provide a unique device ID.
    See the `Migration guide for nRF Connect SDK v3.2.0`_ for more information.
  * The LTE-related integration to obtain PDN information and events through the PDN management functionality in the :ref:`lte_lc_readme` library instead of the :ref:`pdn_readme` library.

* Removed a metric for tracking the unused stack of the Bluetooth TX thread (``ncs_bt_tx_unused_stack``).
  This thread has been removed in Zephyr v3.7.0.

sdk-nrfxlib
===========

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added support for S2RAM resume on nRF54H20 devices.
  MCUboot acts as the S2RAM resume mediator and redirects execution to the application's native resume routine.

* Updated KMU mapping to ``BL_PUBKEY`` when MCUboot is used as the immutable bootloader for nRF54L Series devices.
  You can restore the previous KMU mapping (``UROT_PUBKEY``) with the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_KMU_UROT_MAPPING` Kconfig option.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``911b3da1394dc6846c706868b1d407495701926f``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 911b3da139 ^0fe59bf1e4

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^911b3da139

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Documentation
=============

* Added:

  * The :ref:`debug_log_bt_stack` section to the :ref:`gs_debugging` page.
  * The :ref:`app_power_opt_opp` page to the :ref:`app_optimize` section.

* Updated:

  * The :ref:`emds_readme_application_integration` section in the :ref:`emds_readme` library documentation to clarify the EMDS storage context usage.
  * The Emergency data storage section in the :ref:`bluetooth_mesh_light_lc` sample documentation to clarify the EMDS storage context implementation and usage.
  * The :ref:`ble_mesh_dfu_distributor` sample documentation to clarify the external flash support.
  * The :ref:`ble_mesh_dfu_target` sample documentation to clarify the external flash support.
  * The :ref:`app_power_opt_nRF91` page by moving it under the :ref:`ug_lte` section.
  * The nRF54H20 SoC binaries are now called nRF54H20 IronSide SE binaries.
  * The :ref:`abi_compatibility` page with the newest IronSide SE changelog updates.
  * The :ref:`ug_tfm_services` documentation page.
