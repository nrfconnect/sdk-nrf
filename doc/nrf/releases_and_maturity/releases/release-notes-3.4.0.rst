.. _ncs_release_notes_340:

|NCS| v3.4.0 Release Notes
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

* The |NCS| v3.4.0 is based on Zephyr 4.4, Mbed TLS 4.1.0, and TF-M 2.3.

  This release has long-term support (LTS) for a period of five years.
  During this period, patch releases will provide updates for security vulnerabilities and critical bug fixes.
  These patch releases should not contain breaking changes unless security fixes require them.

  For more information about LTS scope and maintenance, see :ref:`lts_releases`.

* The |NCS| v3.4.0 and its patch releases are the last |NCS| releases that include support for nRF52 Series devices.
  The nRF52 Series devices are considered feature complete, and support for them will be removed from samples and applications in the development branch.

Added the following features as experimental:

* PMIC:

  * The following battery health features have been added to Nordic's fuel gauge algorithm and the :ref:`fuel gauge sample <npm13xx_fuel_gauge>`:

    * State-of-health tracking
    * Cycle count tracking
    * Automatic battery replacement detection

* Bluetooth®:

  * HID Shorter Connection Intervals (SCI) support on both the HID device side (:kconfig:option:`CONFIG_BT_HIDS_SCI`) and the HID host side (:kconfig:option:`CONFIG_BT_HOGP_SCI`).
    This feature lets a Bluetooth LE HID host negotiate sub-7.5 ms connection intervals with a Bluetooth LE HID device for lower input latency in high-performance HID applications.

Deprecated:

* Deprecated features will not be removed from the LTS branch.
  It is recommended that all new designs using Nordic devices exclude deprecated features for active development and deployment.

* The Partition Manager is a component in the |NCS| that handles the flash memory partitioning at build time.
  This functionality is now replaced by Zephyr's default devicetree-based flash partitioning when multi-image builds are involved.
  It is recommended that all new designs using Nordic devices are to be built with DTS instead of Partition Manager.
  Partition Manager will be removed from the |NCS| by the end of 2026 from the main branch.

  Samples will be adapted to showcase the new way of allocating partitions within the v3.4.0 branch.
  Such adaptations will also be reflected in the main for future releases.

Toolchain:

* The |NCS| toolchain is now based on the :ref:`Zephyr SDK v1.0.1 <zephyr:toolchain_zephyr_sdk>`, with GCC as the recommended compiler.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.4.0**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.4.0`_.

Integration test results
************************

The integration test results for this tag can be found in the following external artifactory:

* `Twister test report for nRF Connect SDK v3.4.0`_
* `Hardware test report for nRF Connect SDK v3.4.0`_

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.4.0.
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
     - `Changelog <Modem library changelog for v3.4.0_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.4.0_>`_

Migration notes
***************

See the `Migration notes for nRF Connect SDK v3.4.0`_ for the changes required or recommended when migrating your application from |NCS| v3.3.0 to |NCS| v3.4.0.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.4.0`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_340_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated :ref:`additional_nordic_sw_tools_os_support` with an entry for Linux - Debian 12 (ARM64 as Tier 1).

Board support
=============

* Updated the Thingy:91 and Thingy:91 X devicetree partitions to match the Partition Manager partitions.

Build and configuration system
==============================

* Removed the internal Haltium and Lumos platform abstractions from the Nordic SoC integration.
  All references to ``NRF_PLATFORM_HALTIUM`` and ``NRF_PLATFORM_LUMOS`` have been replaced with explicit checks for ``SOC_SERIES_NRF54H``, ``SOC_SERIES_NRF92``, ``SOC_SERIES_NRF54L``, or ``SOC_SERIES_NRF71``.

  * Renamed Kconfig options and headers:

    * ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR`` to :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR`.
    * The ``HALTIUM_PLATFORM_PSA_KEY_ID()`` macro to ``NRF_PLATFORM_PSA_KEY_ID()`` (in :file:`include/psa/nrf_platform_key_ids.h`).
    * :file:`haltium_power.h` to :file:`soc_power.h`.
    * :file:`haltium_pm_s2ram.h` to :file:`soc_pm_s2ram.h`.

  * Deprecated Kconfig options and headers:

    * The ``CONFIG_NRF_PLATFORM_HALTIUM`` Kconfig option.
      Use :kconfig:option:`CONFIG_SOC_SERIES_NRF54H` or :kconfig:option:`CONFIG_SOC_SERIES_NRF92` instead.
    * The ``CONFIG_NRF_PLATFORM_LUMOS`` Kconfig option.
      Use :kconfig:option:`CONFIG_SOC_SERIES_NRF54L` or :kconfig:option:`CONFIG_SOC_SERIES_NRF71` instead.
    * The ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR`` Kconfig option (alias for :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR`).
    * The ``HALTIUM_PLATFORM_PSA_KEY_ID()`` macro (alias for ``NRF_PLATFORM_PSA_KEY_ID()``).
    * The :file:`haltium_power.h` and :file:`haltium_pm_s2ram.h` headers, which forward to the new headers.

  See `Migration notes for nRF Connect SDK v3.4.0`_ for migration instructions.

  The deprecated symbols, headers, and macros remain available with a deprecation warning and are scheduled for removal in a future release.

