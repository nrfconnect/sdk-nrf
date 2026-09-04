.. _ncs_release_notes_341:

|NCS| v3.4.1 Release Notes
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

This patch release adds the following changes on top of the :ref:`nRF Connect SDK v3.4.0 <ncs_release_notes_340>`:

* Libraries:

  * Added support for the nRF54LC10A SoC in the :ref:`lib_ram_pwrdn` library.

Release tag
***********

The release tag for the |NCS| manifest repository (|ncs_repo|) is **v3.4.1**.
Check the :file:`west.yml` file for the corresponding tags in the project repositories.

To use this release, check out the tag in the manifest repository and run ``west update``.
See :ref:`cloning_the_repositories` and :ref:`gs_updating_repos_examples` for more information.

For information on the included repositories and revisions, see `Repositories and revisions for v3.4.1`_.

IDE and tool support
********************

`nRF Connect extension for Visual Studio Code <nRF Connect for Visual Studio Code_>`_ is the recommended IDE for |NCS| v3.4.1.
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
     - `Changelog <Modem library changelog for v3.4.1_>`_
   * - LwM2M carrier library
     - `Changelog <LwM2M carrier library changelog for v3.4.1_>`_

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.4.1`_ for the list of issues valid for the latest release.

.. _ncs_release_notes_341_changelog:

Changelog
*********

The following sections provide detailed lists of changes by component.

Bootloaders and DFU
===================

* Added support for the application core of the nRF54LS05A SoC to MCUboot and secure boot sysbuild, including the secure boot locking and immutable region handling features aligned with the nRF54LS05B SoC

* Fixed:

  * Sequential updates on the nRF5340 SoC.
    The address-based detection of the update candidate type allows placing the network core update candidate in the same partition as used for as the application update candidate.
    The build system no longer requires dedicated slots for the network core update candidate.
    You can enable software-based downgrade prevention for network core updates.
    MCUboot now erases the secondary slot after the network core is updated.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Matter
------

* Replaced the tables on the :ref:`ug_matter_hw_requirements_ram_flash` and :ref:`ug_matter_hw_requirements_layouts` pages with memory layout charts.

Security
========

* Updated:

  * Oberon PSA Crypto from v2.0.0 to v2.1.0.
    The new version has minor updates in internal APIs, restructures the directory hierarchy, and improves native support for built-in keys.
  * nrf_cc3xx_platform and nrf_cc3xx_mbedcrypto libraries to version v0.9.23.
    Improved PSA driver error reporting and fixed an issue that caused incorrect authentication tag generation in GCM when multiple calls to :c:func:`psa_aead_update_ad` were made.

Mbed TLS
--------

* Updated Mbed TLS to v4.1.1 (from v4.1.0) and TF-PSA-Crypto to v1.1.1 (from v1.1.0).
  For more information, see the upstream `Mbed TLS 4.1.1 release notes`_ and `TF-PSA-Crypto 1.1.1 release notes`_.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Desktop
-----------

* Added:

  * Support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54lc10dk/nrf54lc10a/cpuapp`` board targets.
  * The ``release_fast_pair`` build type for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets.
    The configuration acts as a HID mouse with Fast Pair support.
    It uses MCUboot in direct-xip mode with software-based image signature verification.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* :ref:`bluetooth_central_hids`, :ref:`peripheral_hids_keyboard`, and :ref:`peripheral_hids_mouse` samples:

  * Added support for the ``nrf54ls05dk/nrf54ls05a/cpuapp``, ``nrf54ls05dk/nrf54ls05b/cpuapp``, ``nrf54lc10dk/nrf54lc10a/cpuapp``, and ``nrf54lc10dk/nrf54lc10a/cpuapp/ns`` board targets.

Bluetooth Fast Pair samples
---------------------------

* Added experimental support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` board target in all Bluetooth Fast Pair samples.

