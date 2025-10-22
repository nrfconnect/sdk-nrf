.. _ncs_release_notes_280:

|NCS| v2.8.0 Release Notes
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

  * This is the first release of |NCS| that brings comprehensive support for `nRF54L15 DK`_ and `nRF54L15 SoC <nRF54L15 System-on-Chip_>`_.
    nRF54L15 DK is now publicly available.
    See the `nRF54L15 DK`_ page for distributors with stock.
  * The protocols include Bluetooth® LE 6.0, Bluetooth Mesh, 2.4 GHz proprietary Enhanced ShockBurst (ESB), :ref:`Thread 1.4 <thread_ug_supported_features_v14>`, Matter-over-Thread, Sidewalk, and Google Find My Device.
  * For members signed up for both Apple MFi program and Apple Find My.
  * All standard SoC peripherals.
  * Device Firmware Update (DFU) provided by single-stage immutable bootloader MCUboot.
  * :ref:`PSA Crypto APIs (HW accelerated) <ug_nrf54l_cryptography>` for cryptographic operations and key storage.
  * :ref:`Wi-Fi® Scan <wifi_scan_sample>`, :ref:`wifi_shell_sample`, and :ref:`wifi_station_sample` in combination with nRF7002 EB.
  * :ref:`nPM1300 PMIC <ug_pmic>`.

* Bluetooth LE:

  * LE Power Control that was introduced in |NCS| 2.2.0 as experimental, is now supported.
    This feature automatically adjusts the signal strength between devices, enhancing connection reliability and power efficiency.
  * Connection subrating that was introduced in |NCS| 2.7.0 as experimental, is now supported.
    This feature offers a highly flexible way to change between high and low duty cycle connections without changing the connection interval, and is showcased in the sample :ref:`ble_subrating`.
  * |NCS| 2.8.0 is qualifiable for the Bluetooth LE core 6.0 specifications.

* Bluetooth Mesh:

  * The :ref:`Subnet Bridge <zephyr:bluetooth_mesh_brg_cfg>` profile, part of Bluetooth Mesh Protocol Specification version 1.1, is now qualified.

* Thread:

  * :ref:`Thread 1.4 <thread_ug_supported_features_v14>` is now supported on nRF52840, nRF5340, and nRF54L15 devices.
    It brings new features such as Thread credential sharing, enhanced internet connectivity, Thread-over-infrastructure, and secure commissioning at scale with TCAT.
    For more information about Thread 1.4, see the `Thread Group’s Whitepaper <Thread 1.4 Features White Paper_>`_.

* Sidewalk:

  * Sidewalk version 1.17 is now supported on nRF52840, nRF5340, and nRF54L15 devices.
    It brings improvements in metrics and capability reporting, and enables an end point to limit the uplink traffic (End Points traffic threshold).

* Cellular IoT:

  * Thingy:91 X cellular IoT prototyping platform based on the nRF9151 System in Package (SiP), which includes the nRF7002 Wi-Fi 6 companion IC for Wi-Fi locationing.

Added the following features as experimental:

* nRF54L Series:

  * :ref:`Channel Sounding <nrfxlib:softdevice_controller_cs>`
  * :ref:`Trusted Firmware-M <ug_tfm>`

* LE Audio:

  * The :ref:`nrf_auraconfig` sample, which implements the BIS gateway mode for the :ref:`nrf53_audio_app` and can act as an `Auracast™`_ broadcaster if you are using pre-sets compatible with Auracast.
    It features a shell interface that allows you to configure the broadcast source in many different ways.

* Bluetooth LE:

  * :ref:`Channel Sounding <nrfxlib:softdevice_controller_cs>`, which enables accurate and secure distance measuring with built-in security features and seamless interoperability.

* Wi-Fi :

  * :ref:`Offloaded raw transmit beaconing <ug_nrf70_developing_offloaded_raw_tx>`, allowing the beaconing to be done autonomously by nRF70 Series companion devices instead of being triggered by the host.
    This simplifies the host application firmware.
  * :ref:`Enterprise mode for station EAP-TLS <ug_nrf70_wifi_advanced_security_modes>`, which allows nRF70 Series companion devices to connect to Wi-Fi enterprise installations using EAP-TLS.

* DECT NR+:

  * A new sample :ref:`nRF91x1: DECT NR+ Shell <dect_shell_application>` that demonstrates how to setup a DECT NR+ application and enables testing various modem features and performance.

* nRF Cloud:

  * SMP FOTA, which allows updating a companion device such as an nRF52 Series device running MCUboot from a FOTA image received by an nRF91 Series device from the cloud.
    This is showcased in the sample :ref:`nrf_cloud_rest_fota`.

* Bootloaders and DFU:

  * :ref:`mcuboot_image_compression` on nRF52840, nRF5340, and nRF54L15 targets.
    This feature uses LZMA2 to compress the candidate image by ~30%, which gives more space for the application as it allows slot 1 (DFU slot) to be ~70% the size of slot 0 (application slot).

Improved:

* Wi-Fi:

  * Overall improvements that will bring increased connection stability and better interoperability with Access Points (APs).
  * Significant reduction in the boot-up current on nRF70 Series devices for passive scanning use case, by skipping TX calibration, as TX is not required for passive scanning operation.
    Boot up current drops from ~270 mA to ~80 mA.

Deprecated:

* Zigbee R22, which will be removed no earlier than after the next two releases, as per our :ref:`deprecation <api_deprecation>` policy.
  As a result, nRF52833, nRF52840, and nRF5340 devices are not recommended for new Zigbee designs.
  Support for Zigbee R23 will be available for the nRF54L Series devices as an `Add-on <nRF Connect SDK Add-ons_>`_.

