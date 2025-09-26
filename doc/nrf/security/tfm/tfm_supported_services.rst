.. _ug_tfm_supported_services:

TF-M support and limitations in the |NCS|
#########################################

.. contents::
   :local:
   :depth: 2

This page lists the supported features and limitations of Trusted Firmware-M (TF-M) in the |NCS|.

.. _ug_tfm_supported_services_profiles:

Supported TF-M profiles
***********************

The `Trusted Firmware-M <TF-M documentation_>`_ provides several :ref:`tf-m_profiles` for different use cases and levels of TF-M configuration.

The |NCS| *does not* follow this classification.
Instead, it provides two main configurations for TF-M: minimal and configurable.

.. list-table:: Supported TF-M configurations in the |NCS|
   :header-rows: 1
   :widths: auto

   * - Configuration profile
     - Support status in the |NCS|
     - Description
   * - Minimal
     - :ref:`Supported <software_maturity>`
     - Bare-minimal configuration to allow the device to boot securely. Contains only random number generator and :ref:`allows for a limited number of operations <ug_tfm_supported_services_profiles_minimal>`.
   * - Configurable (full)
     - :ref:`Experimental <software_maturity>`
     - Full configuration that allows you to enable or disable different partitions in the TF-M configuration file. See :ref:`tfm_configurable_build` for more information.
   * - :ref:`TF-M Small <tf-m_profiles>`
     - Not supported
     - Basic lightweight TF-M framework with basic Secure Services to keep smallest memory footprint.
   * - :ref:`TF-M ARoT-less <tf-m_profiles>`
     - Not supported
     - Reference implementation to align with security requirements defined in PSA Certified ARoT-less Level 2 protection profile.
   * - :ref:`TF-M Medium <tf-m_profiles>`
     - Not supported
     - Profile Medium aims to securely connect devices to Cloud services with asymmetric cipher support.
   * - :ref:`TF-M Large <tf-m_profiles>`
     - Not supported
     - Profile Large protects less resource-constrained Arm Cortex-M devices.

.. _ug_tfm_supported_services_profiles_hw_support:

Hardware support matrix for TF-M profiles
=========================================

The following table lists hardware support and software maturity levels for the minimal and configurable TF-M profiles in the |NCS|.

.. include:: ../../releases_and_maturity/software_maturity.rst
   :start-after: tfm_ncs_profiles_support_table_start
   :end-before: tfm_ncs_profiles_support_table_end

For the definitions of the maturity levels, see :ref:`software_maturity`.

.. _ug_tfm_supported_services_profiles_minimal:

Minimal TF-M configuration
==========================

.. minimal_build_overview_start

The default configuration of TF-M has all supported features enabled, which results in a significant memory footprint.
For this reason, the |NCS| provides a minimal version of the TF-M secure application, which shows how to configure a reduced version of TF-M.

The secure services supported by the minimal version allow for:

* Generating random numbers using the random generator available in the device (see ``psa_generate_random()`` in `crypto.h`_).
* Using the :ref:`platform services <ug_tfm_services_platform>` with `tfm_platform_api.h`_ from the non-secure side (except the ``tfm_platform_nv_counter_*`` functions).
  This includes sending platform-specific service requests using `tfm_ioctl_core_api.h`_ and `tfm_ioctl_api.h`_.
* Reading secure memory from the non-secure application (strictly restricted to a list of allowed addresses).
  Depending on the device, this lets you read metadata in the bootloader, verify FICR or UICR values, or access a peripheral that is secure-only.
* Rebooting from the non-secure side.

.. minimal_build_overview_end

See :ref:`ug_tfm_building` for more information on building the TF-M secure application with the minimal build.

.. _ug_tfm_supported_services_profiles_configurable:

Configurable TF-M build
=======================

.. configurable_build_overview_start

The configurable build is the full TF-M implementation that lets you configure all of its features.
It does not have the constraints of the minimal build.

.. configurable_build_overview_end

When you use the configurable build, you can enable or disable any of the features in the application configuration file.
See :ref:`tfm_configurable_build` for more information on building the TF-M secure application with the configurable build.

.. _ug_tfm_supported_services_tfm_services:

Supported TF-M services
***********************

The following TF-M services are supported in the |NCS|:

.. list-table:: TF-M services supported in the nRF Connect SDK
   :header-rows: 1

   * - TF-M service
     - Supported API version in the |NCS|
     - Description
     - Configuration steps
   * - Platform service
     - n/a
     - Mandatory implementation of the :ref:`ug_tfm_architecture_rot_services_platform`. Provides platform-specific services to other TF-M partitions and to non-secure applications.
     - :kconfig:option:`CONFIG_TFM_PARTITION_PLATFORM` (:ref:`details<ug_tfm_services_platform>`)
   * - Crypto
     - `PSA Certified Crypto API`_ v1.0.0 (v1.2.0 when you build with :ref:`nrf_security`, but without TF-M)
     - Provides cryptographic operations like encryption and decryption, hashing, key management, and random number generation.
     - :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO` (:ref:`details<tfm_partition_crypto>`)
   * - Internal Trusted Storage (ITS)
     - `PSA Certified Secure Storage API 1.0`_
     - Provides a secure storage mechanism for sensitive data in internal flash, with optional encryption support.
     - :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` (:ref:`details<ug_tfm_services_its>`)
   * - Protected Storage (PS)
     - `PSA Certified Secure Storage API 1.0`_
     - Provides secure storage with encryption, integrity protection, and rollback protection for non-secure applications.
     - :kconfig:option:`CONFIG_TFM_PARTITION_PROTECTED_STORAGE` (:ref:`details<tfm_partition_ps>`)
   * - Initial Attestation
     - `PSA Certified Attestation API 1.0`_
     - Provides mechanisms to prove the device identity and software state to remote entities.
     - :kconfig:option:`CONFIG_TFM_PARTITION_INITIAL_ATTESTATION` (:ref:`details<ug_tfm_services_initial_attestation>`)
   * - Firmware Update
     - n/a
     - | The nRF Connect SDK does not implement the PSA Firmware Update API.
       | Instead, other options are available for the immutable bootloader and the upgradable bootloader.
       | See :ref:`app_bootloaders` for more information on available bootloaders.
     - n/a

