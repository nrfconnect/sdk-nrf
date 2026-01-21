.. _crypto_aes_cbc:

Crypto: AES CBC
###############

.. contents::
   :local:
   :depth: 2

The AES CBC sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform AES encryption and decryption operations using the CBC block cipher mode without padding and a 128-bit AES key.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES` - Used to enable support for AES key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING` - Used to enable support for the CBC cipher mode without padding from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_cipher_modes`.
* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable random number generation for key and IV generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

.. crypto_sample_overview_driver_selection_start

The sample also configures the cryptographic drivers for each board target using Kconfig options in the overlay files in the :file:`boards` directory.

These Kconfig options are then used by the build system to compile the required cryptographic PSA directives and make the configured cryptographic drivers available at runtime.
See :ref:`crypto_drivers_driver_selection` for more information about this process.

.. crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A random 128-bit AES key is generated using :c:func:`psa_generate_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for both encryption and decryption.

#. Encryption and decryption of a sample plaintext:

   a. An encryption operation is set up using :c:func:`psa_cipher_encrypt_setup` with the ``PSA_ALG_CBC_NO_PADDING`` algorithm.
   #. A random initialization vector (IV) is generated using :c:func:`psa_cipher_generate_iv`.
   #. Encryption is performed using :c:func:`psa_cipher_update` and finalized with :c:func:`psa_cipher_finish`.
   #. A decryption operation is set up using :c:func:`psa_cipher_decrypt_setup`.
   #. The IV from the encryption step is set using :c:func:`psa_cipher_set_iv`.
   #. Decryption is performed using :c:func:`psa_cipher_update` and finalized with :c:func:`psa_cipher_finish`.
   #. The decrypted text is compared with the original plaintext to verify correctness.

#. Cleanup:

   a. The AES key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_cbc`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. crypto_sample_testing_start

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Build and program the application.
#. Observe the logs from the application using the terminal emulator.
   For example, the log output should look like this:

.. crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> aes_cbc: Starting AES-CBC-NO-PADDING example...
   [00:00:00.251,190] <inf> aes_cbc: Generating random AES key...
   [00:00:00.251,342] <inf> aes_cbc: AES key generated successfully!
   [00:00:00.251,373] <inf> aes_cbc: Encrypting using the AES CBC mode...
   [00:00:00.251,708] <inf> aes_cbc: Encryption successful!
   [00:00:00.251,708] <inf> aes_cbc: ---- IV (len: 16): ----
   [00:00:00.251,739] <inf> aes_cbc: Content:
                                    c3 1e 5b 35 97 25 ee a3  ef ba 66 c3 f9 81 37 2a |..[5.%.. ..f...7*
   [00:00:00.251,770] <inf> aes_cbc: ---- IV end  ----
   [00:00:00.251,800] <inf> aes_cbc: ---- Plaintext (len: 64): ----
   [00:00:00.251,831] <inf> aes_cbc: Content:
                                    45 78 61 6d 70 6c 65 20  73 74 72 69 6e 67 20 74 |Example  string t
                                    6f 20 64 65 6d 6f 6e 73  74 72 61 74 65 20 62 61 |o demons trate ba
                                    73 69 63 20 75 73 61 67  65 20 6f 66 20 41 45 53 |sic usag e of AES
                                    20 43 42 43 20 6d 6f 64  65 2e 00 00 00 00 00 00 | CBC mod e.......
   [00:00:00.251,831] <inf> aes_cbc: ---- Plaintext end  ----
   [00:00:00.251,861] <inf> aes_cbc: ---- Encrypted text (len: 64): ----
   [00:00:00.251,892] <inf> aes_cbc: Content:
                                    cc 7d c0 ed 63 5b df 28  08 2b 03 33 a4 3c dc 1d |.}..c[.( .+.3.<..
                                    76 9d a9 cb 1c 49 4f 6d  ef b8 a2 aa 11 2c fc bd |v....IOm .....,..
                                    39 56 54 b5 96 6e 13 e2  7d 22 26 1e 3c 7c 3e eb |9VT..n.. }"&.<|>.
                                    15 60 31 d3 58 02 b6 85  98 63 2c e6 ad dc aa 19 |.`1.X... .c,.....
   [00:00:00.251,922] <inf> aes_cbc: ---- Encrypted text end  ----
   [00:00:00.251,953] <inf> aes_cbc: Decrypting using the AES CBC mode...
   [00:00:00.252,166] <inf> aes_cbc: ---- Decrypted text (len: 64): ----
   [00:00:00.252,197] <inf> aes_cbc: Content:
                                    45 78 61 6d 70 6c 65 20  73 74 72 69 6e 67 20 74 |Example  string t
                                    6f 20 64 65 6d 6f 6e 73  74 72 61 74 65 20 62 61 |o demons trate ba
                                    73 69 63 20 75 73 61 67  65 20 6f 66 20 41 45 53 |sic usag e of AES
                                    20 43 42 43 20 6d 6f 64  65 2e 00 00 00 00 00 00 | CBC mod e.......
   [00:00:00.252,227] <inf> aes_cbc: ---- Decrypted text end  ----
   [00:00:00.252,258] <inf> aes_cbc: Decryption successful!
   [00:00:00.252,288] <inf> aes_cbc: Example finished successfully!
