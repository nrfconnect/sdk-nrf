.. _ug_tfm_supported_services:

Supported services and limitations in the |NCS|
###############################################

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

Hardware support matrix for TF-M configurations
  Expand the following field to list the software maturity levels for the TF-M configurations in the |NCS| for each device.

  .. toggle::

     .. include:: ../../releases_and_maturity/software_maturity.rst
        :start-after: tfm_ncs_profiles_support_table_start
        :end-before: tfm_ncs_profiles_support_table_end

.. _ug_tfm_supported_services_profiles_minimal:

Minimal TF-M configuration
==========================

.. minimal_build_overview_start

The default configuration of TF-M has all supported features enabled, which results in a significant memory footprint.
For this reason, the |NCS| provides a minimal version of the TF-M secure application, which shows how to configure a reduced version of TF-M.

The secure services supported by the minimal version allow for:

* Generating random numbers using the random generator available in the device (see `crypto.h`_).
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

.. _ug_tfm_supported_services_limitations:

Limitations
***********

The following list summarizes the limitations of TF-M in the |NCS|:

* TF-M profiles are not supported.
* From among the TF-M Services, Firmware Update service is not supported.
* The following crypto modules or ciphers are not supported:

  * AES output feedback (AES-OFB) mode.
  * AES cipher feedback (AES-CFB) mode.

* Isolation level 3 is not supported.
* In Isolation level 2 (and 3), the number of peripherals configured as secure in the Application Root of Trust (ARoT) is limited by the number of available MPU regions.
* Nordic Semiconductor devices only support the GCC toolchain for building TF-M.
* :zephyr:code-sample-category:`tfm_integration` samples available in Zephyr are not compatible with the :ref:`ug_tfm_supported_services_tfm_services` implemented in the |NCS| when these samples are built from the upstream Zephyr.
