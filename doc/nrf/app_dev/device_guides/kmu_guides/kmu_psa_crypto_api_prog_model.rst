.. _ug_crypto_kmu_psa_key_programming_model:

PSA Crypto API programming model for KMU keys
#############################################

.. contents::
   :local:
   :depth: 2

The keys that are stored in the KMU can be used by most cryptographic functions and key management functions in the :ref:`PSA Crypto API <ug_psa_certified_api_overview>`, with a built-in key ID representing a particular :ref:`KMU slot <ug_kmu_slots>`.

This page describes the PSA Crypto API programming model for KMU keys.
You can use it as a reference for :ref:`ug_kmu_provisioning_psa_crypto_api`.
Detailed steps for creating a provisioning image for PSA Crypto API for your application are beyond the scope of this documentation.

KMU slot PSA identifiers
************************

To identify that the KMU is used as a persistent storage backend for a specific ``psa_key_id_t``, you need to create a :c:struct:`psa_key_attributes_t` structure and set the required attributes from the following list.

.. list-table:: PSA Key attribute setters for KMU keys
   :header-rows: 1
   :widths: 22 33 45

   * - Attribute (setter function)
     - Parameters
     - Description
   * - ``key_type`` (``psa_set_key_type``)
     - | A supported key type.
       |
       | See :ref:`ug_kmu_guides_supported_key_types` for overview of the supported key types for each driver.
     - Sets the key type and size.
   * - ``key_bits`` (``psa_set_key_bits``)
     - | A supported key size for the key type.
       |
       | See :ref:`ug_kmu_guides_supported_key_types` for overview of the supported key types for each driver.
     - Sets the key type and size.
   * - ``key_lifetime`` (``psa_set_key_lifetime``)
     - | ``PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(persistence, location)``
       |
       | where ``persistence`` is one of:
       |
       |
       | - ``PSA_KEY_PERSISTENCE_DEFAULT``
       | - ``CRACEN_KEY_PERSISTENCE_READ_ONLY``
       | - ``CRACEN_KEY_PERSISTENCE_REVOKABLE``
       |
       | and ``location`` is ``PSA_KEY_LOCATION_CRACEN_KMU``
       |
       | See also :ref:`ug_kmu_key_lifetimes` for more details.
     - | ``CRACEN_KEY_PERSISTENCE_REVOKABLE`` is a custom persistence mode, which will revoke the key slots when the key is destroyed.
       |
       | ``PSA_KEY_PERSISTENCE_DEFAULT`` should be used by applications that have no specific needs beyond what is met by implementation-specific features.
       |
       | ``CRACEN_KEY_PERSISTENCE_READ_ONLY`` is for read-only or write-once keys. A key with this persistence level cannot be destroyed.
       |
       | Keys that are read-only due to policy restrictions, rather than physical limitations, should not have this persistence level.
   * - ``key_id`` (``psa_set_key_id``)
     - | ``PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(kmu_usage_scheme, kmu_slot_nr)``
       |
       | For ``kmu_usage_scheme`` values, see :ref:`ug_kmu_guides_key_usage_schemes`.
       |
       | For ``kmu_slot_nr`` values, see :ref:`ug_kmu_slots`.
     - Identifies the KMU key slot to use.
   * - ``key_usage`` (``psa_set_key_usage_flags``)
     - | Standard PSA Crypto key usage flags.
       |
       | See also :ref:`ug_kmu_guides_key_usage_schemes`.
     - ``PSA_KEY_USAGE_EXPORT`` and ``PSA_KEY_USAGE_COPY`` are not allowed for keys with the usage scheme ``CRACEN_KEY_USAGE_SCHEME_PROTECTED``.

.. _ug_kmu_guides_supported_key_types:

Key types that can be stored in the KMU
***************************************

The following table lists all key types that can be stored in the KMU.
For each key type, the table lists the supported algorithms and indicates which :ref:`ug_kmu_guides_key_usage_schemes` support the key types and the number of key slots they require.

.. note::
   This list does not include the key types that are supported by the CRACEN driver, but not stored in the KMU.
   For the list of supported key types by the CRACEN driver, see :ref:`ug_crypto_supported_features_operations`.

