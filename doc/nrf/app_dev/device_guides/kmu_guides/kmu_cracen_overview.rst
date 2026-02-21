.. _ug_kmu_guides_cracen_overview:

KMU and CRACEN hardware peripherals overview
############################################

.. contents::
   :local:
   :depth: 2

.. kmu_cracen_intro_start

Most nRF54L Series devices are equipped with two hardware peripherals that work together to provide secure cryptographic operations: the Key Management Unit (KMU) and the Crypto Accelerator Engine (CRACEN).
The hardware peripherals are supported in the |NCS| through the CRACEN driver, one of :ref:`nrf_security_drivers`.
The CRACEN driver implements driver-specific parts of the :ref:`PSA Crypto APIs <ug_psa_certified_api_overview_crypto>` and adds some vendor-specific to this API.

Together, these peripherals and the CRACEN driver are central for ensuring that cryptographic assets on these devices are protected.

.. kmu_cracen_intro_end

For more information about these hardware peripherals, see the related pages in the device datasheets, for example `KMU - Key management unit <nRF54L15 Key management unit_>`_ and `CRACEN - Crypto accelerator engine <nRF54L15 CRACEN_>`_ in the nRF54L15 datasheet.

Device support for KMU and CRACEN
*********************************

Expand the following section to see the list of devices that use KMU and CRACEN peripherals.

.. list-table:: KMU and CRACEN support by device
   :widths: auto
   :header-rows: 1

   * - ✔ Has KMU and CRACEN
     - ✗ Does not have KMU and CRACEN
   * - | nRF54L05
       | nRF54L10
       | nRF54L15
       | nRF54LM20
       | nRF54LV10
     - | nRF54LS05

.. _ug_cracen_hardware_peripheral:

CRACEN hardware peripheral
**************************

