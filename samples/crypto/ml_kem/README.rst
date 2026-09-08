.. _crypto_ml_kem:

Crypto: ML-KEM
###############

.. contents::
   :local:
   :depth: 2

The ML-KEM sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to generate shared secret key by using the ML-KEM-768 post-quantum key-encapsulation algorithm.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT` and :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT` - Used to enable support for importing and exporting ML-KEM key pairs from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM` - Used to enable support for the ML-KEM key encapsulation algorithm.
* :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768` - Used to enable support for the ML-KEM-768 key type.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128`, :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256`, :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`, and :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512` - Used to enable the hash and extendable-output functions required internally by ML-KEM.
* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES` and :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM` - Used to store the shared secret returned by the ML-KEM operations as an AES key.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using the :c:func:`psa_crypto_init` function.
   #. A known ML-KEM-768 key pair is imported using the :c:func:`psa_import_key` function.
      The key pair is configured with usage flags for encapsulation, decapsulation, and export.

#. ML-KEM key encapsulation and decapsulattion:

   a. The public key is derived from the imported key pair using the :c:func:`psa_export_public_key` function and imported using :c:func:`psa_import_key`.
   #. A shared secret and ciphertext are generated using the :c:func:`psa_encapsulate` function with the public key.
   #. The ciphertext is decapsulated using the :c:func:`psa_decapsulate` function with the key pair to generate a second shared secret.
   #. The shared secrets are exported using the :c:func:`psa_export_key` function and compared to verify that they match.

#. Cleanup:

   The ML-KEM key pair, public key, and shared secrets are removed from the PSA crypto keystore using the :c:func:`psa_destroy_key` function.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ml_kem`

.. include:: /includes/build_and_run.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.4.99-0fd29dba7821 ***
   *** Using Zephyr OS v4.4.99-3b0f33aa64fb ***
   [00:00:00.006,446] <inf> ml_kem: Starting ML-KEM example...
   [00:00:00.006,451] <inf> ml_kem: Importing an ML-KEM-768 key pair...
   [00:00:00.006,475] <inf> ml_kem: ML-KEM-768 key pair imported successfully!
   [00:00:00.006,488] <inf> ml_kem: ---- ML-KEM-768 key pair seed (total len: 64, printing 16 bytes): ----
   [00:00:00.006,496] <inf> ml_kem: Content:
                                    6d bb c4 37 51 36 df 3b  07 f7 c7 0e 63 9e 22 3e |m..7Q6.; ....c.">
   [00:00:00.006,504] <inf> ml_kem: ---- ML-KEM-768 key pair seed end  ----
   [00:00:00.012,532] <inf> ml_kem: ML-KEM-768 public key extracted successfully!
   [00:00:00.012,546] <inf> ml_kem: ---- ML-KEM-768 public key (total len: 1184, printing 16 bytes): ----
   [00:00:00.012,554] <inf> ml_kem: Content:
                                    01 f6 0a f1 dc 8e 63 60  ae 78 b5 9d 4a 50 42 eb |......c` .x..JPB.
   [00:00:00.012,562] <inf> ml_kem: ---- ML-KEM-768 public key end  ----
   [00:00:00.020,626] <inf> ml_kem: ---- Ciphertext (total len: 1088, printing 16 bytes): ----
   [00:00:00.020,636] <inf> ml_kem: Content:
                                    42 4d c3 50 e7 3f 42 f8  aa ec 98 16 4d a8 21 38 |BM.P.?B. ....M.!8
   [00:00:00.020,644] <inf> ml_kem: ---- Ciphertext end  ----
   [00:00:00.020,647] <inf> ml_kem: ML-KEM encapsulation was successful!
   [00:00:00.036,514] <inf> ml_kem: ---- Shared secret (total len: 32, printing 16 bytes): ----
   [00:00:00.036,524] <inf> ml_kem: Content:
                                    f4 d5 cb 08 64 0f f8 62  a5 3a 15 8d 0a c9 31 17 |....d..b .:....1.
   [00:00:00.036,533] <inf> ml_kem: ---- Shared secret end  ----
   [00:00:00.036,536] <inf> ml_kem: Shared secrets match!
   [00:00:00.036,577] <inf> ml_kem: Example finished successfully!
