.. _ug_matter_device_security:

Security
########

.. contents::
   :local:
   :depth: 3

Nordic Matter samples leverage :ref:`security` features supported in the |NCS| that can be divided into four major categories:

* Secure processing environment
* Cryptography
* Trusted storage
* Securing production devices

In the following sections you will learn more details about each listed category.

Secure processing environment
*****************************

Depending on the board, Matter samples can use a secure processing environment.

nRF54L with Trusted Firmware-M (TF-M)
=====================================

On the nRF54L SoC, Matter samples support :ref:`app_boards_spe_nspe` with Trusted Firmware-M (TF-M).
All cryptographic operations within the Matter stack are performed by utilizing the `Platform Security Architecture (PSA)`_ API and executed in the secure TF-M environment.
The secure materials like Matter Session keys and other keys, except for the DAC private key, are stored in the TF-M secure storage using the :ref:`tfm_encrypted_its` module.
Matter samples use the full TF-M library, so you cannot use the :ref:`tfm_minimal_build` version of TF-M.

To build a Matter sample with the TF-M support, :ref:`build <building>` for the :ref:`board target <app_boards_names>` with the ``/ns`` variant.

To configure partition layout for your application, you can edit the :file:`pm_static_nrf54l15dk_nrf54l15_cpuapp_ns.yml` file that is available in each sample directory.
To read more about the TF-M partitioning, see :ref:`ug_tfm_partition_alignment_requirements`.
While using TF-M, the application partition size and available RAM space for the application is lower than without TF-M.
You must keep this in mind and calculate the available space for the application partition.
The recommended values are provided in the :ref:`ug_matter_hw_requirements_layouts` section.

By default, the DAC private key is stored in the KMU storage while using TF-M.
See the :ref:`matter_platforms_security_dac_priv_key_kmu` section for more information.

Cryptography
************

Depending on the networking backend, the |NCS| Matter samples currently use the following APIs to implement cryptographic operations:

* PSA Cryptography API for Thread networking.
* Mbed TLS for Wi-Fi networking.
  Support for PSA Cryptography API for the Wi-Fi backend is planned for a future release.

Both APIs are integrated in the :ref:`nrf_security` library.
This library offers various configuration possibilities and backends that can be employed to implement :ref:`cryptographic operations <security_index>`.
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

The HUK library is supported for the nRF52840, nRF5340, and nRF54L15 platforms, but for :ref:`matter_samples` in the |NCS|, it is only enabled for the nRF5340 and NRF54L15 platforms:

* For the nRF5340 and nRF54L15 platforms, the HUK is generated at first boot and stored in the Key Management Unit (KMU).
  No changes to the existing partition layout are needed for products in the field.