* Consolidated the previously per-SoC :file:`soc_power.c` and :file:`soc_pm_s2ram.c` sources for the nRF54H and nRF92 Series in a single shared location under :file:`zephyr/soc/nordic/common/`.

Bootloaders and DFU
===================

* Added support for the new LCS API to control bootloader behavior.
  When this API is enabled, the bootloader can skip signature verification in early LCS states and abort the boot process in the decommissioned state.

* Fixed DTS support for nRF5340 network core images when :ref:`b0n <bootloader>` was enabled.

* Reworked the signing procedure for MCUboot images in sysbuild projects that use DTS-defined partitions.
  Signed images now include the expected image address in the image header and the ``rom-fixed-flag`` value, which MCUboot uses to match each image candidate with the correct image target.
  The signing procedure for images used with MCUboot direct-XIP mode remains unchanged.

Developing with nRF91 Series
============================

* Updated the nRF91 Series device to configure partitions using the devicetree.
  See :ref:`nrf91_dt_partitions` for more details.

Developing with nRF54L Series
=============================

* Added the :kconfig:option:`CONFIG_SB_CRACEN_KMU_INVALIDATE_PROTECTED_RAM_SLOTS` sysbuild Kconfig option.
  This option populates the Key Management Unit (KMU) slots for invalidation of the CRACEN-protected RAM using nRF Util.
  This option requires the ``device`` command v2.15.4 or later to work.
  See `Installing and upgrading nRF Util commands`_ for more information.
  When enabled, the :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT` and :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT` Kconfig options become unavailable, as they implement the same feature through alternative provisioning paths.
* Updated builds without Partition Manager to generate the :file:`bootconf.hex` file when |NSIB| is used as the bootloader.

Developing with nRF54H Series
=============================

* Updated MCUboot to support MRAM write protection for up to four images.

Security
========

* Added:

  * Support for X25519 key pair storage in the :ref:`Key Management Unit (KMU) <ug_kmu_guides_supported_key_types>`.
  * Documentation page about :ref:`TF-M memory partitioning <ug_tfm_partitioning>`.

* Updated:

  * Renamed IronSide Secure Element to IronSide Secure Enclave.
  * |ISE| to version 23.7.0+30.
    For more information, see :ref:`abi_compatibility_ise_changelog`.
  * Oberon PSA Crypto from v1.5.4 to v2.0.0.
    The new version aligns with TF-PSA-Crypto v1.1.0.
  * How the :kconfig:option:`CONFIG_NRF_SECURITY` and :kconfig:option:`CONFIG_PSA_CRYPTO` Kconfig options interact with each other.
    :kconfig:option:`CONFIG_NRF_SECURITY` is now promptless and auto-enabled indirectly by :kconfig:option:`CONFIG_PSA_CRYPTO`.
  * Approach to store keys in the KMU so that AEAD algorithms with non-default (shortened) tag lengths are supported.
  * The :ref:`ug_crypto_supported_features` page:

    * The page now lists both nRF54LS05A and nRF54LS05B in the support tables.
    * The CRACEN support status for nRF54LV10A for most of the algorithms changed from Experimental to Supported.
    * The :ref:`ug_crypto_supported_features_countermeasures` section now includes information for nRF54H20.

