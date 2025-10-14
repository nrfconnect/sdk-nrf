.. _crypto_aes_ctr:

Crypto: AES CTR
###############

.. contents::
   :local:
   :depth: 2

The AES CTR sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform AES encryption and decryption operations using the CTR block cipher mode and a 128-bit AES key.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES` - Used to enable support for AES key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR` - Used to enable support for the CTR cipher mode from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_cipher_modes`.
* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable random number generation for key and IV generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

The sample also configures the cryptographic drivers for each board target using Kconfig options in the overlay files in the :file:`boards` directory.

These Kconfig options are then used by Oberon PSA Crypto to compile the required cryptographic PSA directives and select the cryptographic drivers.
See :ref:`crypto_drivers_driver_selection` for more information about the driver selection process.

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 128-bit AES key is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for both encryption and decryption.

#. Encryption and decryption of a sample plaintext:

   a. An encryption operation is set up using :c:func:`psa_cipher_encrypt_setup` with the ``PSA_ALG_CTR`` algorithm.
   #. A random initialization vector (IV) is generated using :c:func:`psa_cipher_generate_iv`.
   #. Encryption is performed using :c:func:`psa_cipher_update` and finalized with :c:func:`psa_cipher_finish`.
   #. A decryption operation is set up using :c:func:`psa_cipher_decrypt_setup`.
   #. The IV from the encryption step is set using :c:func:`psa_cipher_set_iv`.
   #. Decryption is performed using :c:func:`psa_cipher_update` and finalized with :c:func:`psa_cipher_finish`.
   #. The decrypted text is compared with the original plaintext to verify correctness.

#. Cleanup:

   a. The AES key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_ctr`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.123,456] <inf> aes_ctr: Starting AES CTR example...
   [00:00:00.123,489] <inf> aes_ctr: Generating random AES key...
   [00:00:00.123,512] <inf> aes_ctr: AES key generated successfully!
   [00:00:00.123,545] <inf> aes_ctr: Encrypting using AES CTR MODE...
   [00:00:00.123,567] <inf> aes_ctr: Encryption successful!
   [00:00:00.123,589] <inf> aes_ctr: ---- IV (len: 16): ----
   [00:00:00.123,611] <inf> aes_ctr: Content:
                                01 02 03 04 05 06 07 08  09 0a 0b 0c 0d 0e 0f 10 |................
   [00:00:00.123,633] <inf> aes_ctr: ---- IV end  ----
   [00:00:00.123,655] <inf> aes_ctr: ---- Encrypted text (len: 56): ----
   [00:00:00.123,677] <inf> aes_ctr: Content:
                                5e 3a a2 c6 2f 8d 57 89  3c 90 1e a0 b2 d5 6c 48 |^:../.W.<.....lH
                                d8 61 aa 35 4b da 09 83  a4 f6 18 e3 0b dd 92 0c |.a.5K...........
                                7b 61 95 44 09 64 ea ef  ad b8 72 59 65 4f 6a 7c |{a.D.d....rYeOj|
                                7f 81 f4 2a 3b 9d 3e 66  42 e5 db 87 4c 16        |...*;..fB...L.
   [00:00:00.123,699] <inf> aes_ctr: ---- Encrypted text end  ----
   [00:00:00.123,721] <inf> aes_ctr: Decrypting using AES CTR MODE...
   [00:00:00.123,743] <inf> aes_ctr: ---- Decrypted text (len: 56): ----
   [00:00:00.123,765] <inf> aes_ctr: Content:
                                Example string to demonstrate basic usage of AES CTR mode.
   [00:00:00.123,787] <inf> aes_ctr: ---- Decrypted text end  ----
   [00:00:00.123,809] <inf> aes_ctr: Decryption successful!
   [00:00:00.123,831] <inf> aes_ctr: Example finished successfully!
