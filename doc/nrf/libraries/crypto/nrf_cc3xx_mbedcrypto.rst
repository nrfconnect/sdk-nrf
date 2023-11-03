.. _nrf_cc310_mbedcrypto_readme:
.. _nrf_cc3xx_mbedcrypto_readme:

nrf_cc3xx_mbedcrypto library
############################

.. contents::
   :local:
   :depth: 2

The nrf_cc3xx_mbedcrypto library is software library to interface with the Arm CryptoCell hardware accelerator that is available on the nRF52840 SoC, the nRF53 Series, and the nRF91 Series.
The library adds hardware support for selected cryptographic algorithms.

Integration with Mbed TLS
=========================
The nrf_cc3xx_mbedcrypto library provides low-level integration with the Mbed TLS version provided in nRF Connect SDK.
Some of the APIs expressed in this library use the Mbed TLS "alternative implementation" abstraction layer.

.. note::
   It is not recommended to link to this library directly. Use the :ref:`nrf_security`.


Supported cryptographic algorithms
==================================

The following tables show the current state of support.

.. note::
   If `no Mbed TLS support` is listed in limitations, it indicates that the hardware supports it, but it is not exposed in an API that works with Mbed TLS.


AES - Advanced Encryption Standard
----------------------------------
+-----------------------+-------------------------------+
| Cipher                | Limitations                   |
+=======================+===============================+
| CTR                   | 128-bit                       |
+-----------------------+-------------------------------+
| CBC                   | 128-bit                       |
+-----------------------+-------------------------------+
| OFB                   | 128-bit, no Mbed TLS support  |
+-----------------------+-------------------------------+
| CFB                   | 128-bit, no Mbed TLS support  |
+-----------------------+-------------------------------+
| CMAC                  | 128-bit                       |
+-----------------------+-------------------------------+


AEAD - Authenticated Encryption with Associated Data
----------------------------------------------------
+-----------------------+-------------------------------+
| Cipher                | Limitations                   |
+=======================+===============================+
| CCM/CCM*              | 128-bit                       |
+-----------------------+-------------------------------+
| ChaCha-Poly           | 128-bit                       |
+-----------------------+-------------------------------+

Diffie-Hellman-Merkel
---------------------
Supported for key sizes <= 2048 bits.

RSA
---
PKCS#1 v1.5 and v2.1 is supported for signing and encryption including:

* RSASSA-PSS
* RSAES-OEAP

Supported for key sizes <= 2048 bits.

Secure Hash
-----------
SHA-1 and SHA-256 is supported.

ECDSA and ECDH
--------------
ECDSA and ECDH is supported for the following elliptic curves:

SEC 2/NIST 186-4:

* secp160r1
* secp192r1
* secp224r1
* secp256r1
* secp384r1
* secp521r1

Koblitz:

* secp160k1
* secp192k1
* secp224k1
* secp256k1

Edwards/Montgommery:

* Ed25519
* Curve25519

Additional items in mbedtls_extra
---------------------------------

These mbedtls_extra algorithms are supported, but are not in the Mbed TLS API.

* AES key wrap functions
* ECIES
* HKDF
* SRP, up to 3072 bits

Using the library
=================

Providing platform specific calloc/free
---------------------------------------
Just like Mbed TLS, this library calls :c:func:`calloc` and :c:func:`free` for memory management.

The :c:func:`calloc` and :c:func:`free` functions can be changed with the following API:

.. code-block:: c
    :caption: Setting custom calloc/free

    int ret;

    ret = mbedtls_platform_set_calloc_free(calloc_fn, free_fn);
    if (ret != 0) {
            /* Failed to set the alternative calloc/free */
            return ret;
    }

This API must be called prior to calling :c:func:`mbedtls_platform_setup`.
Otherwise, the library will default to use the clib functions :c:func:`calloc` and :c:func:`free`.

PSA driver integration
======================
Starting from version 0.9.13, the nrf_oberon library contains a companion library that provides PSA driver integration for select features.
This must be used with the :ref:`nrf_security`.

Supported features
------------------
The supported features for the PSA driver companion library are:

* AES CTR/CBC/ECB/CCM (192/256 bit keys are only supported by CryptoCell 312)
* AES GCM (only supported by CryptoCell 312)
* ChaCha20 and Poly1305 (256 bit keys only)
* ECDSA (secp224r1, secp256r1 and secp384r1 only)
* ECDH
* RSA (PKCS#1 v1.5 with 1024 bits keys only)
* HMAC
* CMAC (192/256 bit keys are only supported by CryptoCell 312)
* HKDF
* SHA-1
* SHA-224
* SHA-256

Initializing the library
------------------------
The library requires initialization before use.
You can initialize it by calling the :c:func:`mbedtls_platform_setup`/:c:func:`mbedtls_platform_teardown` functions.

.. code-block:: c
    :caption: Initializing the library

    int ret;
    static mbedtls_platform_context platform_context = {0};

    ret = mbedtls_platform_setup(&platform_context);
    if (ret != 0) {
            /* Failed to initialize nrf_cc3xx_mbedcrypto platform */
            return ret,
    }

.. note::
   There is no need to enable/disable the CC310 hardware by writing to the ``NRF_CRYPTOCELL->ENABLE`` and ``NRF_CRYPTOCELL_S->ENABLE`` registers.
   This happens automatically when calling APIs in this library.

RNG initialization memory management
------------------------------------

The nrf_cc3xx_mbedcrypto library allocates a work buffer during RNG initialization using :c:func:`calloc` and :c:func:`free`.
The size of this work buffer is 6112 bytes.
An alternative to allocating this on the heap is to provide a reference to a static variable inside the :c:type:`mbedtls_platform_context` structure type.

.. code-block:: c
    :caption: Preventing heap-allocation for RNG initialization

    int ret;
    static mbedtls_rng_workbuf_internal rng_workbuf;
    static mbedtls_platform_context platform_context = {0};
    platform_context.p_rnd_workbuf = &rng_workbuf;

    ret = mbedtls_platform_setup(&platform_context);
    if (ret != 0) {
            /* Failed to initialize nrf_cc3xx_mbedcrypto platform */
            return ret,
    }

Usage restrictions
------------------

The library cannot be used in the :ref:`Non-Secure Processing Environment (NSPE) <nrf:app_boards_spe_nspe>` of an application that uses ARM TrustZone.

The hardware can only process one request at a time.
Therefore, this library has used mutexes to make the library thread-safe.

On Arm CryptoCell 310 devices (nRF52840 and nRF91 Series), symmetric operations (like hashing and encryption) require data input to be present in DMA acessible RAM.

.. note::

      In Arm CryptoCell 312 devices (nRF5340), there are no restrictions as CryptoCell has DMA access to Flash.

API documentation
=================

:ref:`crypto_api_nrf_cc3xx_mbedcrypto`
