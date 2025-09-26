.. _ug_ot_thread_security:

OpenThread security
###################

.. contents::
   :local:
   :depth: 2

This section describes the security features of the |NCS| OpenThread implementation.
By default, the OpenThread stack uses the :ref:`PSA Certified Security Framework <ug_psa_certified_api_overview>` for cryptographic operations.

Nordic OpenThread samples leverage :ref:`security` features supported in the |NCS| that can be divided into four major categories:

* Cryptography
* Secure processing environment
* Secure storage
* Platform Security Support

For details, see the following sections.

Cryptography
************

The Thread stack uses the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` for cryptographic operations integrated in the nRF Security library.
Both :ref:`ug_crypto_architecture_implementation_standards` are supported, but using TF-M Crypto Service is only possible with Trusted Firmware-M (TF-M).
See :ref:`psa_crypto_support` and :ref:`ug_crypto_supported_features` for more information.

The Thread stack requires the following cryptographic operations:

* True Random Number Generation (TRNG)
* HMAC-SHA-256
* AES-128-CCM
* AES-CMAC-128
* ECDSA-SHA-256
* HKDF-SHA-256
* PBKDF2-AES-CMAC-PRF-128
* TLS-ECJPAKE (TSL 1.2)

Secure processing environment
*****************************

When building for the nRF54L15 DK using the ``nrf54l15dk/nrf54l15/cpuapp/ns`` :ref:`board target <app_boards_names>`, Thread samples can use the :ref:`secure processing environment <ug_tfm_security_by_separation>` with Trusted Firmware-M (TF-M).
In such cases, all cryptographic operations within the Thread stack are performed using the `Platform Security Architecture (PSA)`_ API and executed in the secure TF-M environment using the :ref:`TF-M Crypto Service implementation <ug_crypto_architecture_implementation_standards_tfm>`.
The secure materials like Thread network key can be stored in the TF-M secure storage using the :ref:`tfm_encrypted_its` or :ref:`key_storage_kmu`.

Thread samples use the full, configurable TF-M build, so you cannot use the minimal build.
For more information, see :ref:`ug_tfm_supported_services_profiles`.

For example, to build the Thread CLI sample with the TF-M support, run the following command:

.. code-block:: console

   west build -p -b nrf54l15dk/nrf54l15/cpuapp/ns samples/openthread/cli

Secure storage
**************

The Thread stack uses the PSA Crypto API for key management.
Depending on the platform and configuration, you can use PSA Internal Trusted Storage or CRACEN Key Management Unit (KMU) for storing the Thread cryptographic materials in non-volatile memory.

By default, the Thread stack uses PSA ITS backend for storing the Thread cryptographic materials.
You can enable it by setting the :kconfig:option:`CONFIG_OPENTHREAD_PSA_NVM_BACKEND_ITS` Kconfig option to ``y``.
To store the Thread cryptographic materials in the CRACEN KMU on the supported platforms, set the :kconfig:option:`CONFIG_OPENTHREAD_PSA_NVM_BACKEND_KMU` Kconfig option to ``y``.

PSA Internal Trusted Storage
============================

The PSA Internal Trusted Storage (ITS) is an encrypted storage within the Zephyr settings or dedicated partition that you can use to store the Thread cryptographic materials.
It is accelerated by the hardware through nRF crypto drivers such as CryptoCell or CRACEN.
Thread stack uses PSA ITS range defined by the :kconfig:option:`CONFIG_OPENTHREAD_PSA_ITS_NVM_OFFSET` Kconfig option, and by default it is set to ``0x20000``.
The maximum number of keys that can be stored in the PSA ITS is defined in the :kconfig:option:`CONFIG_OPENTHREAD_PSA_ITS_NVM_MAX_KEYS` Kconfig option, and by default it is set to ``20``.

.. _ug_ot_thread_security_kmu:

CRACEN Key Management Unit
==========================

CRACEN Key Management Unit (KMU) is a hardware-based key storage solution that can be used to store the Thread cryptographic materials.
It is available on the nRF54L Series devices.
It allows storing cryptographic materials in the non-volatile memory and provides a secure way to access them.
To learn more about the CRACEN Key Management Unit (KMU) and its usage, see :ref:`ug_nrf54l_crypto_kmu_cracen_peripherals`.

In this solution, the keys are stored within the available slots in the :ref:`ug_nrf54l_crypto_kmu_slots` range that are not reserved for current and future |NCS| use cases.
The default slots range used for Thread is from ``80`` to ``99``.
To change the starting slot number, set the :kconfig:option:`CONFIG_OPENTHREAD_KMU_SLOT_START` Kconfig option to the desired slot.
The end slot number is calculated as a sum of the starting slot number and the maximum number of keys that can be stored in the PSA ITS defined in the :kconfig:option:`CONFIG_OPENTHREAD_PSA_ITS_NVM_MAX_KEYS` Kconfig option.
The Raw usage scheme defined in the :ref:`ug_nrf54l_crypto_kmu_key_usage_schemes` section is used for all Thread keys.

Key management
==============

In the Thread stack, the following cryptographic materials are stored in the non-volatile memory:

.. list-table:: Thread cryptographic materials
   :widths: auto
   :header-rows: 1

   * - Crypto material
     - Description
     - Persistence
     - Key type and algorithm
     - Key size
     - Amount
     - Number of KMU slots needed for a key [1]_
   * - Network key
     - An OpenThread network master key.
     - Persisted
     - Asymmetric HMAC, HMAC-SHA-256
     - 128 bits
     - 3 (new, active, pending)
     - 1
   * - PSKc
     - A pre-shared key for the Thread network for the device.
       The key is derived based on the Commissioning Credential and used as a passphrase input to PAKE cipher suite to establish the shared secret.
     - Persisted
     - Symmetric AES, AES-128-CCM
     - 128 bits
     - 3 (new, active, pending)
     - 1
   * - Service Registration Protocol (SRP) ECC keypair
     - An ECC keypair used for Verification and Signing messages between the SRP client and server.
     - Persisted
     - Asymmetric ECC secp256r1 key pair, ECDSA-SHA-256
     - 256 bits
     - 1
     - 2
   * - Message Authentication Code (MAC) key
     - A key passed to the MAC layer to protect 802.15.4 data frames, derived from Network key using HKDF.
     - Volatile
     - Symmetric, HKDF-SHA-256
     - 128 bits
     - 1
     - N/A
   * - Mesh Link Establishment (MLE) key
     - A key used for the Mesh Link Establishment (MLE) protocol, derived from Network key using HKDF.
     - Volatile
     - Symmetric, HKDF-SHA-256
     - 128 bits
     - 1
     - N/A
   * - PSKd
     - A pre-shared key for the Thread network for the commissioner.
     - Volatile
     - Asymmetric ECC secp256r1 key pair, ECDSA-SHA-256
     - 256 bits
     - 2
     - N/A
   * - Commissioning Credential
     - A human-readable commissioning credential used to form the PSKc key.
     - Volatile
     - Password
     - 8-255 bytes
     - 1
     - N/A
   * - Key Establishment Key (KEK)
     - A key used to secure delivery of the network-wide key and other network parameters to the Joiner.
     - Volatile
     - Symmetric, HKDF-SHA-256
     - 128 bits
     - 1
     - N/A

.. [1] The KMU slots number is applicable only for the CRACEN KMU backend.

Platform Security Support
*************************

The following table summarizes the current security configuration and features supported for Thread-enabled hardware platforms in the |NCS|.
This is a reference configuration that you can modify in the production firmware by using proper Kconfig settings or implementing custom cryptographic backends.

.. list-table:: Thread platforms security support
   :widths: auto
   :header-rows: 1

   * - Platform
     - Networking backend
     - Cryptography backend
     - ARM TrustZone support
     - PSA Secure Storage backend
   * - nRF52840 SoC
     - Thread
     - Oberon + CryptoCell [2]_
     - No
     - Trusted Storage library + SHA-256 hash
   * - nRF5340 SoC
     - Thread
     - Oberon + CryptoCell [2]_
     - Yes
     - Trusted Storage library + Hardware Unique Key (HUK)
   * - nRF54LM20, nRF54L15, nRF54L10, nRF54L05 SoCs
     - Thread
     - CRACEN [3]_
     - Yes
     - Trusted Storage library + Hardware Unique Key (HUK) + Key Management Unit (KMU)
   * - nRF54L15, nRF54L10, nRF54L05 SoCs + Trusted Firmware-M (TF-M)
     - Thread
     - CRACEN
     - Yes
     - Trusted Firmware-M (TF-M) + Key Management Unit (KMU)

.. [2] The CryptoCell backend is used in parallel with the Oberon backend.
       By default, the CryptoCell backend is used only for the Random Number Generation (RNG) and AEAD key derivation driver.
       To enable the CryptoCell backend for additional operations, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` Kconfig option to ``true``.

.. [3] The CRACEN backend is used in parallel with the Oberon backend.
       The CRACEN backend is used by default for any supported cryptographic operations.
       For all other operations not supported by CRACEN, the Oberon backend is used.
       To use the Oberon backend for specific cryptographic operations supported by both drivers, disable those operations in the CRACEN driver, as it takes priority when both are enabled.
       See the :ref:`crypto_drivers` documentation for more information.
