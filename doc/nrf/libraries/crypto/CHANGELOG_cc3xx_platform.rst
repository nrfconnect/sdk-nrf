.. _crypto_changelog_nrf_cc3xx_platform:

Changelog - nrf_cc3xx_platform
##############################

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented in this file.

nrf_cc3xx_platform - 0.9.17
***************************

New version of the library with the following features:

* Removed :c:struct:`mbedtls_hardware_poll` limitation on buffer size. It now accepts any buffer size.
* Removed Mbed TLS platform functions from library.

Library built against Mbed TLS version 3.3.0.

Added
=====

Added a new build of nrf_cc3xx_platform libraries for nRF91 Series, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`/nrf_cc312_platform/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.17.a`

* nrf_cc310_platform, nRF91 Series variants

  * :file:`/nrf_cc310_platform/lib/cortex-m33/**/libnrf_cc310_platform_0.9.17.a`

* nrf_cc310_platform, nRF52840 variants

  * :file:`/nrf_cc310_platform/lib/cortex-m4/**/libnrf_cc310_platform_0.9.17.a`

nrf_cc3xx_platform - 0.9.16
***************************

New version of the library with the following features:

* Added code-size optimized API for SHA-256 that has no limitation on where the input is stored (flash/RAM).
* Fixed a bug where platform mutexes ended up unallocated if a context holding them was not zeroized.

The library is built against Mbed TLS version 3.1.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`/nrf_cc312_platform/lib/cortex-m33/**/libnrf_cc312_psa_crypto_0.9.16.a`

* nrf_cc310_platform, nRF9160 variants

  * :file:`/nrf_cc310_platform/lib/cortex-m33/**/libnrf_cc310_platform_0.9.16.a`

* nrf_cc310_mbedcrypto, nRF52840 variants

  * :file:`/nrf_cc310_platform/lib/cortex-m4/**/libnrf_cc310_platform_0.9.16.a`


nrf_cc3xx_platform - 0.9.15
***************************

New version of the library with the following features:

* Added new shadow key APIs for key derivation using KMU, which is compatible with multi-part operations.
* Ensured that random seeds (for EITS and attestation) are generated only once during boot.
* Minor bug fixes and improvements.

The library is built against Mbed TLS version 3.1.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.15.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.15.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.15.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.15.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.15.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.15.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.15.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.15.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.15.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.15.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.15.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.15.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.15.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.15.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.15.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.15.a`

* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.15.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.15.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.15.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.15.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.15.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.15.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.15.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.15.a`

nrf_cc3xx_platform - 0.9.14
***************************

New version of the library with the following features:

* Internal restructure of the library.
* APIs for storing encrypted identity key in KMU.
* APIs for retrieving boot generated RNG seed and nonce seed.

The library is built against Mbed TLS version 3.0.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.14.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.14.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.14.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.14.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.14.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.14.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.14.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.14.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.14.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.14.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.14.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.14.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.14.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.14.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.14.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.14.a`

* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.14.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.14.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.14.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.14.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.14.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.14.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.14.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.14.a`


nrf_cc3xx_platform - 0.9.13
***************************

New version of the library with the following features:

* Internal restructure of the library.
* Updated to the Mbed TLS version 3.0.0.

The library is built against Mbed TLS version 3.0.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.13.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.13.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.13.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.13.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.13.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.13.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.13.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.13.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.13.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.13.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.13.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.13.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.13.a`

* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.13.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.13.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.13.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.13.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.13.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.13.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.13.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.13.a`

nrf_cc3xx_platform - 0.9.12
***************************

New version of the library with bug fixes and features:

* Fixed issue with KMU loading for nRF9160 devices.
  The issue is only present in certain builds, but it is highly recommended to update to this version of the library if you are using nRF9160.

The library is built against Mbed TLS version 2.26.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.12.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.12.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.12.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.12.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.12.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.12.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.12.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.12.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.12.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.12.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.12.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.12.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.12.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.12.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.12.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.12.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.12.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.12.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.12.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.12.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.12.a`


nrf_cc3xx_platform - 0.9.11
***************************

New version of the library with the following bug fixes and features:

* Modified the KMU APIs to remove the reservation of slots 0 and 1.
  These slots can be used freely now.
* Fixed an issue where the global CTR_DRBG context would get stuck when it reached the reseed interval.
* Fixed an issue where building with the derived key APIs would not be possible.

The library is built against Mbed TLS version 2.26.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.11.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.11.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.11.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.11.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.11.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.11.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.11.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.11.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.11.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.11.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.11.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.11.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.11.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.11.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.11.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.11.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.11.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.11.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.11.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.11.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.11.a`

