.. _crypto_ml_dsa:

Crypto: ML-DSA
###############

.. contents::
   :local:
   :depth: 2

The ML-DSA sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to verify a message signature using the ML-DSA-65 post-quantum signature algorithm.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY` - Used to enable support for the ML-DSA public key type from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA` - Used to enable support for the ML-DSA signature algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_signature_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65` - Used to enable support for the ML-DSA-65 key type.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128` and :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256` - Used to enable support for the XOF algorithms required internally by ML-DSA.

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A known ML-DSA-65 public key is imported using :c:func:`psa_import_key`.
      The public key is configured with usage flags for verification.
      The public key, the message, and the signature used in this sample are taken from a NIST ACVP ML-DSA signature verification test vector.

#. ML-DSA signature verification:

   a. The signature is verified using :c:func:`psa_verify_message` with the imported public key.

#. Cleanup:

   a. The public key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ml_dsa`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.4.99-baaa74699d09 ***
   *** Using Zephyr OS v4.4.0-bc35f6fe0b34 ***
   [00:00:00.003,144] <inf> ml_dsa: Starting ML-DSA example...
   [00:00:00.003,152] <inf> ml_dsa: Importing an ML-DSA-65 public key...
   [00:00:00.003,368] <inf> ml_dsa: ML-DSA-65 public key imported successfully!
   [00:00:00.003,383] <inf> ml_dsa: ---- ML-DSA-65 public key (total len: 1952, printing 16 bytes): ----
   [00:00:00.003,393] <inf> ml_dsa: Content:
                                    91 52 c0 c6 86 77 60 8f  42 6f b1 6f 8f 75 f7 6c |.R...w`. Bo.o.u.l
   [00:00:00.003,403] <inf> ml_dsa: ---- ML-DSA-65 public key end  ----
   [00:00:00.003,409] <inf> ml_dsa: Verifying the ML-DSA signature...
   [00:00:00.138,565] <inf> ml_dsa: ---- Message (total len: 1554, printing 16 bytes): ----
   [00:00:00.138,578] <inf> ml_dsa: Content:
                                    88 45 ad 39 4b ce 60 b2  84 76 b2 13 99 b0 d4 72 |.E.9K.`. .v.....r
   [00:00:00.138,588] <inf> ml_dsa: ---- Message end  ----
   [00:00:00.138,597] <inf> ml_dsa: ---- Signature (total len: 3309, printing 16 bytes): ----
   [00:00:00.138,606] <inf> ml_dsa: Content:
                                    6e 8c 4b 20 61 c2 cd f2  71 54 bf 70 85 f6 3c b1 |n.K a... qT.p..<.
   [00:00:00.138,615] <inf> ml_dsa: ---- Signature end  ----
   [00:00:00.138,620] <inf> ml_dsa: Signature verification was successful!
   [00:00:00.138,730] <inf> ml_dsa: Example finished successfully!
