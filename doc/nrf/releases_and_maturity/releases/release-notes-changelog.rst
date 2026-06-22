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

|no_changes_yet_note|

Board support
=============

* Added support for the :zephyr:board:`nrf93m1dk`, including a Zephyr cellular modem driver for the nRF93M1 Cat-1 bis LTE module.
  The board uses the nRF93M1 module with the nRF54L15 as the host MCU.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================
* Added the hidden :kconfig:option:`CONFIG_NCS_MCUBOOT_ENCRYPTION_HMAC_SHA256` to select HMAC-SHA256 with X25519 for compatibility with existing projects that use it.
  The option is hidden and requires addition of Kconfig override in your project.
  This is intentional as HMAC-SHA512 is recommended over HMAC-SHA256.

* Removed support for Device Firmware Update (DFU) of the nRF70 Series firmware patch, together with the ``SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH``, ``SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH``, and ``CONFIG_NRF_WIFI_FW_PATCH_DFU`` Kconfig options.
  See the :ref:`migration_3.5` for details.

Developing with nRF91 Series
============================

|no_changes_yet_note|

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

|no_changes_yet_note|

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

|no_changes_yet_note|

Developing with custom boards
=============================

|no_changes_yet_note|

Security
========

* Added:

  * Support for the X25519 key pair storage in the :ref:`Key Management Unit (KMU) <ug_kmu_guides_supported_key_types>`.
  * The :kconfig:option:`CONFIG_TFM_LOG_NS_MEMORY_LAYOUT` Kconfig option, which allows printing the configuration of the Secure Attribution Unit (SAU) and the Memory Protection Controller (MPC) during the initialization of TF-M on the nRF54L Series devices.
    See also :ref:`ug_tfm_logging` for more information.
  * Support for the SHAKE-128 and SHAKE-256 eXtendable Output Functions (XOF) in the CRACEN driver.

* Updated:

  * Oberon PSA Crypto from v1.5.4 to v2.0.0.
    The new version aligns with TF-PSA-Crypto v1.1.0.
  * How the :kconfig:option:`CONFIG_NRF_SECURITY` and :kconfig:option:`CONFIG_PSA_CRYPTO` interact with each other.
    :kconfig:option:`CONFIG_NRF_SECURITY` is now promptless and auto-enabled indirectly by :kconfig:option:`CONFIG_PSA_CRYPTO`.
  * Approach to store keys in the KMU so that AEAD algorithms with non-default (shortened) tag lengths are supported.

* Fixed issues with incorrect support status on the :ref:`ug_crypto_supported_features` page:

  * The :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM` Kconfig option is now correctly listed as unsupported for SoCs with Arm CryptoCell CC310.
  * The tables for supported AES key wrapping algorithms for nRF54L Series devices now list the nRF54LS05 device (not supported in the CRACEN driver; experimental in the nrf_oberon driver).
  * The post-quantum cryptography algorithms for nrf_oberon under Key types and key management are now correctly listed as experimental instead of supported.
  * The SPAKE2+ for Matter is now correctly listed as supported instead of experimental.
  * The WPA3-SAE hash-to-element algorithm is now correctly listed as a KDF algorithm, not a PAKE algorithm.
  * The SHA-256/192 and SHAKE hashing algorithms are now correctly listed as not supported in the CRACEN driver and Experimental in the nrf_oberon driver.
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

* Updated Mbed TLS to v4.1.0 (from v3.6.6).
  For an overview of the changes brought by this update in Mbed TLS and Zephyr, see the following pages:

  * The Mbed TLS sections of the Zephyr v4.4 :ref:`release notes <zephyr_4.4>` and :ref:`migration guide <migration_4.4>`.

  * The release notes from upstream Mbed TLS:

    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.0.0
    * https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/tag/tf-psa-crypto-1.0.0
    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.1.0
    * https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/tag/tf-psa-crypto-1.1.0

  As part of this update, the following changes were made:

  * Removed support for legacy, deprecated Mbed TLS Crypto APIs and related tests.
  * Rearranged Kconfig options related to Mbed TLS in various files under :ref:`nrf_security`.
  * Removed outdated Kconfig options related to Mbed TLS in various files under :ref:`nrf_security`.
  * :ref:`nrf_security` now uses the Mbed TLS integration from Zephyr as-is (:kconfig:option:`CONFIG_MBEDTLS_BUILTIN`) while replacing upstream TF-PSA-Crypto with Oberon PSA Crypto (:kconfig:option:`CONFIG_TF_PSA_CRYPTO_CUSTOM`).

  From this update onwards:

  * Enable :kconfig:option:`CONFIG_MBEDTLS` only if you use TLS or X.509.
  * For cryptographic operations, enable only :kconfig:option:`CONFIG_PSA_CRYPTO`.

Trusted Firmware-M (TF-M)
-------------------------

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

|no_changes_yet_note|

Bluetooth Mesh
--------------

* Added the :ref:`dfu_conf` guide on how to configure DFU for Bluetooth Mesh samples.

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Matter
------

* Replaced the tables on the :ref:`ug_matter_hw_requirements_ram_flash` and :ref:`ug_matter_hw_requirements_layouts` pages with memory layout charts.

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

|no_changes_yet_note|

nRF Audio (formerly nRF5340 Audio)
----------------------------------

* Removed :file:`prj_release.conf` files from all nRF Audio applications, and the buildprog tool.
  Users must specify themselves which configurations are desired in a release build.
  See :ref:`nrf_audio_app_configuration_select_build` for more information.

nRF Desktop
-----------

* Added:

  * Support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` and ``nrf54ls05dk/nrf54ls05a/cpuapp`` board targets.
  * The ``release_fast_pair`` build type for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.
    The configuration acts as a HID mouse with Fast Pair support.
    It uses MCUboot in direct-xip mode with software-based image signature verification.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Installer (MCUboot Firmware Loader installer)
