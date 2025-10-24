.. _crypto_pbkdf2:

Crypto: PBKDF2
##############

.. contents::
   :local:
   :depth: 2

The PBKDF2 sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to derive keys using the PBKDF2 algorithm with HMAC-SHA-256 as the underlying pseudorandom function.
The sample uses a sample password, salt, and iteration count to derive a new key.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC` - Used to enable support for the PBKDF2 key derivation algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_kdf_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` - Used to enable support for the HMAC algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_mac_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. The input password is imported into the PSA crypto keystore using :c:func:`psa_import_key`.
      The password is configured with usage flags for key derivation.

#. Key derivation:

   a. The PBKDF2 key derivation operation is set up using :c:func:`psa_key_derivation_setup` with the ``PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256)`` algorithm.
   #. The iteration count is provided using :c:func:`psa_key_derivation_input_integer`.
   #. The salt is provided using :c:func:`psa_key_derivation_input_bytes`.
   #. The input password is provided using :c:func:`psa_key_derivation_input_key`.
   #. The derived key is generated using :c:func:`psa_key_derivation_output_bytes`.
   #. The derivation operation is cleaned up using :c:func:`psa_key_derivation_abort`.

#. Cleanup:

   a. The input password is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/pbkdf2`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> pkbdf2: Starting PBKDF2 example...
   [00:00:00.251,190] <inf> pkbdf2: Compare derived key with expected value...
   [00:00:00.251,342] <inf> pkbdf2: Key derivation successful!
   [00:00:00.251,373] <inf> pkbdf2: ---- Password (len: 6): ----
   [00:00:00.251,404] <inf> pkbdf2: Content:
                                   70 61 73 73 77 64 |passwd|
   [00:00:00.251,434] <inf> pkbdf2: ---- Password end  ----
   [00:00:00.251,465] <inf> pkbdf2: ---- Salt (len: 4): ----
   [00:00:00.251,495] <inf> pkbdf2: Content:
                                   73 61 6c 74 |salt|
   [00:00:00.251,526] <inf> pkbdf2: ---- Salt end  ----
   [00:00:00.251,556] <inf> pkbdf2: Iteration count: 1
   [00:00:00.251,587] <inf> pkbdf2: ---- Derived key (len: 64): ----
   [00:00:00.251,617] <inf> pkbdf2: Content:
                                   55 ac 04 6e 56 e3 08 9f  ec 16 91 c2 25 44 b6 05 |U..nV.....%D..|
                                   f9 41 85 21 6d de 04 65  e6 8b 9d 57 c2 0d ac bc |.A.!m..e...W....|
                                   49 ca 9c cc f1 79 b6 45  99 16 64 b3 9d 77 ef 31 |I....y.E..d..w.1|
                                   7c 71 b8 45 b1 e3 0b d5  09 11 20 41 d3 a1 97 83 |.q.E...... A....|
   [00:00:00.251,648] <inf> pkbdf2: ---- Derived key end  ----
   [00:00:00.251,678] <inf> pkbdf2: Example finished successfully!