Sign up for the `nRF Connect SDK v2.8.0 webinar`_ to learn more about the new features.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v2.8.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v2.8.0`_.

Integration test results
************************

The integration test results for this tag can be found in the following external artifactory:

* `Twister test report for nRF Connect SDK v2.8.0`_
* `Hardware test report for nRF Connect SDK v2.8.0`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v2.8.0.
See the :ref:`installation` section for more information about supported operating systems and toolchain.

Supported modem firmware
************************

See the following documentation for an overview of which modem firmware versions have been tested with this version of the |NCS|:

* `Modem firmware compatibility matrix for the nRF9151 SoC`_
* `Modem firmware compatibility matrix for the nRF9161 SoC`_
* `Modem firmware compatibility matrix for the nRF9160 SoC`_

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
     - `Changelog <Modem library changelog for v2.8.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v2.8.0_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v2.8.0`_ for the list of issues valid for the latest release.

Migration notes
***************

See the `Migration guide for nRF Connect SDK v2.8.0`_ for the changes required or recommended when migrating your application from |NCS| v2.7.0 to |NCS| v2.8.0.

.. _ncs_release_notes_280_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Added:

  * Explicit mention of the :ref:`requirements_jlink` being required in the :ref:`installing_vsc` section of the installation page.
  * :ref:`nrf_toolchain_docker_image`, a containerized version of the |NCS| toolchain that can be used for development and testing.

* Updated:

  * The required `SEGGER J-Link`_ version to v7.94i.
  * The :ref:`gs_recommended_versions` page by splitting the supported OS table into :ref:`supported_OS` and :ref:`additional_nordic_sw_tools_os_support`.
    The latter has received the following updates:

    * Linux 24.04 and macOS 15 have been added to the list.
    * macOS 10.15, macOS 11, and macOS 12 have been removed from the list.

* Deprecated the :ref:`requirements_clt`.
  They will be removed in an upcoming release.
  Transition to using `nRF Util`_ for all related tasks going forward.

Board support
=============

* Added support for the Thingy:91 X board, which uses the ``nordic,pm-ext-flash`` node instead of external flash device name in static partitions.
* Updated nRF54L15 Series devices to support different revisions of DKs and PDKs.
  For more information, see :ref:`nRF54L15 revisions <ug_nrf54L15_revision>`.

Build and configuration system
==============================

* Added:

  * The :kconfig:option:`SB_CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM` sysbuild Kconfig option to system that allows utilizing all available RAM when using TF-M on an nRF5340 device.

    .. note::
       This has security implications and might allow secrets to be leaked to the non-secure application in RAM.

  * The :kconfig:option:`SB_CONFIG_MCUBOOT_NRF53_MULTI_IMAGE_UPDATE` sysbuild Kconfig option that enables updating the network core on the nRF5340 SoC from external flash.
  * The AP-Protect sysbuild Kconfig options to enable the corresponding AP-Protect Kconfig options for all images in the build:

    * :kconfig:option:`SB_CONFIG_APPROTECT_LOCK` for the :kconfig:option:`CONFIG_NRF_APPROTECT_LOCK` Kconfig option.
    * :kconfig:option:`SB_CONFIG_APPROTECT_USER_HANDLING` for the :kconfig:option:`CONFIG_NRF_APPROTECT_USER_HANDLING` Kconfig option.
    * :kconfig:option:`SB_CONFIG_APPROTECT_USE_UICR` for the :kconfig:option:`CONFIG_NRF_APPROTECT_USE_UICR` Kconfig option.
    * :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_LOCK` for the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_LOCK` Kconfig option.
    * :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_USER_HANDLING` for the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING` Kconfig option.
    * :kconfig:option:`SB_CONFIG_SECURE_APPROTECT_USE_UICR` for the :kconfig:option:`CONFIG_NRF_SECURE_APPROTECT_USE_UICR` Kconfig option.

  * CMake warning when the static :ref:`partition_manager` file has been changed but changes will not be read until a pristine build is performed.
  * Encrypted firmware update to :file:`dfu_application.zip` instead of the unencrypted firmware update when encrypted image support is enabled in sysbuild.
    See :ref:`app_build_mcuboot_output` for details.
  * Sysbuild-assigned MCUboot image IDs feature, which deals with MCUboot image IDs for different components in a project.
  * The :kconfig:option:`SB_CONFIG_LWM2M_CARRIER_DIVIDED_DFU` sysbuild Kconfig option that enables the generation of proprietary application update files required for the LwM2M carrier divided FOTA procedure.

* Fixed:

  * An issue with the |NCS| boot banner using ``CMAKE_SOURCE_DIR`` instead of ``APPLICATION_SOURCE_DIR``.
  * An issue with :kconfig:option:`CONFIG_MBEDTLS_CFG_FILE` having a prompt which would, in some circumstances, lead to a sticky value if the configuration was changed.
  * An issue with changes to image configurations in a ``sysbuild.cmake`` file not being applied to variant images.
  * An issue with signing variables not expanding strings for nRF70 Series devices.

* Deprecated the following devicetree properties:

  * ``owner-id``
  * ``perm-read``
  * ``perm-write``
  * ``perm-execute``
  * ``perm-secure``
  * ``non-secure-callable``

  It is recommended to replace them with the new devicetree property: ``nordic,access``.
  See the :ref:`migration guide <migration_2.8_recommended>` for more information.

* Removed documentation related to non-working support for configuring the NSIB signing key through the environmental or command line variable (:kconfig:option:`SB_SIGNING_KEY_FILE`) along with child image.

Bootloaders and DFU
===================

* Added:

  * Support for handling SHA512 by MCUmgr server protocol implementation.
  * :kconfig:option:`SB_CONFIG_MCUBOOT_NRF53_MULTI_IMAGE_UPDATE` sysbuild Kconfig to allow supporting network core application in MCUboot.
  * :ref:`QSPI XIP split image <qspi_xip_split_image>` support for the nRF52840 SoC and MCUboot's direct-xip mode.
  * Documentation for :ref:`mcuboot_image_compression`.
  * Documentation for :ref:`sysbuild_assigned_images_ids`.
  * A section in the sysbuild-related migration guide about the migration of :ref:`child_parent_to_sysbuild_migration_qspi_xip` from child/parent image to sysbuild.

* Updated the procedure for signing the application image built for booting by MCUboot in direct-xip mode with revert support.
  Now, the Intel HEX file of the application image automatically receives a confirmation flag.

* Fixed an issue with direct-xip (with revert) images not being marked as confirmed.
* Removed the secure bootloader Kconfig option ``CONFIG_SECURE_BOOT_DEBUG`` and replaced with usage of logging subsystem.

See also the :ref:`MCUboot <mcuboot:mcuboot_wrapper>` documentation.

SUIT DFU for nRF54H20
---------------------

* Added:

  * Semantic versioning.
  * Support for push-mode DFU to external flash.
  * ZIP file to allow to update using raw DFU caches.
  * Option to build custom SUIT recovery image (for example, without radio core)
  * Extended SUIT SMP commands to nRF Util and nRF Connect Device Manager.

* Updated:

  * The ``Confirm`` SMP command is now needed to trigger SUIT DFU.
  * Configuration options and SUIT manifests.

* Fixed:

  * An issue where usage of ``nrfutil device recover`` leads to setting SUIT recovery.
  * An issue where envelopes with SUIT components not needed for particular roles are not rejected as candidates.
  * An issue where a duplicate class ID in MPI leads to cyclic SDFW resets.
  * An issue where leftover data in SUIT storage prevents writing to SUIT storage for a new application or radio image.
  * An issue where ``smp_transfer`` west flash fails with cyclic resets afterwards.
  * An issue where the **suit-generator** generation of installed manifest images for custom manifest class ID is not possible.
  * An issue where manifest could declare any ``INSTLD_MFST`` component regardless of its class ID.

Developing with nRF91 Series
============================

* Added:

  * User guides for Thingy:91 X in the :ref:`ug_nrf91` documentation.
  * The :ref:`nRF91 modem tracing with RTT backend snippet <nrf91_modem_trace_rtt_snippet>` to enable modem tracing using the RTT trace backend.
  * The :ref:`nRF91 modem tracing with RAM backend snippet <nrf91_modem_trace_ram_snippet>` to enable modem tracing using the RAM trace backend.

Developing with nRF70 Series
============================

* Added the :ref:`ug_nrf70_features_hostap` section to the :ref:`ug_nrf70_features` page.

Developing with nRF54L Series
=============================

* Added:

  * nRF54l_snippets to emulate these targets on an nRF54L15 DK.
    These are used only for development purposes.
  * The :ref:`ug_nrf54l_cryptography` page that provides more information about the cryptographic peripherals of the nRF54L Series devices, programming model for referencing keys, and configuration.
  * A page on :ref:`zms_memory_storage` system.
  * The :ref:`vpr_flpr_nrf54l` and :ref:`building_nrf54l` pages.
  * The :ref:`ug_nrf54l_developing_ble_fota` page, describing FOTA update process and testing steps.
  * The :ref:`ug_nrf54l_developing_provision_kmu` page, including instructions on generating keys and provisioning them to the board.

* Updated the name and the structure of the section, with :ref:`ug_nrf54l` as the landing page.
* Removed the Getting started with the nRF54L15 PDK page, and instead included the information about the `Quick Start app`_ support.

Security
========

* Added:

  * The :kconfig:option:`CONFIG_CRACEN_IKG_SEED_KMU_SLOT` Kconfig option to allow customization of the KMU slot used to store CRACEN's Internal Key Generator (IKG) seed.
    The default IKG seed slot is now 183 (previously 0).
  * TF-M support for the :zephyr:board:`nrf54l15dk` (board target ``nrf54l15dk/nrf54l15/cpuapp/ns``), replacing the nRF54L15 PDK (board target ``nrf54l15pdk/nrf54l15/cpuapp/ns``).
  * The ``west ncs-provision`` command, which allows to provision signature verification keys to the nRF54L15 SoC over the J-Link interface.

* Deprecated legacy Mbed TLS crypto toolbox APIs that are enabled when the :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND` Kconfig option is set.
  Use the nRF Security (enabled with the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option) and PSA crypto APIs instead.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

* Added:

  * Support for the :zephyr:board:`nrf54l15dk`.
  * Protection mechanism for Sidewalk keys in non-volatile memory (secure storage).
  * Amazon Sidewalk libraries v1.17.
  * New CLI commands:

    * Commands for testing Sidewalk Bulk Data Transfer.
    * A command for entering the Nordic DFU state.
    * A command for retrieving the device ID.

* Updated:

  * The manufacturing storage module.
  * The SPI driver:

    * The recommended pin configuration has been changed for the nRF54L15 device.
    * The chip select pin control has been moved to the SPI driver.
    * The nrfx driver is directly used for the nRF52840 device.

  * The Sidewalk event module for the application.
  * The Sidewalk version print information.

* Removed the deprecated Bluetooth LE address type configuration.

Bluetooth LE
------------

* Added functions ``bt_hci_err_to_str()`` and ``bt_security_err_to_str()`` to allow printing error codes as strings.
  Each function returns string representations of the error codes when the corresponding Kconfig option, :kconfig:option:`CONFIG_BT_HCI_ERR_TO_STR` or :kconfig:option:`CONFIG_BT_SECURITY_ERR_TO_STR`, is enabled.
  The :ref:`ble_samples` and :ref:`nrf53_audio_app` are updated to use these new functions.

* Updated:

  * The SoftDevice Controller library to automatically select the :kconfig:option:`CONFIG_BT_LL_SOFTDEVICE_MULTIROLE` Kconfig option when using coexistence based on :kconfig:option:`CONFIG_MPSL_CX` for nRF52 Series devices.
  * The Bluetooth HCI driver is now present as a devicetree node in the device tree.
    The SoftDevice Controller driver uses a devicetree node named ``bt_hci_sdc`` with a devicetree binding compatible with ``nordic,bt-hci-sdc``.
    The Zephyr Bluetooth LE Controller uses a devicetree node named ``bt_hci_controller`` with a devicetree binding compatible with ``zephyr,bt-hci-ll-sw-split``.
    You need to update applications using the Zephyr Bluetooth Controller (see the :ref:`migration guide <migration_2.8>`).
  * Host calls in GATT Discovery Manager (DM) callbacks are now scheduled in a workqueue.
    The :kconfig:option:`CONFIG_BT_GATT_DM_WORKQ_CHOICE` Kconfig option allows you to select the workqueue implementation.
    You can select either a workqueue specific to GATT DM (default) or the system workqueue.
    You can use the system workqueue if creating a new thread is not viable due to memory constraints, but it is not recommended to have potential blocking calls in the system workqueue.

* Fixed an issue where the Bluetooth subsystem deadlocked when a Bluetooth link was lost during data transfer.
  In this scenario, the disconnected event was never delivered to the application.
  The issue only occurred when the :kconfig:option:`CONFIG_BT_HCI_ACL_FLOW_CONTROL` Kconfig option was enabled.
  This option is enabled by default on the nRF5340 DK.

Bluetooth Mesh
--------------

* Added metadata as optional parameter for the following models:

  * :ref:`bt_mesh_lightness_srv_readme`
  * :ref:`bt_mesh_light_hsl_srv_readme`
  * :ref:`bt_mesh_light_temp_srv_readme`
  * :ref:`bt_mesh_sensor_srv_readme`
  * :ref:`bt_mesh_time_srv_readme`

  To use the metadata, enable the :kconfig:option:`CONFIG_BT_MESH_LARGE_COMP_DATA_SRV` Kconfig option.

* Removed the ``CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE`` Kconfig option, deprecated in the |NCS| v2.6.0, as the old APIs, based on the ``sensor_value`` type, are removed.
  You need to update applications using the old APIs, as described in the :ref:`v2.6.0 migration guide <nrf5340_audio_migration_notes>`.

Matter
------

* Added:

  * The following Kconfig options to configure parameters impacting persistent subscriptions re-establishment:

    * :kconfig:option:`CONFIG_CHIP_MAX_ACTIVE_CASE_CLIENTS`
    * :kconfig:option:`CONFIG_CHIP_MAX_ACTIVE_DEVICES`
    * :kconfig:option:`CONFIG_CHIP_SUBSCRIPTION_RESUMPTION_MIN_RETRY_INTERVAL`
    * :kconfig:option:`CONFIG_CHIP_SUBSCRIPTION_RESUMPTION_RETRY_MULTIPLIER`

  * The :ref:`ug_matter_device_memory_profiling` section to the :ref:`ug_matter_device_optimizing_memory` page.
    The section contains useful commands for measuring memory and troubleshooting tips.
  * The ZMS file subsystem to all devices that contain RRAM, such as the nRF54L Series devices.
  * Migration of the Device Attestation Certificates private key to Key Management Unit (KMU) for the nRF54L Series SoCs.
    See :ref:`matter_platforms_security_dac_priv_key_kmu` to learn how to enable it in your sample.
  * Support for the Device Firmware Upgrade over Bluetooth LE SMP and Matter Over-the-air Software Update (OTA) on nRF54H20 SoC.