.. list-table:: Supported key types
   :widths: auto
   :header-rows: 1

   * - Key type
     - PSA key attributes and algorithms [1]_
     - KMU slots [2]_
     - Protected
     - Encrypted
     - Raw
   * - AES 128-bit keys
     - | ``key_type``: ``PSA_KEY_TYPE_AES``
       |
       | ``key_bits``: 128
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_AES_ECB_NO_PADDING``
       | - ``PSA_ALG_AES_CBC_NO_PADDING``
       | - ``PSA_ALG_AES_CTR``
       | - ``PSA_ALG_CCM``
       | - ``PSA_ALG_GCM``
       | - ``PSA_ALG_CMAC*``
       | - ``PSA_ALG_SP800_108_COUNTER_CMAC``
     - 1
     - Yes
     - Yes
     - Yes
   * - AES 192-bit and 256-bit keys
     - | ``key_type``: ``PSA_KEY_TYPE_AES``
       |
       | ``key_bits``: 192 [3]_
       | ``key_bits``: 256
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_AES_ECB_NO_PADDING``
       | - ``PSA_ALG_AES_CBC_NO_PADDING``
       | - ``PSA_ALG_AES_CTR``
       | - ``PSA_ALG_CCM``
       | - ``PSA_ALG_GCM``
       | - ``PSA_ALG_CMAC*``
       | - ``PSA_ALG_SP800_108_COUNTER_CMAC``
     - 2
     - Yes
     - Yes
     - Yes
   * - ChaCha20-Poly1305
     - | ``key_type``: ``PSA_KEY_TYPE_CHACHA20``
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_CHACHA20``
       | - ``PSA_ALG_CHACHA20_POLY1305``
     - 2
     - No
     - Yes
     - Yes
   * - ECC secp256r1 key pair (ECDSA and ECDH usage)
     - | ``key_type``: ``PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)``
       |
       | ``key_bits``: 256
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_ECDSA``
       | - ``PSA_ALG_ECDH``
     - 2
     - No
     - Yes
     - Yes
   * - ECC secp256r1 public key (ECDSA usage only)
     - | ``key_type``: ``PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)``
       |
       | ``key_bits``: 256
       |
       | ``key_algorithm``: ``PSA_ALG_ECDSA``
     - 4
     - No
     - Yes
     - Yes
   * - ECC secp384r1 key pair (ECDSA and ECDH usage)
     - | ``key_type``: ``PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)``
       |
       | ``key_bits``: 384
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_ECDSA``
       | - ``PSA_ALG_ECDH``
     - 3
     - No
     - Yes
     - Yes
   * - ECC secp384r1 public key (ECDSA usage only)
     - | ``key_type``: ``PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)``
       |
       | ``key_bits``: 384
       |
       | ``key_algorithm``: ``PSA_ALG_ECDSA``
     - 6
     - No
     - No
     - Yes
   * - Ed25519 key pair
     - | ``key_type``: ``PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)``
       |
       | ``key_bits``: 255
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_ED25519``
       | - ``PSA_ALG_ED25519PH``
     - 2
     - No
     - Yes
     - Yes
   * - ED25519 public key
     - | ``key_type``: ``PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS)``
       |
       | ``key_bits``: 255
       |
       | ``key_algorithm`` - one of the following:
       | - ``PSA_ALG_ED25519``
       | - ``PSA_ALG_ED25519PH``
     - 2
     - No
     - Yes
     - Yes
   * - HMAC SHA-256 128-bit keys
     - | ``key_type``: ``PSA_KEY_TYPE_HMAC``
       |
       | ``key_bits``: 128
       |
       | ``key_algorithm``: ``PSA_ALG_HMAC(PSA_ALG_SHA_256)``
     - 1
     - No
     - No
     - Yes
.. [1] Store each key with only one algorithm to follow PSA Crypto best practices.
.. [2] Keys with the Encrypted usage scheme (``CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED``) require two additional KMU slots to store the nonce and the authentication tag.
.. [3] 192-bit key size is not supported on nRF54LM20A.

.. _ug_kmu_key_lifetimes:

Key lifetime policies
*********************

Different key lifetime policies control how keys can be used, managed, and revoked over the device lifecycle.
In the PSA abstraction, these policies are represented as key lifetimes configured with the ``psa_set_key_lifetime()`` function.

The following subsections describe two common lifetime policies for KMU-resident keys: revocable and locked.

Revocable keys
==============

You can set revocable keys as invalid for further use, which prevents the keys from being reused or new keys from being provisioned onto the same slots.
For these keys, the revocation policy (RPOLICY) must be marked as ``revoked``.
For some technical solutions, a few keys of the same key type (for example, generation 0, 1, 2) might be provisioned.
At any given time, only one generation should be active.
If a key generation is compromised, it should be revoked to prevent its use.
The specifics of the revocation scheme depend on the technical solution, but it is recommended to designate one generation as non-revocable (``locked``) to prevent a Denial of Service (DoS) attack on the key's availability for the solution.

Locked keys
===========

