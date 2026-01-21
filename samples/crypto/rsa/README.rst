.. _crypto_rsa:

Crypto: RSA
###########

.. contents::
   :local:
   :depth: 2

The RSA sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to sign and verify messages using the RSA signature algorithm with SHA-256 as the hashing algorithm and a 4096-bit RSA key pair.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT` and :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT` - Used to enable support for RSA key pair types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_pair_ops`.
* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY` - Used to enable support for RSA public key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN` - Used to enable support for the RSA PKCS#1 v1.5 signature algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_signature_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096` - Used to enable support for 4096-bit RSA key sizes from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rsa_key_sizes`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. An RSA key pair is imported into the PSA crypto keystore using :c:func:`psa_import_key`.
      The key pair is configured with usage flags for hash signing.

      .. note::
         The keys are provided in the :file:`key.h` file in the sample directory.
         These are for demonstration purposes only and must never be used in a product.

   #. The public key is imported into the PSA crypto keystore using :c:func:`psa_import_key`.
      The public key is configured with usage flags for hash verification.

#. RSA signing and verification:

   a. The SHA-256 hash of the plaintext is computed using :c:func:`psa_hash_compute`.
   #. The hash is signed using :c:func:`psa_sign_hash` with the ``PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256)`` algorithm.
   #. The signature is verified using :c:func:`psa_verify_hash` with the imported public key.

#. Cleanup:

   a. The RSA key pair and public key are removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/rsa`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> rsa: Starting RSA example...
   [00:00:00.251,190] <inf> rsa: Importing RSA key pair...
   [00:00:00.251,342] <inf> rsa: RSA private key imported successfully!
   [00:00:00.251,373] <inf> rsa: RSA public key imported successfully!
   [00:00:00.251,404] <inf> rsa: Signing a message using RSA...
   [00:00:00.251,708] <inf> rsa: Signing was successful!
   [00:00:00.251,739] <inf> rsa: ---- Plaintext (len: 100): ----
   [00:00:00.251,770] <inf> rsa: Content:
                                    Example string to demonstrate basic usage of RSA.
   [00:00:00.251,800] <inf> rsa: ---- Plaintext end  ----
   [00:00:00.251,831] <inf> rsa: ---- SHA256 hash (len: 32): ----
   [00:00:00.251,861] <inf> rsa: Content:
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
   [00:00:00.251,892] <inf> rsa: ---- SHA256 hash end  ----
   [00:00:00.251,922] <inf> rsa: ---- Signature (len: 512): ----
   [00:00:00.251,953] <inf> rsa: Content:
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
                                    [additional signature bytes...]
   [00:00:00.252,075] <inf> rsa: ---- Signature end  ----
   [00:00:00.252,105] <inf> rsa: Verifying RSA signature...
   [00:00:00.252,136] <inf> rsa: Signature verification was successful!
   [00:00:00.252,166] <inf> rsa: Example finished successfully!
