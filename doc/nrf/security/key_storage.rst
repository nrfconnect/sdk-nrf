.. _key_storage:

Key storage options
###################

.. contents::
   :local:
   :depth: 2

Cryptographic keys are critical security assets that require secure storage and management.

This page provides an overview of the available key storage alternatives in the |NCS|.

For information about storing general data, see :ref:`data_storage`.

General key storage recommendations
***********************************

When implementing key storage in your application:

* Use PSA Crypto API for most cryptographic operations.
* Generate keys on-device when possible rather than importing them.
* Leverage hardware features like HUK and KMU when available.
* Enable APPROTECT in production to prevent debug access.
* Use appropriate key lifetimes (volatile or persistent) based on your needs.
* Implement proper key provisioning workflows for production devices.

.. _key_storage_alternatives:

Key storage alternatives
************************

Each key storage alternative offers different security features and use cases.

.. _key_storage_psa_crypto:

PSA Crypto API
==============

The :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` is the recommended approach for cryptographic key management in the |NCS|.
It provides a standardized, secure interface for key operations that abstracts the underlying hardware and software implementations.

This option offers the following features:

* Secure key handling - Keys are managed securely without exposing key material.
* Hardware acceleration - Utilizes available cryptographic hardware when possible.
* Standardized interface - Follows PSA Certified standards for interoperability.
* Key isolation - Keys can be isolated in the Secure Processing Environment (SPE).

Key generation and import
-------------------------

The PSA Crypto API supports two methods for obtaining keys:

**Key generation**
  Use ``psa_generate_key()`` to create random keys.
  Generated keys can be of one of the following lifetime types:

  * Volatile - Stored in the SPE's RAM, lost on reboot.
  * Persistent - Stored in PSA Internal Trusted Storage, survive reboots.

**Key import**
  Use ``psa_import_key()`` to import existing key material.
  After import, the key is referenced by ID rather than direct access to key material.

.. note::
   Generated keys are more secure than imported keys since they are never exposed to the Non-Secure Processing Environment (NSPE).
   While the key is being imported from the NSPE, it temporarily resides in an untrusted environment.
   Generally, it is safer to generate new keys without exposing them to the NSPE, if possible.

For more information, see `Key management functions`_ in the PSA Crypto API documentation.

Configuration
-------------

PSA Crypto API requires either Oberon PSA Crypto or Trusted Firmware-M (TF-M) to be enabled for full security features.
For more information, see :ref:`psa_crypto_support` or :ref:`ug_tfm_building`.

Usage example
-------------

For usage examples, see :ref:`crypto_samples`, and specifically the :ref:`Persistent key usage <crypto_persistent_key>` sample.

.. _key_storage_huk:

Hardware Unique Key (HUK)
==========================

:ref:`Hardware Unique Keys (HUKs) <lib_hw_unique_key>` are device-specific keys that you can use for key derivation.
HUKs provide a foundation for creating device-unique cryptographic keys without storing key material in software.

This option offers the following features:

* Device uniqueness - Each device has a unique set of HUK values.
* Hardware protected - Keys are stored in hardware-protected memory.
* Key derivation - Used to derive other keys rather than direct cryptographic operations.
* No key material exposure - Application works with derived keys, not the HUK itself.

Types of HUKs
-------------

The |NCS| supports the following types of HUKs:

.. list-table:: Hardware Unique Key (HUK) Types
   :widths: auto
   :header-rows: 1

   * - HUK type
     - Purpose
     - Storage location
     - Additional notes
   * - Master Key Encryption Key (MKEK)
     - Deriving Key Encryption Keys (KEKs) for encrypting :ref:`Internal Trusted Storage (ITS) <ug_psa_certified_api_overview_secstorage>`
     - :ref:`key_storage_kmu`
     - Recommended for nRF91 Series devices and nRF5340
   * - Master Key for External Storage (MEXT)
     - Deriving keys for encrypting data in external storage (flash) or :ref:`Protected Storage <ug_psa_certified_api_overview_secstorage>`
     - :ref:`key_storage_kmu`
     - Recommended for nRF91 Series devices and nRF5340
   * - CRACEN Isolated Key Generator (IKG)
     - Deriving special hardware keys for CRACEN operations
     - :ref:`key_storage_kmu`
     - | - Regenerated on each CRACEN power cycle from the IKG seed
       | - Supported on nRF54L Series devices; see :ref:`ug_nrf54l_cryptography`
   * - Device Root Key (KDR)
     - Deriving general-purpose keys
     - Non-volatile memory locked by `ACL <nRF5240 Access Control Lists_>`_
     - | - Should be written to CryptoCell by the bootloader
       | - Supported on and recommended for nRF52840

Device support
--------------

Different nRF devices support different types of HUKs.
See :ref:`lib_hw_unique_key` for device-specific support information.

Usage
-----

HUKs should not be used directly.
Instead, derive keys from HUKs using device-specific labels:

1. Provide a label to identify the derived key purpose.
2. Use key derivation functions with the HUK and label to obtain the derived key.

The same combination of label and HUK always produces the same derived key.

Keys can be derived from each HUK, and each HUK can have multiple different labels.
Therefore, each HUK can have multiple different derived keys.

Configuration
-------------

To use the HUK storage options for keys, set the :kconfig:option:`CONFIG_HW_UNIQUE_KEY` Kconfig option.
For more configuration options, see the :ref:`lib_hw_unique_key` library.

Usage example
-------------

If you are using TF-M and it needs the HUK, you must ensure that the HUK is generated.
See the :ref:`TF-M Provisioning image <provisioning_image>` sample for an example.

For an example on how to derive keys from HUKs, see the :ref:`Hardware unique key sample <hw_unique_key_usage>`.

.. _key_storage_iak:

Initial Attestation Key (IAK) / Identity Key
=============================================

The :ref:`Initial Attestation Key (IAK) <lib_identity_key>`, also known as Identity Key, is required by PSA Certified Security Framework for :ref:`ug_psa_certified_api_overview_attestation`.
This key is used to sign attestation tokens that prove the device's identity and integrity state.

This option offers the following features:

* Device identity - Provides cryptographic proof of device identity.
* Attestation support - Required for PSA Attestation implementation.
* Hardware protection - Encrypted using MKEK and stored in secure storage.

When using TF-M, the :ref:`Initial Attestation service <ug_tfm_services_initial_attestation>` uses the IAK to sign attestation tokens that prove the device's identity and integrity state.

Configuration
-------------

When using TF-M, you must generate and provision the IAK before running applications.
The configuration is device-specific.

.. tabs::

   .. group-tab:: nRF5340 and nRF91 Series devices

      Complete the following steps:

      1. Generate the MKEK first (required for IAK encryption).
         See the :ref:`TF-M Provisioning image sample <provisioning_image>` for an example.
      #. Generate the IAK using the :ref:`lib_identity_key` library.
         See the :ref:`Identity key generation sample <identity_key_generate>` for an example.

   .. group-tab:: nRF54L15

      For nRF54L15, the IAK is derived from the IKG seed.

      See the :ref:`crypto_persistent_key` sample for an example of the IKG seed generation and write.
      The IAK is automatically derived from the IKG seed and stored in the KMU.

The IAK is automatically encrypted and stored securely.

Usage example
-------------

See the :ref:`Identity key usage sample <identity_key_usage>` for an example.

.. _key_storage_kmu:

Key Management Unit (KMU)
=========================

The Key Management Unit (KMU) is a hardware peripheral for secure key storage available on select nRF devices.
It provides hardware-level protection for cryptographic keys.

This option offers the following features:

* Hardware security - Keys are stored in dedicated hardware.
* CPU isolation - CryptoCell and CRACEN can use KMU keys without CPU access to key material.
* Metadata only - CPU only knows key metadata (slot numbers), not key material.
* Multiple key slots - Supports storing multiple keys simultaneously.

Device support
--------------

KMU is available on the following devices:

* `nRF5340 <nRF5340 Key management unit_>`_
* nRF91 Series devices, such as `nRF9160 <nRF9160 Key management unit_>`_ or `nRF9151 <nRF9151 Key management unit_>`_
* nRF54L Series devices, such as `nRF54L15 <nRF54L15 Key management unit_>`_

  .. note::
     nRF54L devices are equipped with a KMU that works with the CRACEN cryptographic driver.
     For more information, see :ref:`ug_nrf54l_developing_basics_kmu`.

Key storage
-----------

The KMU stores various types of keys:

* HUK keys (MKEK, MEXT)
* Initial Attestation Keys
* User-defined keys

.. note::
   Consider using the PSA Crypto API or HUK system before working directly with KMU APIs, as these higher-level interfaces often provide the functionality you need with better security practices.

Configuration
-------------

Configuring the KMU varies depending on the device.
See the device-specific documentation for more information.

For the nRF54L Series devices, you can use nRF Util for provisioning keys to KMU.
See `Provisioning keys for hardware KMU`_ for more information.

.. _key_storage_otp:

OTP (One Time Programmable)
===========================

One Time Programmable (OTP) memory provides permanent, write-once storage for keys and other critical data.
OTP memory is part of the User Information Configuration Registers (UICR).

This option offers the following features:

* Write-once - Can only be programmed once, providing tamper evidence.
* Non-volatile - Retains data without power.
* No read protection - Data can be read by any code with appropriate access.

Use cases
---------

OTP is suitable for storing the following data:

* User IDs or device identifiers
* Feature activation keys
* Root certificates or trust anchors
* Any data that should never change during device lifetime

.. note::
   OTP provides no read protection.
   Do not store sensitive keys that need to remain confidential in OTP memory.

Device support
--------------

Not all nRF devices have OTP peripherals.
Check device-specific documentation for more information.
For example, `nRF5340 OTP <nRF5340 OTP_>`_ and `nRF9160 OTP <nRF9160 OTP_>`_.

Usage
-----

Use the :ref:`NVMC data storage option <data_storage_nvmc>` for OTP operations, and specifically the ``nrfx_nvmc_otp_halfword_read()`` function for reading OTP data.

.. _key_storage_wifi_credentials:

Wi-Fi Credentials
=================

For Wi-Fi applications, the |NCS| provides the :ref:`Wi-Fi credentials library <lib_wifi_credentials>` for loading and storing Wi-Fi® network credentials.
This library is leveraging either Zephyr's :ref:`Settings subsystem <zephyr:settings_api>` or :ref:`PSA's Internal Trusted Storage (ITS) <ug_psa_certified_api_overview_secstorage>` to store credentials.