* Fixed:

  * Issues with incorrect support status on the :ref:`ug_crypto_supported_features` page:

    * The :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM` Kconfig option is now correctly listed as unsupported for SoCs with Arm CryptoCell CC310.
    * The tables for supported AES key wrapping algorithms for nRF54L Series devices now list the nRF54LS05 device (not supported in the CRACEN driver; experimental in the nrf_oberon driver).
    * The post-quantum cryptography algorithms for nrf_oberon under Key types and key management are now correctly listed as experimental instead of supported.
    * The SPAKE2+ for Matter is now correctly listed as supported instead of experimental.
    * The WPA3-SAE hash-to-element algorithm is now correctly listed as a KDF algorithm, not a PAKE algorithm.
    * The SHA-256/192 and SHAKE hashing algorithms are now correctly listed as not supported in the CRACEN driver and experimental in the nrf_oberon driver.
      The only exception is the SHAKE256 512 bits algorithm, which is supported in both the CRACEN and nrf_oberon drivers.

* Removed the ``CONFIG_NRF_SECURITY_ADVANCED`` Kconfig option.

Security libraries
------------------

* :ref:`nrf_security_readme` library:

  * Updated the documentation to one library page with sections for PSA Crypto, TLS and X.509, dependencies, and API documentation.
    The page includes information about how :kconfig:option:`CONFIG_PSA_CRYPTO` and :kconfig:option:`CONFIG_MBEDTLS` are used after the Mbed TLS v4.1 update.

  * Removed the configuration page for the deprecated legacy crypto backend (:file:`libraries/security/nrf_security/doc/backend_config`).
    Configure cryptographic features using :ref:`psa_crypto_support` and :ref:`ug_crypto_supported_features` instead.

Mbed TLS
--------

* Updated Mbed TLS to v4.1.0 (from v3.6.6):

  * Removed support for legacy, deprecated Mbed TLS Crypto APIs and related tests.
  * Rearranged Kconfig options related to Mbed TLS in various files under :ref:`nrf_security`.
  * Removed outdated Kconfig options related to Mbed TLS in various files under :ref:`nrf_security`.
  * :ref:`nrf_security` now uses the Mbed TLS integration from Zephyr as-is (:kconfig:option:`CONFIG_MBEDTLS_BUILTIN`) while replacing upstream TF-PSA-Crypto with Oberon PSA Crypto (:kconfig:option:`CONFIG_TF_PSA_CRYPTO_CUSTOM`).

  For more information, see the `Migration notes for nRF Connect SDK v3.4.0`_.

Trusted Firmware-M (TF-M)
-------------------------

* Updated to version 2.3.0.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth LE
------------

* Added the following Kconfig options to enable support for certain HCI commands:

  * :kconfig:option:`CONFIG_BT_CTLR_SDC_DATA_RELATED_ADDRESS_CHANGES`
  * :kconfig:option:`CONFIG_BT_CTLR_SDC_RF_PATH_COMPENSATION`
  * :kconfig:option:`CONFIG_BT_CTLR_SDC_ISO_TEST`

Bluetooth Mesh
--------------

* Added:

  * The :ref:`dfu_conf` guide on how to configure DFU for Bluetooth Mesh samples.
  * Recommendations for sizing the persistent storage partitions to handle continuous storage of the replay protection list.

Wi-Fi®
------

* Added the ``nordic-wifi-enterprise`` snippet to be used for Wi-Fi enterprise.
  For more information, see the `Migration notes for nRF Connect SDK v3.4.0`_.
* Updated the hostap_crypto library to use a single backend, :kconfig:option:`CONFIG_HOSTAP_CRYPTO_ALT_PSA`, for all modes.
  The older :kconfig:option:`CONFIG_HOSTAP_CRYPTO_ALT_LEGACY_PSA` backend has been removed.
* Removed support for DES-encrypted private keys in Wi-Fi Enterprise (EAP-TLS) following the update to Mbed TLS 4.1.0/TF-PSA-Crypto 1.1.0 in the |NCS|.
  TF-PSA-Crypto no longer implements DES ciphers, therefore PKCS#8 client keys encrypted with DES-EDE3-CBC cannot be decrypted during the TLS handshake (including the FreeRADIUS ``rsa2k`` test certificates).
  Use :file:`zephyr/samples/net/wifi/test_certs/rsa2k_no_des`, re-encrypt credentials with AES (PBES2), or use unencrypted keys when provisioning enterprise credentials.
  See the :ref:`zephyr:wifi_mgmt` documentation for details.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

* Updated to use devicetree (DTS) for defining NVM partitions instead of the deprecated :ref:`partition_manager`.

High-Performance Framework (HPF)
--------------------------------

* Added support for the nRF54LV10A SoC in HPF MSPI.

nRF Audio (formerly nRF5340 Audio)
----------------------------------

* Renamed the application to nRF Audio from nRF5340 Audio.
* Added:

  * The :kconfig:option:`CONFIG_BT_AUDIO_BROADCAST_BASE_PRINT` Kconfig option to print the contents of the BASE when it is received.
    This option is intended for debugging purposes.
    It is only valid for the :ref:`broadcast sink application <nrf_audio_broadcast_sink_app>`.

  * :kconfig:option:`CONFIG_LTO` to enable link-time optimization for the application.
    It improves application performance and reduces code size.

* Updated:

  * The call to :c:func:`hci_vs_sdc_iso_read_tx_timestamp` so that, when sending ISO data, it is performed at regular intervals instead of every SDU interval.
    This change reduces the frequency of application-controller time synchronization, while significantly reducing processing overhead.
  * The group creation in the :ref:`unicast client application <nrf_audio_unicast_client_app>` so that it waits until all connected devices have finished discovery.
  * The unicast server (headset) now exposes its source endpoint as default.
    This means that the unicast client (gateway) can choose whether to enable it or not.
  * The :file:`buildprog.py` script no longer supports the ``-u`` (user name) option.
    Use the ``-cn`` (custom name) option instead.
    The custom name is now used for both unicast and broadcast.
    Changing this parameter requires a pristine build.
    The option is intended as a convenience argument.
    Set names through configuration options for persistent configuration.

nRF Desktop
-----------

* Updated:

  * The application configurations to use devicetree (DTS) for defining non-volatile memory partitions instead of the deprecated :ref:`partition_manager`.
    This includes configuration of main application image and other sysbuild images built together with it (for example bootloader).
  * The application configurations to change the non-volatile memory self-protection mechanism in the MCUboot image on the nRF54L board targets (:kconfig:option:`CONFIG_SOC_SERIES_NRF54L`).
    The :kconfig:option:`CONFIG_NCS_MCUBOOT_DISABLE_SELF_RWX` Kconfig option is used instead of the :kconfig:option:`CONFIG_FPROTECT` to disable read, write, and execute from the memory area that holds the MCUboot bootloader.
    This is done to improve the protection of the MCUboot immutable bootloader code.

nRF Machine Learning (Edge Impulse)
-----------------------------------

* Removed the deprecated nRF Machine Learning application.
  The application is replaced by the Gesture Recognition application in `Edge AI Add-on for nRF Connect SDK`_.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added:

  * The :ref:`channel_sounding_ipt_initiator` and :ref:`channel_sounding_ipt_reflector` samples to demonstrate how to use the Bluetooth Channel Sounding (CS) Inline Phase Correction Term Transfer (IPT) feature.
  * Support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` board target in the following samples:

    * :ref:`bluetooth_conn_time_synchronization`
    * :ref:`direct_test_mode`
    * :ref:`ble_event_trigger`
    * :ref:`bluetooth_iso_combined_bis_cis`
    * :ref:`ble_llpm`
    * :ref:`bluetooth_radio_coex_1wire_sample`
    * :ref:`ble_radio_notification_conn_cb`
    * :ref:`bt_scanning_while_connecting`
    * :ref:`ble_subrating`