* Updated:

  * The default Trusted Storage AEAD key to Hardware Unique Key (HUK) for supported nRF54L Series devices.
  * Renamed the ``CONFIG_CHIP_FACTORY_RESET_ERASE_NVS`` Kconfig option to :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS`.
    The new Kconfig option now works for both NVS and ZMS file system backends.
  * The firmware version format used for informational purposes when using the :file:`VERSION` file.
    The format now includes the optional ``EXTRAVERSION`` component.
  * Storing the Device Attestation Certificates private key in the Trusted Storage library to be enabled for all platforms that support the PSA crypto API.
    See :ref:`matter_platforms_security_dac_priv_key_its` for more information.
  * The stack sizes for OpenThread and CHIP (Matter) threads to increase them for the targets using the CRACEN crypto backend.
  * The IPv6 multicast subscription mechanism for Wi-Fi to enable the MLDv2 join procedure and improve reliability in multicast communication.
  * Storing the Device Attestation Certificates private key in the Key Management Unit (KMU) to be enabled for nRF54L Series devices.
    See :ref:`matter_platforms_security_dac_priv_key_kmu` for more information.

Matter fork
+++++++++++

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to, and including, the ``v1.3.0.0`` tag.

Thread
------

* Added the :ref:`ug_thread_build_report` and described how to use it.
* Updated the default Trusted Storage AEAD key to Hardware Unique Key (HUK) for supported nRF54L Series devices.

Zigbee
------

* Updated:

  * ZBOSS Zigbee stack to v3.11.5.0 and platform v5.1.6 (``v3.11.5.0+5.1.6``).
    They contain a fix for the ZBOSS traces.
    For details, see ZBOSS changelog.
  * ZBOSS Network Co-processor Host package to the new version v2.2.4.

* Fixed the :file:`zb_add_ota_header.py` script not being able to handle an ``APPLICATION_VERSION_STRING`` which includes a tweak, such as ``1.0.0+3``.

Wi-Fi
-----

* Added:

  * The :ref:`ug_nrf70_developing_offloaded_raw_tx` feature.
  * Support for :ref:`WPA2-EAP-TLS <ug_nrf70_wifi_advanced_security_modes>` enterprise security mode.
  * Support for :ref:`Platform Security Architecture (PSA) crypto APIs <ug_nrf70_developing_wifi_psa_support>` for WPA2™ security profiles.

* Updated:

  * The WPA supplicant is now switched to Zephyr upstream's fork instead of |NCS|.
  * The WPA supplicant now uses ``kernel heap`` instead of ``application (libc) heap``.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Machine learning
----------------

* Added:

  * Support for the :zephyr:board:`nrf54l15dk` (``nrf54l15dk/nrf54l15/cpuapp``).
  * Support for sampling ADXL362 sensor from PPR core on the :zephyr:board:`nrf54h20dk`.

Asset Tracker v2
----------------

* Added a note that the Asset Tracker v2 application is in the maintenance mode and that it is recommended to use the :ref:`nrf_cloud_multi_service` sample instead.

Connectivity bridge
-------------------

* Updated the new nrfx UARTE driver implementation by setting the :kconfig:option:`CONFIG_UART_NRFX_UARTE_LEGACY_SHIM` Kconfig option to ``n``.
  This resolves an issue where data from UART0 ends up in UART1 sometimes after the device was reset.

Matter bridge
-------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option that specifies ZAP files location for the application.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to application's location that contains the ZAP file and :file:`zap-generated` directory.
  * Experimental support for the :zephyr:board:`nrf54h20dk` board.
  * Optional smart plug device functionality.
  * Experimental support for the Thread protocol.
  * The :ref:`multiprotocol_bt_thread` page.

nRF5340 Audio
-------------

* Added:

  * The functions ``bt_hci_err_to_str()`` and ``bt_security_err_to_str()`` that are used to allow printing error codes as strings.
    Each function returns string representations of the error codes when the corresponding Kconfig option, :kconfig:option:`CONFIG_BT_HCI_ERR_TO_STR` or :kconfig:option:`CONFIG_BT_SECURITY_ERR_TO_STR`, is enabled.
  * CSIS to the BIS sink if the scan delegator feature, :ref:`CONFIG_BT_AUDIO_SCAN_DELEGATOR <CONFIG_BT_AUDIO_SCAN_DELEGATOR>`, is enabled.
    Once a phone is connected to a BIS sink, the phone will find and connect to the second headset.
    Also, the phone can control the BIS headset in a group and deliver the PAST to both headsets at the same time.
  * Create CIG after reading the PACS from the first connected unicast server.
  * A minimal scan delegator to the unicast server if the feature, :ref:`CONFIG_BT_AUDIO_SCAN_DELEGATOR <CONFIG_BT_AUDIO_SCAN_DELEGATOR>`, is enabled.
  * Available or support context type to PACS in broadcast sink and unicast client if the feature, :kconfig:option:`CONFIG_BT_PAC_SRC_NOTIFIABLE` is enabled.
  * The :ref:`nrf_auraconfig` sample.

* Updated the :ref:`nrf53_audio_app_overview` documentation page with the :ref:`nrf53_audio_app_overview_files` section.
* Removed the ``EXPERIMENTAL`` flag from the sample rate converter.

nRF Desktop
-----------

* Added:

  * Support for the :zephyr:board:`nrf54l15dk` (board target ``nrf54l15dk/nrf54l15/cpuapp``), replacing the nRF54L15 PDK (board target ``nrf54l15pdk/nrf54l15/cpuapp``).
  * A debug configuration enabling the `Fast Pair`_ feature on the nRF54L15 DK with the ``nrf54l15dk/nrf54l15/cpuapp`` board target.
  * An application versioning using the :file:`VERSION` file.
    The versioning is only applied to the application configurations that use the MCUboot bootloader.
  * The :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option to :ref:`nrf_desktop_usb_state`.
    The option allows to synchronize providing HID data with USB Start of Frame (SOF).
    The feature reduces the negative impact of jitter related to USB polls, but it also increases HID data latency.
    For details, see :ref:`nrf_desktop_usb_state_sof_synchronization`.
  * Local HID report buffering in :ref:`nrf_desktop_usb_state`.
    This ensures that the memory buffer passed to the USB next stack is valid until a HID report is sent and allows to enqueue up to two HID input reports for a USB HID instance (used only when :ref:`CONFIG_DESKTOP_USB_HID_REPORT_SENT_ON_SOF <config_desktop_app_options>` Kconfig option is enabled).
  * Bootup logs with the manifest semantic version information to :ref:`nrf_desktop_dfu_mcumgr` when the module is used for SUIT DFU and the SDFW supports semantic versioning (requires v0.6.2 and higher).
  * Manifest semantic version information to the firmware information response in :ref:`nrf_desktop_dfu` when the module is used for SUIT DFU and the SDFW supports semantic versioning (requires v0.6.2 and higher).
  * A missing DTS node compatible with ``zephyr,hid-device`` to the nRF52840 DK in the MCUboot QSPI configuration.
    This ensures support for HID over USB when the USB next stack is selected.
  * Partial erase feature as implied in the USB next stack (:ref:`CONFIG_DESKTOP_USB_STACK_NEXT <config_desktop_app_options>`) for the nRF SoC flash driver (:kconfig:option:`CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE`).
    This is done to improve stability of the USB next stack.
    The partial erase feature works around device errors that might be reported by Windows USB host in Device Manager if USB cable is connected while erasing secondary image slot in the background.
  * As implied in the Bluetooth connectivity support (:ref:`CONFIG_DESKTOP_BT <config_desktop_app_options>`), a separate workqueue is used for connection TX notify processing (:kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ`) if MPSL is used for synchronization between the flash memory driver and radio (:kconfig:option:`CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL`).
    This is done to work around the timeout in MPSL flash synchronization (``NCSDK-29354`` known issue).
    See :ref:`known_issues` for details.
  * Note informing about :file:`dfu_application.zip` format version ``1`` support as part of :ref:`nrf_desktop_fwupd`.
    The format version ``1`` is supported by the fwupd since the 1.9.25 release.
  * :ref:`nrf_desktop_dvfs` that can be used on the nRF54H20 SoC.
    The module is responsible for switching the frequency and voltage according to the application's needs.
    The module ensures that lowest DVFS frequency and voltage are used unless computing-intensive operation requests higher frequency and voltage.
    The module listens to Event Manager events and applies DVFS frequency according to the application state.
  * Handling of :c:struct:`hid_report_subscriber_event` to :ref:`nrf_desktop_hid_forward`.
    Handling the :c:struct:`hid_report_subscriber_event` allows to take into account the maximum number of reports with different IDs, which can be processed by the subscriber (:c:member:`hid_report_subscriber_event.report_max`).
  * :ref:`nrf_desktop_hid_reportq`, which is integrated in :ref:`nrf_desktop_hid_forward`.
    The utility can be used by an application module to simplify enqueuing HID input reports received from connected HID peripherals before providing them to HID subscriber.
    The HID report queue utility is used to locally enqueue reports at the source to prevent HID report drops.
    The ``CONFIG_DESKTOP_HID_FORWARD_MAX_ENQUEUED_REPORTS`` Kconfig option was removed, use :ref:`CONFIG_DESKTOP_HID_REPORTQ_MAX_ENQUEUED_REPORTS <config_desktop_app_options>` instead.
  * The Kconfig option :ref:`CONFIG_DESKTOP_HID_FORWARD_SUBSCRIBER_COUNT <config_desktop_app_options>` that configures the number of HID subscribers in :ref:`nrf_desktop_hid_forward`.
    The value of the Kconfig option must match the number of USB HID class instances.
  * Bluetooth LE Low Latency Packet Mode (LLPM) dependency (:kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`) to :ref:`nrf_desktop_hfclk_lock`.
    Using LLPM connection parameters reduces HID data latency far more significantly than enabling the hotfix.

* Updated:

  * The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option value in configurations with the Fast Pair support.
    The value is now aligned with the Fast Pair requirements.
  * The :kconfig:option:`CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE` Kconfig option value in the nRF54L15 DK configurations to ensure short write slots.
    It prevents timeouts in the MPSL flash synchronization caused by allocating long write slots while maintaining a Bluetooth LE connection with short intervals and no connection latency.
  * The method of obtaining hardware ID using Zephyr's :ref:`zephyr:hwinfo_api` on the :zephyr:board:`nrf54h20dk`.
    Replaced the custom implementation of the :c:func:`z_impl_hwinfo_get_device_id` function in the nRF Desktop application with the native Zephyr driver function that now supports the :zephyr:board:`nrf54h20dk` board target.
    Removed the ``CONFIG_DESKTOP_HWINFO_BLE_ADDRESS_FICR_POSTFIX`` Kconfig option as a postfix constant is no longer needed for the Zephyr native driver.
    The driver uses ``BLE.ADDR``, ``BLE.IR``, and ``BLE.ER`` fields of the Factory Information Configuration Registers (FICR) to provide 8 bytes of unique hardware ID.
  * The :ref:`nrf_desktop_dfu_mcumgr` to recognize the MCUmgr custom group ID (:kconfig:option:`CONFIG_MGMT_GROUP_ID_SUIT`) from the SUITFU subsystem (:kconfig:option:`CONFIG_MGMT_SUITFU`) as a DFU-related command group.
  * All build configurations with the DFU over MCUmgr support to require encryption for operations on the Bluetooth GATT SMP service (see the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_ENCRYPT` Kconfig option).
    The Bluetooth pairing procedure of the unpaired Bluetooth peers must now be performed before the DFU operation.
  * The :ref:`nrf_desktop_dfu_mcumgr` to enable the MCUmgr handler that is used to report the bootloader information (see the :kconfig:option:`CONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO` Kconfig option).
  * The MCUboot image configurations for the :zephyr:board:`nrf54l15dk` board to enable Link Time Optimization (LTO) (see the :kconfig:option:`CONFIG_LTO` Kconfig option) and reduce the memory footprint of the bootloader.
  * The partition memory configurations for the :zephyr:board:`nrf54l15dk` board to optimize the size of the MCUboot bootloader partition.
  * The :ref:`nrf_desktop_constlat` to use the :c:func:`nrfx_power_constlat_mode_request` and :c:func:`nrfx_power_constlat_mode_free` functions instead of :c:func:`nrf_power_task_trigger` to control requesting Constant Latency sub-power mode.
    This ensures correct behavior if another source requests Constant Latency sub-power mode through the nrfx API.
  * The :ref:`CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY <config_desktop_app_options>` Kconfig option depends on :kconfig:option:`CONFIG_CAF_PM_EVENTS`.
    CAF Power Management events support is necessary to disable constant latency interrupts on standby.
  * The main application's configuration directory (``APPLICATION_CONFIG_DIR`` CMake option) is set only in the :file:`CMakeLists.txt` file of the application.
    Configuration directories used for other built images and sysbuild are set through sysbuild's :file:`CMakeLists.txt` file.

