.. _crypto_changelog_nrf_cc3xx_mbedcrypto:

Changelog - nrf_cc3xx_mbedcrypto
################################

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented in this file.

nrf_cc3xx_mbedcrypto - 0.9.18
*****************************

New version of the runtime library with the following bug fixes and improvements:

* Added support for ECC BrainpoolP256r1 curve.
* Fixed support for ECC Edwards25519 curve.
* Removed claimed support for unsupported ECC secp521r1 curve.

Library built against Mbed TLS version 3.3.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

Added a new build of nRF_cc3xx_mbedcrypto libraries for nRF91 Series, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a wchar_t size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.18.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_legacy_crypto_0.9.18.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_core_0.9.18.a`

* nrf_cc310_mbedcrypto, nRF91 Series variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_psa_crypto_0.9.18.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_legacy_crypto_0.9.18.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_core_0.9.18.a`

* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_psa_crypto_0.9.18.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_legacy_crypto_0.9.18.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_core_0.9.18.a`


nrf_cc3xx_mbedcrypto - 0.9.17
*****************************

New version of the runtime library with the following bug fixes and improvements:

* Updated PSA Crypto drivers to conform to Mbed TLS and PSA Crypto API v1.1.0 changes.
* Removed requirement to call :c:func:`psa_aead_set_lengths` for PSA crypto driver for ChaCha20/Poly1305.
* Updated signature for :c:func:`mbedtls_aes_cmac_prf_128`, which is used in legacy _ALT implementation.
* Improved RSA key size and type checking for PSA Crypto driver.

Library built against Mbed TLS version 3.3.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

Added a new build of nRF_cc3xx_mbedcrypto libraries for nRF91 Series, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a wchar_t size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.17.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_legacy_crypto_0.9.17.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_core_0.9.17.a`

* nrf_cc310_mbedcrypto, nRF91 Series variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_psa_crypto_0.9.17.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_legacy_crypto_0.9.17.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_core_0.9.17.a`

* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_psa_crypto_0.9.17.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_legacy_crypto_0.9.17.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_core_0.9.17.a`


nrf_cc3xx_mbedcrypto - 0.9.16
*****************************

New version of the runtime library with the following bug fixes and improvements:

* The library is now built with ``MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER`` enabled to ensure that PSA key type with owner ID (for TF-M builds) and without owner ID (without TF-M enabled) can be supported from a single library.
* Added support for zero input message length for EdDSA for RFC test compliance.
* Removed unused trace functions.

The library is built against Mbed TLS version 3.1.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.16.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_legacy_crypto_0.9.16.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_core_0.9.16.a`

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_psa_crypto_0.9.16.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_legacy_crypto_0.9.16.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_core_0.9.16.a`

* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_psa_crypto_0.9.16.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_legacy_crypto_0.9.16.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_core_0.9.16.a`

nrf_cc3xx_mbedcrypto - 0.9.15
*****************************

New version of the runtime library with the following bug fixes and improvements:

* Improved parameter-testing for invalid or empty inputs/outputs.
* Changed the API for PSA Cipher for nrf_cc3xx PSA Crypto driver (now includes ``iv`` and ``iv_length`` parameters).
* Corrected invalid return-codes being reported for some PSA crypto driver APIs.
* Fixed PSA Crypto driver APIs for AES CCM, so it supports multiple calls to add AAD.
* Fixed PSA Crypto driver APIs for ECDH using Montgomery curves, so they support 255-bit curves (from 256-bit curves before).
* Other minor bug fixes.

The library is built against Mbed TLS version 3.1.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.15.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_legacy_crypto_0.9.15.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_core_0.9.15.a`

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_psa_crypto_0.9.15.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_legacy_crypto_0.9.15.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_core_0.9.15.a`

* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_psa_crypto_0.9.15.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_legacy_crypto_0.9.15.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_core_0.9.15.a`

nrf_cc3xx_mbedcrypto - 0.9.14
*****************************

New version of the runtime library with the following changes:

* Renamed libraries to distinguish between libraries providing PSA crypto APIs or legacy Mbed TLS APIs.
  New library names are ``nrf_cc3xx_psa_crypto`` and ``nrf_cc3xx_legacy_crypto``.
* Added library ``nrf_cc3xx_core`` that holds proprietary and internal APIs.
  The libraries ``nrf_cc3xx_psa_crypto`` and ``nrf_cc3xx_legacy_crypto`` depend on the core library to run.

