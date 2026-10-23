.. _lib_tls_credentials_keygen_kmu:

TLS credentials KMU key store
#############################

.. contents::
   :local:
   :depth: 2

The TLS credentials KMU key store library stores the TLS private keys that the device generates in the :term:`CRACEN` Key Management Unit (KMU).
The library is :ref:`experimental <software_maturity>`.

Overview
********

The Zephyr TLS credentials key generation feature (:kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`) generates a private key on the device and registers it for a security tag.
It uses a key store to decide where the key is stored and which PSA key ID it gets.
This library is a key store for devices with a CRACEN KMU, for example, the nRF71 Series and nRF54LM20 devices.

The library has the following features:

* The private key is stored in the KMU and it cannot be exported.
  For how CRACEN uses a KMU key, see :ref:`ug_kmu_guides_key_usage_schemes`.
* The key ID is a KMU key ID, so TLS, the certificate signing request (CSR), and JSON Web Tokens (JWT) use the key in the same way as a key in the default PSA key location.
* Deleting a key frees its KMU slots.

Each key uses a block of KMU slots that fits a P-384 key pair, even when P-384 keys are not enabled.
This way, enabling P-384 in a firmware update does not move the existing keys.
The block size depends on the key usage scheme, see :ref:`ug_kmu_guides_supported_key_types`.
The following table shows the number of slots that each key uses for each key usage scheme:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Key usage scheme
     - Slots for each key
   * - Encrypted
     - 5 slots
   * - Raw
     - 3 slots

The library owns a range of KMU slots that starts at :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST` and contains one block for each of the :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_KEY_COUNT` keys.
Do not use these slots for anything else.
Each key ID is built with the :c:macro:`PSA_KEY_ID_FROM_CRACEN_KMU_SLOT` macro from the key usage scheme and the first slot of the key's block.
For more information about KMU key IDs, see :ref:`ug_crypto_kmu_psa_key_programming_model`.

Configuration
*************

The library is enabled by default when TLS credentials key generation is enabled on a device with a CRACEN KMU.
On such devices, :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_CUSTOM` is the default key store option.
To keep the keys in the default PSA key location instead, enable the :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_DEFAULT` Kconfig option.

Configure the following Kconfig options:

* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SCHEME_ENCRYPTED` - Stores the key encrypted with a key that is derived from the CRACEN IKG seed.
  This is the default scheme.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SCHEME_RAW` - Stores the key in plaintext in the KMU.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST` - Sets the first KMU slot of the range.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_KEY_COUNT` - Sets the maximum number of keys.

Usage
*****

The library has no API of its own.
To generate, use, and delete keys, see the TLS credentials on-device key generation section on the :ref:`zephyr:bsd_sockets_interface` page and the :zephyr:code-sample:`tls-keygen` sample.

Limitations
***********

The key ID contains the KMU slot and the key usage scheme.
Do not change :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST` or the key usage scheme after keys are generated, for example in a firmware update.
After such a change, the library cannot use or delete the existing keys.
Before you make the change, delete the keys with :c:func:`tls_credential_keygen_delete`.

Dependencies
************

This library uses the following components:

* The Zephyr TLS credentials key generation feature (:kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`).
* The :ref:`CRACEN driver <crypto_drivers_cracen>` with KMU support (:kconfig:option:`CONFIG_CRACEN_KMU`).
