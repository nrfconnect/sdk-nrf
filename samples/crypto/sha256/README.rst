.. _crypto_sha256:

Crypto: SHA-256
###############

.. contents::
   :local:
   :depth: 2

The SHA-256 sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to compute and verify hashes using the SHA-256 hash algorithm.
The sample demonstrates both single-part and multi-part hash operations.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.

#. Single-part hash computation and verification:

   a. The SHA-256 hash of the plaintext is computed using :c:func:`psa_hash_compute` in a single operation.
   #. The computed hash is verified using :c:func:`psa_hash_compare`.

#. Multi-part hash computation and verification:

   a. A multi-part hash operation is set up using :c:func:`psa_hash_setup`.
   #. The input data is processed in chunks using :c:func:`psa_hash_update` (three chunks of 42, 58, and 50 bytes).
   #. The hash is finalized using :c:func:`psa_hash_finish`.
   #. The computed hash is verified using :c:func:`psa_hash_compare`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/sha256`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> sha256: Starting SHA-256 example...
   [00:00:00.251,190] <inf> sha256: ---- Plaintext to hash (len: 150): ----
   [00:00:00.251,220] <inf> sha256: Content:
                                    Example string to demonstrate basic usage of SHA256.
                                    That uses single and multi-part PSA crypto API's to
                                    perform a SHA-256 hashing operation.
   [00:00:00.251,251] <inf> sha256: ---- Plaintext to hash end  ----
   [00:00:00.251,281] <inf> sha256: Hashing using SHA-256...
   [00:00:00.251,312] <inf> sha256: Hash computation successful!
   [00:00:00.251,342] <inf> sha256: ---- SHA-256 hash (len: 32): ----
   [00:00:00.251,373] <inf> sha256: Content:
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
   [00:00:00.251,404] <inf> sha256: ---- SHA-256 hash end  ----
   [00:00:00.251,434] <inf> sha256: Verifying the SHA-256 hash...
   [00:00:00.251,465] <inf> sha256: SHA-256 verification successful!
   [00:00:00.251,495] <inf> sha256: Hashing using multi-part SHA-256...
   [00:00:00.251,526] <inf> sha256: Added 42 bytes
   [00:00:00.251,556] <inf> sha256: Added 58 bytes
   [00:00:00.251,587] <inf> sha256: Added 50 bytes
   [00:00:00.251,617] <inf> sha256: Hash computation successful!
   [00:00:00.251,648] <inf> sha256: ---- SHA-256 hash (len: 32): ----
   [00:00:00.251,678] <inf> sha256: Content:
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
   [00:00:00.251,709] <inf> sha256: Verifying the SHA-256 hash...
   [00:00:00.251,739] <inf> sha256: SHA-256 verification successful!
   [00:00:00.251,770] <inf> sha256: Example finished successfully!