* Deprecated the :ref:`nrf_desktop_hfclk_lock`.
  Use the :ref:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK <config_desktop_app_options>` Kconfig option instead.
  Setting the peripheral latency Bluetooth LE connection parameter to ``0`` for a connection that uses Low Latency Packet Mode connection interval on peripheral leads to keeping the high frequency clock enabled.
  That mitigates the extra HID report latency caused by the high frequency clock startup delay.

* Removed support for the nRF54L15 PDK revision v0.2.x.

Serial LTE modem
----------------

* Added:

  * DTLS support for the ``#XUDPSVR`` and ``#XSSOCKET`` (UDP server sockets) AT commands when the :file:`overlay-native_tls.conf` configuration file is used.
  * The :ref:`CONFIG_SLM_PPP_FALLBACK_MTU <CONFIG_SLM_PPP_FALLBACK_MTU>` Kconfig option that is used to control the MTU used by PPP when the cellular link MTU is not returned by the modem in response to the ``AT+CGCONTRDP=0`` AT command.
  * Handler for new nRF Cloud event type :c:enumerator:`NRF_CLOUD_EVT_RX_DATA_DISCON`.
  * Support for socket option ``AT_SO_IPV6_DELAYED_ADDR_REFRESH``.

* Updated:

  * AT string parsing to utilize the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.
  * The ``#XUDPCLI`` and ``#XSSOCKET`` (UDP client sockets) AT commands to use Zephyr's Mbed TLS with DTLS when the :file:`overlay-native_tls.conf` configuration file is used.

* Fixed reading network registration status with ``AT+CEREG`` when the :ref:`CONFIG_SLM_AUTO_CONNECT <CONFIG_SLM_AUTO_CONNECT>` option is enabled.

* Removed:

  * Support for the :file:`overlay-native_tls.conf` configuration file with the ``thingy91/nrf9160/ns`` board target.
  * Support for deprecated RAI socket options ``AT_SO_RAI_LAST``, ``AT_SO_RAI_NO_DATA``, ``AT_SO_RAI_ONE_RESP``, ``AT_SO_RAI_ONGOING``, and ``AT_SO_RAI_WAIT_MORE``.
  * The ``#XCARRIERCFG="bootstrap_smartcard"`` AT command.

Thingy:53: Matter weather station
---------------------------------

* Added the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the application.
  By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to application's location that contains the ZAP file and :file:`zap-generated` directory.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added:

  * The :ref:`ble_radio_notification_conn_cb` sample demonstrating how to use the :ref:`ug_radio_notification_conn_cb` feature.
  * The :ref:`bluetooth_conn_time_synchronization` sample demonstrating microsecond-accurate synchronization of connections that are happening over Bluetooth Low Energy Asynchronous Connection-oriented Logical transport (ACL).
  * The :ref:`ble_subrating` sample that showcases the effect of the LE Connection Subrating feature on the duty cycle of a connection.
  * The :ref:`nrf_auraconfig` sample that implements the :ref:`BIS gateway mode <nrf53_audio_app_overview>` and can act as an `Auracast <Auracast™_>`_ broadcaster if you are using a preset compatible with Auracast.
  * Support for the :zephyr:board:`nrf54l15dk` board in the following samples:

    * :ref:`central_bas`
    * :ref:`bluetooth_central_hr_coded`
    * :ref:`multiple_adv_sets`
    * :ref:`peripheral_bms`
    * :ref:`peripheral_cgms`
    * :ref:`peripheral_cts_client`
    * :ref:`peripheral_gatt_dm`
    * :ref:`peripheral_hr_coded`
    * :ref:`peripheral_mds`
    * :ref:`peripheral_nfc_pairing`
    * :ref:`power_profiling`
    * :ref:`peripheral_rscs`
    * :ref:`shell_bt_nus`
    * :ref:`ble_throughput`
    * :ref:`bluetooth_central_dfu_smp`
    * :ref:`peripheral_status`

* :ref:`bluetooth_isochronous_time_synchronization` sample:

  * Fixed **LED** toggling issues on nRF52 and nRF53 Series devices that would occur after RTC wraps that occur approximately every 8.5 minutes.
    The **LED** previously toggled unintentionally, at the wrong point in time, or not at all.

* :ref:`ble_event_trigger` sample:

  * Moved to the :file:`samples/bluetooth/event_trigger` folder.

* :ref:`peripheral_hr_coded` sample:

  * Fixed an issue where the HCI LE Set Extended Advertising Enable command was called with a NULL pointer.

* :ref:`peripheral_mds` sample:

  * Fixed an issue where device ID was incorrectly set during system initialization because MAC address was not available at that time.
    The device ID is now set to ``ncs-ble-testdevice`` by default using the :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID` Kconfig option.

* :ref:`power_profiling` sample:

  * Added :ref:`CONFIG_BT_POWER_PROFILING_LED_DISABLED <CONFIG_BT_POWER_PROFILING_LED_DISABLED>` Kconfig option to disable the LEDs.

* :ref:`ble_llpm` sample:

  * Added support for the :zephyr:board:`nrf54h20dk` board.

* :ref:`bluetooth_radio_coex_1wire_sample` sample:

  * Added support for the ``nrf54h20dk/nrf54h20/cpurad`` and ``nrf54l15dk/nrf54l15/cpuapp`` build targets.

Bluetooth Fast Pair samples
---------------------------

* Added support for the :zephyr:board:`nrf54l15dk` (board target ``nrf54l15dk/nrf54l15/cpuapp``), replacing the nRF54L15 PDK (board target ``nrf54l15pdk/nrf54l15/cpuapp``).

* Updated:

  * The values for the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` Kconfig option in all configurations, and for the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_TX_POWER_CORRECTION_VAL` Kconfig option in configurations with the Find My Device Network (FMDN) extension support.
    The values are now aligned with the Fast Pair requirements.
  * The sample configurations to use a separate workqueue for connection TX notify processing (:kconfig:option:`CONFIG_BT_CONN_TX_NOTIFY_WQ`).
    This is done to work around the timeout in MPSL flash synchronization (``NCSDK-29354`` known issue).
    See :ref:`known_issues` for details.

* :ref:`fast_pair_locator_tag` sample:

  * Added:

    * LED indication on development kits for the Fast Pair advertising state.
    * An application versioning using the :file:`VERSION` file.
    * The DFU support, which can be enabled using the :kconfig:option:`SB_CONFIG_APP_DFU` sysbuild Kconfig option.
      DFU is available for all supported targets except the ``debug`` configurations of :zephyr:board:`nrf52dk` and :zephyr:board:`nrf52833dk` due to size constraints.
    * An application module for the DULT motion detector feature and the new UI for supported board targets to interact with this feature.
      The development kit board targets simulate the motion with the button presses.
      The Thingy:53 target uses a 6-axis IMU with gyroscope to detect motion.

  * Updated:

    * The :ref:`ipc_radio` image configuration by splitting it into the debug and release configurations.
    * The location of the sample configuration.
      It has been moved from the root sample directory to the dedicated folder (:file:`locator_tag/configuration`).
    * The ``fp_adv`` module to use the trigger requests for the Fast Pair advertising state instead of setting the Fast Pair advertising mode directly.

