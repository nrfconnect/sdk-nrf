.. _crypto_hkdf:

Crypto: HKDF
############

.. contents::
   :local:
   :depth: 2

The HKDF sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to derive keys using the HKDF algorithm with SHA-256 as the underlying hash function.
The sample uses a sample key, salt, and additional data to derive a new key.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF` - Used to enable support for the HKDF key derivation algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_kdf_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` - Used to enable support for the HMAC algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_mac_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HMAC` - Used to enable support for the HMAC key type from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. The input key is imported into the PSA crypto keystore using :c:func:`psa_import_key`.
      The key is configured with usage flags for key derivation.

#. Key derivation:

   a. The HKDF key derivation operation is set up using :c:func:`psa_key_derivation_setup` with the ``PSA_ALG_HKDF(PSA_ALG_SHA_256)`` algorithm.
   #. The salt is provided using :c:func:`psa_key_derivation_input_bytes`.
   #. The input key is provided using :c:func:`psa_key_derivation_input_key`.
   #. Additional information is provided using :c:func:`psa_key_derivation_input_bytes`.
   #. The derived key is generated using :c:func:`psa_key_derivation_output_key`.
   #. The derivation operation is cleaned up using :c:func:`psa_key_derivation_abort`.

#. Cleanup:

   a. The input and output keys are removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/hkdf`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> hkdf: Starting HKDF example...
   [00:00:00.251,190] <inf> hkdf: Deriving a key using HKDF with SHA-256...
   [00:00:00.251,342] <inf> hkdf: Key derivation successful!
   [00:00:00.251,373] <inf> hkdf: ---- Input key (len: 22): ----
   [00:00:00.251,404] <inf> hkdf: Content:
                                   0b 0b 0b 0b 0b 0b 0b 0b  0b 0b 0b 0b 0b 0b 0b 0b |................
                                   0b 0b 0b 0b 0b 0b 0b     |.......|
   [00:00:00.251,434] <inf> hkdf: ---- Input key end  ----
   [00:00:00.251,465] <inf> hkdf: ---- Salt (len: 13): ----
   [00:00:00.251,495] <inf> hkdf: Content:
                                   00 01 02 03 04 05 06 07  08 09 0a 0b 0c |.............|
   [00:00:00.251,526] <inf> hkdf: ---- Salt end  ----
   [00:00:00.251,556] <inf> hkdf: ---- Additional data (len: 10): ----
   [00:00:00.251,587] <inf> hkdf: Content:
                                   f0 f1 f2 f3 f4 f5 f6 f7  f8 f9 |..........|
   [00:00:00.251,617] <inf> hkdf: ---- Additional data end  ----
   [00:00:00.251,648] <inf> hkdf: Compare derived key with expected value...
   [00:00:00.251,678] <inf> hkdf: ---- Output key (len: 42): ----
   [00:00:00.251,709] <inf> hkdf: Content:
                                   3c b2 5f 25 fa ac d5 7a  90 43 4f 64 d0 36 2f 2a |<._%...z.COd.6/*
                                   2d 2d 0a 90 cf 1a 5a 4c  5d b0 2d 56 ec c4 c5 bf |--....ZL].-V....
                                   34 00 72 08 d5 b8 87 18  58 65 |4.r.....Xe|
   [00:00:00.251,739] <inf> hkdf: ---- Output key end  ----
   [00:00:00.251,770] <inf> hkdf: Example finished successfully!
