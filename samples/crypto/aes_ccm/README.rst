.. _crypto_aes_ccm:

Crypto: AES CCM
###############

.. contents::
   :local:
   :depth: 2

The AES CCM sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform authenticated encryption and decryption operations using the CCM AEAD algorithm with a 128-bit AES key.
The sample uses additional authenticated data (AAD) and a random nonce.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES` - Used to enable support for AES key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM` - Used to enable support for the CCM AEAD algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_aead_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable random number generation for key generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

The sample also configures the cryptographic drivers for each board target using Kconfig options in the overlay files in the :file:`boards` directory.

These Kconfig options are then used by Oberon PSA Crypto to compile the required cryptographic PSA directives and select the cryptographic drivers.
See :ref:`crypto_drivers_driver_selection` for more information about the driver selection process.

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 128-bit AES key is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for both encryption and decryption.

#. Authenticated encryption and decryption of a sample plaintext:

   a. Authenticated encryption is performed using :c:func:`psa_aead_encrypt` with the ``PSA_ALG_CCM`` algorithm.
      This function encrypts the plaintext with the provided nonce and additional authenticated data, and appends an authentication tag to the ciphertext.
   #. Authenticated decryption is performed using :c:func:`psa_aead_decrypt`.
      This function decrypts the ciphertext, verifies the authentication tag, and authenticates the additional data.
   #. The decrypted text is compared with the original plaintext to verify correctness.

#. Cleanup:

   a. The AES key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_ccm`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block::

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> aes_ccm: Starting AES CCM example...
   [00:00:00.251,190] <inf> aes_ccm: Generating random AES key...
   [00:00:00.251,342] <inf> aes_ccm: AES key generated successfully!
   [00:00:00.251,373] <inf> aes_ccm: Encrypting using AES CCM MODE...
   [00:00:00.251,708] <inf> aes_ccm: Encryption successful!
   [00:00:00.251,708] <inf> aes_ccm: ---- Nonce (len: 13): ----
   [00:00:00.251,739] <inf> aes_ccm: Content:
                                    SAMPLE NONCE
   [00:00:00.251,770] <inf> aes_ccm: ---- Nonce end  ----
   [00:00:00.251,800] <inf> aes_ccm: ---- Additional data (len: 35): ----
   [00:00:00.251,831] <inf> aes_ccm: Content:
                                    Example string of additional data
   [00:00:00.251,831] <inf> aes_ccm: ---- Additional data end  ----
   [00:00:00.251,861] <inf> aes_ccm: ---- Encrypted text (len: 115): ----
   [00:00:00.251,892] <inf> aes_ccm: Content:
                                    c4 36 60 2b 45 59 5b 12  35 00 00 00 00 00 00 00 |.6.+EY[..5.......
                                b3 5d 47 06 89 a5 08 3b  e6 54 57 25 b9 49 02 50 |.]G....;.TW%.I.P
                                d1 55 49 58 11 00 00 00  00 00 00 00 00 00 00 00 |.UX.............
   [00:00:00.251,922] <inf> aes_ccm: ---- Encrypted text end  ----
   [00:00:00.251,953] <inf> aes_ccm: Decrypting using AES CCM MODE...
   [00:00:00.252,166] <inf> aes_ccm: ---- Decrypted text (len: 100): ----
   [00:00:00.252,197] <inf> aes_ccm: Content:
                                    Example string to demonstrate basic usage of AES CCM mode.
   [00:00:00.252,227] <inf> aes_ccm: ---- Decrypted text end  ----
   [00:00:00.252,258] <inf> aes_ccm: Decryption and authentication successful!
   [00:00:00.252,288] <inf> aes_ccm: Example finished successfully!
