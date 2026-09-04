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

* Updated documentation build requirements to replace ``m2r2`` with ``myst-parser``.
  See the :ref:`gs_recommended_versions` page for the updated tool list.

Board support
=============

* Added support for the :zephyr:board:`nrf93m1dk`, including a Zephyr cellular modem driver for the nRF93M1 Cat-1 bis LTE module.
  The board uses the nRF93M1 module with the nRF54L15 as the host MCU.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================
* Added:

  * The hidden :kconfig:option:`CONFIG_NCS_MCUBOOT_ENCRYPTION_HMAC_SHA256` Kconfig option to select HMAC-SHA256 with X25519 for compatibility with existing projects that use it.
    The option is hidden and requires addition of a Kconfig override in your project.
    This is intentional as HMAC-SHA512 is recommended over HMAC-SHA256.
  * The :ref:`ug_bootloader_nrf54l_memory_protection` documentation page to explaining the memory protection features of the bootloader on the nRF54L Series.
  * Support for the application core of the nRF54LS05A SoC to MCUboot and secure boot sysbuild, including the secure boot locking and immutable region handling features aligned with the nRF54LS05B SoC

* Removed support for Device Firmware Update (DFU) of the nRF70 Series firmware patch, together with the ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH``, ``SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH``, and ``CONFIG_NRF_WIFI_FW_PATCH_DFU`` Kconfig options.
  See the :ref:`migration_3.5` for details.

* Fixed  MCUboot build failure with image encryption when the ECDSA P-256 signature was enabled on devices based on SoCs with the CryptoCell 310 peripheral, such as nRF52840 and nRF9160 SoCs.
  These configurations will now use PSA Crypto instead of the cc310 backend that supports signature verification only.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF93M1
=======================

* Added the :ref:`ug_nrf93m1` documentation.

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

* Added:

  * The :kconfig:option:`CONFIG_SB_CRACEN_KMU_INVALIDATE_PROTECTED_RAM_SLOTS` sysbuild Kconfig option to populate the Key Management Unit (KMU) slots for invalidation of the CRACEN-protected RAM using nrfutil.
    This option requires ``nrfutil device`` version 2.15.4 or later to work.
    When enabled, the :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT` and :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT` Kconfig options become unavailable, as they implement the same feature through alternative provisioning paths.

Developing with nRF54H Series
=============================

|no_changes_yet_note|

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
  * Support for signature verification with ML-DSA-44, ML-DSA-65, and ML-DSA-87 when using the CRACEN driver.

* Updated:

  * Oberon PSA Crypto from v2.0.0 to v2.1.0.
    The new version has minor updates in internal APIs, restructures the directory hierarchy, and improves native support for built-in keys.
  * nrf_cc3xx_platform and nrf_cc3xx_mbedcrypto libraries to version v0.9.23.
    Improved PSA driver error reporting and fixed an issue that caused incorrect authentication tag generation in GCM when multiple calls to :c:func:`psa_aead_update_ad` were made.

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
    The option is disabled by default, and enabling it may cause deprecation warnings or errors during compilation.
  * Support for Channel Classification in the SoftDevice Controller through the :kconfig:option:`CONFIG_BT_CTLR_CHANNEL_CLASSIFICATION` Kconfig option.

Bluetooth Mesh
--------------

* Added the :ref:`dfu_conf` guide on how to configure DFU for Bluetooth Mesh samples.

DECT NR+
--------

* Updated by improving half-closed association recovery in the nRF91 DECT driver and L2 stack (DLC discard timer, ``RD_NOT_FOUND``, L2 table full).
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

|no_changes_yet_note|

Wi-Fi®
------

* Updated the Connection Manager Wi-Fi connectivity layer to defer the connect request to its dedicated work queue (``wifi_conn_wq``) instead of running it synchronously in the context of the caller of :c:func:`conn_mgr_if_connect()`.
  This allows the stacks of the application, shell, and Connection Manager monitor threads to be reduced, as they no longer need to accommodate the Wi-Fi connect call chain.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

|no_changes_yet_note|

High-Performance Framework (HPF)
--------------------------------

* Added support for the nRF54LC10A SoC.

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
    The support is limited to the :ref:`unicast client app<nrf_audio_unicast_client_app>` application with USB as audio source, which can be built for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target.
    The support is experimental and not yet fully tested, so it is not recommended for production use.