* For the nRF54L15 NS platform, the HUK generation and management is handled by the Trusted Firmware-M (TF-M) library.
* For the nRF52840 platform, AEAD keys are derived with a SHA-256 hash (:kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_HASH_UID`).
  This approach is less secure than using the library for key derivation as it will only provide integrity of sensitive material.
  It is also possible to implement a custom AEAD keys generation method when the :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_CUSTOM` Kconfig option is selected.

  Using the HUK library with nRF52840 SoC is possible, but it requires employing the :ref:`bootloader` that would generate the AEAD key at first boot and store it in the dedicated HUK partition that can be accessed only by the CryptoCell peripheral.
  Note that adding another partition in the FLASH layout implies breaking the firmware backward compatibility with already deployed devices.

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
     - PSA Secure Storage backend
   * - nRF52840 SoC
     - Thread
     - PSA Crypto API
     - Oberon + CryptoCell [1]_
     - No
     - Trusted Storage library + SHA-256 hash
   * - nRF5340 SoC
     - Thread
     - PSA Crypto API
     - Oberon + CryptoCell [1]_
     - Yes
     - Trusted Storage library + Hardware Unique Key (HUK)
   * - nRF5340 SoC + nRF7002 companion IC
     - Wi-Fi
     - Mbed TLS
     - Oberon + CryptoCell [1]_
     - Yes
     - ---
   * - nRF54L15 SoC
     - Thread
     - PSA Crypto API
     - CRACEN [2]_
     - Yes
     - Trusted Storage library + Hardware Unique Key (HUK) + Key Management Unit (KMU)
   * - nRF54L15 SoC + Trusted Firmware-M (TF-M)
     - Thread
     - PSA Crypto API
     - CRACEN
     - Yes
     - Trusted Firmware-M (TF-M) + Key Management Unit (KMU)

.. [1] The CryptoCell backend is used in parallel with the Oberon backend.
       By default, the CryptoCell backend is used only for Random Number Generation (RNG) and the AEAD key derivation driver.
       To enable the CryptoCell backend for additional operations, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` Kconfig option to true.

.. [2] The CRACEN backend is used in parallel with the Oberon backend.
       The CRACEN backend is used by default for any supported cryptographic operations.
       For all other operations not supported by CRACEN, the Oberon backend is used.
       To use the Oberon backend for specific cryptographic operations supported by both drivers, disable those operations in the CRACEN driver, as it takes priority when both are enabled.
       See the :ref:`nrf_security_drivers` documentation for more information.

.. _matter_platforms_security_dac_priv_key:

Storing Device Attestation Certificate private key
**************************************************

In Matter samples based on the PSA crypto API, the Device Attestation Certificate's private key, which exists in the factory data set, can be migrated to secure storage.
The secure storage used depends on the platform and the cryptographic backend.

The migration of the DAC private key from the factory data set to secure storage is controlled by the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_MIGRATE_DAC_PRIV_KEY` Kconfig option and set to ``y`` by default.

Currently, this feature is available only for the PSA crypto API.
See the following table to learn about the default secure storage backends for the DAC private key and the available secure storage backends for each platform:

.. list-table:: Matter secure storage
   :widths: auto
   :header-rows: 1

   * - Platform
     - Default secure storage backend for DAC private key
     - Available secure storage backends
   * - nRF52840 SoC
     - Trusted Storage library + SHA-256 hash (Zephyr Settings)
     - Trusted Storage library + SHA-256 hash (Zephyr Settings)
   * - nRF5340 SoC
     - Trusted Storage library + Hardware Unique Key (Zephyr Settings)
     - | Trusted Storage library + Hardware Unique Key (Zephyr Settings),
       | Trusted Storage library + SHA-256 hash (Zephyr Settings)
   * - nRF5340 SoC + nRF7002 companion IC
     - Not available
     - Not available
   * - nRF54L15 SoC
     - Key Management Unit (KMU)
     - | Key Management Unit (KMU),
       | Trusted Storage library + Hardware Unique Key (Zephyr Settings),
       | Trusted Storage library + SHA-256 hash (Zephyr Settings)
   * - nRF54L15 SoC + Trusted Firmware-M (TF-M)
     - Key Management Unit (KMU)
     - | Key Management Unit (KMU),
       | Trusted Firmware-M Storage (TF-M)

If you migrate the DAC private key to storage based on Zephyr Settings storage, you cannot use the :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS` Kconfig option.
This is because the factory reset feature will erase the secure storage, including the DAC private key, which has been removed from the factory data.
In this case, the DAC private key will be lost, and the device will not be able to authenticate to the network.

You can use the :kconfig:option:`CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS` Kconfig option if you store the DAC private key in the KMU or TF-M secure storage (available on nRF54L SoCs only).

.. _matter_platforms_security_dac_priv_key_its:

DAC in Trusted Storage library
==============================

The Device Attestation Certificates private key can be stored in the Trusted Storage library.
The key is encrypted with the AEAD key derived from the Hardware Unique Key (HUK) or a SHA-256 hash.
This storage backend is selected by default for all platforms that support the PSA crypto API, except for the nRF54L Series, which uses Key Management Unit (KMU).

To enable storing the DAC private key in the Trusted Storage library, set the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_ITS` Kconfig option to ``y``.
To select which encryption to use, set one of the following Kconfig options:

- To use key derivation from HUK, set :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_DERIVE_FROM_HUK` to ``y``.
- To use key derivation from a SHA-256 hash, set :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_HASH_UID` to ``y``.

Encryption with the AEAD key derived from the Hardware Unique Key (HUK) is available only on the nRF5340 and nRF54L platforms.

.. _matter_platforms_security_dac_priv_key_kmu:

DAC in Key Management Unit (KMU)
================================

The Key Management Unit (KMU) is a hardware peripheral that provides secure storage for cryptographic keys.
It is available in the nRF54L Series SoCs and can be used to store the DAC private key.
This storage backend can be used with Trusted Firmware-M (TF-M).

Storing the DAC private key in the KMU is controlled by the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU` Kconfig option and set to ``y`` by default.

You can additionally encrypt the DAC private key in the KMU storage by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_ENCRYPTED` Kconfig option to ``y``.
This operation requires two additional KMU slots to store the nonce and the authentication tag, making the total number of slots used four.
If the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_ENCRYPTED` Kconfig option is set to ``n``, then the DAC private key is stored in the KMU without encryption and utilizes two KMU slots.

By default, the DAC private key occupies the last slots dedicated for application purposes.
For the non-encrypted version, it occupies last two slots (178 and 179), and for the encrypted version, it occupies the last four slots (176-179).
You can change the default slots by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT` Kconfig option to the first slot number of the desired slots, making sure that all slots fit within the possible range.
This means you can set it to slot numbers 0-176 for encrypted, or 0-178 for non-encrypted.
To read more about KMU slots, see the :ref:`ug_nrf54l_crypto_kmu_slots` section of the :ref:`ug_nrf54l_cryptography` page, which details the KMU peripheral.

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
