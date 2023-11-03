.. _nrf_oberon_readme:

nrf_oberon crypto library
#########################

.. contents::
   :local:
   :depth: 2

The nrf_oberon library contains a collection of cryptographic algorithms created by Oberon Microsystems, licensed to Nordic Semiconductor ASA for redistribution.
The library provides highly optimized software for Nordic Semiconductor SoCs based on Cortex-M0, Cortex-M4, and Cortex-M33 architectures.


Supported cryptographic algorithms
==================================
* AES CTR, EAX, GCM
* ChaCha20-Poly1035
* ECDH using curve NIST secp256r1
* ECDSA sign and verify using curve NIST secp256r1
* Ed25519
* HKDF using SHA-256 and SHA-512
* HMAC using SHA-256 and SHA-512
* RSA PKCS#1 v1.5 and v2.1 (1024, 2048 bits)
* SHA-256
* SHA-512
* SRP
* SRTP


Initializing the library
========================
The library does not require any initialization before the APIs can be used, although some APIs require calling specific initialization functions before use.


Mbed TLS integration
====================
Starting from version 3.0.5, the nrf_oberon library contains a companion library that provides Mbed TLS integration for select features.
This must be used with the :ref:`nrf_security`.


Supported features
------------------
The supported features for the Mbed TLS companion library are:

* AES (all ciphers, all key sizes)
* AES CCM
* CMAC
* ECDSA (secp224r1 and secp256r1 only)
* ECDH (secp224r1 and secp256r1 only)
* ECJPAKE (secp224r1 and secp256r1 only)
* SHA-1
* SHA-256


PSA driver integration
======================
Starting from version 3.0.9, the nrf_oberon library contains a companion library that provides PSA driver integration for select features.
This must be used with the :ref:`nrf_security`.

Supported features
------------------
The supported features for the PSA driver companion library are:

* AES CTR/CBC/ECB/CCM/GCM (all key sizes)
* ChaCha20 and Poly1305 (256 bit keys)
* ECDSA (secp224r1 and secp256r1 only)
* ECDH (secp224r1 and secp256r1 only)
* SHA-1
* SHA-224
* SHA-256
* SHA-512

nrf_oberon crypto library API
=============================
:ref:`crypto_api_nrf_oberon`
