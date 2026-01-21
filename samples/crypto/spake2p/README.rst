.. _crypto_spake2p:

Crypto: Spake2+
###############

.. contents::
   :local:
   :depth: 2

The Spake2+ sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform password-authenticated key exchange using the Spake2+ protocol with HMAC-SHA-256 and the secp256r1 ECC curve.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC` - Used to enable support for the Spake2+ PAKE algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_pake_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF` - Used to enable support for the HKDF key derivation algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_kdf_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` - Used to enable support for the HMAC algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_mac_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` - Used to enable support for the SHA-256 hash algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_hash_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256` - Used to enable support for the secp256r1 ECC curve from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_ecc_curve_types`.
* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT` - Used to enable support for Spake2+ key pair types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_spake2p_key_pair_ops`.
* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable random number generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A Spake2+ key pair is imported into the PSA crypto keystore using :c:func:`psa_import_key`.
      The key pair is configured for password-authenticated key exchange.
   #. A Spake2+ public key is imported into the PSA crypto keystore using :c:func:`psa_import_key`.

#. Spake2+ key exchange:

   a. Two PAKE operations are set up using :c:func:`psa_pake_setup`, one for the client role and one for the server role.
   #. The roles are configured using :c:func:`psa_pake_set_role`.
   #. User and peer identifiers are set using :c:func:`psa_pake_set_user` and :c:func:`psa_pake_set_peer`.
   #. The key shares are exchanged between client and server using :c:func:`psa_pake_output` and :c:func:`psa_pake_input`.
   #. Key confirmation messages are exchanged between client and server.

#. Key derivation:

   a. The shared key is obtained using :c:func:`psa_pake_get_shared_key` for both client and server.
   #. The shared key is used as input for the HKDF key derivation (:c:func:`psa_key_derivation_setup`, :c:func:`psa_key_derivation_input_key`, and :c:func:`psa_key_derivation_output_bytes`) to derive final secrets.
   #. The derived secrets for client and server are verified to be equal.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/spake2p`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> spake2p: Starting Spake2+ example...
   [00:00:00.251,190] <inf> spake2p: ---- Sending message (len: 32): ----
   [00:00:00.251,220] <inf> spake2p: Content:
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
                                    12 34 56 78 9a bc de f0  12 34 56 78 9a bc de f0 |.4Vx.... .4Vx....|
   [00:00:00.251,251] <inf> spake2p: ---- Sending message end  ----
   [00:00:00.251,281] <inf> spake2p: ---- Sending message (len: 32): ----
   [00:00:00.251,312] <inf> spake2p: Content:
                                    34 56 78 9a bc de f0 12  34 56 78 9a bc de f0 12 |4Vx......Vx....|
                                    34 56 78 9a bc de f0 12  34 56 78 9a bc de f0 12 |4Vx......Vx....|
   [00:00:00.251,342] <inf> spake2p: ---- Sending message end  ----
   [00:00:00.251,373] <inf> spake2p: ---- Sending message (len: 32): ----
   [00:00:00.251,404] <inf> spake2p: Content:
                                    56 78 9a bc de f0 12 34  56 78 9a bc de f0 12 34 |Vx........Vx...|
                                    56 78 9a bc de f0 12 34  56 78 9a bc de f0 12 34 |Vx........Vx...|
   [00:00:00.251,434] <inf> spake2p: ---- Sending message end  ----
   [00:00:00.251,465] <inf> spake2p: ---- Sending message (len: 32): ----
   [00:00:00.251,495] <inf> spake2p: Content:
                                    78 9a bc de f0 12 34 56  78 9a bc de f0 12 34 56 |x.................|
                                    78 9a bc de f0 12 34 56  78 9a bc de f0 12 34 56 |x.................|
   [00:00:00.251,526] <inf> spake2p: ---- Sending message end  ----
   [00:00:00.251,556] <inf> spake2p: ---- Server secret (len: 32): ----
   [00:00:00.251,587] <inf> spake2p: Content:
                                    9a bc de f0 12 34 56 78  9a bc de f0 12 34 56 78 |....4Vx...4Vx|
                                    9a bc de f0 12 34 56 78  9a bc de f0 12 34 56 78 |....4Vx...4Vx|
   [00:00:00.251,617] <inf> spake2p: ---- Server secret end  ----
   [00:00:00.251,648] <inf> spake2p: ---- Client secret (len: 32): ----
   [00:00:00.251,678] <inf> spake2p: Content:
                                    9a bc de f0 12 34 56 78  9a bc de f0 12 34 56 78 |....4Vx...4Vx|
                                    9a bc de f0 12 34 56 78  9a bc de f0 12 34 56 78 |....4Vx...4Vx|
   [00:00:00.251,709] <inf> spake2p: ---- Client secret end  ----
   [00:00:00.251,739] <inf> spake2p: Spake2+ key exchange successful!
   [00:00:00.251,770] <inf> spake2p: Example finished successfully!