The library is built against Mbed TLS version 3.0.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.14.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_legacy_crypto_0.9.14.a`
  * :file:`crypto/nrf_cc312_mbedcrypto/lib/cortex-m33/**/libnrf_cc312_core_0.9.14.a`

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_psa_crypto_0.9.14.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_legacy_crypto_0.9.14.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m33/**/libnrf_cc310_core_0.9.14.a`

* nrf_cc310_mbedcrypto, nRF52840 variants
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_psa_crypto_0.9.14.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_legacy_crypto_0.9.14.a`
  * :file:`crypto/nrf_cc310_mbedcrypto/lib/cortex-m4/**/libnrf_cc310_core_0.9.14.a`


nrf_cc3xx_mbedcrypto - 0.9.13
*****************************

New version of the runtime library with the following changes:

* Added compatibility with Mbed TLS 3.0.0.
* The library now also supports PSA APIs.
* The Mbed TLS SHA-256 API now supports data directly from the flash (only for data <= 128 bytes).

The library is built against Mbed TLS version 3.0.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.13.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.13.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.13.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.13.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.13.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.13.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.9.13.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.13.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.13.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.13.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.13.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.13.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.13.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.13.a`

nrf_cc3xx_mbedcrypto - 0.9.12
*****************************

New version of the runtime library with the following fix:

* Corrected the internal size of :c:struct:`mbedtls_cmac_context_t`.
  Note that this size was never used by any code.
  This fix is only for consistency.

The library is built against Mbed TLS version 2.26.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.12.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.12.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.12.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.12.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.12.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.12.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.9.12.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.12.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.12.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.12.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.12.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.12.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.12.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.12.a`


nrf_cc3xx_mbedcrypto - 0.9.11
*****************************

New version of the runtime library with the following bug fix:

* Fixed an issue with the locking of mutex in the CTR_DRBG reseed and random number generator functions.

The library is built against Mbed TLS version 2.26.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.11.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.11.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.11.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.11.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.11.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.11.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.9.11.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.11.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.11.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.11.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.11.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.11.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.11.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.11.a`


nrf_cc3xx_mbedcrypto - 0.9.10
*****************************

New version of the runtime library with a bugfix:

* Fixed configuration issue that only selected 128-bit keys for CTR_DRBG

The library is built against Mbed TLS version 2.26.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.10.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.10.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.10.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.10.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.10.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.10.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.10.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.10.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.10.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.10.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.10.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.10.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.10.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.10.a`


nrf_cc3xx_mbedcrypto - 0.9.9
****************************

New version of the runtime library with new features:

* Support for verifying the RSA key length on nRF52840 and nRF9160

The library is built against Mbed TLS version 2.25.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.9.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.9.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.9.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.9.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.9.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.9.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.9.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.9.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.9.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.9.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.9.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.9.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.9.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.9.a`


nrf_cc3xx_mbedcrypto - 0.9.8
****************************

New version of the runtime library with new features:

* Added support for verifying that the input comes from a DMA addressable address for cryptographic functionality that requires this for nRF52840 and nRF9160.
  Affected algorithms are AES, ChaCha Poly and SHA.

The library is built against Mbed TLS version 2.24.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.8.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.8.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.8.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.8.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.8.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.8.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.8.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.8.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.8.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.8.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.8.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.8.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.8.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.8.a`


nrf_cc3xx_mbedcrypto - 0.9.7
****************************

New version of the runtime library with the following bug fixes:

* Fixed issues where :c:func:`mbedtls_rsa_complete` was not able to deduce missing parameters.
* Fixed an issue with calculating the correct salt length for certain combinations of RSA key and digest sizes.
* Added missing function :c:func:`mbedtls_ecp_write_key`.

The library is built against Mbed TLS version 2.24.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.7.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.7.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.7.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.7.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.7.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.7.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.7.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.7.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.7.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.7.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.7.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.7.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.7.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.7.a`

nrf_cc3xx_mbedcrypto - 0.9.6
****************************

New version of the runtime library fixing a regression in derived keys for ECB, CCM, and GCM.
The library is built against Mbed TLS version 2.24.0.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.6.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.6.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.6.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.6.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.6.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.6.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.6.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.6.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.6.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.6.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.6.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.6.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.6.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.6.a`


nrf_cc3xx_mbedcrypto - 0.9.5
****************************

New version is built against nrf_cc3xx_platform adding correct TRNG categorization for nRF5340 devices.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nRF_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.5.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.5.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.5.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.5.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.5.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.5.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.5.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.5.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.5.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.5.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.5.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.5.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.5.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.5.a`

nrf_cc3xx_mbedcrypto - 0.9.4
****************************