Bluetooth Mesh samples
----------------------

* Added:

  * Support for the :zephyr:board:`nrf54l15dk` (board target ``nrf54l15dk/nrf54l15/cpuapp``), replacing the nRF54L15 PDK (board target ``nrf54l15pdk/nrf54l15/cpuapp``).
  * Support for Zephyr Memory Storage (ZMS) when compiling for the :zephyr:board:`nrf54l15dk` board.

* :ref:`bluetooth_ble_peripheral_lbs_coex` sample:

  * Updated the usage of the :c:macro:`BT_LE_ADV_CONN` macro.
    See the Bluetooth Host section in Zephyr's :ref:`zephyr:migration_3.7`.

Cellular samples
----------------

* Added the :ref:`uicc_lwm2m_sample` sample.

* :ref:`fmfu_smp_svr_sample` sample:

  * Removed the unused :ref:`at_cmd_parser_readme` library.

* :ref:`modem_shell_application` sample:

  * Added ``link modem`` command for initializing and shutting down the modem.
  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`nrf_cloud_rest_fota` sample:

  * Added:

    * Support for setting the FOTA update check interval using the config section in the shadow.
    * Support for SMP FOTA on the nRF9160 DK.

  * Removed redundant logging that is now done by the :ref:`lib_nrf_cloud` library.

* :ref:`nrf_cloud_multi_service` sample:

  * Added:

    * The :ref:`CONFIG_TEST_COUNTER_MULTIPLIER <CONFIG_TEST_COUNTER_MULTIPLIER>` Kconfig option to multiply the number of test counter messages sent, for testing purposes.
    * A handler for new nRF Cloud event type :c:enumerator:`NRF_CLOUD_EVT_RX_DATA_DISCON` to stop sensors and location services.
    * Board support files to enable Wi-Fi scanning for the Thingy:91 X.
    * The :ref:`CONFIG_SEND_ONLINE_ALERT <CONFIG_SEND_ONLINE_ALERT>` Kconfig option to enable calling the :c:func:`nrf_cloud_alert` function on startup.
    * Logging of the `reset reason code <nRF9160 RESETREAS_>`_.
    * The :ref:`CONFIG_POST_PROVISIONING_INTERVAL_M <CONFIG_POST_PROVISIONING_INTERVAL_M>` Kconfig option to reduce the provisioning connection interval once the device successfully connects.

  * Updated:

    * Wi-Fi overlays from newlibc to picolib.
    * Handling of JITP association to improve speed and reliability.
    * Renamed the :file:`overlay_nrf7002ek_wifi_no_lte.conf` overlay to :file:`overlay_nrf700x_wifi_mqtt_no_lte.conf`.
    * Renamed the :file:`overlay_nrf7002ek_wifi_coap_no_lte.conf` overlay to :file:`overlay_nrf700x_wifi_coap_no_lte.conf`.
    * The value of the :kconfig:option:`CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE` Kconfig option in the :file:`overlay_coap.conf` file.
      A larger value is required now that the :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` Kconfig option is enabled.
    * Handling of credentials check to disable network if not using the provisioning service.

  * Fixed an issue where the accepted shadow was not marked as received because the config section did not yet exist in the shadow.
  * Removed redundant logging that is now done by the :ref:`lib_nrf_cloud` library.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Added:

    * Support for dictionary logs using REST.
    * The :ref:`CONFIG_SEND_ONLINE_ALERT <CONFIG_SEND_ONLINE_ALERT>` Kconfig option to enable calling the :c:func:`nrf_cloud_alert` function on startup.
    * Logging of the `reset reason code <nRF9160 RESETREAS_>`_.

  * Updated:

    * Credentials check to also see if AWS root CA cert is likely present.
    * Credentials check failure to disable network if not using the provisioning service.

  * Removed redundant logging that is now done by the :ref:`lib_nrf_cloud` library.

* :ref:`nrf_cloud_rest_cell_location` sample:

  * Removed redundant logging that is now done by the :ref:`lib_nrf_cloud` library.

* :ref:`smp_svr` sample:

  * Added sysbuild configuration files.

Cryptography samples
--------------------

* Added support for the :zephyr:board:`nrf54l15dk` (board target ``nrf54l15dk/nrf54l15/cpuapp/ns``), replacing the nRF54L15 PDK (board target ``nrf54l15pdk/nrf54l15/cpuapp/ns``).

Debug samples
-------------

* :ref:`memfault_sample` sample:

  * Increased the value of the :kconfig:option:`CONFIG_MAIN_STACK_SIZE` Kconfig option to 8192 bytes to avoid stack overflow.

DECT NR+ samples
----------------

* Added the :ref:`dect_shell_application` sample.

Edge Impulse samples
--------------------

* Added support for the :zephyr:board:`nrf54l15dk` board in the following samples:

  * :ref:`ei_data_forwarder_sample`
  * :ref:`ei_wrapper_sample`

Matter samples
--------------

* Added:

  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH` Kconfig option, which specifies ZAP files location for the sample.
    By default, the option points to the :file:`src/default_zap` directory and can be changed to any path relative to sample's location that contains the ZAP file and :file:`zap-generated` directory.
  * Support for the :zephyr:board:`nrf54l15dk`, replacing the nRF54L15 PDK.
  * Support for :ref:`Trusted Firmware-M <ug_tfm>` on the nRF54L15 SoC.
  * The :ref:`matter_smoke_co_alarm_sample` sample that demonstrates implementation of Matter Smoke CO alarm device type.
  * The :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_LEDS` Kconfig option, which can be used to disable the LEDs in the Matter sample or application.

* Updated:

  * All samples to enable the :ref:`ug_thread_build_report` generation.
  * Enabled Matter OTA on the nRF54H20 DK.

* :ref:`matter_lock_sample` sample:

  * Added :ref:`Matter Lock schedule snippet <matter_lock_snippets>` and updated the documentation to use the snippet.

  * Updated:

    * The low power configuration to be enabled for the nRF54L15 DK target.
    * The Link-Time Optimization (LTO) to be enabled for the nRF7002 DK target.

* :ref:`matter_smoke_co_alarm_sample` sample:

  * Updated the low power configuration to be enabled for the nRF54L15 DK target.

* :ref:`matter_template_sample` sample:

  * Updated the DAC private key migration from factory data to KMU to be enabled for the nRF54L Series SoCs by default.

* :ref:`matter_window_covering_sample` sample:

  * Updated the low power configuration to be enabled for the nRF54L15 DK target.


Networking samples
------------------

* Added support for the :zephyr:board:`nrf54l15dk` board with an nRF7002 EB device for the following samples:

  * :ref:`aws_iot`
  * :ref:`download_sample`
  * :ref:`mqtt_sample`
  * :ref:`https_client`
  * :ref:`udp_sample`
  * :ref:`net_coap_client_sample`

* :ref:`http_server` sample:

  * Fixed an issue so that the sample does not fail with a fatal error if IPv4 or IPv6 server setup fails.

nRF5340 samples
---------------

* :ref:`smp_svr_ext_xip` sample:

  * Added:

    * Support for sysbuild.
    * Support to demonstrate direct-xip building and building without network core support.

Peripheral samples
------------------

* :ref:`802154_sniffer` sample:

  * Added sysbuild configuration for nRF5340.
  * Increased the number of RX buffers to reduce the chances of frame drops during high traffic periods.
  * Disabled the |NCS| boot banner.
  * Fixed the dBm value reported for captured frames.

* :ref:`802154_phy_test` sample:

  * Added build configuration for the nRF54H20.

* :ref:`radio_test` sample:

  * Added packet reception limit for the ``start_rx`` command.

PMIC samples
------------

* Added support for the :zephyr:board:`nrf54l15dk` and :zephyr:board:`nrf54h20dk` to the PMIC samples.

* :ref:`nPM1300: Fuel gauge <npm13xx_fuel_gauge>` sample:

  * Updated to accommodate API changes in nRF Fuel Gauge library v0.11.1.

Protocols serialization samples
-------------------------------

* Added the :ref:`nrf_rpc_protocols_serialization_client` and the :ref:`nrf_rpc_protocols_serialization_server` samples.

Trusted Firmware-M (TF-M) samples
---------------------------------

* Added support for the :zephyr:board:`nrf54l15dk` (``nrf54l15dk/nrf54l15/cpuapp/ns``), replacing the nRF54L15 PDK (``nrf54l15pdk/nrf54l15/cpuapp/ns``).

* :ref:`tfm_psa_template` sample:

  * Added support for updating the network core on the nRF5340 DK.

Thread samples
--------------

* Updated all samples to enable the :ref:`ug_thread_build_report` generation.

* :ref:`ot_cli_sample` sample:

  * Added support for the :zephyr:board:`nrf54l15dk` in the low-power snippet.
  * Added experimental support for :ref:`Trusted Firmware-M <ug_tfm>` on the nRF54L15 SoC.

Zigbee samples
--------------

* Zigbee light switch sample:

  * Added the option to configure transmission power.
  * Fixed the FOTA configuration for the nRF5340 DK.

Wi-Fi samples
-------------

* Added:

  * The :ref:`wifi_offloaded_raw_tx_packet_sample` sample that demonstrates offloaded raw transmit beaconing, allowing the beaconing to be done autonomously by nRF7002 instead of being triggered by the host.
    This allows using simpler hosts as it requires less processing from the host side.

* :ref:`wifi_radio_test` sample:

  * Added capture timeout as a parameter for packet capture.
  * Expanded the scope of ``wifi_radio_test show_config`` subcommand and rectified the behavior of ``wifi_radio_test tx_pkt_preamble`` subcommand.

* :ref:`softap_wifi_provision_sample` sample:

  * Increased the value of the :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_THREAD_STACK_SIZE` Kconfig option to 8192 bytes to avoid stack overflow.

* :ref:`wifi_shell_sample` sample:

  * Added:

    * Support for running the full stack on the Thingy:91 X.
      This is a special configuration that uses the nRF5340 as the host chip instead of the nRF9151.
    * Overlay to support Enterprise mode.

Other samples
-------------

