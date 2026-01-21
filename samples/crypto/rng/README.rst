.. _crypto_rng:

Crypto: RNG
###########

.. contents::
   :local:
   :depth: 2

The RNG sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to generate cryptographically secure random numbers.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` - Used to enable support for random number generation from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_rng_algorithms`.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

The sample produces random numbers of 100-byte length.

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.

#. Random number generation:

   a. Five 100-byte random numbers are generated using :c:func:`psa_generate_random`.
   #. Each random number is displayed in hexadecimal format for verification.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/rng`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***
   [00:00:00.251,159] <inf> rng: Starting RNG example...
   [00:00:00.251,190] <inf> rng: Producing 5 random numbers...
   [00:00:00.251,342] <inf> rng: ---- RNG data (len: 100): ----
   [00:00:00.251,373] <inf> rng: Content:
                                    a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
                                   a1 b2 c3 d4 e5 f6 07 18  29 3a 4b 5c 6d 7e 8f 90 |........):\m~..
   [00:00:00.251,404] <inf> rng: ---- RNG data end  ----
   [00:00:00.251,434] <inf> rng: ---- RNG data (len: 100): ----
   [00:00:00.251,465] <inf> rng: Content:
                                    b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
                                   b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
                                   b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
                                   b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
                                   b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
                                   b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
                                   b2 c3 d4 e5 f6 07 18 29  3a 4b 5c 6d 7e 8f 90 a1 |.......):\m~..|
   [00:00:00.251,495] <inf> rng: ---- RNG data end  ----
   [00:00:00.251,526] <inf> rng: ---- RNG data (len: 100): ----
   [00:00:00.251,556] <inf> rng: Content:
                                    c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
                                   c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
                                   c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
                                   c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
                                   c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
                                   c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
                                   c3 d4 e5 f6 07 18 29 3a  4b 5c 6d 7e 8f 90 a1 b2 |.......):\m~..|
   [00:00:00.251,587] <inf> rng: ---- RNG data end  ----
   [00:00:00.251,617] <inf> rng: ---- RNG data (len: 100): ----
   [00:00:00.251,648] <inf> rng: Content:
                                    d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
                                   d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
                                   d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
                                   d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
                                   d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
                                   d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
                                   d4 e5 f6 07 18 29 3a 4b  5c 6d 7e 8f 90 a1 b2 c3 |.......):\m~..|
   [00:00:00.251,678] <inf> rng: ---- RNG data end  ----
   [00:00:00.251,709] <inf> rng: ---- RNG data (len: 100): ----
   [00:00:00.251,739] <inf> rng: Content:
                                    e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
                                   e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
                                   e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
                                   e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
                                   e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
                                   e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
                                   e5 f6 07 18 29 3a 4b 5c  6d 7e 8f 90 a1 b2 c3 d4 |.......):\m~..|
   [00:00:00.251,770] <inf> rng: ---- RNG data end  ----
   [00:00:00.251,800] <inf> rng: Example finished successfully!