Fixed bugs in KDR/KMU key derivation functions exposed in the :file:`mbedtls/cc3xx_kmu.h` file.

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nrf_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.4.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.4.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.4.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.4.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.4.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.4.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.4.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.4.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.4.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.4.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.4.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.4.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.4.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.4.a`


nrf_cc3xx_mbedcrypto - 0.9.3
****************************

This version adds experimental support for interrupts in selected versions of the library (the libraries that do not support interrupts can be found in the :file:`no-interrupts` folders).

This version is dependent on the nrf_cc310_platform or nrf_cc312_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

* Experimental support for devices with Arm CryptoCell CC312 (nRF5340).
* APIs for key derivation of keys stored in the KMU peripheral (nRF9160, nRF5340).
  See :file:`include/mbedlts/cc3xx_kmu.h`.
* APIs for direct usage of keys stored in the KMU peripheral (nRF9160, nRF5340).
  See :file:`include/mbedtls/cc3xx_kmu.h`.
* APIs for key derivation from KDR key loaded into CryptoCell on boot (nRF52840, nRF9160).
  See :file:`include/mbedtls/cc3xx_kmu.h`.
* New version of libraries nrf_cc310_mbedcrypto/nrf_cc312_mbedcrypto built with Mbed TLS version 2.23.0.
* A new build of nrf_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc312_mbedcrypto, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.3.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_mbedcrypto_0.9.3.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.3.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_mbedcrypto_0.9.3.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_mbedcrypto_0.9.3.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_mbedcrypto_0.9.3.a`


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_mbedcrypto_0.9.3.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.3.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.3.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`


* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.3.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.3.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.3.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.3.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.3.a`


nrf_cc310_mbedcrypto - 0.9.2
****************************

New experimental version of nrf_cc310_mbedcrypto with fixes for power management issues with pending interrupts.

This version also adds experimental support for interrupts in selected versions of the library (the libraries that do not support interrupts can be found in the ``no-interrupts`` folders).

This version is dependent on the nrf_cc310_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nrf_cc310_mbedcrypto library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.9.2.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.2.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.2.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.2.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`

* nrf_cc310_mbedcrypto, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.2.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.2.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.2.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.2.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.2.a`


nrf_cc310_mbedcrypto - 0.9.1
****************************

New experimental version of nrf_cc310_mbedcrypto with general bug fixes.

This version is dependent on the nrf_cc310_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nrf_cc310_mbedcrypto library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.9.1.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.1.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.1.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.1.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`

* nrf_cc310_mbedcrypto, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.1.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.1.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.1.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.1.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.1.a`


nrf_cc310_mbedcrypto - 0.9.0
****************************

New experimental version of nrf_cc310_mbedcrypto with general bug fixes.

This version is dependent on the newly added nrf_cc310_platform library for low-level initialization of the system and proper RTOS integration.

Added
=====

A new build of nrf_cc310_mbedcrypto library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.9.0.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.9.0.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`

* nrf_cc310_mbedcrypto, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.9.0.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.9.0.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.9.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.9.0.a`


nrf_cc310_mbedcrypto - 0.8.1
****************************

New experimental version of nrf_cc310_mbedcrypto with general bug fixes.

.. note::
  This version should be used for nRF9160 devices.
  Using earlier versions may lead to undefined behavior on some nRF9160 devices.

Added
=====

A new build of nrf_cc310_mbedcrypto library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.8.1.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.8.1.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.1.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.1.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`

* nrf_cc310_mbedcrypto, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.8.1.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.8.1.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.1.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.1.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.1.a`


nrf_cc310_mbedcrypto - 0.8.0
****************************

New experimental version of nrf_cc310_mbedcrypto with changes to platform initialization and general bug fixes.

.. note::
   This version may lead to undefined behavior on some nRF9160 devices.
   Hence, use a newer version.

Added
=====

A new build of nrf_cc310_mbedcrypto library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.

* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.8.0.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.8.0.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`

* nrf_cc310_mbedcrypto, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.8.0.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.8.0.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.8.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.8.0.a`


nrf_cc310_mbedcrypto - 0.7.0
****************************

Initial release.

Added
=====

The following nrf_cc310_mbedcrypto libraries for nRF9160 and nRF52 architectures:

.. note::
   The *short-wchar* libraries are compiled with a ``wchar_t`` size of 16 bits.


* nrf_cc310_mbedcrypto, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_mbedcrypto_0.7.0.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_mbedcrypto_0.7.0.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.7.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.7.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`

* nrf_cc310_mbedcrypto, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_mbedcrypto_0.7.0.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_mbedcrypto_0.7.0.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_mbedcrypto_0.7.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_mbedcrypto_0.7.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_mbedcrypto_0.7.0.a`
