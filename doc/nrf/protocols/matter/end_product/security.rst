.. _ug_matter_device_security:

Security
########

.. contents::
   :local:
   :depth: 3

Nordic Matter samples leverage :ref:`security` features supported in the |NCS| that can be divided into four major categories:

* Cryptography
* Secure processing environment
* Trusted storage
* Securing production devices

In the following sections you will learn more details about each listed category.

Cryptography
************

Depending on the networking backend, the |NCS| Matter samples currently use the following APIs to implement cryptographic operations:

* :ref:`PSA Cryptography API <psa_crypto_support>` for Thread networking.
  Both :ref:`ug_crypto_architecture_implementation_standards` are supported, but using TF-M Crypto Service is only possible with Trusted Firmware-M (TF-M).
* :ref:`Legacy Mbed TLS crypto API <legacy_crypto_support>` for Wi-Fi® networking.
  Support for PSA Cryptography API for the Wi-Fi backend is planned for a future release.

Both APIs are integrated through the nRF Security library.
For an overview of the PSA Crypto API, see the :ref:`ug_crypto_architecture` page.
For an overview of the cryptography layer configuration supported for each |NCS| Matter-enabled platform, see the :ref:`matter_platforms_security_support` section.

Secure processing environment
*****************************

Depending on the board target, Matter samples can use the :ref:`secure processing environment <ug_tfm_security_by_separation>` with Trusted Firmware-M (TF-M).

nRF54L with Trusted Firmware-M (TF-M)
=====================================

On the nRF54L SoC, all cryptographic operations within the Matter stack are performed by utilizing the `Platform Security Architecture (PSA)`_ API and executed in the secure TF-M environment using the :ref:`TF-M Crypto Service implementation <ug_crypto_architecture_implementation_standards_tfm>`.
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

Trusted storage
***************

The :ref:`trusted_storage_in_ncs` is a security mechanism designed to securely store and manage sensitive data.
Currently, all :ref:`matter_samples` in the |NCS| use the :ref:`trusted_storage_readme` library as the Trusted Storage backend for all supported platforms.
You can find an overview of the Trusted Storage layer configuration supported for each |NCS| Matter-enabled platform in the :ref:`matter_platforms_security_support` section.