* Added:

  * The :ref:`nrf_compression_mcuboot_compressed_update` sample that demonstrates how to enable and use :ref:`image compression within MCUboot <mcuboot_image_compression>`.
  * A sample for the :ref:`multicore_idle_gpio_test`.
  * A sample for the :ref:`multicore_idle_with_pwm_test`.

* :ref:`coremark_sample` sample:

  * Added support for the :zephyr:board:`nrf54l15dk` (board target ``nrf54l15dk/nrf54l15/cpuapp``), replacing the nRF54L15 PDK (board target ``nrf54l15pdk/nrf54l15/cpuapp``).
  * Updated the logging mode to minimal (:kconfig:option:`CONFIG_LOG_MODE_MINIMAL`) to reduce the sample's memory footprint and ensure no logging interference with the running benchmark.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

Wi-Fi drivers
-------------

* The nRF70 Series Wi-Fi driver has been upstreamed to Zephyr, so the source code has been removed from the |NCS|.
  However, the :ref:`driver documentation <wifi_drivers>` remains available within the |NCS| documentation.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Updated to v3.6.0.
    See the :ref:`liblwm2m_carrier_changelog` for detailed information.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_BN` Kconfig option that enables support for the Battery Notification extension.
      You must enable this option to access Fast Pair API elements associated with the Battery Notification extension.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option allowing the user to control the support for the Fast Pair subsequent pairing feature.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE` Kconfig choice option allowing the user to select their target Fast Pair use case.
      The :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_UNKNOWN`, :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_INPUT_DEVICE`, :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_LOCATOR_TAG` and :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_MOUSE` Kconfig options represent the supported use cases that can be selected as part of this Kconfig choice option.
    * The :kconfig:option:`CONFIG_BT_FAST_PAIR_BOND_MANAGER` Kconfig option that enables the Fast Pair bond management functionality.
      If this option is enabled, the Fast Pair subsystem tracks the Bluetooth bonds created through the Fast Pair Procedure and unpairs them if the procedure is incomplete or the Account Key associated with the bonds is removed.
      It also unpairs the Fast Pair Bluetooth bonds on Fast Pair factory reset.
      The option is enabled by default for Fast Pair use cases that are selected using :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_INPUT_DEVICE` and :kconfig:option:`CONFIG_BT_FAST_PAIR_USE_CASE_MOUSE` Kconfig options.
    * The DULT motion detector feature integration in the FMDN extension.
      You can enable this feature using the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR` Kconfig option.

  * Updated the default values of the following Fast Pair Kconfig options:

    * :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING`
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_REQ_PAIRING`
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_PN`
    * :kconfig:option:`CONFIG_BT_FAST_PAIR_GATT_SERVICE_MODEL_ID`

    These Kconfig options are now disabled by default and are selected only by the Fast Pair use cases that require them.

  * Removed:

    * The Mbed TLS cryptographic backend support in Fast Pair, because it is superseded by the PSA backend.
      Consequently, the ``CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS`` Kconfig option has also been removed.
    * The default overrides for the :kconfig:option:`CONFIG_BT_DIS` and :kconfig:option:`CONFIG_BT_DIS_FW_REV` Kconfig options that enable these options together with the Google Fast Pair Service.
      This configuration is now selected only by the Fast Pair use cases that require the Device Information Service (DIS).
    * The default override for the :kconfig:option:`CONFIG_BT_DIS_FW_REV_STR` Kconfig option that was set to :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` if :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` was enabled.
      The default override is now handled in the Kconfig of the Zephyr Device Information Service (DIS) module and is based on Zephyr's :ref:`zephyr:app-version-details` that uses the :file:`VERSION` file.
    * The ``bt_fast_pair_factory_reset_user_action_prepare()`` weak function definition, which could previously be overridden to prepare for the incoming Fast Pair factory reset.
      You can still override the ``bt_fast_pair_factory_reset_user_action_perform()`` weak function to perform custom actions during the Fast Pair factory reset.

* :ref:`bt_le_adv_prov_readme`:

  * Added the :c:member:`bt_le_adv_prov_adv_state.adv_handle` field to the :c:struct:`bt_le_adv_prov_adv_state` structure to store the advertising handle.
    If the :kconfig:option:`CONFIG_BT_EXT_ADV` Kconfig option is enabled, you can use the ``bt_hci_get_adv_handle()`` function to obtain the advertising handle for the advertising set that employs :ref:`bt_le_adv_prov_readme`.
    If the Kconfig option is disabled, the :c:member:`bt_le_adv_prov_adv_state.adv_handle` field must be set to ``0``.
    This field is currently used by the TX Power provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`).
  * Updated the :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_SHOW_UI_PAIRING` Kconfig option and the :c:func:`bt_le_adv_prov_fast_pair_show_ui_pairing` function to require the enabling of the :kconfig:option:`CONFIG_BT_FAST_PAIR_SUBSEQUENT_PAIRING` Kconfig option.

Common Application Framework
----------------------------

* :ref:`caf_ble_state`:

  * The Low Latency Packet Mode (:kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`) cannot be enabled on nRF53 SoC Series (:kconfig:option:`CONFIG_SOC_SERIES_NRF53X`).
    The nRF53 SoC Series does not support LLPM.

Debug libraries
---------------

* :ref:`mod_memfault` library:

  * Added location metrics, including GNSS, cellular, and Wi-Fi specific metrics.
    The metrics are enabled with the :kconfig:option:`CONFIG_MEMFAULT_NCS_LOCATION_METRICS` Kconfig option.

DFU libraries
-------------

* Added the ``subsys_suit`` library that provides functionality to a local domain for managing the update based on the SUIT manifest.

* :ref:`lib_dfu_target` library:

  * Added SUIT cache processing to the DFU Target SUIT library, as described in the ``lib_dfu_target_suit_style_update`` section.
  * Updated the DFU Target SUIT implementation to the newest version of the SUIT.

Modem libraries
---------------

* Added:

  * The :ref:`at_parser_readme` library that parses AT command responses, notifications, and events.
    Compared to the deprecated :ref:`at_cmd_parser_readme` library, it does not allocate memory dynamically and has a smaller footprint.
    For more information on how to transition from the :ref:`at_cmd_parser_readme` library to the :ref:`at_parser_readme` library, see the :ref:`migration guide <migration_2.8_recommended>`.
  * The :ref:`lib_uicc_lwm2m` library that reads the LwM2M bootstrap configuration from SIM.

* :ref:`at_cmd_parser_readme` library:

  * Updated to use the :c:func:`at_parser_cmd_type_get` function instead of :c:func:`at_parser_at_cmd_type_get` to prevent a name collision.
  * Deprecated:

    * The :ref:`at_cmd_parser_readme` library in favor of the :ref:`at_parser_readme` library.
      The :ref:`at_cmd_parser_readme` library will be removed in a future version.
      For more information on how to transition from the :ref:`at_cmd_parser_readme` library to the :ref:`at_parser_readme` library, see the :ref:`migration guide <migration_2.8_recommended>`.
    * The Kconfig option :kconfig:option:`CONFIG_AT_CMD_PARSER`, which will be removed in a future release.