* Removed:

  * :file:`prj_release.conf` files from all nRF Audio applications and from the buildprog tool.
    You must now explicitly specify which configurations to include in a release build.
    See :ref:`nrf_audio_app_configuration_select_build` for more information.
  * The :kconfig:option:`CONFIG_BT_BAP_UNICAST_CONFIGURABLE` option from the unicast client and server applications.
    This option was not useful because the unicast server range settings overwrite the bitrate configuration.

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
  * The ``hid_sci`` and ``release_hid_sci`` build types for the ``nrf54l15dk/nrf54l15/cpuapp`` board target.
    The configurations act as a HID mouse peripheral with HID SCI support.

* Removed:

  * Partition Manager support from the :ref:`nrf_desktop` application.

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

* :ref:`bluetooth_conn_time_synchronization` and :ref:`bluetooth_isochronous_time_synchronization` samples:

  * Fixed an issue on nRF52 and nRF53 Series devices where timed LED toggling did not work due to incorrect GPPI group setup after the nrfx 4.0 API migration.

* :ref:`bluetooth_central_hids`, :ref:`peripheral_hids_keyboard`, and :ref:`peripheral_hids_mouse` samples:

  * Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` board target.
  * Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` board target.
  * Added support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.

* :ref:`peripheral_hids_mouse` sample:

  * Added a "release" HID SCI configuration that lowers the minimum connection interval from 875 µs to 750 µs.

* :ref:`bluetooth_central_hids` sample:

  * Updated the minimum supported connection interval from 875 µs to 750 µs in the HID SCI configuration.
  * Enabled the Frame Space Update feature in the single peripheral HID SCI configuration.

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

  * Updated:

    * The references to the deleted ``CONFIG_CRACEN_LIB_KMU`` Kconfig option to use the :kconfig:option:`CONFIG_CRACEN_KMU` replacement.
    * The TX power calibration for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-13`` dBm to ``-11`` dBm to meet the Fast Pair distance certification requirements.
    * The TX power calibration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp`` board targets.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-15`` dBm to ``-2`` dBm to meet the Fast Pair distance certification requirements.
    * The location of the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options.
      The options were moved from the sample-wide configuration files to the board configuration files in the :file:`configuration/boards` directory, as the TX power correction is hardware-specific.
      Every supported board target now declares its own calibration.