.. _ug_tfm_supported_services_isolation:

Isolation Level support
***********************

TF-M provides different :ref:`isolation levels <ug_tfm_architecture_isolation_lvls>` between security domains.
The following table lists the isolation level support in Nordic Semiconductor's implementation of TF-M:

.. list-table:: TF-M isolation levels in the nRF Connect SDK
   :header-rows: 1
   :widths: 15 20 65

   * - Isolation level
     - Support status in the |NCS|
     - Description
   * - Level 1
     - Supported
     - SPE isolation - Secure Processing Environment is protected from access by Non-Secure application firmware and hardware.
   * - Level 2
     - Supported with limitations
     - | Platform RoT isolation - In addition to Level 1, Platform RoT is protected from Application RoT (App RoT).
       |
       | *Limitation in the nRF Connect SDK*: The number of peripherals configured as secure in App RoT is limited by available MPU regions.
   * - Level 3
     - Not supported
     - Maximum firmware isolation, with each Secure Partition sandboxed and only permitted to access its own resources.

.. _ug_tfm_ncs_zephyr_differences:

Differences between TF-M in the |NCS| and upstream Zephyr
*********************************************************

The TF-M implementation in the |NCS| differs from the implementation in :term:`upstream <Upstream repository>` Zephyr.
These are the main differences:

* Board support:

  * The :ref:`ug_tfm_supported_services_tfm_services` implemented in the |NCS| are not recommended for use with boards in upstream Zephyr because these boards have limited TF-M support:

    * Upstream Zephyr only fully supports TF-M for nRF91 Series DKs and nRF5340 DK.
    * Upstream Zephyr's TF-M integration for nRF54L15 is limited to Zephyr's :zephyr:code-sample:`tfm_ipc` and the ``config_build`` samples (both part of :zephyr:code-sample-category:`tfm_integration`).
    * In upstream Zephyr, TF-M integration for boards based on nRF5340 (other than the nRF5340 DK) is not supported.

* TF-M configuration:

  * The |NCS| allows for more customization of TF-M features:

    * The |NCS| provides a :ref:`minimal build <tfm_minimal_build>` option that is enabled by default for nRF53 and nRF91 Series devices.
    * The |NCS| offers a :ref:`configurable build <tfm_configurable_build>` that allows for more fine-grained control over TF-M features.

  * Upstream Zephyr uses predefined, basic TF-M profiles that might result in larger memory footprint.

* Logging:

  * The |NCS| provides enhanced :ref:`logging capabilities <ug_tfm_logging>` with options to configure UART instances.
    For nRF5340 and nRF91 Series devices, the |NCS| allows TF-M and the application to share the same UART for logging.

* Security features:

  * The |NCS| integrates TF-M with :ref:`nrf_security` to enable hardware acceleration.
  * The |NCS| provides additional security hardening measures specific to Nordic Semiconductor devices.

* Bootloader integration:

  * The |NCS| uses its own version of MCUboot (`sdk-mcuboot`_) that is specifically integrated with TF-M.
  * Upstream Zephyr uses the standard MCUboot implementation.

* Samples:

  * The |NCS| provides its own set of :ref:`TF-M samples <tfm_samples>`.
  * :zephyr:code-sample-category:`tfm_integration` samples from upstream Zephyr are not compatible with the :ref:`ug_tfm_supported_services_tfm_services` implemented in the |NCS| when built from upstream Zephyr.

.. _ug_tfm_supported_services_limitations:

Limitations of TF-M in the |NCS|
********************************

The following sections summarize the limitations of TF-M in the |NCS|, organized by category.

Core features
=============

* TF-M profiles are not supported.
* Isolation level 3 is not supported.
* TF-M's second-stage bootloader (BL2) is not supported.
  Instead, the |NCS| uses its own version of MCUboot (`sdk-mcuboot`_), which is supported with TF-M and provides.
  See :ref:`ug_bootloader_mcuboot_nsib` for more information.
* For Isolation level 2, the number of peripherals that are configurable as secure in the Application Root of Trust (ARoT) is limited by available MPU regions.

Security services
=================

* |encrypted_its_not_supported_on_nrf54lm20|
* Firmware Update service is not supported.
* Firmware verification is not supported.
* Firmware encryption is not supported.
* Protected off-chip data storage and retrieval are not supported.
* Audit logging is not supported.

Cryptography
============

The following crypto modules and ciphers are not supported:

* AES output feedback (AES-OFB) mode.
* AES cipher feedback (AES-CFB) mode.

Development and integration
===========================

* GCC is the only supported toolchain for building TF-M on Nordic Semiconductor devices.
