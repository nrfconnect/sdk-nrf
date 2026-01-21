.. _crypto_ecjpake:

Crypto: EC J-PAKE
#################

.. contents::
   :local:
   :depth: 2

The EC J-PAKE sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform password-authenticated key exchange using the EC J-PAKE algorithm.
The sample uses the elliptic curve (EC) version of the password-authenticated key exchange by juggling (J-PAKE) protocol with a shared password.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY` - Used to enable support for ECC public key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE` - Used to enable support for the J-PAKE key agreement algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_pake_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A password key is imported using :c:func:`psa_import_key` with the ``PSA_KEY_TYPE_PASSWORD`` type.
      The key is configured with usage flags for key derivation.

#. EC J-PAKE key exchange:

   a. PAKE operations are set up for both client and server using :c:func:`psa_pake_setup`.
   #. Key exchange rounds are performed using :c:func:`psa_pake_output` and :c:func:`psa_pake_input`.
      This includes key sharing, zero-knowledge public values, and zero-knowledge proofs.
   #. Shared secrets are derived using :c:func:`psa_pake_get_shared_key` and key derivation functions.
   #. The derived secrets are compared to verify that both parties obtained the same shared secret.

#. Cleanup:

   a. The password key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecjpake`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> ecjpake: Starting EC J-PAKE example...
   [00:00:00.251,190] <inf> ecjpake: Importing password key...
   [00:00:00.251,342] <inf> ecjpake: Password key imported successfully!
   [00:00:00.251,373] <inf> ecjpake: Performing EC J-PAKE key exchange rounds...
   [00:00:00.251,708] <inf> ecjpake: EC J-PAKE key exchange completed successfully!
   [00:00:00.251,739] <inf> ecjpake: Deriving shared secrets...
   [00:00:00.251,770] <inf> ecjpake: Shared secrets derived successfully!
   [00:00:00.251,800] <inf> ecjpake: ---- server_secret (len: 32): ----
   [00:00:00.251,831] <inf> ecjpake: Content:
                                    c3 1e 5b 35 97 25 ee a3  ef ba 66 c3 f9 81 37 2a |..[5.%.. ..f...7*
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
   [00:00:00.251,861] <inf> ecjpake: ---- server_secret end  ----
   [00:00:00.251,892] <inf> ecjpake: ---- client_secret (len: 32): ----
   [00:00:00.251,922] <inf> ecjpake: Content:
                                    c3 1e 5b 35 97 25 ee a3  ef ba 66 c3 f9 81 37 2a |..[5.%.. ..f...7*
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
   [00:00:00.251,953] <inf> ecjpake: ---- client_secret end  ----
   [00:00:00.251,984] <inf> ecjpake: Shared secrets match!
   [00:00:00.252,014] <inf> ecjpake: Example finished successfully!