Once provisioned, locked keys are permanently available for use and cannot be deleted without erasing the device.
For these keys, the revocation policy (RPOLICY) must be marked as ``locked``.

Limitations on key lifetimes and trade-offs
===========================================

Access to the KMU is restricted to secure code.
When an SoC runs applications code as secure, the application has some control over the KMU.
A provisioned key cannot be overwritten, but its content might be blocked from use at runtime (push block mechanism) by unexpected routines.
However, applications can revoke a revocable key on the nRF54L15, nRF54L10, and nRF54L05 devices.
The revoked key might not necessarily belong to an application; it could, for example, belong to a bootloader.

Revocable KMU keys are exposed to revocation by applications running in secure mode.
If you cannot trust the application running in the secure mode, you should provision locked keys.
You can consider the following options:

* Supporting key generations with revocable keys - The advantage of this method is that the application or bootloader can revoke a key generation if the private key is compromised or lost.
  The disadvantage is that a malicious application could also revoke the key.
* Supporting one locked key - The advantage of this method is that the key cannot be deleted by the application.
  The disadvantage is that the method does not support multiple generations of keys.

Always consider the specific security needs of your application and choose the most appropriate key management approach to safeguard your digital assets.

.. _ug_kmu_guides_key_usage_schemes:

KMU key usage schemes
*********************

To see the supported key types with Protected, Encrypted and Raw usage schemes, refer to the table under :ref:`ug_kmu_guides_supported_key_types`.

The following list shows available schemes that determine how the keys are used:

+-------------+--------------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Scheme name | Macro name                                 | Description                                                                                                                          |
+=============+============================================+======================================================================================================================================+
| Protected   | ``CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED``  | The keys are pushed to a RAM that is only accessible by the CRACEN.                                                                  |
+-------------+--------------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Encrypted   | ``CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED``  | The keys are encrypted, and are decrypted on-the-fly to a CPU-accessible RAM location before being used by the CRACEN.               |
|             |                                            |                                                                                                                                      |
|             |                                            | Encrypted keys require two additional KMU slots to store the authentication nonce and tag.                                           |
+-------------+--------------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Raw         | ``CRACEN_KMU_KEY_USAGE_SCHEME_RAW``        | The keys are stored as plain text and pushed to a CPU-accessible RAM location before being used by the CRACEN.                       |
+-------------+--------------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Seed        | ``CRACEN_KMU_KEY_USAGE_SCHEME_SEED``       | The slots are pushed to CRACEN's SEED registers.                                                                                     |
|             |                                            |                                                                                                                                      |
|             |                                            | This scheme is typically not meant for the application use.                                                                          |
|             |                                            |                                                                                                                                      |
|             |                                            | It is only used for the platform keys, ``CRACEN_BUILTIN_IDENTITY_KEY_ID``, ``CRACEN_BUILTIN_MKEK_ID`` or ``CRACEN_BUILTIN_MEXT_ID``. |
+-------------+--------------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+

.. _ug_kmu_guides_kmu_storing_keys:

PSA Crypto API operations for storing keys in KMU
*************************************************

Applications can store keys in KMU slots using the standard PSA Crypto API key management operations, such as ``psa_import_key``, ``psa_generate_key``, or ``psa_copy_key``.
Additionally, you can provision the KMU slots :ref:`for development or for production <ug_kmu_provisioning_methods>`.
Provisioning for production requires manually setting up the :c:struct:`kmu_metadata` data structure for each key slot.

.. note::
   If a power failure occurs during provisioning of a key with persistence ``CRACEN_KEY_PERSISTENCE_READ_ONLY`` or ``CRACEN_KEY_PERSISTENCE_REVOKABLE``, it might not be possible to recover the key slot.
   Provisioning of read-only keys should be restricted to controlled environments (production environments).

You might encounter the following KMU-specific error codes when storing keys in KMU:

* ``PSA_ERROR_ALREADY_EXIST``: One of the required key slots has already been provisioned.
* ``PSA_ERROR_NOT_SUPPORTED``: Unsupported key type.

.. _ug_kmu_guides_kmu_metadata:

KMU metadata
============

After you store the keys in KMU, you need the CRACEN driver to be able to verify some properties of the key and its intended usage.
The driver can then use the key material for different operations, such as a decryption or a signature verification.

For the verification of the key, the CRACEN driver requires information from the METADATA field of the SRC data struct, as explained in the device datasheet (for example, `KMU - Key management unit <nRF54L15 Key management unit_>`_ in the nRF54L15 datasheet).
The encoding of information in the METADATA field is detailed in the :c:struct:`kmu_metadata` structure, a 32-bit bitfield defined in the CRACEN driver source code (:file:`nrf/subsys/nrf_security/src/drivers/cracen/cracenpsa/src/kmu.c`).

