.. _ug_tfm_supported_services:

Supported services and limitations in the |NCS|
###############################################

.. contents::
   :local:
   :depth: 2

TF-M is the reference implementation of `Platform Security Architecture (PSA)`_.

It provides a highly configurable set of software components to create a Trusted Execution Environment.
This is achieved by a set of secure run time services such as Secure Storage, Cryptography, Audit Logs, and Attestation.
Additionally, secure boot through MCUboot in TF-M ensures integrity of runtime software and supports firmware upgrade.

Supported TF-M services and functionalities
*******************************************

Currently, only the :ref:`Minimal TF-M configuration <tfm_minimal_build>` is :ref:`supported <software_maturity_security_features_tfm>` in the |NCS|.
Configuring TF-M to use features beyond the minimal configuration (with so called :ref:`tfm_configurable_build`) is :ref:`experimental <software_maturity_security_features_tfm>`.

.. note::
   Only the TF-M :ref:`minimal build <tfm_minimal_build>` implementation in the |NCS| is currently :ref:`supported <software_maturity_security_features_tfm>`.
   Support for TF-M with minimal version *disabled* in the |NCS| is :ref:`experimental <software_maturity_security_features_tfm>`.

For official documentation, see the `TF-M documentation`_.

The TF-M implementation in |NCS| is demonstrated in the following samples:

* All :ref:`tfm_samples` in this SDK
* All :ref:`cryptography samples <crypto_samples>` in this SDK
* A series of :zephyr:code-sample-category:`tfm_integration` samples available in Zephyr

In addition, the TF-M implementation is used in all samples and applications in this SDK that support the ``*/ns`` :ref:`variant <app_boards_names>` of the boards, due to :ref:`Cortex-M Security Extensions (CMSE) <app_boards_spe_nspe>` support.

Minimal TF-M configuration
==========================

.. minimal_build_overview_start

The default configuration of TF-M has all supported features enabled, which results in a significant memory footprint.
For this reason, the |NCS| provides a minimal version of the TF-M secure application, which shows how to configure a reduced version of TF-M.

The secure services supported by this minimal version allow for:

* Generating random numbers using the CryptoCell peripheral.
* Using the :ref:`platform services <ug_tfm_services_platform>`.
* Reading secure memory from the non-secure application (strictly restricted to a list of allowed addresses).
  Depending on the device, this lets you read metadata in the bootloader, verify FICR or UICR values, or access a peripheral that is secure-only.
* Rebooting from the non-secure side.

.. minimal_build_overview_end

See :ref:`ug_tfm_building` for more information on building the TF-M secure application with the minimal build.

Configurable TF-M build
=======================

.. configurable_build_overview_start

The configurable build is the full TF-M implementation that lets you configure all of its features.
It does not come with the constraints of the minimal build.

.. configurable_build_overview_end

When you use the configurable build, you are free to enable or disable any of the features in the TF-M configuration file.
See :ref:`tfm_configurable_build` for more information on building the TF-M secure application with the configurable build.

Limitations
***********

The following limitations apply to TF-M and its usage in the |NCS|:

* Firmware Update service is not supported.
* The following crypto modules or ciphers are not supported:

  * AES output feedback (AES-OFB) mode.
  * AES cipher feedback (AES-CFB) mode.

* Isolation level 3 is not supported.
* In Isolation level 2 (and 3), the number of peripherals configured as secure in Application Root of Trust (ARoT) is limited by the number of available MPU regions.
* Nordic Semiconductor devices only support the GCC toolchain for building TF-M.
