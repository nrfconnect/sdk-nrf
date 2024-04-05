.. _ug_matter_device_security:

Security
########

.. contents::
   :local:
   :depth: 3

Nordic Matter samples leverage security features supported in the |NCS| that can be divided into three major categories:

* Cryptography
* Trusted storage
* Securing production devices

In the following sections you will learn more details about each listed category.

Cryptography
************

Depending on the networking backend, the |NCS| Matter samples currently use the following APIs to implement cryptographic operations:

* PSA Cryptography API for Thread networking.
* Mbed TLS for Wi-Fi networking.
  Support for PSA Cryptography API for the Wi-Fi backend is planned for a future release.

Both APIs are integrated in the :ref:`nrf_security` library.
This library offers various configuration possibilities and backends that can be employed to implement :ref:`cryptographic operations <cryptographic_operations_in_ncs>`.
You can find an overview of the cryptography layer configuration supported for each |NCS| Matter-enabled platform in the :ref:`matter_platforms_security_support` section.

Trusted storage
***************

:ref:`trusted_storage_in_ncs` is a security mechanism designed to securely store and manage sensitive data, such as cryptographic keys, device credentials, and configuration data.
The mechanism is essential for IoT devices, as it allows the implementation of secure communication between devices.

The Trusted Storage can utilize one of the following backends to implement PSA Certified Secure Storage API:

* TF-M Platform Root of Trust (PRoT).
  This can only be utilized if the `ARM TrustZone`_ technology and hardware-accelerated firmware isolation are supported by the platform in use.
* The :ref:`trusted_storage_readme` |NCS| software library.

Currently all :ref:`matter_samples` in the |NCS| use the trusted storage library as a Trusted Storage backend for all supported platforms.

The trusted storage library provides confidentiality, integrity, and authenticity for the assets it operates on.
It handles sensitive data in two contexts:

* Volatile - Before data is forwarded to the non-volatile storage and after it is retrieved from the non-volatile storage.
* Non-volatile - When data is written to the non-volatile memory in encrypted form.

In the case of the volatile context, the trusted storage library leverages the Authenticated Encryption with Associated Data (AEAD) encryption backend (:kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD`).
It is used to encrypt and decrypt the assets that are being securely stored in the non-volatile memory.
AEAD can be configured with the set of Kconfig options described in the library's :ref:`trusted_storage_configuration` section.

An important setting, that depends on the hardware platform in use, is the way of generating the AEAD keys.
The recommended and the most secure option is to use :ref:`lib_hw_unique_key` (HUK) library.
HUK support is automatically enabled with the :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK` Kconfig option for compatible configurations.

The HUK library is supported for both the nRF52840 and nRF5340 platforms, but for :ref:`matter_samples` in the |NCS|, it is only enabled for the nRF5340 platform:

* For the nRF5340 platform, the HUK is generated at first boot and stored in the Key Management Unit (KMU).
  No changes to the existing partition layout are needed for products in the field.
* For the nRF52840 and nRF54L15 platforms, AEAD keys are derived with a SHA-256 hash (:kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_HASH_UID`).
  This approach is less secure than using the library for key derivation as it will only provide integrity of sensitive material.
  It is also possible to implement a custom AEAD keys generation method when the :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_CUSTOM` Kconfig option is selected.

  Using the HUK library with nRF52840 SoC is possible, but it requires employing the :ref:`bootloader` that would generate the AEAD key at first boot and store it in the dedicated HUK partition that can be accessed only by the CryptoCell peripheral.
  Note that adding another partition in the FLASH layout implies breaking the firmware backward compatibility with already deployed devices.

  .. note::
     Using the HUK library with the nRF54L15 SoC is not possible yet.
     This means that you need to use AEAD keys derived with a SHA-256 hash.

You can find an overview of the Trusted Storage layer configuration supported for each |NCS| Matter-enabled platform in the :ref:`matter_platforms_security_support` section.

.. _matter_platforms_security_support:

Matter platforms security support
*********************************

The following table summarizes the current security configuration and features supported for Matter-enabled hardware platforms in the |NCS|.
This is a reference configuration that can be modified in the production firmware by using proper Kconfig settings or implementing custom cryptographic backends.

.. list-table:: Matter platforms security support
   :widths: auto
   :header-rows: 1

   * - Platform
     - Networking backend
     - Cryptography API
     - Cryptography backend
     - ARM TrustZone support
     - Trusted Storage backend
     - Trusted Storage AEAD key
   * - nRF52840 SoC
     - Thread
     - PSA
     - Oberon + CryptoCell [1]_
     - No
     - Trusted Storage library
     - SHA-256 hash
   * - nRF5340 SoC
     - Thread
     - PSA
     - Oberon + CryptoCell [1]_
     - Yes
     - Trusted Storage library
     - Hardware Unique Key (HUK)
   * - nRF5340 SoC + nRF7002 companion IC
     - Wi-Fi
     - Mbed TLS
     - Oberon + CryptoCell [1]_
     - Yes
     - ---
     - ---
   * - nRF54L15 SoC
     - Thread
     - PSA
     - CRACEN + Oberon [2]_
     - Yes
     - Trusted Storage backend
     - SHA-256 hash

.. [1] The CryptoCell backend is used in parallel with the Oberon backend.
       It is only used to implement Random Nuber Generation (RNG), and the AED keys derivation driver (only for Thread networking).
       For all other cryptographic operations, the Oberon backend is used.

.. [2] When the CRACEN driver is used in parallel with the Oberon driver, you need to disable specific CRACEN cryptographic operations to use the Oberon ones.
       This is because the CRACEN driver has priority when both are enabled.
       See the :ref:`nrf_security_drivers` documentation for more information.

Securing production devices
***************************

When finalizing work on a Matter device, make sure to adopt the following recommendations before sending the device to production.

Enable AP-Protect
=================

Make sure to enable the AP-Protect feature for the production devices to disable the debug functionality.

.. include:: ../../../security/ap_protect.rst
   :start-after: app_approtect_info_start
   :end-before: app_approtect_info_end

See :ref:`app_approtect` for more information.

Disable debug serial port
=========================

Make sure to disable the debug serial port, for example UART, so that logs and shell commands are not accessible for production devices.
See the :file:`prj_release.conf` files in :ref:`Matter samples <matter_samples>` for an example of how to disable debug functionalities.
