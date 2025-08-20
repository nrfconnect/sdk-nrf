.. _key_storage:

Key storage in the |NCS|
########################

.. contents::
   :local:
   :depth: 2

Cryptographic keys are critical security assets that require secure storage and management.

This page provides an overview of the available key storage options in the |NCS|.

For information about storing general data, see :ref:`data_storage`.

General key storage recommendations
***********************************

When implementing key storage in your application:

* Use the PSA Crypto API for all cryptographic operations.
* Generate keys on-device when possible rather than importing them.
* Leverage hardware features like Hardware Unique Key (HUK) and Key Management Unit (KMU) when available.
* Enable access port protection in production to prevent debug access.
* Use persistent keys through the PSA Crypto API when you need to store key material.
* Implement proper key provisioning workflows for production devices.

.. _key_storage_options:

Key storage options
*******************

Each option offers different security features and use cases.

.. _key_storage_psa_crypto:

PSA Crypto API
==============

The :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` is the recommended approach for cryptographic key management in the |NCS|.
It provides a standardized, secure interface for key operations that abstracts the underlying hardware and software implementations.

This option offers the following features:

* Secure key handling - Keys are managed securely without exposing key material.
* Hardware acceleration - PSA Crypto API uses available cryptographic hardware when possible.
* Standardized interface - PSA Crypto API follows PSA Certified standards for interoperability.
* Key isolation - Keys can be isolated in the Secure Processing Environment (SPE) when using :ref:`Trusted Firmware-M <ug_tfm>`.

Key generation and import
-------------------------

Keys can have one of the following lifetimes:

* Volatile - Stored in RAM, lost on reset.
* Persistent - Stored in PSA Internal Trusted Storage, retained during reboots.

The PSA Crypto API supports two methods for using keys:

* Key generation - Use :c:func:`psa_generate_key` to create random keys.
* Key import - Use :c:func:`psa_import_key` to import existing key material.
  After import, the key is referenced by ID rather than direct access to key material.

.. note::
   Generated keys are more secure than imported keys.
   This is because when generating keys the key material is never exposed to the application directly.

For more information, see `Key management functions`_ in the PSA Crypto API documentation.

Configuration
-------------

For information about how to configure the PSA Crypto API in the |NCS|, see :ref:`psa_crypto_support`.

Usage example
-------------

For usage examples, see :ref:`crypto_samples`, specifically the :ref:`Persistent key usage <crypto_persistent_key>` sample.

.. _key_storage_huk:

Hardware Unique Key (HUK)
=========================

Hardware Unique Keys (HUKs) are device-specific keys that you can use for key derivation.
HUKs provide a foundation for creating device-unique cryptographic keys without storing key material in software.

This option offers the following features:

* Device uniqueness - Each device has a unique set of HUK values.
* Hardware protection - Keys are stored in hardware-protected memory.
* Key derivation - HUKs are used to derive other keys rather than directly perform cryptographic operations.
  The derived keys are then passed to the application.

See the :ref:`lib_hw_unique_key` library for the list of supported HUKs in the |NCS|.

Device support
--------------

Different Nordic Semiconductor devices support different types of HUKs.
See :ref:`lib_hw_unique_key` for device-specific support information.

Usage
-----

You cannot use HUKs directly.
You can however use the :ref:`lib_hw_unique_key` library to derive keys from HUKs using known labels.

You can use HUKs indirectly through PSA Secure Storage without TF-M, for example through the :ref:`trusted_storage_readme` library.
In such case, the HUK library will be used to derive keys that are then used to encrypt storage entries.
This is done to prevent the application from accessing the HUK directly.

Configuration
-------------

To use the HUK storage options for keys, enable the :kconfig:option:`CONFIG_HW_UNIQUE_KEY` Kconfig option.
For more configuration options, see the :ref:`lib_hw_unique_key` library.

Usage example
-------------

If you use the HUK library, ensure that the HUKs are written.
For an example of how to write HUKs, see the :ref:`TF-M Provisioning image <provisioning_image>` or :ref:`Crypto persistent key usage <crypto_persistent_key>` samples.

For an example of how to derive keys from HUKs, see the :ref:`Hardware unique key sample <hw_unique_key_usage>`.

.. _key_storage_kmu:

Key Management Unit (KMU)
=========================

The Key Management Unit (KMU) is a hardware peripheral for secure key storage available on select nRF devices.
It provides hardware-level protection for cryptographic keys.

This option offers the following features:

* Hardware security - Keys are stored in dedicated hardware.
* CPU isolation - CryptoCell and CRACEN can use KMU keys without CPU access to key material.
* Metadata only - CPU only knows key metadata, not key material.
* Multiple key slots - Supports storing multiple keys separately.

Device support
--------------

KMU is available on the following devices:

* `nRF5340 <nRF5340 Key management unit_>`_
* nRF91 Series devices, such as `nRF9160 <nRF9160 Key management unit_>`_ or `nRF9151 <nRF9151 Key management unit_>`_
* nRF54L Series devices, such as `nRF54L15 <nRF54L15 Key management unit_>`_

  .. note::
     nRF54L devices are equipped with a KMU that works with the CRACEN peripheral.
     For more information, see :ref:`ug_nrf54l_developing_basics_kmu`.

Key storage
-----------

The KMU stores various types of keys (when they are not just derived):

* Hardware Unique Keys (MKEK, MEXT)
* Initial Attestation Key
* User-defined keys

.. note::
   Consider using the PSA Crypto API or HUK library before working directly with KMU APIs, as these higher-level interfaces often provide the functionality you need with better security practices.

Configuration
-------------

Configuring the KMU varies depending on the device.
See the device-specific documentation for more information.

For the nRF54L Series devices, you can use nRF Util for provisioning keys to KMU.
See `Provisioning cryptographic keys`_ in the nRF Util documentation for more information.

.. _key_storage_otp:

One-Time Programmable (OTP)
===========================

.. caution::
   OTP provides no read protection.
   Use it only for non-sensitive data.
   Do not use it to store sensitive keys that need to remain confidential.

One-Time Programmable (OTP) memory provides permanent, write-once storage for keys and other critical data.
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
This library is leveraging either Zephyr's :ref:`Settings subsystem <zephyr:settings_api>` or :ref:`PSA Internal Trusted Storage (ITS) <ug_psa_certified_api_overview_secstorage>` to store credentials.

This option offers the following features:

* PSA Internal Trusted Storage backend - Benefits from encryption and integrity protection.
* Easy-to-use API - Simplified interface for Wi-Fi credential management.
* Multiple networks - Support for storing multiple Wi-Fi network credentials.

Configuration
-------------

To use the Wi-Fi credentials library, enable the :kconfig:option:`CONFIG_WIFI_CREDENTIALS` Kconfig option.
For more configuration options, see the :ref:`lib_wifi_credentials` page.

Usage example
-------------

See the :ref:`Wi-Fi Bluetooth LE based provision sample <wifi_provisioning>` for an example.

Additional considerations
*************************

The |NCS| includes several additional features that are related to cryptographic key storage.

.. _key_storage_iak:

Initial Attestation Key (IAK) / Identity Key
============================================

The :ref:`Initial Attestation Key (IAK) <lib_identity_key>`, also known as Identity Key, is required by PSA Certified Security Framework for :ref:`ug_psa_certified_api_overview_attestation`.
This key is used to sign attestation tokens that prove the device's identity and integrity state.

This option offers the following features:

* Device identity - Provides cryptographic proof of device identity.
* Attestation support - Meets the requirement for PSA Attestation implementation.
* Hardware protection - Ensures access is limited to hardware.
  For devices other than the nRF54L Series, the key is encrypted using MKEK and stored in :ref:`key_storage_kmu`.
  For nRF54L Series devices, the key is derived each time.

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

      For nRF54L15, the IAK is derived from the :ref:`CRACEN Isolated Key Generator (IKG) <ug_nrf54l_crypto_cracen_ikg>` seed.

      See the :ref:`crypto_persistent_key` sample for an example of the IKG seed generation and write.

      The IAK is automatically derived from the IKG seed and is not retained after reset.
      Like all IKG-generated keys, it must be regenerated on each CRACEN power cycle.

The IAK is automatically encrypted and stored securely.

Usage example
-------------

See the :ref:`Identity key usage sample <identity_key_usage>` for an example.

.. _key_storage_modem_certificate:

Modem certificate storage
=========================

The nRF91 Series modem includes a dedicated certificate storage and integrated TLS/DTLS driver.
The application can write (provision) certificates to slots in the modem and choose which slots the modem should use for TLS in communication.

Certificates are provisioned to the modem before the main application runs.
Once written, they cannot be read back by the application, which provides a level of protection similar to hardware-backed key storage such as the KMU.

Configuration
-------------

For more information on provisioning certificates to the nRF91 Series modem, see the :ref:`nrf9160_ug_updating_cloud_certificate` page.
You can also see the `Cellular IoT Fundamentals course`_ on DevAcademy.

.. _key_storage_flash_protection:

Hardware flash write protection
===============================

The |NCS| provides the :ref:`fprotect driver <fprotect_readme>` for hardware-level flash protection.
This driver uses underlying hardware peripherals (BPROT, ACL, or SPU) to protect flash regions from unauthorized writes or reads.

Use fprotect to protect stored keys from modification, secure bootloader regions, and implement additional layers of security.

Configuration
-------------

To use the fprotect driver, enable the :kconfig:option:`CONFIG_FPROTECT` Kconfig option.
For more information, see the :ref:`fprotect_readme` page.

.. _key_storage_approtect:

Access port protection (AP-Protect)
===================================

The :ref:`access port protection mechanism <app_approtect>` (AP-Protect) prevents the debugger from accessing device resources, including keys and sensitive data.
When enabled, AP-Protect blocks read and write access to CPU registers and memory-mapped addresses.

Nordic Semiconductor recommends enabling AP-Protect in production devices to prevent extraction of keys and sensitive data through debug interfaces.

Configuration
-------------

AP-Protect configuration is device-specific.
See :ref:`app_approtect_implementation_overview` for detailed information.