* :ref:`peripheral_hids_mouse` sample:

  * Added:

    * Support for continuous HID report sending (one report notification for each connection interval) as well as statistics for continuous report sending.
    * A configuration with support for the HID Shorter Connection Intervals (SCI).
      It is enabled with the ``hid_sci`` suffix.

* :ref:`bluetooth_central_hids` sample:

  * Added:

    * Continuous receive mode when a high report rate is detected, with periodic statistics for each batch of received reports for timing quality assessment (for use with continuous sending on a HID peripheral).
    * A configuration with support for the HID Shorter Connection Intervals (SCI) on the HID host side.
      It is enabled with the ``hid_sci`` suffix.
    * Support for connecting multiple HID peripherals.

  * Updated the sample to use deferred logging to reduce the overhead of logging.
    This can cause messages to be delayed up to around 100 ms.

* :ref:`direct_test_mode` sample:

  * Updated the :ref:`nrfxlib:softdevice_controller` to run DTM commands.

  * Removed:

    * USB support.
    * CTE receiver support.
      CTE transmitter support is still available.
    * Antenna switching support.

Bluetooth Mesh samples
----------------------

* :ref:`bluetooth_mesh_light` sample:

  * The partition layout for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target is not backward compatible with the previous versions of the sample.
    This affects builds with support of point-to-point Device Firmware Update (DFU) over the Simple Management Protocol (SMP).

  * Updated the approach to build the sample with point-to-point Device Firmware Update (DFU) over the Simple Management Protocol (SMP).
    There are no :file:`overlay-dfu.conf` and :file:`sysbuild-dfu.conf` files anymore.
    DFU support is enabled as a suffixed configuration.

* :ref:`bluetooth_mesh_light_lc` sample:

  * The partition layout for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target is not backward compatible with the previous versions of the sample.
    This affects builds with support for the EMDS.

* :ref:`ble_mesh_dfu_distributor` sample:

  * The partition layout for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets is not backward compatible with the previous versions of the sample.

* :ref:`ble_mesh_dfu_target` sample:

  * The partition layout for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets is not backward compatible with the previous versions of the sample.

Bluetooth Fast Pair samples
---------------------------

* Updated the DTS overlays of the Fast Pair samples to use the new ``zephyr,mapped-partition`` compatible property for each partition node.
  The change aligns the board target overlays with the layout produced by the Partition Manager-to-DTS helper script (:file:`scripts/pm_to_dts.py`).
  See the :ref:`migration_3.4_google_fast_pair` guide to learn how to align the DTS partition map in your project using the new property.

