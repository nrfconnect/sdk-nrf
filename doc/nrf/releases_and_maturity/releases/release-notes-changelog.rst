.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.4.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and other components that implement or support protocol functionality.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.4.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

* Updated the required `SEGGER J-Link`_ version to v9.82.
* Updated documentation build requirements to replace ``m2r2`` with ``myst-parser``.
  See the :ref:`gs_recommended_versions` page for the updated tool list.

Board support
=============

* Added support for the :zephyr:board:`nrf93m1dk`, including a Zephyr cellular modem driver for the nRF93M1 Cat-1 bis LTE module.
  The board uses the nRF93M1 module with the nRF54L15 as the host MCU.

Build and configuration system
==============================

* Removed Partition Manager, which was deprecated in the |NCS| v3.3.0.
  All boards and applications must now use DTS partitioning.
  For details about porting from |NCS| 3.3, see the :ref:`migration_partitions` page.

Bootloaders and DFU
===================

* Added:

  * The hidden :kconfig:option:`CONFIG_NCS_MCUBOOT_ENCRYPTION_HMAC_SHA256` Kconfig option to select HMAC-SHA256 with X25519 for compatibility with existing projects that use it.
    The option is hidden and requires the addition of a Kconfig override in your project.
    This is intentional as HMAC-SHA512 is recommended over HMAC-SHA256.
  * The :ref:`ug_bootloader_nrf54l_memory_protection` documentation page explaining the memory protection features of the bootloader on the nRF54L Series.
  * Support for the application core of the nRF54LS05A SoC to MCUboot and secure boot sysbuild, including the secure boot locking and immutable region handling features aligned with the nRF54LS05B SoC.
  * The :ref:`mcuboot_on_nrf54l_series` documentation page that summarizes MCUboot feature support across the nRF54L SoC series.

* Updated MCUboot to feed the watchdog more aggressively during the following time-consuming operations, to prevent a watchdog timeout:

  * Full slot erase procedures.
  * Swap-move algorithm in the move sectors up loop and in the sectors swap loop.
* Updated:

  * MCUboot to feed the watchdog more aggressively during the following time-consuming operations, to prevent a watchdog timeout:

    * Full slot erase procedures.
    * Swap-move algorithm in the move sectors up loop and in the sectors swap loop.
    * Image hash calculation procedure in the hash calculation loop.

  * The default crypto backend for ECDSA-P256 on the nRF54L Series from TinyCrypt to nRF Security.

* Fixed:

  * MCUboot build failure with image encryption when the ECDSA P-256 signature was enabled on devices based on SoCs with the CryptoCell 310 peripheral, such as the nRF52840 SoC and the nRF9160 SiP.
    These configurations now use PSA Crypto instead of the CC310 backend, which supports only signature verification.
  * MCUboot serial recovery issue on the nRF5340 SoC where the direct upload of the network core image failed when MCUboot was configured for single-slot mode (``CONFIG_SINGLE_APPLICATION_SLOT``).
  * An issue with signing MCUboot direct-XIP variant images where sysbuild used the Zephyr signing path instead of the |NCS| signing script for the secondary slot image.
    The signed variant images could miss |NCS|-specific image metadata, such as the security counter or the ROM-fixed flag.
    As a result, MCUboot could reject the secondary slot image unexpectedly.
  * Potential out-of-bounds access in application's SMP by validating image and slot indexes before use, returning safe fallback values for invalid inputs.

* Removed support for Device Firmware Update (DFU) of the nRF70 Series firmware patch, together with the ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH``, ``SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH``, and ``CONFIG_NRF_WIFI_FW_PATCH_DFU`` Kconfig options.
  See the :ref:`migration_3.5` page for details.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF93M1
=======================

* Added the :ref:`ug_nrf93m1` documentation.

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF71 Series
============================

* Added the :ref:`ug_nrf71` documentation.

Developing with nRF54L Series
=============================

* Added the :kconfig:option:`CONFIG_SB_CRACEN_KMU_INVALIDATE_PROTECTED_RAM_SLOTS` sysbuild Kconfig option to populate the Key Management Unit (KMU) slots for invalidation of the CRACEN-protected RAM using nrfutil.
  This option requires ``nrfutil device`` version 2.15.4 or later to work.
  When enabled, the :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT` and :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT` Kconfig options become unavailable, as they implement the same feature through alternative provisioning paths.

Developing with nRF54H Series
=============================

* Updated the :ref:`ug_nrf54h20_ironside_se_snapshot` page with the following information:

  * Support for the snapshot services only on nRF54H20B devices.
  * Instructions for configuring snapshot in one-time programmable (OTP) memory.
  * The list of predefined snapshot regions.

* Updated the nRF54H20 IronSide SE binaries to v23.8.3+36.
  For the list of changes, see the :ref:`abi_compatibility_ise_changelog`.

Developing with nRF53 Series
============================

* Added a workaround for anomaly 166 on the nRF5340 devices and the :kconfig:option:`CONFIG_SOC_NRF53_ANOMALY_166_WORKAROUND` Kconfig option, which allows enabling the workaround.
  You can use the option also in builds with TF-M.

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Thingy:91 X
===========================

|no_changes_yet_note|

Developing with Thingy:91
=========================

|no_changes_yet_note|

Developing with Thingy:53
=========================

|no_changes_yet_note|

Developing with PMICs
=====================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

* Added experimental support for TX/RX bypass for the SKY66409-11 FEM on the nRF54L Series devices.
* Removed support for the nRF2220 Front-End Module (FEM).

Developing with custom boards
=============================

|no_changes_yet_note|

Security
========

* Added:

  * The :kconfig:option:`CONFIG_TFM_LOG_NS_MEMORY_LAYOUT` Kconfig option, which allows printing the configuration of the Secure Attribution Unit (SAU) and the Memory Protection Controller (MPC) during the initialization of TF-M on the nRF54L Series devices.
    See also :ref:`ug_tfm_logging` for more information.
  * Support for the SHAKE-128 and SHAKE-256 eXtendable Output Functions (XOF) in the CRACEN driver.
  * Support for signature generation and verification with the ML-DSA-44, ML-DSA-65, and ML-DSA-87 algorithms when using the CRACEN driver.
  * Support for key encapsulation and decapsulation with ML-KEM-512, ML-KEM-768, and ML-KEM-1024 when using the CRACEN driver.
  * Support for the SHAKE128/256 hash algorithm in the CRACEN driver on the nRF54L Series devices.

* Updated:

  * Oberon PSA Crypto from v2.0.0 to v2.1.0.
    The new version has minor updates in internal APIs, restructures the directory hierarchy, and improves native support for built-in keys.
  * nrf_cc3xx_platform and nrf_cc3xx_mbedcrypto libraries to version v0.9.23.
    Improved PSA driver error reporting and fixed an issue that caused incorrect authentication tag generation in GCM when multiple calls to :c:func:`psa_aead_update_ad` were made.

* Fixed:

  * The :kconfig:option:`CONFIG_MBEDTLS_X509_REMOVE_INFO` Kconfig option, which had no effect.
    It now defaults to ``n``.
  * The following Kconfig options, which were not passed to the PSA Crypto core:

    * :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
    * :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
    * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
    * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
    * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
    * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
    * :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
    * :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
    * :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
    * :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`

* Removed:

  * The following Kconfig options, which had no effect:

    * ``CONFIG_MBEDTLS_SSL_EXPORT_KEYS``
    * ``CONFIG_MBEDTLS_X509_CHECK_KEY_USAGE``
    * ``CONFIG_MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE``
    * ``CONFIG_MBEDTLS_SSL_DTLS_BADMAC_LIMIT``
    * ``CONFIG_PSA_WANT_ALG_CHACHA20``
    * ``CONFIG_PSA_WANT_ECC_SECT_R1_233``, ``CONFIG_PSA_WANT_ECC_SECT_R1_283``, ``CONFIG_PSA_WANT_ECC_SECT_R1_409``, and ``CONFIG_PSA_WANT_ECC_SECT_R1_571``
    * ``CONFIG_PSA_WANT_KEY_TYPE_PEPPER``

  * The ``CONFIG_PSA_WANT_ALG_ECDSA_ANY`` Kconfig option.
    Use the :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` Kconfig option instead, which also enables ``PSA_ALG_ECDSA_ANY``.

Security libraries
------------------

|no_changes_yet_note|

* :ref:`trusted_storage_readme` library:

  * Added the deprecation note in the library documentation.
    The library is replaced by the :ref:`Secure Storage subsystem <secure_storage>` (:kconfig:option:`CONFIG_SECURE_STORAGE`).

Mbed TLS
--------

* Updated Mbed TLS to v4.1.1 (from v4.1.0) and TF-PSA-Crypto to v1.1.1 (from v1.1.0).
  For more information, see the upstream `Mbed TLS 4.1.1 release notes`_ and `TF-PSA-Crypto 1.1.1 release notes`_.

Trusted Firmware-M (TF-M)
-------------------------

* Updated TF-M to v2.3.1 (from v2.3.0).
  For more information, see the upstream `TF-M 2.3.1 release notes`_.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

* Added:

  * The :kconfig:option:`CONFIG_BT_HCI_SUPPORT_DEPRECATED_COMMANDS` Kconfig option to support deprecated HCI commands.
    The option is disabled by default, and enabling it might cause deprecation warnings or errors during compilation.
  * Support for Channel Classification in the SoftDevice Controller through the :kconfig:option:`CONFIG_BT_CTLR_CHANNEL_CLASSIFICATION` Kconfig option.
  * Support for Decision Based Advertising Filtering in the SoftDevice Controller through the :kconfig:option:`CONFIG_BT_CTLR_DECISION_BASED_ADV_FILTERING` Kconfig option.