nrf_cc3xx_platform - 0.9.10
***************************

New version of the library with a new feature:

* Added HMAC_DRBG APIs in the :file:`nrf_cc3xx_platform_hmac_drbg.h` file.

The library is built against Mbed TLS version 2.26.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.10.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.10.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.10.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.10.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.10.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.10.a`

* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.10.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.10.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.10.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.10.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.10.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.10.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.10.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.10.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.10.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.10.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.10.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.10.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.10.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.10.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.10.a`


nrf_cc3xx_platform - 0.9.9
**************************

New version of the library with the following bug fixes and new features:

* Support for using ChaCha20 keys directly from the KMU on nRF9160 and nRF5340 devices.
* APIs for key derivation in the :file:`nrf_cc3xx_platform_derived_key.h` file.
* Support for using derived keys for ChaCha20 encryption/decryption.
* Modified CTR_DRBG APIs to use internal context when the context argument is NULL.
* New API for storing keys in the KMU.

The library is built against Mbed TLS version 2.25.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.9.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.9.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.9.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.9.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.9.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.9.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.9.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.9.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.9.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.9.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.9.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.9.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.9.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.9.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.9.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.9.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.9.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.9.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.9.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.9.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.9.a`


nrf_cc3xx_platform - 0.9.8
**************************

New version of the library with the following improvements and bug fixes:

* Decreased stack usage for PRNG using ``CTR_DRBG``.
* Fixed issue with ``CTR_DRBG`` usage on the platform library when multiple backends are enabled in the Nordic Secure Module (nrf_security).
* Fixed issues in the entropy module.
* APIs for key derivation in the :file:`nrf_cc3xx_platform_kmu.h`.

The library is built against Mbed TLS version 2.24.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.8.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.8.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.8.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.8.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.8.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.8.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.8.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.8.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.8.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.8.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.8.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.8.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.8.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.8.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.8.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.8.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.8.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.8.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.8.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.8.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.8.a`


nrf_cc3xx_platform - 0.9.7
**************************

New version of the library with a bug fix:

* Fixed an issue with mutex slab allocation in Zephyr RTOS platform file.

The library is built against Mbed TLS version 2.24.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.7.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.7.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.7.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.7.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.7.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.7.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.7.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.7.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.7.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.7.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.7.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.7.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.7.a`

* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.7.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.7.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.7.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.7.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.7.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.7.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.7.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.7.a`


nrf_cc3xx_platform - 0.9.6
**************************

New version of the library with Mbed TLS sources :file:`ctr_drbg.c` and :file:`entropy.c` built in.
The library is built against Mbed TLS version 2.24.0.

Added
=====

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.6.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.6.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.6.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.6.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.6.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.6.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.6.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.6.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.6.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.6.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.6.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.6.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.6.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.6.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.6.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.6.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.6.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.6.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.6.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.6.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.6.a`


nrf_cc3xx_platform - 0.9.5
**************************

Added
=====

* Correct TRNG categorization values for nRF5340 devices.

A new build of nrf_cc3xx_platform libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.5.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.5.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.5.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.5.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.5.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.5.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.5.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.5.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.5.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.5.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.5.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.5.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.5.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.5.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.5.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.5.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.5.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.5.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.5.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.5.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.5.a`


nrf_cc3xx_platform - 0.9.4
**************************

Added
=====

* API to push KMU slot 0 on nRF9160 devices into CryptoCell KDR registers.
  See :file:`include/nrf_cc3xx_platform_kmu.h`.
* API to load a key from an address into CryptoCell KDR registers on nRF52840 devices.
  See :file:`include/nrf_cc3xx_platform_kmu.h`.