.. toggle:: How the CRACEN driver handles KMU metadata

   When the application uses ``psa_import_key`` or ``psa_generate_key`` to store a key in KMU slots, the CRACEN driver encodes :c:struct:`kmu_metadata` with the appropriate values based on the :ref:`PSA key attributes given to the functions <ug_crypto_kmu_psa_key_programming_model>` (:c:struct:`psa_key_attributes_t`).
   The CRACEN driver performs the following steps:

   1. Checks ``key_id`` to determine the KMU slot usage.
   #. Converts PSA key attributes to :c:struct:`kmu_metadata` bitfields.
   #. Sets the KMU slot's RPOLICY field (whether the key should be revokable, read-only, or deletable).
   #. Sets the KMU slot's DEST address to one of the following locations:

     * CRACEN's protected RAM if the key is a symmetric key (has the usage scheme set to :ref:`Protected <ug_kmu_guides_key_usage_schemes>`)
     * CRACEN's SEED register (for IKG keys)
     * The reserved RAM area ``kmu_push_area`` (0x20000000-0x20000060) for other keys

   #. Stores the :c:struct:`kmu_metadata` information in the KMU slot's METADATA field.
   #. Stores the key material in the KMU slot's VALUE field.

   When the keys are larger than 128 bits and span several consecutive KMU slots, the following rules apply:

   * Only the first slot (primary slot) contains the complete :c:struct:`kmu_metadata` structure.
   * Secondary slots have their METADATA field set to ``0xffffffff`` (``SECONDARY_SLOT_METADATA_VALUE``).
   * Each secondary slot's DEST address is incremented by 16 bytes (128 bits) compared to the previous slot.
   * All slots used by a key must have the same RPOLICY value.

KMU metadata handling in development and production
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

During development, you can use the tools for :ref:`ug_kmu_provisioning_development_tools` to automatically set up and encode the :c:struct:`kmu_metadata` data structure based on :c:struct:`psa_key_attributes_t` in a way that they match the required values for different key types and CRACEN driver's own requirements.

When programming for production, you need to manually make sure that the values set in the :c:struct:`kmu_metadata` data structure's bitfields match the required values.
See :ref:`ug_kmu_provisioning_production_tools` for more information.

PSA Crypto API operations for KMU key deletion and revocation
*************************************************************

You can delete or revoke keys using the ``psa_destroy_key`` function.
Calling the ``psa_destroy_key`` function on keys that have the persistence ``CRACEN_KEY_PERSISTENCE_REVOKABLE`` marks the associated KMU slots as revoked, preventing them from being reused for new keys.

PSA Crypto API operations for KMU key usage
*******************************************

Keys stored in the KMU can be used in standard PSA Crypto API operations for encryption, decryption, signing a hash or a message, and verifying a hash or a message, given that the corresponding ``PSA_KEY_USAGE_*`` flags are set.

Depending on the usage scheme:

* Keys with the usage schemes Protected (``CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED``) and Seed (``CRACEN_KMU_KEY_USAGE_SCHEME_SEED``) can push data from the slots directly to CRACEN registers that are not accessible by the CPU.
  These usage schemes are supported for pushing symmetric keys used for cipher operations, and for seeds used by the CRACEN IKG.

* Keys with the usage scheme Raw (``CRACEN_KMU_KEY_USAGE_SCHEME_RAW``), such as symmetric keys, are temporarily pushed to a RAM location by the CRACEN driver, and then loaded by CRACEN into the asymmetric engine before running operations like sign or verify.

* Key slots with the usage scheme Encrypted (``CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED``) also have to be decrypted to a temporary push location in RAM before they are used by CRACEN, which is handled by the CRACEN driver.

When the application is built with TF-M (for nRF54L Series devices that :ref:`support TF-M <ug_tfm_supported_services>`), this temporary push location is protected inside the secure processing environment to avoid exposing the key material to the non-secure application.
If TF-M is not used, the keys are pushed to a reserved RAM area at location 0x20000000-0x20000060 (``kmu_push_area``).

You might encounter the following KMU-specific error codes when using the KMU keys:

* ``PSA_ERROR_INVALID_HANDLE``: Attempting an operation on an empty KMU slot.
* ``PSA_ERROR_NOT_PERMITTED``: Attempting an operation on a revoked key.
* ``PSA_ERROR_HARDWARE_FAILURE``: The key slot has invalid data.
* ``PSA_ERROR_CORRUPTION_DETECTED``: The key slot has invalid data.