* Removed the remaining ``nrf54h20dk/nrf54h20/cpuapp`` board target configurations from the Fast Pair samples.
  Removed references to the ``nrf54h20dk/nrf54h20/cpuapp`` board target from the Fast Pair sample documentation, including descriptions of nRF54H-specific solutions such as the legacy SUIT DFU integration.
  Support for this target was already dropped during the IronSide SE migration in the |NCS| v3.1.0 release.

* :ref:`fast_pair_locator_tag` sample:

  * Added initial support for the nRF54L15 TAG (``nrf54l15tag/nrf54l15/cpuapp`` board target).
    The ``nrf54l15tag/nrf54l15/cpuapp`` board target is a complete prototyping platform in a compact form factor.
    It features the nRF54L15 SoC with additional sensors, RGB LED, button, and a battery.
    The sample on this board target can closely represent a final product based on the nRF54L15 SoC.

  * Updated:

    * UI thread handling for reference board targets with a speaker by moving the speaker control into the common indication thread.
      The signaling is now done using the :ref:`zephyr:events` API.
    * The configuration of the non-volatile memory self-protection mechanism in the MCUboot image on the nRF54L board targets.
      The :kconfig:option:`CONFIG_NCS_MCUBOOT_DISABLE_SELF_RWX` Kconfig option now replaces the :kconfig:option:`CONFIG_FPROTECT`, which is associated with the :ref:`fprotect_readme` library.
      The new mechanism uses a dedicated RRAMC region to disable read, write, and execute access to the MCUboot partition right before jumping to the application image.

  * Fixed the ringing status indications with the green LED flashes for reference board targets.
    A ringing status indication was often skipped during the motion detection event.

Cellular samples
----------------

* Removed:

  * All nRF Cloud REST samples as the nRF Cloud REST library has been removed.
  * Usage of nRF Cloud logging in samples as the feature is being sunset.
  * The Cellular: nRF Cloud multi-service sample.

* :ref:`crypto_tls` sample:

  * Updated the TLS version support section after the Mbed TLS v4.1.0 update.

* :ref:`gnss_sample` sample:

  * Updated to use the :ref:`lib_nrf_cloud_coap` library instead of the removed nRF Cloud REST library.

* :ref:`modem_shell_application` sample:

  * Added devicetree overlays for data throughput testing.
  * Updated to use the :ref:`lib_nrf_cloud_coap` library instead of the removed nRF Cloud REST library.

Cryptography samples
--------------------

* Added support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` board target to all samples, except :ref:`crypto_aes_kw`, :ref:`crypto_kmu_cracen_usage`, and :ref:`crypto_persistent_key`.

DFU samples
-----------

* Updated:

  * The :ref:`mcuboot_minimal_configuration` has been moved to the :file:`samples/dfu` directory.

Edge Impulse samples
--------------------

* Removed the deprecated Edge Impulse samples: Wrapper and Data forwarder.
  The removed samples are replaced by samples in `Edge AI Add-on for nRF Connect SDK`_.

Matter samples
--------------

* Updated the :ref:`matter_window_covering_sample` sample to use the Thread Sleepy End Device (SED) device type by default.
  You can now enable the Thread Synchronized Sleepy End Device (SSED) device type as an optional feature.

* Removed:

  * Support for the nRF7002 DK and nRF5340 DK used with the nRF7002 EK shield from all Matter samples, as this configuration was deprecated in the |NCS| v3.2.0.
    The removal is mainly due to the very limited non-volatile memory available for application code.
    As an alternative, use the nRF54LM20A or nRF54LM20B SoC with the nRF7002-EB II shield.
    This combination provides significantly more non-volatile memory for Matter over Wi-Fi applications.
  * The implementation of the Matter Door lock sample has been relocated to the `nRF Door Lock and Access Control Add-on`_.
    The add-on is an extensible solution offering additional features and expanded integration capabilities.
    It enables the development of not only Matter-compliant door locks, but also those supporting Aliro and hybrid Matter and Aliro combined functionalities, thereby facilitating the design of versatile smart lock solutions.
  * The AWS IoT integration variant from the :ref:`matter_light_bulb_sample` sample.

Networking samples
------------------

* Removed Partition Manager for all Wi-Fi targets from the following samples:

  * :ref:`mqtt_sample` sample
  * :ref:`udp_sample` sample
  * :ref:`net_coap_client_sample` sample
  * :ref:`download_sample` sample
  * :ref:`https_client` sample
  * :ref:`http_server` sample
  * :ref:`aws_iot` sample
  * :ref:`azure_iot_hub` sample

  Flash and SRAM partitions are supplied using devicetree overlays instead.

* :ref:`mqtt_sample` sample:

  * Added the :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384` Kconfig option to the TLS overlay files, which is required by the CA certificate of ``mqtt.nordicsemi.academy``.
  * Updated the default MQTT broker from ``test.mosquitto.org`` to ``mqtt.nordicsemi.academy`` because of recurring instability in the previous broker.