* :ref:`fast_pair_input_device` sample:

  * Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` and ``nrf54lc10dk/nrf54lc10a/cpuapp/ns`` board targets.

Cellular samples
----------------

* Updated Kconfig configuration fragment and devicetree overlay naming by removing the ``overlay-`` prefix for the following samples:

  * :ref:`gnss_sample`
  * :ref:`http_application_update_sample`
  * :ref:`location_sample`
  * :ref:`modem_shell_application`

* :ref:`nrf_cloud_coap_fota_sample` sample:

  * Updated the sample to use the new nRF Cloud CoAP FOTA API.

Cryptography samples
--------------------

* Added:

  * Support for the nRF54LC10A SoC (with and without TF-M) in the crypto samples.
  * The :ref:`crypto_ml_dsa` sample.

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

  * Added support for buttonless entry into firmware loader mode over Bluetooth LE by using the SMP MCUmgr reset command with boot-mode selection.
    To build with this feature, append ``FILE_SUFFIX=ble_enter`` to the build command.
* Updated:

  * The :ref:`mcuboot_minimal_configuration` has been moved to the :file:`samples/dfu` directory.
  * The :ref:`single_slot_sample` sample with support for entering the firmware loader mode over Bluetooth LE using the SMP MCUmgr reset command with boot-mode selection, available through the ``ble_enter`` build variant.
    This is a buttonless DFU enter mechanism.
  * The :ref:`single_slot_sample` sample with support for the ``nrf52840dk/nrf52840`` board target.

* Removed the Firmware loader entrance sample from the :file:`samples/mcuboot` directory.
  Its functionality has been consolidated into the :ref:`single_slot_sample` sample, which now covers all supported MCUmgr transports for the MCUboot firmware updater mode.

DECT NR+ samples
----------------

* :ref:`dect_shell_application` sample:

* Added:

  * Configurable auto-connect with L4-driven trigger.
  * Optional mDNS/DNS-SD advertise (``_dect-nr._udp``) and ``dect discover`` shell command to browse and resolve DECT NR+ peers on the network.

* Updated ``ping`` to use the Zephyr ``net_icmp`` API (IPv6).
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

* Moved all Matter samples from :file:`nrf/samples/matter/` to the `Matter add-on <ncs-matter add-on repository_>`_ repository under :file:`ncs-matter/samples/`.
  Sample paths no longer use the ``samples/matter/`` prefix (for example, :file:`ncs-matter/samples/template` replaces :file:`nrf/samples/matter/template`).
  See :ref:`migration_sdk_nrf_to_ncs_matter` for the full list of path changes, Kconfig renames, and build instructions.

* Renamed Matter-specific Zephyr snippets in the add-on:

  * ``matter-debug`` → ``debug``
  * ``matter-diagnostic-logs`` → ``diagnostic-logs``

* Updated Matter sample CMake integration to use :file:`ncs-matter/cmake/sample.cmake` and the ``ZEPHYR_NCS_MATTER_MODULE_DIR`` variable instead of :file:`nrf/samples/matter/common/cmake/` helpers and ``ZEPHYR_NRF_MODULE_DIR``.

* Moved shared Matter sample code from :file:`nrf/samples/matter/common/` to :file:`ncs-matter/subsys/`.

* Moved Matter partition devicetree include files from :file:`nrf/dts/samples/matter/` to :file:`ncs-matter/dts/`.
  Board overlays must use ``#include <nrf52840_partitions.dtsi>`` instead of ``#include <samples/matter/nrf52840_partitions.dtsi>``.

Networking samples
------------------

* Updated the TF-M configuration in nRF7002 DK networking samples by removing :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_SMALL` (as it is not supported by the |NCS|).
  Instead, use the configurable profile (:kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET`), select the SFN backend (:kconfig:option:`CONFIG_TFM_SFN`) to save RAM and select memory-mapped iovecs (:kconfig:option:`CONFIG_TFM_PSA_FRAMEWORK_HAS_MM_IOVEC`) to reduce copying of client vectors during TLS handshakes.

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

* :ref:`download_sample` sample:

  * Added:

    * Support for mutual TLS (client X.509 certificate authentication), using the new :option:`CONFIG_SAMPLE_PROVISION_CLIENT_CERT` Kconfig option.
    * A :file:`wifi-dtls.conf` extra-conf file with example client certificate and CA trust chain for testing against the Eclipse Californium CoAP interop server.

  * Updated:

    * Enabled CoAP by default so that the sample always builds with support for both HTTP and CoAP.
      The transport is selected automatically at runtime.
    * Enabled the :option:`CONFIG_SAMPLE_COMPUTE_HASH` and :option:`CONFIG_SAMPLE_COMPARE_HASH` options by default.

  * Fixed the HTTP file link, which was previously broken.

* :ref:`net_coap_client_sample` sample:

  * Added:

    * Support for mutual DTLS (client X.509 certificate authentication), using the new :option:`CONFIG_COAP_SAMPLE_DTLS` Kconfig option
    * A :file:`wifi-dtls.conf` extra-conf file with example client certificate and CA trust chain for testing against the Eclipse Californium CoAP interop server.

  * Fixed an issue with the sample's IPv6 support, where the device crashes when trying to communicate over IPv6.

* :ref:`http_server` sample:

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

NFC samples
-----------

|no_changes_yet_note|

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

* Added the :ref:`ppi_seq_spi_sample` sample that demonstrates use of :ref:`ppi_seq_i2c_spi`.

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

Thread samples
--------------

* Added experimental support for the nRF54LC10A SoC to all Thread samples.

Wi-Fi samples
-------------

* :ref:`wifi_nrf_cloud` sample:

  * Added:

    * Support for FOTA and device monitoring through `Memfault`_, using the CoAP transport.
    * Support for dual-slot MCUboot (with FOTA support) on the ``nrf7002dk/nrf5340/cpuapp/ns``, ``nrf54lm20dk/nrf54lm20a/cpuapp/ns``, ``nrf54lm20dk/nrf54lm20b/cpuapp/ns``, and ``nrf7120dk/nrf7120/cpuapp`` board targets.

  * Updated:

    * Transport selection.
      The sample no longer defaults to MQTT.
      You must now explicitly select either the MQTT or the CoAP transport, using the new :file:`mqtt.conf` or the existing :file:`coap.conf` configuration file, respectively.
    * By re-enabling the :kconfig:option:`CONFIG_NET_IPV6` Kconfig option in the :file:`coap.conf` file.
      The option was previously disabled as a workaround for the slow IPv6-to-IPv4 fallback issue that has been fixed in :ref:`lib_nrf_cloud`.

  * Fixed:

    * An issue where the sample entered a reboot loop after Wi-Fi credentials were installed, caused by a workqueue stack that was too small for the Wi-Fi connect call.
    * An issue where enabling :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` could permanently wedge the Wi-Fi connection when the device boots with stored Wi-Fi credentials.

  * Removed:

    * Networking shell support from nRF7002 DK and nRF54LM20 DK.
    * Unused button functionality that only printed a log message.

* Removed support from the following Zephyr samples:

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

* Removed support for the ``nrf5340dk/nrf5340/cpuapp`` board target with the nRF7002 EK shield from the following Zephyr samples:

  * :zephyr:code-sample:`mqtt-sn-publisher`
  * :zephyr:code-sample:`coap-server`

* Added support for the ``nrf7120dk/nrf7120/cpuapp`` board target in the following Zephyr samples:

  * :zephyr:code-sample:`mqtt-sn-publisher`
  * :zephyr:code-sample:`coap-server`

Other samples
-------------

* Added:

  * The :ref:`vtf_monitoring_sample` sample that demonstrates how to capture voltage, temperature, and frequency data using the :ref:`vtf_monitoring` subsystem.
  * The :ref:`pulse_meas` sample that checks the pulse width of an externally provided signal.

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added:

  * The :ref:`ppi_seq` driver for triggering periodic hardware tasks using PPI.
  * The :ref:`ppi_seq_i2c_spi` driver, which is using :ref:`ppi_seq` to perform batches of periodic I2C/SPI transfers without waking up the CPU.
  * The :ref:`vtf_monitoring` for battery voltage, temperature, and frequency monitoring.
  * The :ref:`nrf71_sr_coex` driver, which coordinates Wi-Fi and short-range coexistence on an nRF71 Series device.

SPI drivers
-----------

* SPIM:

  * RTIO based device driver for SPIM has been introduced. This device driver is selected if
    :kconfig:option:`CONFIG_SPI_RTIO` is enabled.

Wi-Fi drivers
-------------

* Added the :ref:`nRF71 Series Wi-Fi driver <nrf71_wifi_fw_if>` page documenting its firmware interface.

* Updated:

  * The :ref:`wifi_drivers` page by restructuring it into separate nRF70 Series and nRF71 Series sections.
  * The default values of the following Kconfig options to reduce the default RAM footprint of the Wi-Fi drivers for the nRF70 and nRF71 Series:
    * :kconfig:option:`CONFIG_NRF70_RX_NUM_BUFS` (or :kconfig:option:`CONFIG_NRF71_RX_NUM_BUFS`) from ``48`` to ``16``.
    * :kconfig:option:`CONFIG_NRF70_MAX_TX_AGGREGATION` (or :kconfig:option:`CONFIG_NRF71_MAX_TX_AGGREGATION`) from ``12`` to ``4``.
    * :kconfig:option:`CONFIG_NRF_WIFI_DATA_HEAP_SIZE` from ``130000`` to ``65536``.

  See :ref:`migration_3.5` for more information.

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

Gazell libraries
----------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`lib_location` library:

  * Updated the library to always use the chosen ``zephyr,wifi`` node instead of ``ncs,location-wifi`` to find the used Wi-Fi device.

* :ref:`modem_key_mgmt` library:

  * Added the :c:func:`modem_key_mgmt_certexpiry` function that would retrieve the expiry date of a credential from the modem.
  * Added :ref:`TLS Credentials Subsystem <zephyr:sockets_tls_credentials_subsys>` support for TLS credential expiry retrieval when using the modem as TLS credentials storage.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud_pgps` library:

  * Updated to use a new parser for assistance data.

* :ref:`lib_nrf_cloud_agnss` library:

  * Updated to use a new parser for assistance data.

* :ref:`lib_nrf_cloud` library:

  * Added:

    * On-device key and CSR generation, enabled with the :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN` Kconfig option.
      The device private key is generated in PSA as a persistent, non-exportable key and referenced for TLS by its key ID, so it never leaves the device.
      The :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL` Kconfig option adds the ``nrf_cloud_cred`` shell commands (``keygen``, ``csr``, ``delete``, and ``pubkey``).
      The :kconfig:option:`CONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY` Kconfig option (enabled by default) exports the on-device public key so that host tooling can verify the key against the device certificate.
      See :ref:`lib_nrf_cloud_credentials_keygen` for more information.

  * Fixed an issue where the library would always attempt an IPv6 connection to nRF Cloud, even if the device had no IPv6 address.
    This led to delays of seconds or tens of seconds, as well as unnecessary traffic and warnings.
    The library now checks for its own IPv6 (or IPv4) address before attempting a connection over that address family.

* :ref:`lib_nrf_provisioning` library:

  * Added a configurable heap allocator for the library's dynamic allocations, selected with the :kconfig:option:`CONFIG_NRF_PROVISIONING_HEAP_KERNEL` (default) and :kconfig:option:`CONFIG_NRF_PROVISIONING_HEAP_SYSTEM` Kconfig options.

Libraries for NFC
-----------------

* :ref:`nfc_ndef_parser_readme`:

  * Fixed an issue where parsing a malformed long-format NDEF record could produce an incorrect payload length.
    The parser now validates type, ID, and payload lengths against the remaining input buffer.

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* Added the :ref:`vtf_monitoring` subsystem for battery voltage, temperature, and frequency monitoring used by the nRF Wi-Fi subsystem.

* :ref:`lib_ram_pwrdn` library:

  * Added support for the nRF54LC10A SoC.

* :ref:`lib_hw_id` library:

  * Added UUID support for the nRF54L Series and the nRF5340 SoC.

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

* Updated:

  * The SPDX output format from ``SPDX-2.2`` to ``SPDX-2.3``.
  * The ScanCode Toolkit detector to support macOS on ARM64 and Linux aarch64.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Removed the nRF53 Series-specific information from the :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide, following the removal of the nRF52 and nRF53 Series support from the Fast Pair samples.

Memfault integration
--------------------

* Added support for setting the Memfault project key at runtime using the :kconfig:option:`CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS` Kconfig option.

* Updated Memfault to version 1.44.0.
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

|no_changes_yet_note|

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

  * Made :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON` the default ECDSA P-256 implementation for nRF54LS05A and nRF54LS05B.
  * The ``--header-size`` imgtool parameter to accept ``auto`` as a value and automatically calculate the smallest possible header.
  * The sysbuild signing scripts for nRF5340 network core to automatically use the smallest needed MCUboot image header instead of the :kconfig:option:`CONFIG_ROM_START_OFFSET` Kconfig value, which has to account for proper VTOR alignment.
    The size of network core firmware image signed for MCUboot is reduced by 480 bytes.

* Deprecated the following non-PSA Crypto implementations:

  * :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON`
  * :kconfig:option:`CONFIG_BOOT_ECDSA_TINYCRYPT`
  * :kconfig:option:`CONFIG_BOOT_ECDSA_CC310`
  * :kconfig:option:`CONFIG_BOOT_ED25519_TINYCRYPT`
  * :kconfig:option:`CONFIG_BOOT_ED25519_MBEDTLS`

  Use their PSA Crypto counterparts instead.

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

  *  The :ref:`kconfig:kconfig_diff` page, displaying differences between available Kconfig options across releases.
     To generate the new documentation page, set the ``KCONFIGDIFF`` CMake option to ``ON``.
  * The API Reference documentation set to serve as an entry point to doxygen-generated API documentation for various components.
  * The page for each sample now contains an `Open in VS Code` button allowing to quickly open the sample and install required version of the |NCS| toolchain.

* Updated:

  * The :ref:`thingy53_app_guide` page by removing the Bluetooth Mesh samples from the list of samples that enable FOTA for Thingy:53 by default.
  * The :ref:`dfu_over_ble` page by removing the section about out-of-the-box point-to-point DFU over Bluetooth Low Energy on Thingy:53.
  * The :ref:`ug_bt_mesh_configuring` page by removing Thingy:53 as an example of a board with the Bluetooth LE Controller on a separate image.