* :ref:`fast_pair_locator_tag` sample:

  * Updated:

    * The TX power calibration for the ``nrf54l15tag/nrf54l15/cpuapp`` board target.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-13`` dBm to ``-11`` dBm to meet the Fast Pair distance certification requirements.
    * The TX power calibration for the ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp`` board targets.
      The :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options were changed from ``-15`` dBm to ``-2`` dBm to meet the Fast Pair distance certification requirements.
    * The location of the :kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER_CORRECTION_VAL` and :kconfig:option:`CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL` Kconfig options.
      The options were moved from the sample-wide configuration files to the board configuration files in the :file:`configuration/boards` directory, as the TX power correction is hardware-specific.
      Every supported board target now declares its own calibration.

* :ref:`fast_pair_input_device` sample:

  * Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp``, and ``nrf54lc10dk/nrf54lc10a/cpuapp/ns`` board targets.

Cryptography samples
--------------------

* Added support for the nRF54LC10A SoC (with and without TF-M) in the crypto samples.

DFU samples
-----------

* Added:

  * Support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` and ``nrf54ls05dk/nrf54ls05b/cpuapp`` board targets to the following samples:

    * :ref:`single_slot_sample`
    * :ref:`nrf_smp_svr_sample`
    * :ref:`mcuboot_minimal_configuration`
    * :ref:`mcuboot_with_decompression`
    * :ref:`mcuboot_with_encryption`, with ECIES-P-256 image encryption using :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON` or :kconfig:option:`CONFIG_BOOT_ECDSA_PSA`.

Enhanced ShockBurst samples
---------------------------

* Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp``, ``nrf54lc10dk/nrf54lc10a/cpuapp/ns``, and ``nrf54ls05dk/nrf54ls05a/cpuapp`` board targets in all samples.

Matter samples
--------------

* Added support for the ``nrf54lc10dk/nrf54lc10a/cpuapp`` board target for the following samples:

  * :ref:`matter_template_sample`
  * :ref:`matter_temperature_sensor_sample`

  DFU is not supported on this board target, as the nRF54LC10 DK is not equipped with external flash.
  See :ref:`ug_matter_hw_requirements_external_flash` for more information.

Trusted Firmware-M (TF-M) samples
---------------------------------

* Added support for the nRF54LC10A SoC in the TF-M samples.

Thread samples
--------------

* Added experimental support for the nRF54LC10A SoC to all Thread samples.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Bluetooth libraries and services
--------------------------------

* :ref:`hids_readme` library:

  * Added support for runtime customization of connection parameters for a given HID SCI mode through the newly added :c:func:`bt_hids_sci_mode_conn_rate_param_get` API.

Libraries for networking
------------------------

* :ref:`lib_nrf_provisioning` library:

  * Added a configurable heap allocator for the library's dynamic allocations.
    You can select the allocator using the :kconfig:option:`CONFIG_NRF_PROVISIONING_HEAP_KERNEL` (default) or :kconfig:option:`CONFIG_NRF_PROVISIONING_HEAP_SYSTEM` Kconfig option.

Other libraries
---------------

* :ref:`lib_ram_pwrdn` library:

  * Added:

    * Support for the nRF54LC10A SoC and the nRF54LS05A SoC
    * ECIES-P-256 encrypted image support when using :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON`.
      It uses the ``ocrypto`` software backend instead of TinyCrypt.
    * ECIES-P-256 encrypted image support for the :kconfig:option:`CONFIG_BOOT_ECDSA_PSA` path by auto-selecting the required PSA algorithms.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added:

  * Support for the nRF54LC10A SoC.
  * Support for compiling multiple image verification keys into MCUboot.
    The :kconfig:option:`CONFIG_BOOT_SIGNATURE_KEY_FILE` Kconfig option accepts a comma-separated list of PEM files.
    Only public key material is embedded in the bootloader image.
    This enables a production or development signing custody model in which, for example, an updatable development bootloader can boot images signed with either key, while a production bootloader embeds only the production verification key.
    MCUboot ``imgtool`` adds the ``keyinfo`` subcommand and the ``--name-suffix`` option for ``getpub`` and ``getpubhash`` to support multiple keys embedded in the bootloader image.

* Updated:

  * The :kconfig:option:`CONFIG_BOOT_ECDSA_NRF_OBERON` Kconfig option.
    This option has been reinstated and is no longer deprecated.
    It has also been configured as the default ECDSA P-256 implementation for the nRF54LS05A and nRF54LS05B SoCs.

  * MCUboot now feeds the watchdog more frequently during time-consuming procedures to prevent watchdog timeouts during long-running operations, including the following:

    * Full slot erase procedures
    * The move-sectors-up loop and sectors-swap loop of the swap-move algorithm
    * The hash calculation loop during image hash calculation

* Fixed an issue where UICR was not provisioned with monotonic counter structures when :kconfig:option:`SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION` was enabled, MCUboot was the only bootloader, and Partition Manager was disabled.

Zephyr
======

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``684c9e8f32e4373a21098559f748f06915f950c9``.

For a complete list of |NCS| specific commits and cherry-picked commits since v3.4.0, run the following command:

.. code-block:: none

   git log --oneline manifest-rev ^ncs-v3.4.1

Additions specific to |NCS|
---------------------------

* Added the `release.yaml file`_ with device classification support overview.

Documentation
=============

* Added the :ref:`kconfig:kconfig_diff` page, displaying differences between available Kconfig options across releases.
  To generate the new documentation page, set the ``KCONFIGDIFF`` CMake option to ``ON``.

* Updated the :ref:`ug_bootloader_nrf54l_memory_protection` documentation page to explain the memory protection features of the bootloader on the nRF54L Series.