* :ref:`azure_iot_hub` sample:

  * Added support for the nRF54LM20 DK with the nRF7002-EB II shield.

Peripheral samples
------------------

* :ref:`802154_phy_test` sample:

  * Removed unused functions from the :file:`rf_proc.c` file and the :file:`rf_proc.h` header file.

* :ref:`radio_test` sample:

  * Added:

    * ``start_tx_sweep_with_sleep`` shell command that allows for running a TX sweep with silent periods between each frequency.
    * ``start_tx_sweep_with_sleep_modulated`` shell command that allows for running a modulated TX sweep with silent periods between each frequency.
    * ``set_channel_sequence`` shell command that allows for setting a custom channel sequence for the ``start_tx_sweep_with_sleep`` and ``start_tx_sweep_with_sleep_modulated`` commands.
    * ``print_channel_sequence`` shell command that prints the currently configured channel sequence.
    * ``set_channel_sequence_hopping_mode`` shell command that allows for setting the hopping mode for the channel sequence.
    * Support for pin debugging of radio events on nRF54L Series devices.

  * Updated the ``start_duty_cycle_modulated_tx`` command to accept an optional argument specifying the number of packets to transmit.
    If no additional argument is provided, the command works as before.
  * Fixed an issue where the ``start_duty_cycle_modulated_tx`` command would not work correctly on nRF52 Series devices if the command was triggered more than once.

PMIC samples
------------

* Updated the :ref:`npm13xx_fuel_gauge` and :ref:`npm2100_fuel_gauge` samples to use :ref:`nrfxlib:nrf_fuel_gauge` v2.0.0, and added support for state storage.
  The :ref:`npm13xx_fuel_gauge` uses the new state-of-health feature in the :ref:`nrfxlib:nrf_fuel_gauge` v2.0.0.

Wi-Fi samples
-------------

* Added the :ref:`wifi_nrf_cloud` sample, refactored from the former Cellular: nRF Cloud multi-service sample.
  This sample targets the nRF7002 Wi-Fi companion IC, using Wi-Fi as the transport instead of LTE.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Fixed an issue where the low power UART can fail after a reset if the reset occurs during transmission.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

* :ref:`liblwm2m_carrier_readme` library:

  * Added support for building without Partition Manager.
    This requires an ``lwm2m_carrier`` partition to be defined in the devicetree.

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Fixed missing ATT write length validation in the GATT write handler for the Fast Pair Additional Data characteristic, used by the experimental Personalized Name extension (:kconfig:option:`CONFIG_BT_FAST_PAIR_PN`).

* :ref:`hogp_readme` library:

  * Added support for Bluetooth HID Shorter Connection Intervals (SCI) on the HID device and HID host sides (:kconfig:option:`CONFIG_BT_HIDS_SCI`, :kconfig:option:`CONFIG_BT_HOGP_SCI`).
  * Fixed an issue where the :c:func:`bt_hogp_rep_unsubscribe` function did not clear the notification callback, which prevented the :c:func:`bt_hogp_rep_subscribe` function from succeeding after unsubscribing.

Modem libraries
---------------

* Removed the deprecated PDN library.
  Use the PDN management functionality in the :ref:`lte_lc_readme` library instead.

* :ref:`lib_location` library:

  * Fixed an issue where cloud location requests were not sent when cellular profile 1 was active.

* :ref:`nrf_modem_lib_readme` library:

  * Added support for building for the nRF91 board without Partition Manager.
    This requires ``cpuapp_cpucell_ipc_shm_ctrl``, ``cpuapp_cpucell_ipc_shm_heap``, and ``cpucell_cpuapp_ipc_shm_heap`` partitions to be defined in the devicetree.
    Use a separate ``cpucell_cpuapp_ipc_shm_trace`` partition for modem tracing instead of splitting the ``cpucell_cpuapp_ipc_shm_heap`` partition.

* :ref:`modem_info_readme` library:

  * Fixed a bug where supplying a very large buffer would cause a buffer overflow internally due to incorrect format string usage.

Libraries for networking
------------------------

* :ref:`lib_azure_fota` library:

  * Fixed a potential NULL-pointer dereference that occurred when the device twin JSON parsers used the ``jobId``, ``fwUpdateStatus``, ``currentFwVersion``, ``pendingFwVersion``, ``host``, and ``path`` fields without verifying that the JSON nodes were strings.

* :ref:`lib_nrf_provisioning` library:

  * Removed dependency on the :ref:`lte_lc_readme` library.
    The :ref:`at_monitor_readme` library is now used for tracking network connectivity status.

