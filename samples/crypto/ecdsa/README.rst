.. _crypto_ecdsa:

Crypto: ECDSA
#############

.. contents::
   :local:
   :depth: 2

The ECDSA sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to sign and verify messages using the ECDSA algorithm with SHA-256 as the hashing algorithm and a 256-bit ECC key pair on the secp256r1 curve.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`, :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`, :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT` - Used to enable support for ECC key pair types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` - Used to enable support for the ECDSA signature algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_signature_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 256-bit ECC key pair is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key pair is configured with usage flags for signing.
   #. The public key is exported using :c:func:`psa_export_public_key`.
   #. The exported public key is imported using :c:func:`psa_import_key` for verification purposes.
      The public key is configured with usage flags for verification.

#. ECDSA signing and verification:

   a. A SHA-256 hash is computed using :c:func:`psa_hash_compute`.
   #. The hash is signed using :c:func:`psa_sign_hash` with the ``PSA_ALG_ECDSA(PSA_ALG_SHA_256)`` algorithm.
   #. The signature is verified using :c:func:`psa_verify_hash` with the imported public key.

#. Cleanup:

   a. The ECC key pair and public key are removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdsa`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> ecdsa: Starting ECDSA example...
   [00:00:00.251,190] <inf> ecdsa: Generating random ECDSA key pair...
   [00:00:00.251,342] <inf> ecdsa: ECDSA key pair generated successfully!
   [00:00:00.251,373] <inf> ecdsa: Signing a message using the ECDSA algorithm...
   [00:00:00.251,708] <inf> ecdsa: Message signed successfully!
   [00:00:00.251,739] <inf> ecdsa: ---- Plaintext (len: 100): ----
   [00:00:00.251,770] <inf> ecdsa: Content:
                                    Example string to demonstrate basic usage of ECDSA.
   [00:00:00.251,800] <inf> ecdsa: ---- Plaintext end  ----
   [00:00:00.251,831] <inf> ecdsa: ---- SHA256 hash (len: 32): ----
   [00:00:00.251,861] <inf> ecdsa: Content:
                                    c3 1e 5b 35 97 25 ee a3  ef ba 66 c3 f9 81 37 2a |..[5.%.. ..f...7*
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
   [00:00:00.251,892] <inf> ecdsa: ---- SHA256 hash end  ----
   [00:00:00.251,922] <inf> ecdsa: ---- Signature (len: 64): ----
   [00:00:00.251,953] <inf> ecdsa: Content:
                                    cc 7d c0 ed 63 5b df 28  08 2b 03 33 a4 3c dc 1d |.}..c[.( .+.3.<..
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
                                    39 56 54 b5 96 6e 13 e2  7d 22 26 1e 3c 7c 3e eb |9VT..n.. }"&.<|>.
                                    15 60 31 d3 58 02 b6 85  98 63 2c e6 ad dc aa 19 |.`1.X... .c,.....
   [00:00:00.251,984] <inf> ecdsa: ---- Signature end  ----
   [00:00:00.252,014] <inf> ecdsa: Verifying the ECDSA signature...
   [00:00:00.252,045] <inf> ecdsa: Signature verification was successful!
   [00:00:00.252,075] <inf> ecdsa: Example finished successfully!