A new build of nrf_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.4.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.4.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.4.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.4.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.4.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.4.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.4.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.4.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.4.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.4.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.4.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.4.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.4.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.4.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.4.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.4.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.4.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.4.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.4.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.4.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.4.a`

nrf_cc3xx_platform - 0.9.3
**************************

This version adds experimental support for interrupts in selected versions of the library (the libraries that do not support interrupts can be found in the :file:`no-interrupts` folders).

Changed name of configurations from CC310 to CC3XX.
This is reflected in the header file and APIs as well, where ``nrf_cc310_xxxx`` is renamed to ``nrf_cc3xx_xxxx``.

Added
=====

* Experimental support for devices with Arm CryptoCell CC312 (nRF5340).
* New version of libraries nrf_cc310_platform/nrf_cc312_platform built with Mbed TLS version 2.23.0.
* APIs for storing keys in the KMU peripheral (nRF9160, nRF5340).
  See :file:`include/nrf_cc3xx_platform_kmu.h`.
* APIs for generating CSPRNG using CTR_DRBG.
  See :file:`include/nrf_cc3xx_platform_ctr_drbg.h`.

A new build of nrf_cc3xx_mbedcrypto libraries for nRF9160, nRF52840, and nRF5340.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc312_platform, nRF5340 variants

  * :file:`cortex-m33/hard-float/libnrf_cc312_platform_0.9.3.a`
  * :file:`cortex-m33/soft-float/libnrf_cc312_platform_0.9.3.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc312_platform_0.9.3.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc312_platform_0.9.3.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc312_platform_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc312_platform_0.9.3.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc312_platform_0.9.3.a`


* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.3.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.3.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.3.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.3.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.3.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.3.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.3.a`


* nrf_cc310_platform, nRF52840 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.3.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.3.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.3.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.3.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.3.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.3.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.3.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.3.a`


nrf_cc310_platform - 0.9.2
**************************

New version of nrf_cc310_platform library fixing power management issues with pending interrupts.

This version also adds experimental support for interrupts in selected versions of the library (the libraries that do not support interrupts can be found in the :file:`no-interrupts` folders).

This version must match the version of nrf_cc310_mbedcrypto if it is also used.

Added
=====

A new build of nrf_cc310_platform library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.2.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.2.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.2.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.2.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.2.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.2.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.2.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.2.a`

* nrf_cc310_platform, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.2.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.2.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.2.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.2.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.2.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.2.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.2.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.2.a`


nrf_cc310_platform - 0.9.1
**************************

New version of nrf_cc310_platform library containing Arm CC310 hardware initialization and entropy gathering APIs.
Added to match with the nrf_cc310_mbedcrypto v0.9.1 library.

.. note::

   The library version must match with nrf_cc310_mbedcrypto if this is also used.

Added
=====

A new build of nrf_cc310_platform library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.1.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.1.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.1.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.1.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.1.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.1.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.1.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.1.a`

* nrf_cc310_platform, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.1.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.1.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.1.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.1.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.1.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.1.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.1.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.1.a`


nrf_cc310_platform - 0.9.0
**************************

Initial, experimental release of nrf_cc310_platform library containing Arm CC310 hardware initialization and entropy gathering APIs.

The library also contains APIs and companion source-files to setup RTOS dependent mutex and abort functionality for the nrf_cc310_mbedcrypto library in Zephyr RTOS and FreeRTOS.

.. note::

   The library version must match with nrf_cc310_mbedcrypto if this is also used

Added
=====

A new build of nrf_cc310_platform library for nRF9160 and nRF52 architectures.

.. note::

   The *short-wchar* libraries are compiled with a :c:type:`wchar_t` size of 16 bits.

* nrf_cc310_platform, nRF9160 variants

  * :file:`cortex-m33/hard-float/libnrf_cc310_platform_0.9.0.a`
  * :file:`cortex-m33/soft-float/libnrf_cc310_platform_0.9.0.a`

  * No interrupts

    * :file:`cortex-m33/soft-float/no-interrupts/libnrf_cc310_platform_0.9.0.a`
    * :file:`cortex-m33/hard-float/no-interrupts/libnrf_cc310_platform_0.9.0.a`

  * short-wchar

    * :file:`cortex-m33/hard-float/short-wchar/libnrf_cc310_platform_0.9.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/libnrf_cc310_platform_0.9.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.0.a`
    * :file:`cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.0.a`

* nrf_cc310_platform, nRF52 variants

  * :file:`cortex-m4/soft-float/libnrf_cc310_platform_0.9.0.a`
  * :file:`cortex-m4/hard-float/libnrf_cc310_platform_0.9.0.a`

  * No interrupts

    * :file:`cortex-m4/hard-float/no-interrupts/libnrf_cc310_platform_0.9.0.a`
    * :file:`cortex-m4/soft-float/no-interrupts/libnrf_cc310_platform_0.9.0.a`

  * short-wchar

    * :file:`cortex-m4/soft-float/short-wchar/libnrf_cc310_platform_0.9.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/libnrf_cc310_platform_0.9.0.a`

  * short-wchar, no interrupts

    * :file:`cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.0.a`
    * :file:`cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_platform_0.9.0.a`