* :ref:`lib_nrf_cloud` library:

  * Added the :kconfig:option:`CONFIG_NRF_CLOUD_SEND_DEVICE_INFO_BOOTLOADER_VERSION` Kconfig option to include the active MCUboot (B1) ``fw_info`` version as ``bootloaderVersion`` in ``deviceInfo``.

  * Updated by changing the default device ID source from :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` to the device UUID.
    On an nRF9160 device the default is now :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID`, which reads the UUID from the JWT ``iss`` claim and preserves compatibility with field devices provisioned through older nrfcloud-utils flows.
    On the nRF91x1 and future SoCs, the default is :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_MDM_DEVICE_UUID`, which reads the UUID using ``AT%DEVICEUUID`` without pulling in :kconfig:option:`CONFIG_MODEM_JWT`.
    Each option is constrained to its target SoC at the Kconfig level.
    See the `Migration notes for nRF Connect SDK v3.4.0`_ for the impact on existing devices that relied on the previous IMEI default.

  * Deprecated the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` and :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID` Kconfig options.
    Use the default UUID source for new applications.

  * Fixed:

    * An issue where PEM private keys with CRLF line endings could not be decoded correctly for JWT signing used in nRF Cloud CoAP authentication.
    * A DTLS socket option setup to inspect ``errno`` after ``setsockopt()`` failures instead of comparing the return value to ``EOPNOTSUPP`` or ``EINVAL``, which could mishandle unsupported options during DTLS configuration.
    * The DTLS handshake timeout configuration on native (non-modem) sockets by using ``TLS_DTLS_HANDSHAKE_TIMEOUT_MIN`` and ``TLS_DTLS_HANDSHAKE_TIMEOUT_MAX`` instead of the modem-only ``TLS_DTLS_HANDSHAKE_TIMEO`` option, avoiding handshake failures when connecting to nRF Cloud over Wi-Fi.
    * The internal CoAP client socket descriptor not being updated immediately after a new DTLS socket was connected, which could leave ``client->cc.fd`` stale during reconnect and interfere with CoAP polling, receive, and retransmit paths.

  * Removed:

    * The nRF Cloud Alerts library.
      As part of the migration to *nRF Cloud powered by Memfault*, the nRF Cloud Alerts feature is now redundant.
      `Memfault's Trace Events <Memfault: Error Tracking with Trace Events_>`_ feature replaces the Alerts feature, as it provides equivalent functionality for event reporting, and it also adds enhanced debugging capabilities that were not available with Alerts.
    * The nRF Cloud REST library.
    * The nRF Cloud logging library.

* :ref:`lib_nrf_cloud_pgps` library:

  * Updated:

    * The range for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` and :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` Kconfig options to values supported by nRF Cloud.
    * The name of the CoAP endpoint used for requesting predictions from ``loc/pgps`` to ``loc/pgnss``.
      The CBOR wire format is unchanged.

  * Fixed:

    * An issue that caused predictions to be stored into an incorrect index in the flash memory, leading to an error when the affected predictions were used.
    * An issue where the library tried to use predictions that had not yet been flushed to the flash memory, leading to a prediction validation error.
    * An issue where the library kept trying to use corrupted predictions.

* :ref:`nrf_802154_callbacks_dispatcher` library:

  * Updated the status from experimental to supported.

* :ref:`lib_mqtt_helper` library:

  * Fixed an issue where the library could overflow the broker address buffer if an IPv6 address was returned by the DNS resolver while the :kconfig:option:`CONFIG_NET_IPV6` Kconfig option was disabled.

Other libraries
---------------

* :ref:`nrf_profiler` library:

  * Added sending :kconfig:option:`CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC` through the info RTT channel as part of the system description.
    This enables support for the nRF54L Series SoCs (:kconfig:option:`CONFIG_SOC_SERIES_NRF54L`) by providing accurate hardware clock information to the profiler host tools.
    The hardware clock frequency varies between different SoCs, and the host tools require this information to correctly interpret the received timestamps.
    This change breaks the backward compatibility between the library and host tools due to modifications in the info RTT channel string layout.
    The nRF Profiler event descriptions are now sent within the ``<ev_info_start>`` and ``<ev_info_stop>`` markers.
    System configuration details are now sent within the ``<sys_config_start>`` and ``<sys_config_stop>`` markers.
    The system configuration details are formatted as a list of key-value data, for example, ``key1,value1``.
    Subsequent entries are separated by newlines.
    The value returned by the :c:func:`sys_clock_hw_cycles_per_sec` function is sent using the ``sys_clock_hw_cycles_per_sec`` key.