* :ref:`lte_lc_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL_MODULE` Kconfig option to enable the Connection Parameters Evaluation module.
    * The :kconfig:option:`CONFIG_LTE_LC_EDRX_MODULE` Kconfig option to enable the eDRX module.
    * The :kconfig:option:`CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE` Kconfig option to enable the Neighboring Cell Measurements module.
    * The :kconfig:option:`CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` Kconfig option to enable the Periodic Search Configuration module.
    * The :kconfig:option:`CONFIG_LTE_LC_PSM_MODULE` Kconfig option to enable the PSM module.
    * The :kconfig:option:`CONFIG_LTE_LC_RAI_MODULE` Kconfig option to enable the RAI module.
    * The :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_MODULE` Kconfig option to enable the Modem Sleep Notifications module.
    * The :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE` Kconfig option to enable the TAU Pre-warning module.
    * The :c:enumerator:`LTE_LC_EVT_RAI_UPDATE` event that is enabled with the :kconfig:option:`CONFIG_LTE_RAI_REQ` Kconfig option.
      This requires the :kconfig:option:`CONFIG_LTE_LC_RAI_MODULE` Kconfig option to be enabled.

  * Updated:

    * To use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.
    * The :c:func:`lte_lc_neighbor_cell_measurement` function to return an error for an invalid GCI count.
    * The library was reorganized into modules that are enabled through their respective Kconfig options.
      By default, the library now enables only the core features related to the network connectivity.
      This reorganization reduces flash memory consumption for applications that only use the core features of the library related to network connectivity.
      For more information on how to update your project, see the :ref:`migration guide <migration_2.8_required>`.

      * The :c:func:`lte_lc_conn_eval_params_get` function now requires the new :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL_MODULE` Kconfig option to be enabled.
      * The :c:enumerator:`LTE_LC_EVT_EDRX_UPDATE` event and the :c:func:`lte_lc_ptw_set`, :c:func:`lte_lc_edrx_param_set`, :c:func:`lte_lc_edrx_req`, and :c:func:`lte_lc_edrx_get` functions require the new :kconfig:option:`CONFIG_LTE_LC_EDRX_MODULE` Kconfig option to be enabled.
      * The :c:enumerator:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS` event and the :c:func:`lte_lc_neighbor_cell_measurement_cancel`, and :c:func:`lte_lc_neighbor_cell_measurement` functions require the new :kconfig:option:`CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE` Kconfig option to be enabled.
      * The :c:func:`lte_lc_periodic_search_request`, :c:func:`lte_lc_periodic_search_clear`, :c:func:`lte_lc_periodic_search_get`, and :c:func:`lte_lc_periodic_search_set` functions require the new :kconfig:option:`CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` Kconfig option to be enabled.
      * The :c:enumerator:`LTE_LC_EVT_PSM_UPDATE` event and the :c:func:`lte_lc_psm_param_set`, :c:func:`lte_lc_psm_param_set_seconds`, :c:func:`lte_lc_psm_req`, :c:func:`lte_lc_psm_get`, and :c:func:`lte_lc_proprietary_psm_req` functions require the new :kconfig:option:`CONFIG_LTE_LC_PSM_MODULE` Kconfig option to be enabled.
      * The :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING`, :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_ENTER`, and :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT` events require the new :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_MODULE` Kconfig option to be enabled.
      * The :c:enumerator:`LTE_LC_EVT_TAU_PRE_WARNING` event requires the new :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE` Kconfig option to be enabled.

  * Deprecated:

    * The :c:macro:`LTE_LC_ON_CFUN` macro.
      Use the :c:macro:`NRF_MODEM_LIB_ON_CFUN` macro instead.
    * The :c:func:`lte_lc_factory_reset` function.
      Use the ``AT%XFACTORYRESET`` AT command instead.
      Refer to the :ref:`migration guide <migration_2.8>` for more details.
    * The :c:enum:`lte_lc_factory_reset_type` type.
    * The :c:func:`lte_lc_reduced_mobility_get` and :c:func:`lte_lc_reduced_mobility_set` functions.
      Refer to the :ref:`migration guide <migration_2.8>` for more details.
    * The :c:enum:`lte_lc_reduced_mobility_mode` type.
      Refer to the :ref:`migration guide <migration_2.8>` for more details.

  * Removed:

    * The ``lte_lc_init()`` function.
      All instances of this function can be removed without any additional actions.
    * The ``lte_lc_deinit()`` function.
      Use the :c:func:`lte_lc_power_off` function instead.
    * The ``lte_lc_init_and_connect()`` function.
      Use the :c:func:`lte_lc_connect` function instead.
    * The ``lte_lc_init_and_connect_async()`` function.
      Use the :c:func:`lte_lc_connect_async` function instead.
    * The ``CONFIG_LTE_NETWORK_USE_FALLBACK`` Kconfig option.
      Instead, use either :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT` or :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS`.
      In addition, you can control the priority between LTE-M and NB-IoT using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.

* :ref:`lib_location` library:

  * Fixed:

    * A bug causing the GNSS obstructed visibility detection to sometimes count only part of the tracked satellites.
    * A bug causing the GNSS obstructed visibility detection to be sometimes performed twice.

  * Removed the unused :ref:`at_cmd_parser_readme` library.

* :ref:`lib_zzhc`:

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.

* :ref:`modem_info_readme` library:

  * Updated:

    * To use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.
    * The formulas of RSRP and RSRQ values in :c:macro:`RSRP_IDX_TO_DBM` and :c:macro:`RSRQ_IDX_TO_DB` based on AT command reference guide updates.
      The formulas are now aligned with the modem implementation that has not changed.

  * Fixed a potential issue with scanf in the :c:func:`modem_info_get_current_band` function, which could lead to memory corruption.

  * Removed ``RSRP_OFFSET_VAL``, ``RSRQ_OFFSET_VAL`` and ``RSRQ_SCALE_VAL`` from the API.
    Clients should use the :c:macro:`RSRP_IDX_TO_DBM` and the :c:macro:`RSRQ_IDX_TO_DB` macros.

* :ref:`nrf_modem_lib_readme`:

  * Added:

	  * Support for socket option ``SO_IPV6_DELAYED_ADDR_REFRESH``.
	  * A log warning suggesting a SIM card to be installed if a UICC error is detected by the modem in the :ref:`nrf_modem_lib_lte_net_if`.

  * Updated:

    * The RTT trace backend to allocate the RTT channel at boot, instead of when the modem is activated.
    * The flash trace backend to solve concurrency issues when reading traces while writing, and when reinitializing the application (warm start).
    * The :ref:`modem_trace_module` to let the trace thread sleep when the modem trace level is set to :c:enumerator:`NRF_MODEM_LIB_TRACE_LEVEL_OFF` using the :c:func:`nrf_modem_lib_trace_level_set` function, and the trace backend returns ``-ENOSPC``.
      The trace thread wakes up when another trace level is set.
    * The RTT trace backend to return ``-ENOSPC`` when the RTT buffer is full.
      This allows the trace thread to sleep to save power.
    * The nRF91 socket offload layer is renamed from ``nrf91_sockets`` to ``nrf9x_sockets`` to reflect that the offload layer is not exclusive to the nRF91 Series SiPs.

  * Fixed a bug causing the cell network to be treated as offline if IPv4 is not assigned in the :ref:`nrf_modem_lib_lte_net_if`.

  * Removed:

    * Support for deprecated RAI socket options ``SO_RAI_LAST``, ``SO_RAI_NO_DATA``, ``SO_RAI_ONE_RESP``, ``SO_RAI_ONGOING``, and ``SO_RAI_WAIT_MORE``.
    * The mutex in the :c:func:`nrf9x_socket_offload_getaddrinfo` function after updating the :c:func:`nrf_getaddrinfo` function to handle concurrent requests.

* :ref:`modem_key_mgmt` library:

  * Added the :c:func:`modem_key_mgmt_clear` function to delete all credentials associated with a security tag.
  * Updated the :c:func:`modem_key_mgmt_read()` function to return the actual size buffer required to read the certificate if the size provided is too small.

* :ref:`pdn_readme` library:

  * Added the event ``PDN_EVENT_CTX_DESTROYED`` to indicate when a PDP context is destroyed.
    This happens when the modem is switched to minimum functionality mode (``CFUN=0``).

* :ref:`sms_readme` library:

  * Added the :kconfig:option:`CONFIG_SMS_STATUS_REPORT` Kconfig option to configure whether the SMS status report is requested.

  * Updated:

    * To use the ``AT+CMMS`` AT command when sending concatenated SMS message.
    * To set ``7`` as a fallback SMS service center address for type approval SIM cards that do not have it set.

* :ref:`lib_at_shell` library:

  * Added the :kconfig:option:`CONFIG_AT_SHELL_UNESCAPE_LF` Kconfig option to enable reception of multiline AT commands.
  * Updated the :c:func:`at_shell` function to replace ``\n`` with ``<CR><LF>`` if :kconfig:option:`CONFIG_AT_SHELL_UNESCAPE_LF` is enabled.

Multiprotocol Service Layer libraries
-------------------------------------

* Added a 1-wire coexistence implementation that you can enable using the Kconfig option :kconfig:option:`CONFIG_MPSL_CX_1WIRE`.
* Updated the name of the Kconfig option ``CONFIG_MPSL_CX_THREAD`` to :kconfig:option:`CONFIG_MPSL_CX_3WIRE` to better indicate multiprotocol compatibility.
* Fixed an issue where the HFXO would be left on after uninitializing MPSL when the RC oscillator was used as the Low Frequency clock source (DRGN-22809).
* Deprecated the Kconfig option :kconfig:option:`CONFIG_MPSL_CX_BT_1WIRE`.

Libraries for networking
------------------------

* :ref:`lib_lwm2m_client_utils` library:

  * Updated to use the :ref:`at_parser_readme` library instead of the :ref:`at_cmd_parser_readme` library.
  * Fixed an issue where a failed delta update for the modem would not clear the state and blocks future delta updates.
    This only occurred when an LwM2M Firmware object was used in push mode.

* :ref:`lib_nrf_cloud_rest` library:

  * Added the function :c:func:`nrf_cloud_rest_shadow_transform_request` to request shadow data using a JSONata expression.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * The function :c:func:`nrf_cloud_client_id_runtime_set` to set the device ID string if the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option is enabled.
    * The functions :c:func:`nrf_cloud_sec_tag_set` and :c:func:`nrf_cloud_sec_tag_get` to set and get the sec tag used for nRF Cloud credentials.
    * A new nRF Cloud event type ``NRF_CLOUD_EVT_RX_DATA_DISCON``, which is generated when a device is deleted from nRF Cloud.
    * The functions :c:func:`nrf_cloud_print_details` and :c:func:`nrf_cloud_print_cloud_details` to log common nRF Cloud connection information in a uniform way.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_PRINT_DETAILS` Kconfig option to enable the above functions.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_VERBOSE_DETAILS` Kconfig option to print all details instead of only the device ID.
    * Experimental support for shadow transform requests over MQTT using the :c:func:`nrf_cloud_shadow_transform_request` function.
      This functionality is enabled by the :kconfig:option:`CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS` Kconfig option.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_AWS_CA_CERT_SIZE_THRESHOLD` and :kconfig:option:`CONFIG_NRF_CLOUD_COAP_CA_CERT_SIZE_THRESHOLD` Kconfig options to compare with the current root CA certificate size.
    * The functions :c:func:`nrf_cloud_sec_tag_coap_jwt_set` and :c:func:`nrf_cloud_sec_tag_coap_jwt_get` to set and get the sec tag used for nRF Cloud CoAP JWT signing.
    * FOTA status callback.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_SMP` Kconfig option to enable experimental support for SMP FOTA.

  * Updated:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME` Kconfig option to be available with CoAP and REST.
    * The JSON string representing longitude in ``PVT`` reports from ``lng`` to ``lon`` to align with nRF Cloud.
      nRF Cloud still accepts ``lng`` for backward compatibility.
    * The handling of MQTT JITP device association to improve speed and reliability.
    * To use nRF Cloud's custom MQTT topics instead of the default AWS topics.
    * MQTT and CoAP transports to use a single unified DNS lookup mechanism that supports IPv4 and IPv6, fallback to IPv4, and handling of multiple addresses returned by :c:func:`getaddrinfo`.
    * The log module in the :file:`nrf_cloud_fota_common.c` file from ``NRF_CLOUD`` to ``NRF_CLOUD_FOTA``.
    * The :c:func:`nrf_cloud_credentials_configured_check` function to retrieve the size of the root CA, and compare it to thresholds to decide whether the CoAP, AWS, or both root CA certs are present.
      Use this information to log helpful information and decide whether the root CA certificates are compatible with the configured connection type.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE` Kconfig option to be available and used also when the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_POLL` Kconfig option is enabled.
      The range of the option is now from 128 to 1900 bytes, and the default value is 1700 bytes.
    * The function :c:func:`nrf_cloud_fota_poll_process` to be used asynchronously if a callback to handle errors is provided.

  * Fixed:

    * An issue in the :c:func:`nrf_cloud_send` function that prevented data in the provided :c:struct:`nrf_cloud_obj` structure from being sent to the bulk and bin topics.
    * An issue where the modem was not shut down from bootloader mode before attempting to initialize in normal mode after an unsuccessful update.

  * Deprecated:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_IPV6` Kconfig option, which now no longer forces the nRF Cloud MQTT transport to use IPv4 when not enabled.
      Instead, use the :kconfig:option:`CONFIG_NET_IPV4` and :kconfig:option:`CONFIG_NET_IPV6` Kconfig options to customize which IP versions the :ref:`lib_nrf_cloud` library uses.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_STATIC_IPV4` and :kconfig:option:`CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR` Kconfig options.
      Support for statically configured nRF Cloud IP Addresses will soon be removed.
      Leave :kconfig:option:`CONFIG_NRF_CLOUD_STATIC_IPV4` disabled to instead use automatic DNS lookup.