The CRACEN is a hardware peripheral that provides hardware-accelerated cryptographic operations and entropy generation.
It is not directly accessed by the CPU and is typically not used directly by end-users and their applications.
Instead, applications interact with CRACEN through the CRACEN driver, which provides a standard :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` interface.

CRACEN supports various cryptographic operations including encryption, decryption, signing, verification, and key derivation.
It can execute these operations using keys that are stored in the KMU or derived from the :ref:`CRACEN Isolated Key Generator (IKG) <ug_kmu_guides_cracen_ikg>`.

.. note::

   The CRACEN hardware peripheral relies on microcode for asymmetric cryptography operations like signature validation.
   On the nRF54L15, nRF54L10, and nRF54L05 devices, this microcode must be uploaded to a special CRACEN RAM area before first use and after each reset.
   On these devices, the :kconfig:option:`CONFIG_CRACEN_LOAD_MICROCODE` Kconfig option is enabled by default.

   If a bootloader uploads this microcode, there is no need to re-upload it for application use.
   This saves approximately 5 KB in the crypto driver code.

.. _ug_kmu_guides_cracen_ikg:

CRACEN Isolated Key Generator
=============================

The CRACEN Isolated Key Generator (IKG) is a hardware component inside CRACEN.
The IKG generates secret keys in a way that keeps them isolated from the CPU and software.

The IKG can derive three keys from a 384-bit seed value.
These keys are also called special hardware keys.
They are typically not meant for application use.
This is because they are not accessible by any CPU, but CRACEN can use them for cryptographic operations, provided directly to the cryptographic engine as a hardware signal.
IKG keys are not retained, and have to be regenerated for every CRACEN power cycle.

The 384-bit seed value is provisioned to the device or generated automatically during the first boot of the device using the CRACEN Random Number Generator (RNG), and then stored in the KMU.
Before the keys can be generated, the KMU pushes the seed to the SEED register and validates it.
Generating keys without a valid seed will fail.

IKG keys are also accessed using the standard PSA Crypto APIs, and are referenced by special built-in key IDs.

+-----------------+-------------------------------------+---------------------------------------------------+
| Key type        | Key ID                              | Description                                       |
+=================+=====================================+===================================================+
| ECC secp256r1   | ``CRACEN_BUILTIN_IDENTITY_KEY_ID``  | Used for signing/verification.                    |
+-----------------+-------------------------------------+---------------------------------------------------+
| AES 256-bit     | ``CRACEN_BUILTIN_MKEK_ID``          | Used for key derivation.                          |
+-----------------+-------------------------------------+---------------------------------------------------+
| AES 256-bit     | ``CRACEN_BUILTIN_MEXT_ID``          | Used for key derivation.                          |
+-----------------+-------------------------------------+---------------------------------------------------+

The keys are not exportable, except for the public key associated with the asymmetric key.

.. _ug_kmu_hardware_peripheral:

KMU hardware peripheral
***********************

The Key Management Unit (KMU) is a hardware peripheral that facilitates secure and confidential storage of cryptographic keys in a dedicated region of RRAM (secure information configuration region, or SICR).
The KMU acts as an intermediary between the CPU and the CRACEN hardware peripheral.
It is designed to hide key material from the CPU, while providing operations to import, use, revoke, or delete assets (keys and metadata).

The KMU can store cryptographic keys and 384-bit random seeds for the :ref:`CRACEN Isolated Key Generator (IKG) <ug_kmu_guides_cracen_ikg>` in key storage slots.
Each KMU slot can store 128 bits of key material, along with metadata and destination address information.
Keys larger than 128 bits are stored across multiple consecutive slots.

Only the KMU can push assets to CRACEN's protected RAM and the SEED register.
This hardware-level isolation ensures that sensitive key material never needs to be exposed to the CPU during cryptographic operations.

.. _ug_kmu_slots:

KMU slots
=========

The :ref:`KMU hardware peripheral <ug_kmu_hardware_peripheral>` is partitioned into 256 numbered slots.
Each KMU slot is capable of storing 128 bits of key material, a 32-bit destination address (``kmu_push_area``), and 32 bits of KMU metadata (:c:struct:`kmu_metadata`).
Storing keys that are larger than 128 bits is supported by using multiple, consecutive slots.

The application can use the KMU slots to store key data for its own purposes.
Some KMU slots are reserved for current and future |NCS| use cases.
The following table gives an overview of the KMU slots and their usage:

.. list-table:: List of reserved KMU slots
   :widths: auto
   :header-rows: 1

   * - | Reserved KMU slots
       | (range inclusive)
     - | |NCS| usage
       |
     - | Description
       |
   * - 180-182
     - Reserved
     - --
   * - 183-185
     - IKG seed
     - 384-bit random seed to generate keys using the CRACEN IKG.
   * - 186
     - Provisioning slot
     - | Reserved slot for internal KMU usage.
       | This slot is used to validate that provisioning of the KMU slots is completed.
   * - 187-225
     - Reserved
     - --
   * - 226-227
     - UROT_PUBKEY_0
     - | Revokable firmware image key for upgradable bootloader, generation 0.
       | ED25519 public key.
   * - 228-229
     - UROT_PUBKEY_1
     - | Revokable firmware image key for upgradable bootloader, generation 1.
       | ED25519 public key.
   * - 230-231
     - UROT_PUBKEY_2
     - | Revokable firmware image key for upgradable bootloader, generation 2.
       | ED25519 public key.
   * - 232-241
     - Reserved
     - --
   * - 242-243
     - BL_PUBKEY_0
     - | Revokable firmware image key for immutable bootloader, generation 0.
       | ED25519 public key.
   * - 244-245
     - BL_PUBKEY_1
     - | Revokable firmware image key for immutable bootloader, generation 1.
       | ED25519 public key.
   * - 246-247
     - BL_PUBKEY_2
     - | Revokable firmware image key for immutable bootloader, generation 2.
       | ED25519 public key.
   * - 248-249
     - Reserved
     - Random bytes which invalidate the protected RAM content after an operation.
   * - 250-255
     - Reserved
     - --

For more information about the KMU slots, see the device datasheet, for example `KMU - Key management unit <nRF54L15 Key management unit_>`_ for nRF54L15.

.. _ug_kmu_metadata:

KMU metadata
============

.. include:: ../../../../../doc/nrf/app_dev/device_guides/kmu_guides/kmu_provisioning_overview.rst
   :start-after: .. kmu_metadata_fields_start
   :end-before: .. kmu_metadata_fields_end

The configuration must follow the device datasheet (for example, `KMU - Key management unit <nRF54L15 Key management unit_>`_ for nRF54L15) and the CRACEN driver's expectations.
These expectations are different depending on the method you choose to provision keys to the KMU - see :ref:`ug_kmu_provisioning_methods`.

KMU provisioning
================

.. include:: ../../../../../doc/nrf/app_dev/device_guides/kmu_guides/kmu_provisioning_overview.rst
   :start-after: .. kmu_provisioning_overview_start
   :end-before: .. kmu_provisioning_overview_end

For more information about KMU provisioning, see :ref:`ug_kmu_provisioning_overview`.

How KMU and CRACEN work together
********************************

The KMU and CRACEN hardware peripherals work together to provide secure cryptographic operations while protecting key material from exposure to the CPU.

The KMU stores cryptographic keys securely in hardware.
When a cryptographic operation is needed, the KMU pushes keys directly to CRACEN's protected RAM for symmetric operations, or to a reserved RAM area for asymmetric operations.
For symmetric keys used in cipher operations, the KMU pushes the key material directly to CRACEN's engine without exposing it to the CPU.
For asymmetric keys, the KMU pushes the key to a reserved RAM location, which CRACEN loads into its engine before performing operations like signing or verification.

The CRACEN driver in the |NCS| exposes KMU operations through standard PSA Crypto API calls, with some vendor-specific extensions.
The driver :ref:`cryptographic operations <ug_crypto_supported_features>` using the CRACEN hardware peripheral and supports the following KMU operations:

* Importing (provisioning) keys to KMU slots
* Deleting a key from the KMU, allowing the underlying storage location to be reused
* Revoking a key from the KMU, preventing reuse of the underlying key slots
* Pushing symmetric keys directly from the KMU to the CRACEN symmetric engine, without exposing the key material to the CPU
* Directly pushing a 384-bit seed to CRACEN's IKG to derive isolated keys
* Using isolated keys derived from CRACEN's Isolated Key Generator (IKG) for encryption and signing purposes
* Pushing asymmetric keys to CPU RAM before loading them into CRACEN's engine for asymmetric operations

The keys that are stored in the KMU or generated by the IKG are referenced using the built-in keys that are key IDs in the range from ``MBEDTLS_PSA_KEY_ID_BUILTIN_MIN`` to and including ``MBEDTLS_PSA_KEY_ID_BUILTIN_MAX``.

Additionally, the CRACEN driver supports storing encrypted keys in KMU slots, transparently decrypting them to a temporary RAM location before using them in cryptographic operations.
