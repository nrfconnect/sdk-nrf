.. _crypto_chacha_poly:

Crypto: Chacha20-Poly1305 example
#################################

.. contents::
   :local:
   :depth: 2

The Chacha20-Poly1305 sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform authenticated encryption and decryption operations using the Chacha20-Poly1305 AEAD algorithm with a 256-bit Chacha20 key.
The sample uses additional authenticated data (AAD) and a random nonce.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20` - Used to enable support for Chacha20 key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305` - Used to enable support for the Chacha20-Poly1305 AEAD algorithm from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_aead_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable random number generation for key and nonce generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 256-bit Chacha20 key is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for both encryption and decryption.

#. Authenticated encryption and decryption of a sample plaintext:

   a. A random nonce is generated using :c:func:`psa_generate_random`.
   #. Authenticated encryption is performed using :c:func:`psa_aead_encrypt` with the ``PSA_ALG_CHACHA20_POLY1305`` algorithm.
      This function encrypts the plaintext with the provided nonce and additional authenticated data, and appends an authentication tag to the ciphertext.
   #. Authenticated decryption is performed using :c:func:`psa_aead_decrypt`.
      This function decrypts the ciphertext, verifies the authentication tag, and authenticates the additional data.
   #. The decrypted text is compared with the original plaintext to verify correctness.

#. Cleanup:

   a. The Chacha20 key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/chachapoly`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> chachapoly: Starting Chacha20-Poly1305 example...
   [00:00:00.251,190] <inf> chachapoly: Generating random Chacha20 key...
   [00:00:00.251,342] <inf> chachapoly: Chacha20 key generated successfully!
   [00:00:00.251,373] <inf> chachapoly: Encrypting using the Chacha20-Poly1305 mode...
   [00:00:00.251,708] <inf> chachapoly: Encryption successful!
   [00:00:00.251,708] <inf> chachapoly: ---- Nonce (len: 12): ----
   [00:00:00.251,739] <inf> chachapoly: Content:
                                    c3 1e 5b 35 97 25 ee a3  ef ba 66 c3 |..[5.%.. ..f.
   [00:00:00.251,770] <inf> chachapoly: ---- Nonce end  ----
   [00:00:00.251,800] <inf> chachapoly: ---- Plaintext (len: 100): ----
   [00:00:00.251,831] <inf> chachapoly: Content:
                                    Example string to demonstrate basic usage of Chacha20-Poly1305.
   [00:00:00.251,861] <inf> chachapoly: ---- Plaintext end  ----
   [00:00:00.251,892] <inf> chachapoly: ---- Additional data (len: 35): ----
   [00:00:00.251,922] <inf> chachapoly: Content:
                                    Example string of additional data
   [00:00:00.251,953] <inf> chachapoly: ---- Additional data end  ----
   [00:00:00.251,984] <inf> chachapoly: ---- Encrypted text (len: 116): ----
   [00:00:00.252,014] <inf> chachapoly: Content:
                                    cc 7d c0 ed 63 5b df 28  08 2b 03 33 a4 3c dc 1d |.}..c[.( .+.3.<..
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
                                    39 56 54 b5 96 6e 13 e2  7d 22 26 1e 3c 7c 3e eb |9VT..n.. }"&.<|>.
                                    15 60 31 d3 58 02 b6 85  98 63 2c e6 ad dc aa 19 |.`1.X... .c,.....
                                    4a 2b 3c 1d 5e 6f 7a 8b  9c ad be cf d0 e1 f2 03 |J+<.^oz. ........
   [00:00:00.252,045] <inf> chachapoly: ---- Encrypted text end  ----
   [00:00:00.252,075] <inf> chachapoly: Decrypting using the Chacha20-Poly1305 mode...
   [00:00:00.252,166] <inf> chachapoly: ---- Decrypted text (len: 100): ----
   [00:00:00.252,197] <inf> chachapoly: Content:
                                    Example string to demonstrate basic usage of Chacha20-Poly1305.
   [00:00:00.252,227] <inf> chachapoly: ---- Decrypted text end  ----
   [00:00:00.252,258] <inf> chachapoly: Decryption and authentication was successful!
   [00:00:00.252,288] <inf> chachapoly: Example finished successfully!
