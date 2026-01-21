.. _crypto_eddsa:

Crypto: EdDSA
#############

.. contents::
   :local:
   :depth: 2

The EdDSA sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to sign and verify messages using the EdDSA algorithm with a 255-bit ECC key pair on the Edwards25519 curve.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`, :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`, :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT` - Used to enable support for ECC key pair types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_ecc_key_pair`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` - Used to enable support for the EdDSA signature algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_signature_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255` - Used to enable support for the Edwards25519 curve from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_ecc_curve_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512` - Used to enable support for the SHA-512 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 255-bit ECC key pair is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key pair is configured with usage flags for signing.
   #. The public key is exported using :c:func:`psa_export_public_key`.
   #. The exported public key is imported using :c:func:`psa_import_key` for verification purposes.
      The public key is configured with usage flags for verification.

#. EdDSA signing and verification:

   a. The message is signed using :c:func:`psa_sign_message` with the ``PSA_ALG_PURE_EDDSA`` algorithm.
   #. The signature is verified using :c:func:`psa_verify_message` with the imported public key.

#. Cleanup:

   a. The ECC key pair and public key are removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/eddsa`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> eddsa: Starting EdDSA example...
   [00:00:00.251,190] <inf> eddsa: Generating random EdDSA key pair...
   [00:00:00.251,342] <inf> eddsa: EdDSA key pair generated successfully!
   [00:00:00.251,373] <inf> eddsa: Signing a message using the EdDSA algorithm...
   [00:00:00.251,708] <inf> eddsa: Message signed successfully!
   [00:00:00.251,739] <inf> eddsa: ---- Plaintext (len: 100): ----
   [00:00:00.251,770] <inf> eddsa: Content:
                                    Example string to demonstrate basic usage of EdDSA.
   [00:00:00.251,800] <inf> eddsa: ---- Plaintext end  ----
   [00:00:00.251,831] <inf> eddsa: ---- Signature (len: 64): ----
   [00:00:00.251,861] <inf> eddsa: Content:
                                    cc 7d c0 ed 63 5b df 28  08 2b 03 33 a4 3c dc 1d |.}..c[.( .+.3.<..
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
                                    39 56 54 b5 96 6e 13 e2  7d 22 26 1e 3c 7c 3e eb |9VT..n.. }"&.<|>.
                                    15 60 31 d3 58 02 b6 85  98 63 2c e6 ad dc aa 19 |.`1.X... .c,.....
   [00:00:00.251,892] <inf> eddsa: ---- Signature end  ----
   [00:00:00.251,922] <inf> eddsa: Verifying the EdDSA signature...
   [00:00:00.252,045] <inf> eddsa: Signature verification was successful!
   [00:00:00.252,075] <inf> eddsa: Example finished successfully!
