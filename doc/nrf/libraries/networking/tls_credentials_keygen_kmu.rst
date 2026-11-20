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

The Zephyr TLS credentials key generation (:kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`) generates a private key on the device and registers it for a security tag.
It uses a key store to decide where the key is stored and which PSA key ID it gets.
This library is a key store for devices with a CRACEN KMU, for example, the nRF71 Series and nRF54LM20 devices.

The library has the following features:

* The private key is generated in the KMU, so it is not stored in flash memory and only CRACEN can use it.
* The key ID is a KMU key ID, so TLS, the certificate signing request (CSR), and JSON Web Tokens (JWT) use the key in the same way as a key in the default PSA key location.
* Deleting a key frees its KMU slots.

Each key uses a block of KMU slots that fits a P-384 key, also when P-384 keys are not enabled:

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Key usage scheme
     - Slots for each key
   * - Encrypted
     - 5 slots
   * - Raw
     - 3 slots

The library owns the KMU slots from :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST`, one block for each of :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_KEY_COUNT` keys.
Do not use these slots for anything else.
A key in these slots that no security tag is registered for is not destroyed, and its slots are not used for a new key.
To free the slots, delete the key with :c:func:`tls_credential_keygen_delete` or the ``cred keydel`` shell command.
Deleting only the registration with :c:func:`tls_credential_delete` does not free the slots.
To free the slots of such a key, call the ``psa_destroy_key`` function with the key ID from ``PSA_KEY_ID_FROM_CRACEN_KMU_SLOT``, using the key usage scheme and the first slot of the block.

The key ID contains the KMU slot and the key usage scheme.
Do not change :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST` or the key usage scheme after keys are generated, for example in a firmware update.
After such a change, the library cannot use or delete the existing keys.
Delete the keys with :c:func:`tls_credential_keygen_delete` before the change.

Configuration
*************

The library is enabled by default when the TLS credentials key generation is enabled on a device with a CRACEN KMU.
On such devices, the :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_CUSTOM` Kconfig option is the default key store.
To keep the keys in the default PSA key location instead, enable the :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KEY_STORE_DEFAULT` Kconfig option.

Configure the following Kconfig options:

* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SCHEME_ENCRYPTED` - The key is stored encrypted with a key that is derived from the CRACEN IKG seed (default).
  The seed must be provisioned.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SCHEME_RAW` - The key is stored in plaintext in the KMU.
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST` - The first KMU slot of the keys.
  On nRF71 Series devices, the slots must not overlap the Wi-Fi KMU slots (:kconfig:option:`CONFIG_NRF_WIFI_KMU_SLOT_MIN`).
* :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_KEY_COUNT` - The maximum number of keys.

The slots must end before KMU slot 180, which the non-secure image cannot use.
The build fails if the slots overlap the Wi-Fi KMU slots or reach slot 180.