This option offers the following features:

* PSA Protected Storage backend - Benefits from encryption and integrity protection.
* Easy-to-use API - Simplified interface for Wi-Fi credential management.
* Multiple networks - Support for storing multiple Wi-Fi network credentials.

Configuration
-------------

To use the Wi-Fi credentials library, enable the :kconfig:option:`CONFIG_WIFI_CREDENTIALS` Kconfig option.
For more configuration options, see the :ref:`lib_wifi_credentials` page.

Usage example
-------------

See the :ref:`Wi-Fi Bluetooth LE based provision sample <wifi_provisioning>` for an example.

Additional security considerations
**********************************

.. _key_storage_flash_protection:

Hardware Flash Write Protection
===============================

The |NCS| provides the :ref:`fprotect driver <fprotect_readme>` for hardware-level flash protection.
This driver uses underlying hardware peripherals (BPROT, ACL, or SPU) to protect flash regions from unauthorized writes or reads.

Use fprotect to protect stored keys from modification, secure bootloader regions, and implement additional layers of security.

Configuration
-------------

To use the fprotect driver, enable the :kconfig:option:`CONFIG_FPROTECT` Kconfig option.
For more information, see the :ref:`fprotect_readme` page.

.. _key_storage_approtect:

Access Port Protection (AP-Protect)
===================================

The :ref:`access port protection mechanism <app_approtect>` (AP-Protect) prevents the debugger from accessing device resources, including keys and sensitive data.
When enabled, AP-Protect blocks read and write access to CPU registers and memory-mapped addresses.

Nordic Semiconductor recommends enabling AP-Protect in production devices to prevent extraction of keys and sensitive data through debug interfaces.

Configuration
-------------

AP-Protect configuration is device-specific.
See :ref:`app_approtect_implementation_overview` for detailed information.
