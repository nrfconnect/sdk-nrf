.. _crypto_persistent_key:

Crypto: Persistent key usage
############################

.. contents::
   :local:
   :depth: 2

The persistent key sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to generate and use persistent keys that are stored in the Internal Trusted Storage (ITS) of the device and retain their value between resets.
The implementation of the PSA ITS API is provided in one of the following ways, depending on your configuration:

* Through TF-M using Internal Trusted Storage and Protected Storage services.
* When building without TF-M: using either Zephyr's :ref:`secure_storage` subsystem or the :ref:`trusted_storage_readme` library.

A persistent key becomes unusable when the ``psa_destroy_key`` function is called.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. note::
   |encrypted_its_not_supported_on_nrf54lm20|

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_STORAGE_C` - Used to enable support for the `PSA Certified Secure Storage API`_.
* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES` - Used to enable support for AES key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR` - Used to enable support for the CTR mode algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_cipher_modes`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

In this sample, an AES 128-bit key is created.
Persistent keys can be of any type supported by the PSA APIs.

Builds without TF-M use the :ref:`secure_storage` subsystem as the PSA Secure Storage API provider.
The :ref:`lib_hw_unique_key` is used to encrypt the key before storing it.

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 128-bit AES key is generated using :c:func:`psa_generate_key` with persistent lifetime.
      The key is configured with usage flags for encryption and decryption.

#. Key management:

   a. The key is purged from RAM using :c:func:`psa_purge_key` to simulate device reset behavior.
   #. The persistent key is used for encryption and decryption operations.

#. Encryption and decryption:

   a. A message is encrypted using :c:func:`psa_cipher_encrypt` with the persistent key.
   #. The encrypted message is decrypted using :c:func:`psa_cipher_decrypt` with the persistent key.

#. Cleanup:

   a. The persistent AES key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

.. note::
   The read-only type of persistent keys cannot be destroyed with the ``psa_destroy_key`` function.
   The ``PSA_KEY_PERSISTENCE_READ_ONLY`` macro is used for read-only keys.
   The key ID of a read-only key is writable again after a full erase of the device memory.
   Use the ``west -v flash --erase`` command for the full erase.

.. note::
   Builds without TF-M and all nRF54L15 builds use the :ref:`hardware unique key (HUK) <lib_hw_unique_key>` to encrypt the key before storing it.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/persistent_key_usage`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> persistent_key_usage: Starting persistent key example...
   [00:00:00.251,190] <inf> persistent_key_usage: Generating random persistent AES key...
   [00:00:00.251,342] <inf> persistent_key_usage: Persistent key generated successfully!
   [00:00:00.251,373] <inf> persistent_key_usage: Encryption successful!
   [00:00:00.251,404] <inf> persistent_key_usage: ---- Plaintext (len: 100): ----
   [00:00:00.251,434] <inf> persistent_key_usage: Content:
                                    Example string to demonstrate basic usage of a persistent key.
   [00:00:00.251,465] <inf> persistent_key_usage: ---- Plaintext end  ----
   [00:00:00.251,495] <inf> persistent_key_usage: ---- Encrypted text (len: 100): ----
   [00:00:00.251,526] <inf> persistent_key_usage: Content:
                                    a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
   [00:00:00.251,556] <inf> persistent_key_usage: ---- Encrypted text end  ----
   [00:00:00.251,587] <inf> persistent_key_usage: ---- Decrypted text (len: 100): ----
   [00:00:00.251,617] <inf> persistent_key_usage: Content:
                                    Example string to demonstrate basic usage of a persistent key.
   [00:00:00.251,648] <inf> persistent_key_usage: ---- Decrypted text end  ----
   [00:00:00.251,678] <inf> persistent_key_usage: Decryption successful!
   [00:00:00.251,709] <inf> persistent_key_usage: Example finished successfully!