* Removed the deprecated Edge Impulse wrapper library.
  The library has been replaced by the Edge Impulse SDK in `Edge AI Add-on for nRF Connect SDK`_.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom` script:

  * Added:

    * License handling of the LLVM toolchain libraries.
    * Support for HEX-only ``softdevice`` sysbuild domains.
    * Updated the SPDX License List database to version 3.28.0.

* :ref:`nrf_profiler_script` script:

  * Updated the scripts to use hardware clock information from SoC to enable accurate conversion from hardware clock cycles to seconds.
    Refactored the parser to support the new RTT communication layout, including the handling of ``<ev_info_...>`` and ``<sys_config_...>`` data markers.
  * Fixed the ``ms_per_timestamp_tick`` parameter (defined in the :file:`rtt_nordic_config.py`) to accurately align with the system clock's frequency of the nRF52, nRF53, and nRF91 Series SoCs.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Edge Impulse integration
------------------------

* Removed the deprecated Edge Impulse integration from the |NCS|.
  The integration has been replaced by `Edge AI Add-on for nRF Connect SDK`_.

Memfault integration
--------------------

* Added:

  * Devicetree support for the internal flash coredump backend (:kconfig:option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`).
    The backend now locates its storage from a fixed partition labeled ``memfault_coredump_partition`` when Partition Manager is disabled, while continuing to support Partition Manager when enabled.
  * The :ref:`snippet-memfault-coredump` snippet that supplies a board-specific partition overlay for flash-backed coredump storage on common DKs and Thingy:91 or Thingy:91 X.

* Updated Memfault to version 1.40.0.
  See the `Memfault firmware SDK changelog`_ for details.

nRF Cloud integration
---------------------

* Added a ``memfaultEn`` control key in the Device Shadow, enabling dynamic enable/disable of Memfault chunk uploads at runtime.
  This is done through Device Shadow updates.
  A new control variable is used.

MCUboot
=======

The MCUboot fork in the |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed a possible infinite loop in the bootloader information sharing procedure when the Kconfig options :kconfig:option:`CONFIG_BOOT_SHARE_DATA_BOOTINFO` and :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION` are enabled.
  Iterating boot-info security counters could spin forever if the :c:func:`boot_nv_security_counter_get` function failed for an image index (for example, backends that expose only one NV counter when :kconfig:option:`CONFIG_BOOT_IMAGE_NUMBER` is greater than one).

* The following non-PSA Crypto implementations were deprecated:

  * :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON`
  * :kconfig:option:`CONFIG_BOOT_ECDSA_TINYCRYPT`
  * :kconfig:option:`CONFIG_BOOT_ECDSA_CC310`
  * :kconfig:option:`CONFIG_BOOT_ED25519_TINYCRYPT`
  * :kconfig:option:`CONFIG_BOOT_ED25519_MBEDTLS`

  Use their PSA Crypto counterparts instead.

* Updated image matching to use the explicit image address when :kconfig:option:`CONFIG_MCUBOOT_CHECK_HEADER_LOAD_ADDRESS` is enabled.
  MCUboot now uses the ``ih_load_address`` field of the image header instead of the reset vector address, allowing it to match an incoming image to the correct partition even when the image is encrypted.
  The new method is enabled by default when MCUboot is built with DTS partitioning.
  Firmware must be signed with the ``imgtool.py sign --rom-fixed <partition_address>`` parameter, which is handled by the |NCS| signing procedures.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in the |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``684c9e8f32e4373a21098559f748f06915f950c9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 684c9e8f32 ^93b5f19f99

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^684c9e8f32

The current |NCS| main branch is based on revision ``684c9e8f32`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

* Added the :file:`release.yaml` file with device classification support overview.
* Updated the :file:`VERSION` file to follow the common version format structure.
  The common version file format structure is extended with a ``VERSION_METADATA`` field for |NCS|.

Documentation
=============

* Added:

  * Description of the MRAM auto-powerdown tradeoff in the :ref:`ug_nrf54h20_pm_optimization` documentation page.
  * MSD disable step in the :ref:`ug_nrf54h20_gs` guide.
  * MRAM11 IronSide SE update limitations in the :ref:`ug_nrf54h20_ironside_update` and :ref:`ug_nrf54h20_ironside_services` documentation pages.
  * The :ref:`ncs_release_model` documentation page.
  * The :ref:`ug_nrf54h20_ironside_se_snapshot` documentation page for using IronSide SE snapshot features.

Updated:

  * The :ref:`ug_nrf54h20_architecture_lifecycle` documentation page, expanding the description of the LCS states.
  * The :ref:`app_boards` documentation page by moving the pages with board definitions under the :ref:`nrf_boards` section.
    This update removes the Supported boards section from the top level of the |NCS| documentation.
  * The :ref:`abi_compatibility` documentation page.
