.. _crypto_ecdh:

Crypto: ECDH
############

.. contents::
   :local:
   :depth: 2

The ECDH sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform an Elliptic-curve Diffie-Hellman key exchange using the ECDH algorithm with two 256-bit ECC key pairs.
The sample allows two parties (Alice and Bob) to obtain a shared secret through key exchange.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`, :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`, :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT` - Used to enable support for ECC key pair types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH` - Used to enable support for the ECDH key agreement algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_agreement_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable random number generation for key generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. Two random 256-bit ECC key pairs are generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key pairs are configured with usage flags for key derivation.

#. ECDH key exchange using the generated key pairs:

   a. Public keys are exported using :c:func:`psa_export_public_key`.
   #. ECDH key agreement is performed using :c:func:`psa_raw_key_agreement` with the ``PSA_ALG_ECDH`` algorithm.
      This function calculates the shared secret using one party's private key and the other party's public key.
   #. The calculated secrets are compared to verify that both parties obtained the same shared secret.

#. Cleanup:

   a. The ECC key pairs are removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecdh`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> ecdh: Starting ECDH example...
   [00:00:00.251,190] <inf> ecdh: Creating ECDH key pair for Alice
   [00:00:00.251,342] <inf> ecdh: ECDH key pair generated successfully!
   [00:00:00.251,373] <inf> ecdh: Creating ECDH key pair for Bob
   [00:00:00.251,708] <inf> ecdh: ECDH key pair generated successfully!
   [00:00:00.251,739] <inf> ecdh: Export Alice's public key
   [00:00:00.251,770] <inf> ecdh: ECDH public key exported successfully!
   [00:00:00.251,800] <inf> ecdh: Export Bob's public key
   [00:00:00.251,831] <inf> ecdh: ECDH public key exported successfully!
   [00:00:00.251,861] <inf> ecdh: Calculating the secret value for Alice
   [00:00:00.251,892] <inf> ecdh: ECDH secret calculated successfully!
   [00:00:00.251,922] <inf> ecdh: Calculating the secret value for Bob
   [00:00:00.251,953] <inf> ecdh: ECDH secret calculated successfully!
   [00:00:00.251,984] <inf> ecdh: Comparing the secret values of Alice and Bob
   [00:00:00.252,014] <inf> ecdh: The secret values of Alice and Bob match!
   [00:00:00.252,045] <inf> ecdh: Example finished successfully!