.. note::
   For the nRF52840 devices, in regards to :ref:`matter_samples` in |NCS|, AEAD keys are derived using hashes of entry UIDs (:kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_HASH_UID`).
   This approach is less secure than using the :ref:`lib_hw_unique_key` library for key derivation as it only provides integrity of sensitive material.
   It is also possible to implement a custom AEAD key generation method when the :kconfig:option:`CONFIG_TRUSTED_STORAGE_BACKEND_AEAD_KEY_CUSTOM` Kconfig option is selected.

For more details about AEAD key generation and backend configuration, see the :ref:`trusted_storage_readme` readme.

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
   * - nRF54L10 SoC
     - Thread
     - PSA Crypto API
     - CRACEN [2]_
     - Yes
     - Trusted Storage library + Hardware Unique Key (HUK) + Key Management Unit (KMU)
   * - nRF54LM20 SoC
     - Thread
     - PSA Crypto API
     - CRACEN [2]_
     - Yes
     - Trusted Storage library + Hardware Unique Key (HUK) + Key Management Unit (KMU)

.. [1] The CryptoCell backend is used in parallel with the Oberon backend.
       By default, the CryptoCell backend is used only for Random Number Generation (RNG) and the AEAD key derivation driver.
       To enable the CryptoCell backend for additional operations, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` Kconfig option to true.

.. [2] The CRACEN backend is used in parallel with the Oberon backend.
       The CRACEN backend is used by default for any supported cryptographic operations.
       For all other operations not supported by CRACEN, the Oberon backend is used.
       To use the Oberon backend for specific cryptographic operations supported by both drivers, disable those operations in the CRACEN driver, as it takes priority when both are enabled.
       See the :ref:`nrf_security_drivers` documentation for more information.

.. _matter_platforms_security_kmu:

nRF54L Key Management Unit (KMU)
================================

nRF54L Series devices include :ref:`ug_nrf54l_crypto_kmu_cracen_peripherals` that can be used to store cryptographic keys in Matter.
In this solution, the keys are stored within the available slots in the :ref:`ug_nrf54l_crypto_kmu_slots` range that are not reserved for current and future |NCS| use cases.

The default slots range used for Matter is from ``100`` to ``180``, excluding the DAC private key.
For details on the DAC private key configuration, see :ref:`matter_platforms_security_dac_priv_key_kmu`.
To change the slots range, set the :kconfig:option:`CONFIG_CHIP_KMU_SLOT_RANGE_START` and :kconfig:option:`CONFIG_CHIP_KMU_SLOT_RANGE_END` Kconfig options.
The Raw usage scheme defined in the :ref:`ug_nrf54l_crypto_kmu_key_usage_schemes` section is used for all Matter keys.

To use this feature, set the :kconfig:option:`CONFIG_CHIP_STORE_KEYS_IN_KMU` Kconfig option to ``y``, and switch to the ``KMUKeyAllocator`` by calling the ``chip::Crypto::SetPSAKeyAllocator`` method in your code during the Matter stack initialization.

For example:

.. code-block:: cpp

    #include <platform/nrfconnect/KMUKeyAllocator.h>

    static KMUKeyAllocator kmuAllocator;
    Crypto::SetPSAKeyAllocator(&kmuAllocator);

See the :file:`samples/matter/common/src/app/matter_init.cpp` file for a usage example.

Due to limited slots available in the KMU, the maximum number of Matter fabric is limited.
The following table shows all the crypto materials used in Matter, the number of slots needed for each key and a Kconfig option that can be used to adjust the number of each key type:

.. list-table:: KMU slots used by Matter crypto materials
   :widths: auto
   :header-rows: 1

   * - Crypto material in Matter
     - Key type
     - Number of slots needed for a key
     - Multiplication
     - Minimum number of keys
     - Kconfig option
   * - Node Operational Certificate (NOC) private key
     - ECC secp256r1 key pair
     - 2
     - For each Matter fabric
     - 5 (5 Matter fabrics for each device)
     - :kconfig:option:`CONFIG_CHIP_MAX_FABRICS`
   * - Intermittently Connected Device (ICD) Token
     - HMAC SHA-256 128-bit keys
     - 1
     - For each ICD user
     - 10 (2 ICD users for each Matter fabric)
     - :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC`
   * - Intermittently Connected Device (ICD) symmetric Key
     - AES 128-bit keys
     - 1
     - For each ICD user
     - 10 (2 ICD users for each Matter fabric)
     - :kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC`
   * - Group key [3]_
     - AES 128-bit keys
     - 1
     - For each group
     - 15 (3 groups for each Matter fabric)
     - Not a Kconfig option, see ``CHIP_CONFIG_MAX_GROUP_ENDPOINTS_PER_FABRIC`` Matter config.
   * - Device Attestation Certificate (DAC) private key
     - ECC secp256r1 key pair
     - 2
     - For each device
     - 1
     - Not configurable

.. [3] Group keys are not stored in the KMU yet, but the slots are reserved for the future usage.
       A key may be shared between multiple groups, so the number of slots needed for group keys may be lower than the number of groups.
       Three group keys are assumed for each Matter fabric.

The default slots range allows storing more cryptographic materials than the minimum required according to the Matter specification.
A minimum of 46 slots is required for cryptographic materials, including the DAC private key to support five fabrics (minimum required by the specification).
A single Matter fabric requires at least nine KMU slots.

The default range has been chosen to support up to eight Matter fabrics, 32 ICD keys (16 ICD users), and 24 group keys.
You can change the default Kconfig option for each crypto material to adjust the number of slots, but you must ensure that the total number of slots fits within the defined range.
If you overlap the slots range, the appropriate compilation error will be shown:

.. code-block:: console

    error: "The number of slots exceeds the range of the KMU defined in CONFIG_CHIP_KMU_SLOT_RANGE_START and CONFIG_CHIP_KMU_SLOT_RANGE_END"

For example, to fill the default slots range and support eight Matter fabrics, three group keys per fabric, and two ICD users for each fabric, set the :kconfig:option:`CONFIG_CHIP_MAX_FABRICS` Kconfig option to ``8``.

Slots arrangement within the slot range defined by :kconfig:option:`CONFIG_CHIP_KMU_SLOT_RANGE_START` and :kconfig:option:`CONFIG_CHIP_KMU_SLOT_RANGE_END` Kconfig options is calculated automatically in the code in the following way:

.. list-table:: KMU slots arrangement for Matter crypto materials
  :widths: auto
  :header-rows: 1

  * - Crypto material
    - Slot range start
    - Slot range end
    - Default slot range for eight Matter fabrics to fit the default range (100-180)
  * - Node Operational Certificate (NOC) private key
    - :kconfig:option:`CONFIG_CHIP_KMU_SLOT_RANGE_START`
    - :kconfig:option:`CONFIG_CHIP_KMU_SLOT_RANGE_START` + (:kconfig:option:`CONFIG_CHIP_MAX_FABRICS` * 2)
    - 100 - 115
  * - Intermittently Connected Device (ICD) Token + ICD symmetric Key
    - End of NOC slots
    - End of NOC slots + (:kconfig:option:`CONFIG_CHIP_ICD_CLIENTS_PER_FABRIC` * :kconfig:option:`CONFIG_CHIP_MAX_FABRICS` * 2)
    - 116 - 147
  * - Group keys
    - End of ICD slots
    - End of ICD slots + (3 * :kconfig:option:`CONFIG_CHIP_MAX_FABRICS`)
    - 148 - 171
  * - Device Attestation Certificate (DAC) private key
    - Configurable with :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID`
    - DAC private key start slot + 1 (non-encrypted) or + 3 (encrypted)
    - 176 - 179 (encrypted) or 178 - 179 (non-encrypted)

.. important::

  Once a slot range is defined, it should not be changed during the firmware update process.
  Changing the slot range will result in loss of all cryptographic materials stored in the KMU.
  You can safely change slot ranges only when the device is in the factory state and no cryptographic materials are stored in the KMU.
  Otherwise, to extend the slot range beyond the previously defined range and keep the existing cryptographic materials stored in the KMU, complete the following steps:

  1. Locate the :file:`src/platform/nrfconnect/KMUKeyAllocator.h` file in the Matter repository, or create a new one with the same functionality.
  #. Modify the file content to redirect the keys to the new slot range by implementing the ``GetDacKeyId``, ``GetOpKeyId``, and ``AllocateICDKeyId`` functions.
  #. If you want to use your own implementation of the KMUKeyAllocator, assign it in your application code by calling the ``Crypto::SetPSAKeyAllocator`` method.
  #. Ensure that you have chosen an available slot range that does not overlap with any existing one.
  #. Build your application with the new configuration.
  #. Perform a firmware update on the device.

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
   * - nRF54L10 SoC
     - Key Management Unit (KMU)
     - | Key Management Unit (KMU),
       | Trusted Storage library + Hardware Unique Key (Zephyr Settings),
       | Trusted Storage library + SHA-256 hash (Zephyr Settings)

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
For the non-encrypted version, it occupies the last two slots (178 and 179), and for the encrypted version, it occupies the last four slots (176-179).
You can change the default slots by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_DAC_PRIV_KEY_KMU_SLOT_ID` Kconfig option to the first slot number of the desired slots, making sure that all slots fit within the possible range.
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