* :ref:`lib_nrf_cloud_coap` library:

  * Added:

    * The :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DISCONNECT_ON_FAILED_REQUEST` Kconfig option to disconnect the CoAP client on a failed request.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_COAP_JWT_SEC_TAG` Kconfig option to allow for a separate sec tag to be used for nRF Cloud CoAP JWT signing.

  * Updated:

    * To use a shorter resource string for the ``d2c/bulk`` resource.
    * The function :c:func:`nrf_cloud_coap_shadow_get` to return ``-E2BIG`` if the received shadow data was truncated because the provided buffer was not big enough.
    * The :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` Kconfig option to be enabled by default if either :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_POLL` or :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` is enabled.

  * Fixed:

    * A hard fault that occurred when encoding AGNSS request data and the ``net_info`` field of the :c:struct:`nrf_cloud_rest_agnss_request` structure is NULL.
    * An issue where certain CoAP functions would return zero, indicating success, even though there was an error.

  * Removed the experimental status (:kconfig:option:`CONFIG_EXPERIMENTAL`) from the :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` Kconfig option.

* :ref:`lib_nrf_cloud_log` library:

  * Added:

    * Support for dictionary logs using REST.
    * Support for dictionary (binary) logs when connected to nRF Cloud using CoAP.

  * Updated to use INF log level when the cloud side changes the log level.

  * Fixed the missing log source when passing a direct log call to the nRF Cloud logging backend.
    This caused the log parser to incorrectly use the first declared log source with direct logs when using dictionary mode.

* :ref:`lib_fota_download` library:

  * Fixed an issue where the download client instance did not use native TLS although the :kconfig:option:`CONFIG_FOTA_DOWNLOAD_NATIVE_TLS` Kconfig option was enabled.

* :ref:`lib_mqtt_helper` library:

  * Added the function :c:func:`mqtt_helper_msg_id_get` to get valid message IDs for MQTT messages.
  * Updated the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option to depend on :kconfig:option:`CONFIG_TLS_CREDENTIALS` instead of specific boards.

* :ref:`lib_nrf_provisioning` library:

  * Added support for the ``SO_KEEPOPEN`` socket option to keep the socket open even during PDN disconnect and reconnect.
  * Updated the check interval logging to use INF to improve customer experience.

* :ref:`lib_nrf_cloud_alert` library:

  * Updated to use INF log level when cloud side changes the alert enable flag.

Libraries for NFC
-----------------

* Added an experimental serialization of NFC tag 2 and tag 4 APIs.
* Fixed a potential issue with handling data pointers in the function ``ring_buf_get_data()`` in the :file:`platform_internal_thread` file.

nRF RPC libraries
-----------------

* Added:

  * An experimental serialization of OpenThread APIs.
  * The logging backend that sends logs through nRF RPC events.

* Updated the internal Bluetooth serialization API and Bluetooth callback proxy API to become part of the public :ref:`NRF RPC API <nrf_rpc_ipc_readme>`.

Other libraries
---------------

* Added the :ref:`nrf_compression` library with support for the LZMA decompression.

* :ref:`lib_date_time` library:

  * Added:

    * A retry feature that reattempts failed date-time updates up to a certain number of consecutive times.
    * The following Kconfig options:

      * :kconfig:option:`CONFIG_DATE_TIME_RETRY_COUNT` to control whether a retry would be performed and the number of consecutive date-time update retries.
      * :kconfig:option:`CONFIG_DATE_TIME_RETRY_INTERVAL_SECONDS` to control how quickly date-time update retries occur.

  * Fixed a bug that caused date-time updates to not be rescheduled under certain circumstances.

* :ref:`lib_ram_pwrdn` library:

  * Added support for the nRF54L15 SoC.

* :ref:`dult_readme` library:

  * Added DULT motion detector feature.
    You can enable this feature using the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR` Kconfig option.

Security libraries
------------------

* :ref:`nrf_security_readme` library:

  * Added the :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC` Kconfig option to key derivation function configurations in :ref:`nrf_security_driver_config`.
    The Kconfig option enables support for the derivation function SP 800-108r1 CMAC in counter mode, which is supported by the :ref:`nrf_security_drivers_cracen`.
  * Removed the Kconfig options ``CONFIG_PSA_WANT_ALG_CFB`` and ``CONFIG_PSA_WANT_ALG_OFB`` since the Cipher Feedback (CFB) mode and the Output Feedback (OFB) mode are not tested in the test framework.

* :ref:`trusted_storage_readme` library:

  * Added support for Zephyr Memory Storage (ZMS), as an alternative to the NVS file system.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`nrf_desktop_config_channel_script` script:

  * Added semantic version support for devices that use the ``SUIT DFU``.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * :ref:`Compressed firmware update support <mcuboot_image_compression>`.
  * Support for handling SHA512 by serial recovery protocol.
  * Support for ED25519 'PURE' signature (``CONFIG_BOOT_SIGNATURE_TYPE_PURE`` in MCUboot).
  * Integration of PSA crypto API as cryptographic backend for integration with cryptographic acceleration on the nRF54L15 SoC (``CONFIG_BOOT_USE_PSA_CRYPTO`` in MCUboot).
  * Support for signature verification using hardware cryptography (Cracken) on the nRF54L15 SoC.
    This is supported either for the ED25519 'PURE' signature or the ED25519 signature.
  * Support for storing signature verification keys in hardware KMU on the nRF54L15 SoC.
    See Kconfig option :kconfig:option:`CONFIG_BOOT_SIGNATURE_USING_KMU` in MCUboot for details.
  * Integration of nRF54L15 SoC's HW cryptography (:kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519`), KMU sysbuild configuration (:kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU`), and signing script (:kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE`) for building and signing the application in a project.

* Updated the immutable bootloader to now be protected using the :ref:`fprotect_readme` library on the nRF54L15 SoC (:kconfig:option:`SB_CONFIG_MCUBOOT_FPROTECT_ALLOW_COMBINED_REGIONS` and the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option).

* Fixed:

  * An issue with wrong network core address checking in MCUboot for nRF5340 network core firmware updates.
  * An issue with MCUboot updates in MCUboot when a swap mode was selected, which could lead to possible data corruption by implementing additional NSIB-specific (overwrite-only) upgrade mode for sysbuild.
  * Multiple issues with checking firmware update slot selection in MCUboot for sysbuild.
  * An issue with MCUboot cleanup unused slots feature, which could lead to possible empty firmware update of the main image and bricking the device for sysbuild.

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

Trusted Firmware-M
==================

* Added a possibility to read UICR.OTP registers through platform services.
* Updated the TF-M version to v2.1.0.

Mbed TLS
========

* Updated the Mbed TLS version to v3.6.2.

Documentation
=============

* Added:

  * The :ref:`ug_app_dev` section, which includes pages from the :ref:`configuration_and_build` section and from the removed Device configuration guides section.
  * The :ref:`dfu_tools_mcumgr_cli` page after it was removed from the Zephyr repository.
  * The ``ug_nrf54h20_suit_soc_binaries`` page.
  * The ``ug_nrf54h20_suit_push`` page documenting the SUIT push model-based update process.
  * The ``ug_nrf54h20_suit_recovery`` page.
  * The ``ug_nrf54h20_suit_device_config`` page.
  * The :ref:`ug_nrf54h20_flpr` page.
  * The :ref:`nrf_rpc_uart` page.
  * The :ref:`ug_bt_stack_arch` and the :ref:`ug_bt_solution` documentation to the :ref:`Bluetooth protocols <ug_bt>` page.
  * The :ref:`nfc_rpc` page documenting the NFC Remote Procedure Call (RPC) solution.

* Updated:

  * The nRF7002 DK device guide's :ref:`ug_nrf70_developing_debugging` page with the new snippets added for the nRF70 driver debug and WPA supplicant debug logs.
  * The :ref:`programming_params` section on the :ref:`programming` page with information about readback protection moved from the :ref:`ug_nrf5340_building` page.
  * The :ref:`security` page with a table that provides an overview of the available general security features.
    This table replaces the subpage that was previously describing these features in more detail.
  * Restructured the :ref:`app_bootloaders` documentation and combined the DFU and bootloader articles.
    Additionally, created a new bootloader :ref:`bootloader_quick_start`.
  * Separated the instructions about building from :ref:`configure_application` and moved it to a standalone :ref:`building` page.
  * Restructured the :ref:`ug_bt_mesh` documentation for clearer distinction between concepts or overview topics and how-to topics, thus moved some information from the Bluetooth Mesh library sections.
  * The :ref:`nrf_security_drivers_cracen` section with a reference to the :ref:`ug_nrf54l_cryptography` page.
  * The :ref:`ug_tfm` page with the correct list of samples demonstrating TF-M.
  * The :ref:`app_approtect_ncs` section on the :ref:`app_approtect` page with details on setting the Kconfig options and register values to enable AP-Protect.
  * The :ref:`companion_components` page with Secure Domain Firmware and System Controller Firmware components.
  * The :ref:`ug_bt_fast_pair` guide to document integration steps for the motion detector feature that is a part of the FMDN extension.
  * The :ref:`ug_dult` guide to document integration steps for the motion detector feature.

* Removed:

  * The Device configuration guides section and moved its contents to :ref:`ug_app_dev`.
  * The Advanced building procedures page and moved its contents to the :ref:`building` page.
  * The standalone pages for getting started with nRF52 Series and with the nRF5340 DK.
    These pages have been replaced by the `Quick Start app`_ that now supports the nRF52 Series devices and the nRF5340 DK.