Bluetooth Mesh
--------------

* Added the :ref:`dfu_conf` guide on how to configure DFU for Bluetooth Mesh samples.

* Removed:

  * The experimental Distance Measurement Server and Distance Measurement Client vendor models, together with the ``CONFIG_BT_MESH_DM_SRV``, ``CONFIG_BT_MESH_DM_CLI``, and ``CONFIG_BT_MESH_SHELL_DM_CLI`` Kconfig options and the related Bluetooth Mesh shell commands.
  * The key importer functionality, together with the ``CONFIG_BT_MESH_KEY_IMPORTER`` and ``CONFIG_BT_MESH_KEY_IMPORTER_PRIO`` Kconfig options.

DECT NR+
--------

* Added:

  * Experimental DECT NR+ RPC (:kconfig:option:`CONFIG_DECT_NR_RPC`): nRF91 keeps the modem and L2 while an external MCU gets a familiar ``dect0`` ``net_if`` over :ref:`nrf_rpc`, leaving more room on the host for your application.
    DECT NR+ L2 and L2 shell integration mirrors IPv6 and link state and supports Connection Manager connect and disconnect and optional remote ``dect`` shell on the client side.
    See :ref:`dect_rpc` for roles and Kconfig options.

  * DECT NR+ L2 Ethernet sink mode for IPv6 bridging over an Ethernet uplink:

    * Delegated ``/96`` prefix and ULA on DECT NR+
    * ND proxy on behalf of associated PT devices
    * Optional DHCPv6 client
    * Upstream routing through the default router link-local address

  * The :kconfig:option:`CONFIG_NET_L2_DECT_IPV6_IFACE_UNICAST_INSTALL` Kconfig option to the DECT NR+ L2, enabled by default, that installs derived ULA and delegated GUA addresses (and their on-link ULA prefixes) on the DECT NR+ interface.
    Disabling it derives the same addresses without installing them (or their on-link ULA prefixes) on the DECT NR+ interface, keeping only the link-local address on the interface.
    This is an enabler for the tethering GW use case.
    A new :c:func:`dect_net_l2_ipv6_off_iface_unicast_get` function lets upper layers read the addresses that are kept off the interface address list.

  * The :kconfig:option:`CONFIG_DECT_MDM_RX_PRIVATE_POOL` Kconfig option to the nRF91 Series DECT modem driver to allocate DECT RX ``net_pkt``/``net_buf`` from a DECT-only pool isolated from the global RX pools, so that RX bursts on another interface (for example, the tethering GW's Ethernet host leg) cannot starve DECT NR+ RX.
    Pool size is tunable through the ``CONFIG_DECT_MDM_RX_PRIVATE_{PKT,BUF}_COUNT`` and ``_BUF_SIZE`` Kconfig options, and pool usage can be printed with the new ``dect_mdm rx_pool`` shell command.

  * The DECT ICMPv6 ping shell library (:kconfig:option:`CONFIG_DECT_ICMP_PING_LIB`), extracted from the :ref:`dect_shell_application` sample's ``ping`` command, that registers the same top-level ``ping`` shell for IPv6 ICMP echo on DECT NR+ interfaces.
    A hidden :kconfig:option:`CONFIG_DECT_NET_LIBS` gate is set automatically when a DECT network library is selected.
    It enables the shared ``subsys/net/lib/dect/`` build, while each library (including ping) still requires its own Kconfig symbol.

* Updated:

  * The shared DECT shell print callback type.
    ``dect_net_l2_shell_print_fns`` was renamed to :c:struct:`dect_net_lib_shell_print_fns` and moved into the :file:`dect_net_lib_shell.h` file for :kconfig:option:`CONFIG_DECT_L2_SHELL_LIB` and :kconfig:option:`CONFIG_DECT_ICMP_PING_LIB`.
  * By improving half-closed association recovery in the nRF91 DECT driver and L2 stack (DLC discard timer, ``RD_NOT_FOUND``, L2 table full).
    The :c:func:`dect_net_l2_child_association_created` function now returns ``int``; check for ``-ENOSPC`` and release the MAC association.
    The :c:struct:`dect_settings` structure is extended with ``DECT_SETTINGS_WRITE_SCOPE_DLC`` write scope and ``rach_conf_resp_win_length`` field.

* Fixed the DLC TX transaction ID wrap on retry and made the cluster RACH response window length configurable through ``dect sett``.

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Matter
------

* Moved all Matter samples, shared sample infrastructure, devicetree partition files, and Matter-specific snippets from ``sdk-nrf`` to the separate `Matter add-on <ncs-matter add-on repository_>`_ repository (``ncs-matter``).
  The Matter bridge and Thingy:53 weather station reference applications are also relocated into the add-on.
  See :ref:`migration_sdk_nrf_to_ncs_matter` for the migration guide.

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

* Updated the OpenThread stack to `OpenThread v2026.10.0`_.

* Removed:

  * Support for nRF52 Series devices from Thread in the |NCS|.
    OpenThread cannot be enabled on nRF52 SoCs.
  * The prebuilt OpenThread libraries for the nRF52840 device from :ref:`nrfxlib:nrfxlib`.

Wi-Fi®
------

* Updated:

  * The Connection Manager Wi-Fi connectivity layer to defer the connect request to its dedicated work queue (``wifi_conn_wq``) instead of running it synchronously in the context of the caller of :c:func:`conn_mgr_if_connect()`.
    This allows the stacks of the application, shell, and Connection Manager monitor threads to be reduced, as they no longer need to accommodate the Wi-Fi connect call chain.
  * The default value of the :kconfig:option:`CONFIG_NET_MGMT_EVENT_QUEUE_TIMEOUT` Kconfig option to ``50`` milliseconds when :kconfig:option:`CONFIG_NET_L2_WIFI_SHELL` is enabled to prevent Wi-Fi scan events from being dropped when using slow shell backends.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

* Fixed an issue on the Thingy:91 X where the CMSIS-DAP interface could not connect to the nRF91 Series SiP.
  The connection failed because the SiP was held in reset while the debug port was enabled.

High-Performance Framework (HPF)
--------------------------------

* Added:

  * Support for the nRF54LC10A SoC.
  * The :kconfig:option:`SB_CONFIG_HPF_MSPI_IPC_NO_COPY` sysbuild Kconfig option that sets the same IPC data passing mode for both the application image and the HPF MSPI image.
  * Validation of the length of the received IPC message in the HPF MSPI application.

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

* Moved the Matter bridge application to the `Matter add-on <ncs-matter add-on repository_>`_ repository.
  See :ref:`migration_sdk_nrf_to_ncs_matter` for the migration guide.

nRF Audio (formerly nRF5340 Audio)
----------------------------------

* Added:

  * A generalized audio time module that provides a unified interface for retrieving the current time in microseconds across a SoC.
    The module uses the GRTC timer on nRF54L Series devices and the audio sync timer on other devices.
  * Experimental support for the nRF54LM20 SoC in the nRF Audio applications.
    The support is limited to the :ref:`unicast client app <nrf_audio_unicast_client_app>` application with USB as audio source, which can be built for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.
    The support is experimental and not yet fully tested, so it is not recommended for production use.

* Removed:

  * :file:`prj_release.conf` files from all nRF Audio applications and from the buildprog tool.
    You must now explicitly specify which configurations to include in a release build.
    See :ref:`nrf_audio_app_configuration_select_build` for more information.
  * The :kconfig:option:`CONFIG_BT_BAP_UNICAST_CONFIGURABLE` Kconfig option from the unicast client and server applications.
    This option was not useful because the unicast server range settings overwrite the bitrate configuration.

* Updated audio_datapath to use the new audio rate control API.

nRF Desktop
-----------

* Added:

  * Support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` and ``nrf54ls05dk/nrf54ls05a/cpuapp`` board targets.
  * The ``release_fast_pair`` build type for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.
    The configuration acts as a HID mouse with Fast Pair support.
    It uses MCUboot in direct-xip mode with software-based image signature verification.
  * Optional support for dongles with HID SCI, configurable through the :option:`CONFIG_DESKTOP_HID_FORWARD_HID_SCI_ENABLE` Kconfig option.
    The :ref:`nrf_desktop_hid_forward` module now uses :c:macro:`APP_EVENT_SUBSCRIBE_FIRST` to subscribe to the :c:struct:`ble_discovery_complete_event` event.
    The module updates event data to ensure all other modules are notified about the SCI support.
  * HID Shorter Connection Intervals (SCI) support on the peripheral side.
    The :ref:`nrf_desktop_hids` module enables support for the feature in the underlying HID GATT Service.
    The :ref:`nrf_desktop_ble_latency` module handles HID SCI mode change requests and the related connection parameter updates.
    Enable the feature with the :option:`CONFIG_DESKTOP_HIDS_SCI_ENABLE` Kconfig option.
  * The ``hid_sci`` and ``release_hid_sci`` build types for the following board targets:

    * ``nrf54l15dk/nrf54l05/cpuapp``
    * ``nrf54l15dk/nrf54l10/cpuapp``
    * ``nrf54l15dk/nrf54l15/cpuapp``
    * ``nrf54lm20dk/nrf54lm20a/cpuapp``
    * ``nrf54lm20dk/nrf54lm20b/cpuapp``
    * ``nrf54ls05dk/nrf54ls05a/cpuapp``
    * ``nrf54ls05dk/nrf54ls05b/cpuapp``

    The configurations act as a HID mouse peripheral with HID SCI support.
  * The ``hid_sci_keyboard`` and ``release_hid_sci_keyboard`` build types for the following board targets:

    * ``nrf54l15dk/nrf54l05/cpuapp``
    * ``nrf54l15dk/nrf54l10/cpuapp``
    * ``nrf54l15dk/nrf54l15/cpuapp``
    * ``nrf54ls05dk/nrf54ls05a/cpuapp``
    * ``nrf54ls05dk/nrf54ls05b/cpuapp``

    The configurations act as a HID keyboard peripheral with HID SCI support.
  * LLPM dongle application configurations for the nRF54LM20 DK (``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp`` board targets).
  * The :kconfig:option:`CONFIG_NCS_MCUBOOT_DISCARDS_HEADER_IN_SECONDARY_MCUBOOT` Kconfig option that allows you to drop the MCUboot image header in secondary MCUboot image update, when the update is installed to a designated slot by MCUboot.
    If the option is off for compatibility, MCUboot cannot update itself with MCUboot that was built with the option enabled.
    The same condition applies to NSIB that cannot boot MCUboot built with the option enabled, as it has been hardcoded to skip the header.
    For NSIB to work with MCUboot built with the option enabled, you need to build NSIB with :kconfig:option:`CONFIG_SB_IMAGE_BOOT_OFFSET` set to ``0``, which would turn off the header skipping.
    An NSIB + MCUboot setup discards the header.
    You can build this setup using the sysbuild Kconfig option :kconfig:option:`SB_CONFIG_SECURE_BOOT_MCUBOOT_DISCARDS_MCUBOOT_IMAGE_HEADER`.

    .. note::
       The header is still added for DFU as MCUboot requires it for processing the image.
       It is the minimum allowed header, which slightly reduces the DFU size of the MCUboot image, for example by 2016 bytes for nRF54L devices, and increases the partition size available for an MCUboot executable.

* Removed:

  * Partition Manager support from the :ref:`nrf_desktop` application.
  * The deprecated Kconfig options ``CONFIG_DESKTOP_HID_REPORT_EXPIRATION`` and ``CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE``.
  * Support for the nRF52 Series devices from the :ref:`nrf_desktop` application.

Thingy:53: Matter weather station
---------------------------------

* Moved the Thingy:53 Matter Weather Station application to the `Matter add-on <ncs-matter add-on repository_>`_ repository.
  See :ref:`migration_sdk_nrf_to_ncs_matter` for migration instructions.

Installer (MCUboot Firmware Loader installer)
-----------------------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added the :ref:`ble_channel_classification` sample that demonstrates peripheral-initiated channel classification reports and the central applying them as the channel map.

* Removed:

  * Support for the nRF52 Series devices from the following Bluetooth LE samples used in automated Bluetooth LE sample system tests:

    * :ref:`ble_channel_classification`
    * :ref:`bluetooth_conn_time_synchronization` and :ref:`bluetooth_isochronous_time_synchronization`
    * :ref:`direct_test_mode`
    * :ref:`ble_event_trigger`
    * :ref:`bluetooth_iso_combined_bis_cis`
    * :ref:`bt_peripheral_with_multiple_identities`
    * :ref:`bluetooth_radio_coex_1wire_sample`
    * :ref:`ble_radio_notification_conn_cb`
    * :ref:`bt_scanning_while_connecting`
    * :ref:`ble_shorter_conn_intervals`
    * :ref:`ble_subrating`
    * :ref:`ble_le_adv_report_chan_idx`
    * :ref:`bluetooth_direction_finding_central`, :ref:`direction_finding_connectionless_rx`, :ref:`direction_finding_connectionless_tx`, and :ref:`direction_finding_peripheral`
    * :ref:`ble_llpm`
    * :ref:`bluetooth_path_loss_monitoring`
    * :ref:`bluetooth_automated_power_control`
    * :ref:`timeslot_sample`
    * :ref:`radio_test`

  * The nRF52 Series board targets from the :ref:`samples_test_app` Twister configuration.

* :ref:`central_nfc_pairing` and :ref:`peripheral_nfc_pairing` samples:

  * Removed support for the nRF52 Series devices.

* :ref:`bluetooth_conn_time_synchronization` and :ref:`bluetooth_isochronous_time_synchronization` samples:

  * Fixed an issue on nRF52 and nRF53 Series devices where timed LED toggling did not work due to incorrect GPPI group setup after the nrfx 4.0 API migration.

* :ref:`bluetooth_central_hids`, :ref:`peripheral_hids_keyboard`, and :ref:`peripheral_hids_mouse` samples:

  * Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp``, ``nrf54ls05dk/nrf54ls05a/cpuapp``, and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.

  * Removed support for the nRF52 Series devices.

* :ref:`peripheral_hids_mouse` sample:

  * Added a "release" HID SCI configuration that lowers the minimum connection interval from 875 µs to 750 µs.

* :ref:`bluetooth_central_hids` sample:

  * Updated the minimum supported connection interval from 875 µs to 750 µs in the HID SCI configuration.
  * Enabled the Frame Space Update feature in the single peripheral HID SCI configuration.

* :ref:`channel_sounding_ras_initiator` and :ref:`channel_sounding_ras_reflector` samples:

  * Removed support for the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

* :ref:`bluetooth_central_dfu_smp` sample:

  * Updated the sample to use the :ref:`lib_dfu_target` library with the new Bluetooth LE transport to update a remote device over SMP.

Bluetooth Mesh samples
----------------------

* Removed support for the ``thingy53/nrf5340/cpuapp`` board target from the following samples:

  * :ref:`bluetooth_mesh_light`
  * :ref:`bluetooth_mesh_light_lc`
  * :ref:`bluetooth_mesh_light_dim`
  * :ref:`bluetooth_mesh_light_switch`
  * :ref:`bluetooth_mesh_sensor_server`

* :ref:`bluetooth_mesh_light_switch` sample:

  * Added support for the ``nrf54l15tag/nrf54l15/cpuapp`` board target in the LPN configuration.

Bluetooth Fast Pair samples
---------------------------

* Added experimental support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` board target in all Bluetooth Fast Pair samples.

* Removed support for the nRF52 and nRF53 Series devices from the :ref:`fast_pair_locator_tag` and :ref:`fast_pair_input_device` samples.
  The following board targets have been removed from both samples:

  * ``nrf52dk/nrf52832``
  * ``nrf52840dk/nrf52840``
  * ``nrf5340dk/nrf5340/cpuapp``
  * ``nrf5340dk/nrf5340/cpuapp/ns``

  Additionally, the following board targets have been removed from the :ref:`fast_pair_locator_tag` sample:

  * ``nrf52833dk/nrf52833``
  * ``thingy53/nrf5340/cpuapp``
  * ``thingy53/nrf5340/cpuapp/ns``

* :ref:`fast_pair_locator_tag` sample:

  * Added experimental support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` board target.

  * Updated:

    * References to the deleted ``CONFIG_CRACEN_LIB_KMU`` Kconfig option to use the :kconfig:option:`CONFIG_CRACEN_KMU` replacement.
    * The TX power calibration for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-13`` dBm to ``-11`` dBm to meet the Fast Pair distance certification requirements.
    * The TX power calibration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp`` board targets.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-15`` dBm to ``-2`` dBm to meet the Fast Pair distance certification requirements.
    * The TX power calibration for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-13`` dBm to ``0`` dBm to meet the Fast Pair distance certification requirements.
    * The location of the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options.
      The options were moved from the sample-wide configuration files to the board configuration files in the :file:`configuration/boards` directory, as the TX power correction is hardware-specific.
      Every supported board target now declares its own calibration.
    * The sample configuration to use the DULT API variant v2 (:kconfig:option:`CONFIG_DULT_API_VARIANT_V2`).

* :ref:`fast_pair_input_device` sample:

  * Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` and ``nrf54lc10dk/nrf54lc10a/cpuapp/ns`` board targets.

Cellular samples
----------------

* Updated Kconfig configuration fragment and devicetree overlay naming by removing the ``overlay-`` prefix for the following samples:

  * :ref:`gnss_sample`
  * :ref:`http_application_update_sample`
  * :ref:`location_sample`
  * :ref:`modem_shell_application`

* :ref:`modem_shell_application` sample:

  * Added Wi-Fi scan parameter options to the ``location get`` command.
  * Removed support for Bluetooth shell.

* :ref:`nrf_cloud_coap_fota_sample` sample:

  * Updated the sample to use the new nRF Cloud CoAP FOTA API.

Cryptography samples
--------------------

* Added:

  * Support for the nRF54LC10A SoC (with and without TF-M) in the crypto samples.
  * The :ref:`crypto_ml_dsa` sample.
  * The :ref:`crypto_ml_kem` sample.

Debug samples
-------------

|no_changes_yet_note|

DFU samples
-----------

* Added:

  * The :ref:`encrypted_bootloader` sample that demonstrates how to secure device firmware update (DFU) with image encryption enabled for both the application and MCUboot.
  * Support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets to the following samples:

    * :ref:`single_slot_sample`
    * :ref:`nrf_smp_svr_sample`
    * :ref:`mcuboot_minimal_configuration`
    * :ref:`mcuboot_with_decompression`
    * :ref:`mcuboot_with_encryption`, with ECIES-P-256 image encryption using :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON` or :kconfig:option:`CONFIG_BOOT_ECDSA_PSA`.

* :ref:`single_slot_sample` sample:

  * Added:

    * Support for buttonless entry into firmware loader mode over Bluetooth LE by using the SMP MCUmgr reset command with boot-mode selection.
      To build with this feature, append ``FILE_SUFFIX=ble_enter`` to the build command.
    * Support for entering the firmware loader mode over Bluetooth LE using the SMP MCUmgr reset command with boot-mode selection, available through the ``ble_enter`` build variant.
      This is a buttonless DFU entry mechanism.
    * Support for the ``nrf52840dk/nrf52840`` board target.

  * Updated by moving the :ref:`mcuboot_minimal_configuration` to the :file:`samples/dfu` directory.

  * Removed the Firmware loader entrance sample from the :file:`samples/mcuboot` directory.
    Its functionality has been consolidated into the :ref:`single_slot_sample` sample, which now covers all supported MCUmgr transports for the MCUboot firmware updater mode.

DECT NR+ samples
----------------

* Added:

  * The :ref:`dect_tether_ipv6_sample` sample for an nRF91x1 PT device that bridges a DECT NR+ uplink to a wired Ethernet host, acting as an IPv6 gateway (GW) using the new :ref:`lib_dect_tethering` library.
    Ethernet connectivity uses a W5500 SPI shield (Arceli interrupt-driven or Seeed poll-mode) on the nRF9151 DK.
  * The :ref:`dect_rpc_sample` sample that builds RPC client and server images from one tree using the :ref:`dect_rpc` library.
    It showcases a split design.
    Run your application on an nRF54L15 device while the nRF9151 device keeps the DECT NR+ modem and L2 stack.
    On the host MCU, you use ``dect0`` ``net_if`` - IPv6, Connection Manager connect and disconnect, and optional ``dect`` shell and ping, without integrating the modem on that chip.

* :ref:`dect_shell_application` sample:

  * Added:

    * Configurable auto-connect with L4-driven trigger.
    * Optional mDNS/DNS-SD advertise (``_dect-nr._udp``) and ``dect discover`` shell command to browse and resolve DECT NR+ peers on the network.
    * Ethernet border router sink variant using a W5500 shield (``arceli_eth_w5500`` or ``seeed_w5500``), with configuration and devicetree overlays for modem shared memory, static or random MAC, and optional DHCPv6.
    * Shared mDNS configuration overlay (:file:`mdns-common.conf`) and Ethernet mDNS overlay (:file:`eth_mdns.conf`) for DNS-SD on both ``dect0`` and ``eth0``.
    * The :file:`dect_rx_pool.conf` configuration file that enables :kconfig:option:`CONFIG_DECT_MDM_RX_PRIVATE_POOL` with sink/FT-tuned pool sizes, so that DECT NR+ uplink RX bursts no longer starve the shared global RX pools, and rebalances the global TX/RX pool sizes to match observed usage.
    * The :file:`dlc_resilient.conf` configuration file for a loss-resilient DLC profile when using the Ethernet sink mode.

  * Updated ``ping`` to use the Zephyr ``net_icmp`` API (IPv6) and moved it into the location defined by the :kconfig:option:`CONFIG_DECT_ICMP_PING_LIB` Kconfig option.
    The sample selects the library, wires :c:struct:`dect_net_lib_shell_print_fns`, and aborts handling.

  * Fixed the routing logs.
    They are now available through the shell backend only.

Enhanced ShockBurst samples
---------------------------

* Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp``, ``nrf54lc10dk/nrf54lc10a/cpuapp/ns``, and ``nrf54ls05dk/nrf54ls05a/cpuapp`` board targets in all samples.

Gazell samples
--------------

|no_changes_yet_note|

|ISE| samples
-------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Updated:

  * By moving all Matter samples from :file:`nrf/samples/matter/` to the `Matter add-on <ncs-matter add-on repository_>`_ repository under :file:`ncs-matter/samples/`.
    Sample paths no longer use the ``samples/matter/`` prefix (for example, :file:`ncs-matter/samples/template` replaces :file:`nrf/samples/matter/template`).
    See :ref:`migration_sdk_nrf_to_ncs_matter` for the full list of path changes, Kconfig renames, and build instructions.

  * By renaming Matter-specific Zephyr snippets in the add-on:

    * ``matter-debug`` → ``debug``
    * ``matter-diagnostic-logs`` → ``diagnostic-logs``

  * Matter sample CMake integration to use :file:`ncs-matter/cmake/sample.cmake` and the ``ZEPHYR_NCS_MATTER_MODULE_DIR`` variable instead of :file:`nrf/samples/matter/common/cmake/` helpers and ``ZEPHYR_NRF_MODULE_DIR``.

  * By moving the shared Matter sample code from :file:`nrf/samples/matter/common/` to :file:`ncs-matter/subsys/`.

  * By moving the Matter partition devicetree include files from :file:`nrf/dts/samples/matter/` to :file:`ncs-matter/dts/`.
    Board overlays must use ``#include <nrf52840_partitions.dtsi>`` instead of ``#include <samples/matter/nrf52840_partitions.dtsi>``.

Networking samples
------------------

* Updated:

  * The TF-M configuration in nRF7002 DK networking samples by removing :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_SMALL` (as it is not supported by the |NCS|).
    Instead, use the configurable profile (:kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET`), select the SFN backend (:kconfig:option:`CONFIG_TFM_SFN`) to save RAM and select memory-mapped iovecs (:kconfig:option:`CONFIG_TFM_PSA_FRAMEWORK_HAS_MM_IOVEC`) to reduce copying of client vectors during TLS handshakes.
  * The following samples to explicitly select the Protected Storage backend of the :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` (:kconfig:option:`CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE`) in their Wi-Fi configuration, so that TLS credentials persist across reboots:

    * :ref:`aws_iot`
    * :ref:`azure_iot_hub`
    * :ref:`net_coap_client_sample`
    * :ref:`download_sample`
    * :ref:`http_server`
    * :ref:`https_client`
    * :ref:`mqtt_sample`

    The backend was previously selected by a Kconfig default in each sample.
    On boards that do not use TF-M, the :ref:`Secure Storage subsystem <secure_storage>` (:kconfig:option:`CONFIG_SECURE_STORAGE`) is now explicitly enabled in the board configuration files, as it is required by this backend.

* Removed:

  * Support for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target from the following samples:

    * :ref:`mqtt_sample`
    * :ref:`udp_sample`
    * :ref:`net_coap_client_sample`
    * :ref:`https_client`
    * :ref:`http_server`
    * :ref:`download_sample`

  * Support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target from the following samples:

    * :ref:`mqtt_sample`
    * :ref:`net_coap_client_sample`
    * :ref:`http_server`
    * :ref:`download_sample`
    * :ref:`aws_iot`

  * The ``nrf54l15dk/nrf54l15/cpuapp`` with nRF7002 EB shield from the following samples, keeping only the nRF7002-EB II shield for the ``nrf54l15dk/nrf54l15/cpuapp`` board target:

    * :ref:`https_client`
    * :ref:`udp_sample`

* :ref:`aws_iot` sample:

  * Added support for FOTA on the ``nrf7120dk/nrf7120/cpuapp/ns`` board target.

* :ref:`download_sample` sample:

  * Added:

    * Support for mutual TLS (client X.509 certificate authentication), using the new :option:`CONFIG_SAMPLE_PROVISION_CLIENT_CERT` Kconfig option.
    * A :file:`wifi-dtls.conf` extra-conf file with example client certificate and CA trust chain for testing against the Eclipse Californium CoAP interop server.
    * A retry mechanism with exponential backoff, improving robustness when only one IP family becomes ready at a time.

  * Updated:

    * By enabling CoAP by default so that the sample always builds with support for both HTTP and CoAP.
      The transport is selected automatically at runtime.
    * By enabling the :option:`CONFIG_SAMPLE_COMPUTE_HASH` and :option:`CONFIG_SAMPLE_COMPARE_HASH` options by default.

  * Fixed:

    * The HTTP file link, which was previously broken.
    * An issue where network interface teardown was attempted with uninitialized ``net_if``.
    * An issue where the sample could try to download over an IP family for which the device had no address.
      The sample now catches IPv4 and IPv6 connectivity events separately and restricts the download to the available family or families.

* :ref:`udp_sample` sample:

  * Fixed an issue where the sample did not work when configured with an IPv6 server address.
    The sample now extracts the IP family from the :kconfig:option:`CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC` Kconfig option and waits for the corresponding network management connected event before transmitting.

* :ref:`net_coap_client_sample` sample:

  * Added:

    * Support for mutual DTLS (client X.509 certificate authentication), using the new :option:`CONFIG_COAP_SAMPLE_DTLS` Kconfig option.
    * A :file:`wifi-dtls.conf` extra-conf file with example client certificate and CA trust chain for testing against the Eclipse Californium CoAP interop server.
  * Updated the sample to only reboot on fatal errors and reconnect to the server on recoverable errors.
  * Fixed:

    * An issue with the sample's IPv6 support, where the device crashes when trying to communicate over IPv6.
    * An issue where the DTLS handshake failed on the ``nrf7120dk/nrf7120/cpuapp/ns`` board target due to an under-dimensioned TF-M crypto IOVEC buffer.


* :ref:`azure_iot_hub` sample:

  * Added support for FOTA on the ``nrf7120dk/nrf7120/cpuapp/ns`` board target.

  * Updated the MCUboot boot partition size from 54 KB to 64 KB on the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` and ``nrf54lm20dk/nrf54lm20b/cpuapp/ns`` board targets.

* :ref:`https_client` sample:

  * Fixed an issue where the sample could try to connect over IPv4 or IPv6 even when the device had no local address for that family.
    The sample now uses only resolved addresses that match a ready local IPv4/IPv6 address.

* :ref:`http_server` sample:

  * Updated the number of available network connection slots for Wi-Fi builds.
    The sample's base setup already consumed 6 of the 10 slots configured through :kconfig:option:`CONFIG_NET_MAX_CONN`, leaving only 4 for incoming client connections.
    The value was increased from 10 to 16 for more headroom.

  * Fixed:

    * The pregenerated server and client TLS certificates.
      They had only a Common Name (CN) and no Subject Alternative Name (SAN).
      This caused hostname verification to fail with strict TLS clients (for example, Python's ``ssl``/``requests``, used by ``httpie``) even though the certificate chain itself was valid.
      The certificates were regenerated with a SAN entry for ``httpserver.local``.
    * The sample's TLS credential provisioning.
      It silently kept stale credentials in persistent storage if they already existed, so replacing the certificate files and reflashing had no effect on the device.
      Credentials are now re-added if they already exist.
    * IPv6 address and multicast group limits.
      When the access point advertised both SLAAC and stateful DHCPv6, the default :kconfig:option:`CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT` and :kconfig:option:`CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT` values were too low to hold all resulting addresses, causing ``Failed to configure DHCPv6 address`` and ``Cannot join solicit node address ... (-12)`` errors.
      Both Kconfig option values were increased.
    * The HTTP response to GET requests.
      An extra blank line after the ``Content-Type`` header terminated the header section early, putting the ``Content-Length`` header in the response body.

* :ref:`nrf_coap_server_sample` sample:

  * Added a dedicated sample documentation page, and updated the :file:`wifi_zephyr.rst` page to link to it instead of the Zephyr sample documentation.
  * Updated the test scenarios in the :file:`sample.yaml` file to cover the nRF7002 DK, IPv6, and secure CoAP (DTLS) builds.

* :ref:`nrf_mqtt_sn_publisher_sample` sample:

  * Added a dedicated sample documentation page, and updated the :file:`wifi_zephyr.rst` page to link to it instead of the Zephyr sample documentation.
  * Updated the test scenario in the :file:`sample.yaml` file to cover the nRF7002 DK build.

NFC samples
-----------

* Removed support for the nRF52 Series devices in the NFC samples.

nRF5340 samples
---------------

|no_changes_yet_note|

nRF93M1 DK samples
------------------

* Added:

  * The :ref:`nrf93m1dk_modem_bypass` sample that forwards the nRF93M1 modem UART to the USB CDC-ACM VCOM port for direct AT command access from a host PC.
  * The :ref:`nrf93m1dk_ppp_shell` sample that establishes a PPP connection between the nRF54L15 host core and the nRF93M1 modem over CMUX, with shell-driven network management and zperf support.

Peripheral samples
------------------

* Added:

  * The :ref:`ppi_seq_spi_sample` sample that demonstrates use of :ref:`ppi_seq_i2c_spi`.
  * The :ref:`vpr_offloading_sample` sample that demonstrates how to use the VPR core (PPR or FLPR) to offload the application core.

* :ref:`802154_phy_test` sample:

  * Removed support for the nRF52 Series devices.

* :ref:`802154_sniffer` sample:

  * Removed support for the nRF52 Series devices.

* :ref:`radio_test` sample:

  * Added:

    * USB CDC ACM shell support for the ``nrf54lm20dk/nrf54lm20a/cpuapp``, ``nrf54lm20dk/nrf54lm20b/cpuapp``, and ``nrf54lm20dongle/nrf54lm20b/cpuapp`` board targets.
    * SEGGER RTT shell support for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
    * Transport-specific configuration fragments in the :file:`conf/usb.conf` and :file:`conf/rtt.conf` files, used together with the ``cdc-acm-console`` and ``rtt-console`` Zephyr snippets and the ``EXTRA_CONF_FILE`` CMake option.

  * Updated:

    * The sample documentation with build instructions for the USB and RTT shell variants, including ``west rtt`` usage for the nRF54L15 TAG.
    * The nRF5340 DK USB build to use the ``EXTRA_CONF_FILE=conf/usb.conf`` configuration fragment.

PMIC samples
------------

|no_changes_yet_note|

Protocol serialization samples
------------------------------

|no_changes_yet_note|

SDFW samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

SUIT samples
------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

* Added support for the nRF54LC10A SoC in the TF-M samples.
* :ref:`tfm_psa_template` sample:

  * Added support for nRF54L Series devices.

* :ref:`provisioning_image` sample:

  * Added support for nRF54L Series devices.

Thread samples
--------------

* Added experimental support for the nRF54LC10A SoC to all Thread samples.

* Removed support for nRF52 Series devices from all Thread samples.
  The following board targets are no longer supported:

  * ``nrf52840dk/nrf52840``
  * ``nrf52840dongle/nrf52840``
  * ``nrf52833dk/nrf52833``
  * ``nrf21540dk/nrf52840``

Wi-Fi samples
-------------

* Added support for the ``nrf7120dk/nrf7120/cpuapp`` board target in the following Zephyr samples:

  * :zephyr:code-sample:`mqtt-sn-publisher`
  * :zephyr:code-sample:`coap-server`

* :ref:`wifi_shutdown_sample` sample:

  * Added the ``nrf71-idle-power``, ``nrf71-idle-power-quiet``, and ``nrf71-idle-power-diag`` snippets, which use the new :ref:`lib_nrf71_idle_power` library to measure the host idle current of the nRF71 Series device in One-shot mode.

* :ref:`wifi_station_sample`, :ref:`wifi_scan_sample`, and :ref:`wifi_shell_sample` samples:

  * Added the :ref:`lib_nrf71_idle_power` library to the ``nrf7120dk/nrf7120/cpuapp`` board configuration, reducing the System ON idle current.
    The Station and Scan samples also suspend the console while idle between connection or scan events.

* :ref:`wifi_p2p_sample` sample:

  * Added:

    * Support for a TCP echo server in Group Owner mode and a matching TCP echo client in Client mode to exercise the data path over the P2P link.
    * Support for an autonomous persistent Group Owner mode to verify that the stored persistent group is reused across group-add cycles.

* :ref:`wifi_nrf_cloud` sample:

  * Added:

    * Support for FOTA and device monitoring through `Memfault`_, using the CoAP transport.
    * Support for dual-slot MCUboot (with FOTA support) on the ``nrf7002dk/nrf5340/cpuapp/ns``, ``nrf54lm20dk/nrf54lm20a/cpuapp/ns``, ``nrf54lm20dk/nrf54lm20b/cpuapp/ns``, and ``nrf7120dk/nrf7120/cpuapp`` board targets.
    * Button-triggered Memfault FOTA checks and chunk posting (:kconfig:option:`CONFIG_SAMPLE_MEMFAULT_BUTTONS`), demonstrating how the application can drive these actions on demand.

  * Updated:

    * Transport selection.
      The sample no longer defaults to MQTT.
      You must now explicitly select either the MQTT or the CoAP transport, using the new :file:`mqtt.conf` or the existing :file:`coap.conf` configuration file, respectively.
    * By re-enabling the :kconfig:option:`CONFIG_NET_IPV6` Kconfig option in the :file:`coap.conf` file.
      The option was previously disabled as a workaround for the slow IPv6-to-IPv4 fallback issue that has been fixed in :ref:`lib_nrf_cloud`.
    * The MCUboot boot partition size from 48 KB to 64 KB on the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` and ``nrf54lm20dk/nrf54lm20b/cpuapp/ns`` board targets.
    * The :ref:`Secure Storage subsystem <secure_storage>` (:kconfig:option:`CONFIG_SECURE_STORAGE`) to now explicitly enabled on the ``nrf7120dk/nrf7120/cpuapp`` board target, which does not use TF-M.
      It is required by the Protected Storage backend of the TLS Credentials subsystem, which persists TLS credentials across reboots.
    * The settings storage on the ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` and ``nrf54lm20dk/nrf54lm20b/cpuapp/ns`` board targets from :ref:`NVS <zephyr:nvs_api>` (:kconfig:option:`CONFIG_NVS`) to :ref:`ZMS <zephyr:zms_api>` (:kconfig:option:`CONFIG_ZMS`).
    * Temperature reporting to use the on-die temperature sensor instead of simulated values.
      The ``nrf7002dk/nrf5340/cpuapp/ns`` board target still reports simulated values, as no temperature sensor is available to the nRF5340 application core.

  * Fixed:

    * An issue where the sample entered a reboot loop after Wi-Fi credentials were installed, caused by a workqueue stack that was too small for the Wi-Fi connect call.
    * An issue where enabling :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` could permanently wedge the Wi-Fi connection when the device boots with stored Wi-Fi credentials.

  * Removed:

    * Networking shell support from nRF7002 DK and nRF54LM20 DK.
    * The temperature alert and the ``CONFIG_TEMP_ALERT_LIMIT`` Kconfig option, which only printed a log message.

* Removed:

  * Support from the following Zephyr samples:

    * :zephyr:code-sample:`dns-resolve`
    * :zephyr:code-sample:`ipv4-autoconf`
    * :zephyr:code-sample:`mdns-responder`
    * :zephyr:code-sample:`mqtt-publisher`
    * :zephyr:code-sample:`async-sockets-echo`
    * :zephyr:code-sample:`sockets-echo-client`
    * :zephyr:code-sample:`sockets-echo-server`
    * :zephyr:code-sample:`sockets-http-get`
    * :zephyr:code-sample:`sntp-client`
    * :zephyr:code-sample:`syslog-net`
    * :zephyr:code-sample:`telnet-console`

  * Support for the ``nrf5340dk/nrf5340/cpuapp`` board target with the nRF7002 EK shield from the following Zephyr samples:

    * :zephyr:code-sample:`mqtt-sn-publisher`
    * :zephyr:code-sample:`coap-server`

Other samples
-------------

* Added:

  * The :ref:`power_consumption_sample` sample for evaluating System ON Idle power consumption with configurable application RAM retention on the nRF7120 SoC.
  * The :ref:`vtf_monitoring_sample` sample that demonstrates how to capture voltage, temperature, and frequency data using the :ref:`vtf_monitoring` subsystem.
  * The :ref:`pulse_meas` sample that checks the pulse width of an externally provided signal.
  * The :ref:`rtfw_timer_gpio_sample` and :ref:`rtfw_hid_sample` samples demonstrating the RTFW control, fast-path, and event-delivery mechanisms.
  * The :ref:`saadct_sample` sample that demonstrates timer-triggered SAADC sampling using the :ref:`saadct` driver.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added:

  * The :ref:`ppi_seq` driver for triggering periodic hardware tasks using PPI.
  * The :ref:`ppi_seq_i2c_spi` driver, which uses :ref:`ppi_seq` to perform batches of periodic I2C/SPI transfers without waking up the CPU.
  * The :ref:`vtf_monitoring` for battery voltage, temperature, and frequency monitoring.
  * The :ref:`nrf71_sr_coex` driver, which coordinates Wi-Fi and short-range coexistence on an nRF71 Series device.
  * The :ref:`saadct` driver, which uses an external TIMER and GPPI to trigger SAADC sampling at a configured rate.
  * The :ref:`can_scan` device driver, which integrates the :ref:`sCAN` soft peripheral, exposing it through the :ref:`can` device driver API.

SPI drivers
-----------

* SPIM:

  * Added RTIO-based device driver for SPIM.
    This device driver is selected if :kconfig:option:`CONFIG_SPI_RTIO` is enabled.

Wi-Fi drivers
-------------

* Added the :ref:`nRF71 Series Wi-Fi driver <nrf71_wifi_fw_if>` page documenting its firmware interface.

* Updated:

  * The :ref:`wifi_drivers` page by restructuring it into separate nRF70 Series and nRF71 Series sections.
  * The default values of the following Kconfig options to reduce the default RAM footprint of the Wi-Fi drivers for the nRF70 and nRF71 Series:
    * :kconfig:option:`CONFIG_NRF70_RX_NUM_BUFS` (or :kconfig:option:`CONFIG_NRF71_RX_NUM_BUFS`) from ``48`` to ``16``.
    * :kconfig:option:`CONFIG_NRF70_MAX_TX_AGGREGATION` (or :kconfig:option:`CONFIG_NRF71_MAX_TX_AGGREGATION`) from ``12`` to ``4``.
    * :kconfig:option:`CONFIG_NRF_WIFI_DATA_HEAP_SIZE` from ``130000`` to ``65536``.

  See the :ref:`migration_3.5` page for more information.

* Fixed spurious ``set_key`` errors on the nRF71 Series when Wi-Fi keys are deleted after the RPU context has already been torn down.
  Late key deletion is now treated as success when the RPU context was already removed.

Flash drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`hids_readme` library:

  * Added support for runtime customization of connection parameters for a given HID SCI mode through the newly added :c:func:`bt_hids_sci_mode_conn_rate_param_get` API.

* :ref:`bt_fast_pair_readme` library:

  * Added integration of the Find Hub Network (FHN) extension with the DULT API variant v2.
    With this integration, the FHN extension can coexist during the pre-association window with other accessory-locating networks that are registered as DULT users.
    The FHN extension now ties the DULT association to its provisioning state.
    It also reports the DULT ownership state, which is the association state, through the new :c:member:`bt_fast_pair_fhn_info_cb.dult_ownership_state_changed` callback.

  * Fixed the validation of the Data Length parameter in the requests that the FHN extension receives on the Beacon Actions characteristic.
    Requests that declare a Data Length above the maximum supported value are now rejected with the Invalid Value error.

  * Removed the nRF52 and nRF53 Series support.

* :ref:`dtm_twowire_to_hci_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS` Kconfig option to support vendor-specific DTM 2-wire commands.
      The option is enabled by default if :kconfig:option:`CONFIG_BT_HCI_VS` is enabled.
    * A vendor-specific DTM 2-wire command for constant carrier transmission.

Common Application Framework
----------------------------

* :ref:`caf_ble_state`:

  * Added:

    * The :c:struct:`ble_peer_sci_conn_rate_event` to report connection rate changes or failed connection rate change requests when shorter connection intervals are enabled.
    * The :c:struct:`ble_peer_frame_space_updated_event` to report frame space changes or failed frame space update requests.
    * The :c:struct:`ble_peer_phy_updated_event` to report PHY changes.

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

* Added the :ref:`lib_fw_loader_settings` library to pass the firmware loader Bluetooth advertising name from the main application to the firmware loader image using Settings storage.

* :ref:`lib_dfu_target` library:

  * Added an experimental Bluetooth LE transport for the SMP backend.
  * Removed the deprecated ``CONFIG_DFU_TARGET_MCUBOOT_SAVE_PROGRESS`` Kconfig symbol.
  * Updated the library to clear the last page of the flash area when scheduling an update, in case the page contains leftover data from a previous operation.

Gazell libraries
----------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

  * Updated the diagnostic counters of memory allocation failures to depend on the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` Kconfig option.

* :ref:`lib_location` library:

  * Added runtime configuration of the following Wi-Fi scan parameters to the :c:struct:`location_wifi_config` structure:

    * Scan type
    * Bands
    * Active and passive dwell times
    * Maximum number of access points
    * Channels

    The default values are set with the new ``CONFIG_LOCATION_REQUEST_DEFAULT_WIFI_*`` Kconfig options.

  * Updated:

    * The library to always use the chosen ``zephyr,wifi`` node instead of ``ncs,location-wifi`` to find the used Wi-Fi device.
    * Cloud location failures to be logged as warnings instead of errors, as they can happen in normal circumstances, for example, when nRF Cloud cannot resolve a location from the reported Wi-Fi access points.
    * Cloud location errors over MQTT to be reported as errors instead of timeouts.
      The nRF Cloud error code is now logged with both MQTT and CoAP transports.

  * Deprecated the :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SCANNING_PARAMS_OVERRIDE` Kconfig option.

* :ref:`modem_key_mgmt` library:

  * Added:

    * The :c:func:`modem_key_mgmt_certexpiry` function that would retrieve the expiry date of a credential from the modem.
    * :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` support for TLS credential expiry retrieval when using the modem as TLS credentials storage.

* :ref:`sms_readme` library:

  * Fixed an issue where the :c:func:`sms_register_listener` function returned a positive value instead of a negative error code when the modem responded with an AT error during SMS client registration.
    The function now returns ``-EFAULT`` in this case.

* :ref:`nrf_modem_lib_readme` library:

  * Fixed RTT channel allocation after warm boot.

* :ref:`lte_lc_readme` library:

  * Added:

    * The :kconfig:option:`CONFIG_LTE_LC_UICC_STATE_MODULE` Kconfig option to enable UICC (SIM card) state notifications, reported in the :c:enumerator:`LTE_LC_EVT_UICC_STATE` event.
    * Support for the ``%MDMEV: SEARCH STATUS 3`` and ``%MDMEV: SEARCH STATUS 4`` notifications, reported in the :c:enumerator:`LTE_LC_MODEM_EVT_PERIODIC_SEARCH_START` and :c:enumerator:`LTE_LC_MODEM_EVT_PERIODIC_SEARCH_DONE` modem events.
    * The :kconfig:option:`CONFIG_LTE_LC_BAND_LOCK_MODULE` Kconfig option and the :c:func:`lte_lc_band_lock_set` and :c:func:`lte_lc_band_lock_remove` functions to set and remove the permanent and runtime band locks.
      The runtime band lock is stored by the library and set again when the modem is initialized or has been powered off.

  * Updated the :kconfig:option:`CONFIG_LTE_LOCK_BANDS` Kconfig option to select the :kconfig:option:`CONFIG_LTE_LC_BAND_LOCK_MODULE` Kconfig option.
    The bands set using the :kconfig:option:`CONFIG_LTE_LOCK_BAND_MASK` and :kconfig:option:`CONFIG_LTE_LOCK_BAND_LIST` Kconfig options are now used as the initial runtime band lock.
    You can change the runtime band lock using the :c:func:`lte_lc_band_lock_set` function.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* Added:

  * The :ref:`lib_dect_tethering` library (:kconfig:option:`CONFIG_DECT_TETHER_IPV6_LIB`, :ref:`experimental <software_maturity>`) that turns a DECT NR+ uplink into an IPv6 gateway (GW) for a tethered host on Ethernet, without relying on host-side SLAAC.
    It performs the following operations:

    * Sends ICMPv6 Router Advertisements (default router, RDNSS, Managed flag, PIO with ``A=0``/``L=0``) so that the host uses DHCPv6 for addressing.
    * Provides a built-in minimal DHCPv6 server (UDP 547) that offers ULA and delegated GUA ``/128`` addresses derived from the DECT NR+ interface.
    * Optionally forwards IPv6 mDNS (UDP 5353) between the Ethernet and DECT NR+ interfaces through an ``AF_PACKET`` tap.

  * The :ref:`dect_rpc` library (:kconfig:option:`CONFIG_DECT_NR_RPC`, :ref:`experimental <software_maturity>`) that offloads the DECT NR+ modem and L2 stack to the nRF91 device while an external MCU keeps a standard Zephyr ``net_if`` over :ref:`nrf_rpc` (UART by default).
    It provides the following:

    * A client role that mirrors server ``dect0`` IPv6 addresses, link state, and MTU, and tunnels send and receive over RPC.
      With :kconfig:option:`CONFIG_DECT_NR_RPC_CONN_MGR`, Connection Manager on the tunneled ``net_if`` proxies connect/disconnect to the server.
      The DECT association policy remains on the nRF91 device, with L4 events from the mirrored interface state.
    * A server role on :kconfig:option:`CONFIG_NET_L2_DECT` (:kconfig:option:`CONFIG_DECT_NR_RPC_SERVER`) that injects client traffic into the DECT NR+ path, forwards IPv6 RX and link state to an active client, and drives Connection Manager connect and disconnect on server ``dect0`` when :kconfig:option:`CONFIG_NET_L2_DECT_CONN_MGR` is enabled.
    * Optional remote ``dect`` L2 shell over RPC (:kconfig:option:`CONFIG_DECT_NR_RPC_SHELL`, :kconfig:option:`CONFIG_DECT_L2_SHELL_RPC`).

    See the :ref:`dect_rpc_sample` sample for a two-board nRF54L15 + nRF9151 reference design.

  * The DECT ICMPv6 ping shell library (:kconfig:option:`CONFIG_DECT_ICMP_PING_LIB`), which provides ICMPv6 ping functionality for use from shell commands.
    The functionality was extracted from the ``ping`` command of the :ref:`dect_shell_application` application into a reusable library.
    It supports hostname resolution, configurable echo request parameters (such as count, interval, and payload size), round-trip time (RTT) statistics, and cancelling an ongoing ping using a :c:struct:`k_poll_signal`.
    Shell output is handled through the shared :c:struct:`dect_net_lib_shell_print_fns` API (:file:`dect_net_lib_shell.h`).

* :ref:`lib_nrf_cloud_pgps` library:

  * Updated to use a new parser for assistance data.

* :ref:`lib_nrf_cloud_agnss` library:

  * Updated to use a new parser for assistance data.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * Application-provided JWT generation, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_JWT_SOURCE_APP` Kconfig option.
      This allows applications communicating with an nRF91 modem over an external link to keep JWT signing in the modem.

    * On-device key and CSR generation, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` Kconfig option.
      The device private key is generated in PSA as a persistent, non-exportable key and referenced for TLS by its key ID, so it never leaves the device.
      The :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` Kconfig option adds the ``nrf_cloud_cred`` shell commands (``keygen``, ``csr``, ``delete``, and ``pubkey``).
      The :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` Kconfig option (enabled by default) exports the on-device public key so that host tooling can verify the key against the device certificate.
      See :ref:`lib_nrf_cloud_credentials_keygen` for more information.

    * The :kconfig:option:`CONFIG_NRF_CLOUD_COAP_SEND_TIMEOUT_SEC` Kconfig option to set a send timeout on the nRF Cloud CoAP socket.
      Previously, when ``SO_KEEPOPEN`` was in use, a send could block indefinitely when the modem send buffer was full, for example, after losing LTE coverage.
      The default timeout is 20 seconds.

  * Updated the logging of location requests that nRF Cloud cannot resolve (error code ``40410``).
    Instead of several errors, the library now logs a single warning, as this can happen in normal circumstances, for example, with unknown Wi-Fi access points.

  * Fixed an issue where the library would always attempt an IPv6 connection to nRF Cloud, even if the device had no IPv6 address.
    This led to delays of seconds or tens of seconds, as well as unnecessary traffic and warnings.
    The library now checks for its own IPv6 (or IPv4) address before attempting a connection over that address family.

* :ref:`lib_mqtt_helper` library:

  * Fixed an issue where the library could stop after one resolved broker address.
    The library now iterates all resolved IPv4/IPv6 addresses and only tries families for which the device has a local address.

* :ref:`lib_nrf_provisioning` library:

  * Added a configurable heap allocator for the library's dynamic allocations, selected with the :kconfig:option:`CONFIG_NRF_PROVISIONING_HEAP_KERNEL` (default) and :kconfig:option:`CONFIG_NRF_PROVISIONING_HEAP_SYSTEM` Kconfig options.
  * Updated the library to send the :c:enumerator:`NRF_PROVISIONING_EVENT_FATAL_ERROR` event when the modem fails to generate the attestation token for an unclaimed device.
    Previously, the :c:enumerator:`NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED` event was sent without a token.

* :ref:`lib_aws_fota` library:

  * Added:

    * The :c:enum:`aws_fota_error_cause` enumeration and the ``cause`` field in the :c:struct:`aws_fota_event` structure.
      The field is set when the :c:enumerator:`AWS_FOTA_EVT_ERROR` event is sent, and reports why the FOTA job failed.

  * Updated:

    * The job execution update that marks a job as ``FAILED`` to report the failure in the AWS IoT Jobs ``statusDetails`` field, as ``{"reason":"<cause>","progress":"<percentage>"}``, instead of sending ``null``.
      This makes it possible to diagnose a failed update from the AWS IoT Jobs console without a serial log from the device.
    * The handling of a job message that is larger than the :kconfig:option:`CONFIG_AWS_FOTA_PAYLOAD_SIZE` Kconfig option.
      If the library can identify the job in the beginning of the message, it now sets the job execution to ``FAILED`` with the ``PAYLOAD_TOO_LARGE`` reason in the status details.
      AWS IoT Jobs then does not send the job again.
    * By increasing the default value of the :kconfig:option:`CONFIG_AWS_FOTA_PAYLOAD_SIZE` Kconfig option from 1350 to 4096 bytes, so that a job message with a presigned URL fits in the buffer.

  * Fixed an issue where a job message that was larger than the :kconfig:option:`CONFIG_AWS_FOTA_PAYLOAD_SIZE` Kconfig option closed the MQTT connection, which made the :ref:`aws_iot` reboot in a loop.
    The library now drops the message, keeps the connection and reports the :c:enumerator:`AWS_FOTA_ERROR_CAUSE_PAYLOAD_TOO_LARGE` error cause to the application.

  * Removed the variable-length arrays that were used to log the received job document and topic when the :kconfig:option:`CONFIG_AWS_FOTA_LOG_LEVEL_DBG` Kconfig option is enabled.
    The job document is now logged as a hexdump of at most 256 bytes, which does not use the stack.

* :ref:`lib_aws_iot` library:

  * Updated the :c:enumerator:`AWS_IOT_EVT_FOTA_ERROR` event to report the cause of the FOTA failure in the ``data.err`` field, as a value of the :c:enum:`aws_fota_error_cause` enumeration.
    Previously, the event carried no payload.

Libraries for NFC
-----------------

* Fixed an issue where calling the NFC platform callback in the :file:`platform_internal_thread` file with zero-length data and ``copy_data`` enabled could corrupt the ring buffer and cause incorrect header parsing in the consumer thread.
* Removed support for the nRF52 Series devices in the NFC subsystem and library.

* :ref:`nfc_ndef_parser_readme`:

  * Fixed an issue where parsing a malformed long-format NDEF record could produce an incorrect payload length.
    The parser now validates type, ID, and payload lengths against the remaining input buffer.

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added:

  * The :ref:`lib_nrf71_idle_power` library to reduce the System ON idle current of an nRF71 Series application core by applying a configurable RAM retention level at boot and enabling the device power management options needed for peripherals to suspend while idle.
    The library also provides the :c:func:`nrf71_idle_power_suspend_console` and :c:func:`nrf71_idle_power_resume_console` functions, which suspend and resume the console or UART device during an application's idle period, for use by any sample that requires the lowest possible idle current.
  * The :ref:`vtf_monitoring` subsystem for battery voltage, temperature, and frequency monitoring used by the nRF Wi-Fi subsystem.
  * The :ref:`lib_rtfw` proof-of-concept library for executing bounded, latency-sensitive operations in an application-core zero-latency interrupt.

* :ref:`lib_ram_pwrdn` library:

  * Added support for the nRF54LC10A SoC.

* :ref:`lib_hw_id` library:

  * Added UUID support for the nRF54L Series and the nRF5340 SoC.

* :ref:`lib_date_time` library:

  * Fixed an issue where periodic time updates stopped permanently if the time was obtained, for example, from an ``%XTIME`` notification shortly before a scheduled update.

* :ref:`dult_readme` library:

  * Added:

    * The DULT API variant v2 (:kconfig:option:`CONFIG_DULT_API_VARIANT_V2`), which supports registering more than one accessory-locating network at the same time.
      The :kconfig:option:`CONFIG_DULT_USER_MAX` Kconfig option sets the maximum number of registered networks.
      See the :ref:`ug_dult` guide for details.
      The following API elements are exclusive to this variant:

      * The :c:func:`dult_user_unregister` function for performing the final teardown of a registered user.
        In contrast, the :c:func:`dult_reset` function only releases the association and keeps the user registered.
      * The :c:func:`dult_multi_user_cb_register` function for notifying the registered users about the DULT association arbitration outcome.
      * The :c:func:`dult_multi_user_conn_claim` function for routing the Accessory Non-owner Service operations that arrive before the DULT association to the DULT user that owns the Bluetooth connection.

    * The :c:func:`dult_user_is_associated` and :c:func:`dult_is_any_associated` functions for checking the DULT association state.
    * The :c:func:`dult_bt_adv_data_fill` function and the :c:struct:`dult_bt_adv_data` structure for serializing the DULT location-enabled advertising payload.
    * The :c:struct:`dult_bt_anos_cb` callback structure and the :c:func:`dult_bt_anos_cb_register` function that let a network layered on top of DULT override the default separated-state access gate for the Accessory Information operations.
    * Support for the optional ``Get_Network_Version`` operation through the new ``network_version`` field of the :c:struct:`dult_user` structure.
    * The :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` and :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_LARGE` Kconfig options for declaring the accessory size.
      The declared size controls whether the mandatory accessory capabilities are enforced during the DULT user registration.
    * A runtime override of the motion detector test timing through the :c:func:`dult_test_motion_detector_separated_ut_period_set` function.
      This override is available when you enable the :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` Kconfig option.

  * Updated:

    * The DULT public headers by moving them to a dedicated :file:`include/dult/` directory.
      Include :file:`dult/dult.h` instead of the deprecated :file:`dult.h`.
    * The ``struct dult_firmware_version`` structure by renaming it to :c:struct:`dult_version`.
      The deprecated :file:`dult.h` header still provides the ``dult_firmware_version`` alias to maintain backward compatibility.

  * Deprecated the DULT API variant v1 (:kconfig:option:`CONFIG_DULT_API_VARIANT_V1`), which remains the default for backward compatibility.

Shell libraries
---------------

* Fixed a potential lockup in the NUS shell transport after a Bluetooth disconnect.

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom` script:

* Added:

  * ``--package-download-format`` option to control the SPDX PackageDownloadLocation format.
  * ``--input-dir`` option to the :ref:`west ncs-sbom <west_sbom>` command.
    It recursively adds all files in the given directory to the report, equivalent to ``--input-files DIR/**/*``.
  * ``--spdx-version`` option to generate SPDX 3.0 (JSON-LD) reports.
    The default remains SPDX 2.3.
  * Package dependency relationships collected from Ninja build inputs and GNU linker maps,
    including nested GN and CMake builds in both SPDX output formats.

* Updated:

  * The SPDX output format from ``SPDX-2.2`` to ``SPDX-2.3``.
  * The ScanCode Toolkit detector to support macOS on ARM64 and Linux aarch64.

* :ref:`nrf_profiler_script` script:

  * Added the ``-o <float>`` / ``--offset`` argument to the :file:`merge_data.py` script.
    The argument accepts a floating-point peripheral synchronization event offset in microseconds and is applied before clock drift compensation.
    This allows tuning merged results for the repeatable, constant delays.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Updated the :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide to describe how the FHN extension claims and releases the DULT association in each DULT API variant.
  The guide also describes how to handle the new :c:member:`bt_fast_pair_fhn_info_cb.dult_ownership_state_changed` callback when your product registers multiple accessory-locating networks.

* Removed the nRF53 Series-specific information from the :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide, following the removal of the nRF52 and nRF53 Series support from the Fast Pair samples.

Memfault integration
--------------------

* Added support for setting the Memfault project key at runtime using the :kconfig:option:`CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS` Kconfig option.

* Updated Memfault to version 1.45.0.
  See the `Memfault firmware SDK changelog`_ for details.

* Removed the ``CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES`` Kconfig option from nRF91x targets.
  Certificate provisioning for nRF91x targets is now handled automatically by the `Memfault firmware SDK`_.
  The option remains available for nRF7002 targets, which do not have automatic certificate provisioning.

* Deprecated the ``CONFIG_MEMFAULT_NCS_PROJECT_KEY`` Kconfig option in favor of the Memfault SDK-native ``CONFIG_MEMFAULT_PROJECT_KEY`` option.

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

* Added:

  * A ``memfaultModemKey`` control key in the Device Shadow, enabling the Memfault modem FOTA project key to be provisioned at runtime through Device Shadow updates.
    This is applied using the :c:func:`memfault_zephyr_fota_modem_project_key_set()` function and requires the :kconfig:option:`CONFIG_MEMFAULT_FOTA_MODEM_UPDATE` Kconfig option to be enabled.

  * The :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_POLL_JOB_CHECK_PROGRESS_THRESHOLD` Kconfig option to the :ref:`lib_nrf_cloud` FOTA polling helpers, allowing the progress-based FOTA job re-check to be configured or disabled.

* Fixed a memory leak in the :ref:`lib_nrf_cloud` FOTA polling helpers where the temporary job info returned by each FOTA job check was not released on all code paths.

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

* Updated the :ref:`ug_dult` guide to describe both DULT API variants, the multi-user coexistence workflow, and the serialization of the location-enabled advertising payload.
  The guide also describes the Accessory Non-owner Service access policy, including access before the accessory is associated with an accessory-locating network.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Support for the nRF54LC10A and nRF54LS05A SoCs.
  * ECIES-P-256 encrypted image support when using :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON`, with the ocrypto software backend used instead of TinyCrypt.
  * ECIES-P-256 encrypted image support for the :kconfig:option:`CONFIG_BOOT_ECDSA_PSA` path by auto-selecting the required PSA algorithms.
  * Support for multiple image verification keys compiled into MCUboot.
    The :kconfig:option:`CONFIG_BOOT_SIGNATURE_KEY_FILE` Kconfig option accepts a comma-separated list of PEM files.
    Only public key material is embedded in the bootloader image.
    This enables a production or development signing custody model in which, for example, an updatable development bootloader can boot images signed with either key while a production bootloader embeds only the production verification key.
    MCUboot ``imgtool`` adds the ``keyinfo`` subcommand and ``--name-suffix`` for ``getpub`` and ``getpubhash``, which support the multiple keys embedded in the bootloader image.

* Updated:

  * The :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON` Kconfig option to be the default ECDSA P-256 implementation for the nRF54LS05A and nRF54LS05B SoCs.
  * The ``--header-size`` imgtool parameter to accept ``auto`` as a value and automatically calculate the smallest possible header.
  * The sysbuild signing scripts for nRF5340 network core to automatically use the smallest needed MCUboot image header instead of the :kconfig:option:`CONFIG_ROM_START_OFFSET` Kconfig value, which has to account for proper VTOR alignment.
    The size of network core firmware image signed for MCUboot is reduced by 480 bytes.

* Fixed:

  * An issue where key revocation might fail in multi-image configurations with KMU enabled.
  * An issue where MCUboot did not clear a serial recovery boot request after entering serial recovery when the :kconfig:option:`CONFIG_NRF_BOOT_SERIAL_BOOT_REQ` Kconfig option is enabled, causing the device to re-enter serial recovery on every reset instead of booting the application.
  * A bus fault in the custom type-length-value (TLV) validation hook when MCUboot runs in single-slot mode on the nRF54H20 SoC.
  * An issue where MCUboot did not validate image header size, image size, and protected TLV size against the boundaries of the flash partition that holds the installed image.
  * An issue where MCUboot serial recovery with progressive erase (:kconfig:option:`CONFIG_BOOT_ERASE_PROGRESSIVELY`) could erase the last image sector of a large upload when the image extended into the slot trailer area in external flash (for example, on the nRF54H20 SoC).

* Restored the :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON` Kconfig option (for the Crypto implementation support), which was marked as deprecated in the previous release.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``7d46db352251f85a6bc7b5961fb8a86e2f3125e4``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 7d46db3522 ^684c9e8f32

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^7d46db3522

The current |NCS| main branch is based on revision ``7d46db3522`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

* Updated the :file:`VERSION` file to follow the common version format structure.
  The common version file format structure is extended with a ``VERSION_METADATA`` field for |NCS|.

zcbor
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * The :ref:`kconfig:kconfig_diff` page, displaying differences between available Kconfig options across releases.
    To generate the new documentation page, set the ``KCONFIGDIFF`` CMake option to ``ON``.
  * The API Reference documentation set to serve as an entry point to doxygen-generated API documentation for various components.
  * An :guilabel:`Open in VS Code` button for each sample page.
    It lets you quickly open the sample and install the required version of the |NCS| toolchain.

* Updated:

  * The :ref:`install_ncs` page by adding a button for installing the |nRFVSC| with the latest stable release of the |NCS|.
    This has also simplified the structure of the page.
  * The :ref:`ug_wifi` documentation to make it agnostic to Nordic Semiconductor Wi-Fi devices by moving nRF70 Series specific pages, including Wi-Fi stack configuration and performance, Debugging, and Wi-Fi certification, under :ref:`ug_nrf70`.
  * The :ref:`thingy53_app_guide` page by removing the Bluetooth Mesh samples from the list of samples that enable FOTA for Thingy:53 by default.
  * The :ref:`dfu_over_ble` page by removing the section about out-of-the-box point-to-point DFU over Bluetooth Low Energy on Thingy:53.
  * The :ref:`ug_bt_mesh_configuring` page by removing Thingy:53 as an example of a board with the Bluetooth LE Controller on a separate image.
  * The :ref:`ug_nrf54h20_ironside_se_snapshot` page by adding a note about the snapshot services limitations.
  * The :ref:`abi_compatibility` page by adding a section about |ISE| known issues.
  * The :ref:`ug_nrf70_features` page by moving the :ref:`ug_nrf70_features_hostap` section to the :ref:`ug_wifi_overview` page.