-----------------------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

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

|no_changes_yet_note|

Bluetooth Fast Pair samples
---------------------------

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

  * Updated the references to the deleted ``CONFIG_CRACEN_LIB_KMU`` Kconfig option with the :kconfig:option:`CONFIG_CRACEN_KMU` replacement.

* :ref:`fast_pair_input_device` sample:

    * Added support for the ``nrf54ls05dk/nrf54ls05a/cpuapp``, ``nrf54ls05dk/nrf54ls05b/cpuapp``, and ``nrf54lc10dk/nrf54lc10a/cpuapp`` board targets.

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

* :ref:`crypto_tls` sample:

  * Updated the TLS version support section after the Mbed TLS v4.1.0 update.

Debug samples
-------------

|no_changes_yet_note|

DFU samples
-----------

* Added the :ref:`encrypted_bootloader` sample that demonstrates how to secure device firmware update (DFU) with image encryption enabled for both the application and MCUboot.

DECT NR+ samples
----------------

|no_changes_yet_note|


Enhanced ShockBurst samples
---------------------------

* Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` and ``nrf54ls05dk/nrf54ls05a/cpuapp`` board targets in all samples.

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

|no_changes_yet_note|

Networking samples
------------------

* Removed support for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target from the following samples:

  * :ref:`mqtt_sample`
  * :ref:`udp_sample`
  * :ref:`net_coap_client_sample`
  * :ref:`https_client`
  * :ref:`http_server`
  * :ref:`download_sample`

* Removed support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target from the following samples:

  * :ref:`mqtt_sample`
  * :ref:`net_coap_client_sample`
  * :ref:`http_server`
  * :ref:`download_sample`
  * :ref:`aws_iot`

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

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

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

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added:

  * The :ref:`ppi_seq` driver for triggering periodic hardware tasks using PPI.
  * The :ref:`ppi_seq_i2c_spi` driver, which is using :ref:`ppi_seq` to perform batches of periodic I2C/SPI transfers without waking up the CPU.
  * The :ref:`vtf_monitoring` for battery voltage, temperature, and frequency monitoring.

SPI drivers
-----------

* SPIM:

  * RTIO based device driver for SPIM has been introduced. This device driver is selected if
    :kconfig:option:`CONFIG_SPI_RTIO` is enabled.

Wi-Fi drivers
-------------

|no_changes_yet_note|

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

* :ref:`bt_fast_pair_readme` library:

  * Removed the nRF52 and nRF53 Series support.

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Gazell libraries
----------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`lib_location` library:

  * Updated the library to always use the chosen ``zephyr,wifi`` node instead of ``ncs,location-wifi`` to find the used Wi-Fi device.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud_pgps` library:

  * Fixed an issue with parsing invalid payloads.

* :ref:`lib_nrf_cloud_agnss` library:

  * Fixed an issue with parsing invalid payloads.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

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

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

* Removed the nRF53 Series-specific information from the :ref:`Google Fast Pair integration <ug_bt_fast_pair_integration>` guide, following the removal of the nRF52 and nRF53 Series support from the Fast Pair samples.

Memfault integration
--------------------

* Removed the ``CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES`` Kconfig option from nRF91x targets.
  Certificate provisioning for nRF91x targets is now handled automatically by the `Memfault firmware SDK`_.
  The option remains available for nRF7002 targets, which do not have automatic certificate provisioning.

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

|no_changes_yet_note|

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

* Added support for the nRF54LC10A SoC.

* The following non-PSA Crypto implementations were deprecated:

  * :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON`
  * :kconfig:option:`CONFIG_BOOT_ECDSA_TINYCRYPT`
  * :kconfig:option:`CONFIG_BOOT_ECDSA_CC310`
  * :kconfig:option:`CONFIG_BOOT_ED25519_TINYCRYPT`
  * :kconfig:option:`CONFIG_BOOT_ED25519_MBEDTLS`

  Use their PSA Crypto counterparts instead.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``684c9e8f32e4373a21098559f748f06915f950c9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 684c9e8f32 ^911b3da139

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^684c9e8f32

The current |NCS| main branch is based on revision ``684c9e8f32`` of Zephyr.

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

|no_changes_yet_note|
