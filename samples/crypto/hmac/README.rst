.. _crypto_hmac:

Crypto: HMAC
############

.. contents::
   :local:
   :depth: 2

The HMAC sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to generate and verify message authentication codes using the HMAC algorithm with SHA-256 as the underlying hash function.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HMAC` - Used to enable support for HMAC key types.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` - Used to enable support for the HMAC message authentication code algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_mac_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 256-bit HMAC key is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for signing and verification.

#. HMAC signing and verification:

   a. The HMAC signing operation is set up using :c:func:`psa_mac_sign_setup` with the ``PSA_ALG_HMAC(PSA_ALG_SHA_256)`` algorithm.
   #. The message data is processed using :c:func:`psa_mac_update`.
   #. The HMAC is finalized using :c:func:`psa_mac_sign_finish`.
   #. The HMAC verification operation is set up using :c:func:`psa_mac_verify_setup`.
   #. The message data is processed again using :c:func:`psa_mac_update`.
   #. The HMAC is verified using :c:func:`psa_mac_verify_finish`.

#. Cleanup:

   a. The HMAC key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/hmac`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> hmac: Starting HMAC example...
   [00:00:00.251,190] <inf> hmac: Generating random HMAC key...
   [00:00:00.251,342] <inf> hmac: HMAC key generated successfully!
   [00:00:00.251,373] <inf> hmac: Signing using the HMAC algorithm...
   [00:00:00.251,708] <inf> hmac: Signing successful!
   [00:00:00.251,739] <inf> hmac: ---- Plaintext (len: 100): ----
   [00:00:00.251,770] <inf> hmac: Content:
                                    Example string to demonstrate basic usage of HMAC signing/verification.
   [00:00:00.251,800] <inf> hmac: ---- Plaintext end  ----
   [00:00:00.251,831] <inf> hmac: ---- HMAC (len: 32): ----
   [00:00:00.251,861] <inf> hmac: Content:
                                    a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
   [00:00:00.251,892] <inf> hmac: ---- HMAC end  ----
   [00:00:00.251,922] <inf> hmac: Verifying the HMAC signature...
   [00:00:00.252,045] <inf> hmac: HMAC verified successfully!
   [00:00:00.252,075] <inf> hmac: Example finished successfully!
