.. _ug_crypto_supported_features:

Supported cryptographic operations in the |NCS|
###############################################

.. contents::
   :local:
   :depth: 2

This reference page lists the supported features and limitations of cryptographic operations in the |NCS|.

A supported feature is implemented and maintained, and is suitable for product development.
See :ref:`software_maturity` for more details and other definitions.

.. note::
   Cryptographic features and algorithms that are not listed on this page are not supported by Oberon PSA Crypto in the |NCS|.

.. _ug_crypto_supported_features_hardware:

Hardware support for PSA Crypto implementations
***********************************************

The following tables list hardware support for the PSA Crypto implementations in the |NCS|.

.. include:: ../../releases_and_maturity/software_maturity.rst
    :start-after: crypto_support_table_start
    :end-before: crypto_support_table_end

.. _ug_crypto_supported_features_operations:

Cryptographic feature support
*****************************

The following sections list the supported cryptographic features and algorithms for each of the drivers: :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`, :ref:`CRACEN <crypto_drivers_cracen>`, and :ref:`nrf_oberon <crypto_drivers_oberon>`.
The listed Kconfig options enable the features and algorithms for the drivers that support them.

The Kconfig options follow the ``CONFIG_PSA_WANT_`` + ``CONFIG_PSA_USE_`` configuration scheme, which is described in detail on the :ref:`crypto_drivers` page.

Key types and key management
============================

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific key types, as well as their support for each driver:

.. list-table:: Key type configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - Key type
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - AES
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
     - Supported
     - Supported
     - Supported
   * - ARIA
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ARIA`
     - Not supported
     - Not supported
     - Not supported
   * - DES (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_DES`
     - Not supported
     - Not supported
     - Not supported
   * - CAMELLIA
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CAMELLIA`
     - Not supported
     - Not supported
     - Not supported
   * - SM4
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SM4`
     - Not supported
     - Not supported
     - Not supported
   * - ARC4 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ARC4`
     - Not supported
     - Not supported
     - Not supported
   * - Chacha20
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
     - Supported
     - Supported
     - Supported
   * - ECC Key Pair
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR`
     - Supported
     - Supported
     - Supported
   * - ECC Public Key
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
     - Supported
     - Supported
     - Supported
   * - RSA Key Pair
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR`
     - Supported
     - Supported
     - Supported
   * - RSA Public Key
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
     - Supported
     - Supported
     - Supported
   * - DH Key Pair
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_DH_KEY_PAIR`
     - Not supported
     - Not supported
     - Not supported
   * - DH Public Key
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY`
     - Not supported
     - Not supported
     - Not supported

Key management
--------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for management of the wanted key type.

.. list-table:: Key management support per driver
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported key types
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
     - AES, Chacha20, ECC Key Pair, ECC Public Key, RSA Key Pair, RSA Public Key
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_MANAGEMENT_DRIVER`
     - AES, Chacha20, ECC Key Pair, ECC Public Key, RSA Key Pair, RSA Public Key
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled key types. Acts as fallback for the other drivers.
     - AES, Chacha20, ECC Key Pair, ECC Public Key, RSA Key Pair, RSA Public Key

Cipher modes
============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific cipher modes, as well as their support for each driver:

.. list-table:: Cipher mode configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - Cipher mode
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - ECB no padding
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
     - Supported
     - Supported
     - Supported
   * - CBC no padding
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
     - Supported
     - Supported
     - Supported
   * - CBC PKCS#7 padding
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
     - Supported
     - Supported
     - Supported
   * - CTR
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
     - Supported
     - Supported
     - Supported
   * - CCM* no tag
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
     - Not supported
     - Not supported
     - Supported
   * - XTS
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_XTS`
     - Not supported
     - Not supported
     - Not supported
   * - Stream cipher
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
     - Supported
     - Supported
     - Supported

.. note::
   Key size configuration is supported as described in `AES key size configuration`_, for all algorithms except the stream cipher.

Cipher driver
-------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted cipher modes.

.. list-table:: Cipher driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported cipher modes
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_CIPHER_DRIVER`
     - ECB no padding, CBC no padding, CBC PKCS#7 padding, CTR, Stream cipher
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_CIPHER_DRIVER`
     - ECB no padding, CBC no padding, CBC PKCS#7 padding, CTR, Stream cipher
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled cipher modes. Acts as fallback for the other drivers.
     - ECB no padding, CBC no padding, CBC PKCS#7 padding, CTR, CCM* no tag, Stream cipher

.. note::
   The :ref:`nrf_security_drivers_cc3xx` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310 (nrf_cc310).

Key agreement algorithms
========================

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific key agreement algorithms, as well as their support for each driver:

.. list-table:: Key agreement algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - Key agreement algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - ECDH
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
     - Supported
     - Supported
     - Supported
   * - FFDH
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_FFDH`
     - Not supported
     - Not supported
     - Not supported

.. note::
   The algorithm support when using ECC key types is dependent on one or more Kconfig options enabling curve support according to `ECC curve types`_.

Key agreement driver
--------------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted key agreement algorithms.

.. list-table:: Key agreement driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported key agreement algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_AGREEMENT_DRIVER`
     - ECDH
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_AGREEMENT_DRIVER`
     - ECDH
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled key agreement algorithms. Acts as fallback for the other drivers.
     - ECDH (limited to curve types secp224r1, secp256r1, secp384r1, and Curve25519)

KDF algorithms
==============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific key derivation function (KDF) algorithms, as well as their support for each driver:

.. list-table:: KDF algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - KDF algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - HKDF
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
     - Not supported
     - Supported
     - Supported
   * - HKDF-Extract
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
     - Not supported
     - Supported
     - Supported
   * - HKDF-Expand
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
     - Not supported
     - Supported
     - Supported
   * - PBKDF2-HMAC
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
     - Not supported
     - Supported
     - Supported
   * - PBKDF2-AES-CMAC-PRF-128
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
     - Not supported
     - Supported
     - Supported
   * - TLS 1.2 PRF
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
     - Not supported
     - Supported
     - Supported
   * - TLS 1.2 PSK to MS
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
     - Not supported
     - Supported
     - Supported
   * - TLS 1.2 EC J-PAKE to PMS
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
     - Not supported
     - Supported
     - Supported
   * - SP 800-108r1 CMAC w/counter
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
     - Not supported
     - Supported
     - Not supported
   * - SP 800-108 HMAC counter mode
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`
     - Not supported
     - Supported
     - Supported

Key derivation function driver
------------------------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted KDF algorithms.

.. list-table:: KDF driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported KDF algorithms
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_DERIVATION_DRIVER`
     - HKDF, HKDF-Extract, HKDF-Expand, PBKDF2-HMAC, PBKDF2-AES-CMAC-PRF-128, TLS 1.2 EC J-PAKE to PMS, SP 800-108r1 CMAC w/counter, SP 800-108 HMAC counter mode
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled KDF algorithms. Acts as fallback for the other drivers.
     - HKDF, HKDF-Extract, HKDF-Expand, PBKDF2-HMAC, PBKDF2-AES-CMAC-PRF-128, TLS 1.2 PRF, TLS 1.2 PSK to MS, TLS 1.2 EC J-PAKE to PMS, SP 800-108 HMAC counter mode

MAC algorithms
==============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific Message Authentication Code (MAC) algorithms, as well as their support for each driver:

.. list-table:: MAC algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - MAC algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - CMAC
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
     - Supported
     - Supported
     - Supported
   * - HMAC
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
     - Supported
     - Supported
     - Supported
   * - CBC-MAC
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_MAC`
     - Not supported
     - Not supported
     - Not supported

.. note::
   Key size configuration for CMAC is supported as described in `AES key size configuration`_.

MAC driver
----------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted MAC algorithms.

.. list-table:: MAC driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported MAC algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_MAC_DRIVER`
     - CMAC (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310), HMAC (limited to SHA-1, SHA-224, and SHA-256)
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_MAC_DRIVER`
     - CMAC, HMAC
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled MAC algorithms. Acts as fallback for the other drivers.
     - CMAC, HMAC

AEAD algorithms
===============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific Authenticated Encryption with Associated Data (AEAD) algorithms, as well as their support for each driver:

.. list-table:: AEAD algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - AEAD algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - CCM
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
     - Supported
     - Supported
     - Supported
   * - GCM
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
     - Supported
     - Supported
     - Supported
   * - ChaCha20-Poly1305
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
     - Supported
     - Supported
     - Supported

.. note::
   - Key size configuration for CCM and GCM is supported as described in `AES key size configuration`_.
   - CRACEN only supports a 96-bit IV for AES GCM.

AEAD driver
-----------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted AEAD algorithms.

.. list-table:: AEAD driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported AEAD algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_AEAD_DRIVER`
     - CCM, GCM (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310, no hardware support for GCM on CC310), ChaCha20-Poly1305
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_AEAD_DRIVER`
     - CCM, GCM, ChaCha20-Poly1305
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled AEAD algorithms. Acts as fallback for the other drivers.
     - CCM, GCM, ChaCha20-Poly1305

Asymmetric signature algorithms
===============================

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific asymmetric signature algorithms, as well as their support for each driver:

.. list-table:: Asymmetric signature algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - Asymmetric signature algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - ECDSA
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
     - Supported
     - Supported
     - Supported
   * - ECDSA without hashing
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
     - Supported
     - Supported
     - Supported
   * - ECDSA (deterministic)
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
     - Supported
     - Supported
     - Supported
   * - PureEdDSA
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
     - Supported
     - Supported
     - Supported
   * - HashEdDSA Edwards25519
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
     - Not supported
     - Supported
     - Not supported
   * - HashEdDSA Edwards448
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED448PH`
     - Not supported
     - Not supported
     - Not supported
   * - RSA PKCS#1 v1.5 sign
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
     - Supported
     - Supported
     - Supported
   * - RSA raw PKCS#1 v1.5 sign
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
     - Supported
     - Not supported
     - Supported
   * - RSA PSS
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
     - Not supported
     - Supported
     - Supported
   * - RSA PSS any salt
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
     - Not supported
     - Not supported
     - Supported

.. note::
   * The algorithm support when using ECC key types is dependent on one or more Kconfig options enabling curve support according to `ECC curve types`_.
   * RSA key size configuration is supported as described in `RSA key size configuration`_.

Asymmetric signature driver
---------------------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted asymmetric signature algorithms.

.. list-table:: Asymmetric signature driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported asymmetric signature algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER`
     - ECDSA, ECDSA without hashing, ECDSA (deterministic), PureEdDSA, RSA PKCS#1 v1.5 sign, RSA raw PKCS#1 v1.5 sign (limited to RSA key sizes ≤ 2048 bits)
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER`
     - ECDSA, ECDSA without hashing, ECDSA (deterministic), PureEdDSA, HashEdDSA Edwards25519, RSA PKCS#1 v1.5 sign, RSA PSS
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled asymmetric signature algorithms. Acts as fallback for the other drivers.
     - ECDSA (limited to ECC curve types secp224r1, secp256r1, and secp384r1), ECDSA without hashing, ECDSA (deterministic), PureEdDSA (limited to ECC curve type Ed25519), RSA PKCS#1 v1.5 sign, RSA raw PKCS#1 v1.5 sign, RSA PSS, RSA PSS any salt (does not support RSA key pair generation)

Asymmetric encryption algorithms
================================

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific asymmetric encryption algorithms, as well as their support for each driver:

.. list-table:: Asymmetric encryption algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - Asymmetric encryption algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - RSA OAEP
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
     - Supported
     - Supported
     - Supported
   * - RSA PKCS#1 v1.5 crypt
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
     - Supported
     - Supported
     - Supported

.. note::
   RSA key size configuration is supported as described in `RSA key size configuration`_.

Asymmetric encryption driver
----------------------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted asymmetric encryption algorithms.

.. list-table:: Asymmetric encryption driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported asymmetric encryption algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER`
     - RSA OAEP, RSA PKCS#1 v1.5 crypt (limited to key sizes ≤ 2048 bits)
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER`
     - RSA OAEP, RSA PKCS#1 v1.5 crypt
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled asymmetric encryption algorithms. Acts as fallback for the other drivers.
     - RSA OAEP, RSA PKCS#1 v1.5 crypt (does not support RSA key pair generation)

ECC curve types
===============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific ECC curve types, as well as their support for each driver:

.. list-table:: ECC curve type configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - ECC curve type
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - BrainpoolP160r1 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_160`
     - Not supported
     - Not supported
     - Not supported
   * - BrainpoolP192r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
     - Not supported
     - Supported
     - Not supported
   * - BrainpoolP224r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
     - Not supported
     - Supported
     - Not supported
   * - BrainpoolP256r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
     - Supported
     - Supported
     - Not supported
   * - BrainpoolP320r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
     - Not supported
     - Supported
     - Not supported
   * - BrainpoolP384r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
     - Not supported
     - Supported
     - Not supported
   * - BrainpoolP512r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
     - Not supported
     - Supported
     - Not supported
   * - Curve25519 (X25519)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
     - Supported
     - Supported
     - Supported
   * - Curve448 (X448)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_448`
     - Not supported
     - Not supported
     - Not supported
   * - Edwards25519 (Ed25519)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
     - Supported
     - Supported
     - Supported
   * - Edwards448 (Ed448)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
     - Not supported
     - Not supported
     - Supported
   * - secp192k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
     - Supported
     - Supported
     - Not supported
   * - secp224k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_224`
     - Not supported
     - Not supported
     - Not supported
   * - secp256k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
     - Supported
     - Supported
     - Not supported
   * - secp192r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
     - Supported
     - Supported
     - Not supported
   * - secp224r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
     - Supported
     - Supported
     - Supported
   * - secp256r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
     - Supported
     - Supported
     - Supported
   * - secp384r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
     - Supported
     - Supported
     - Supported
   * - secp521r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
     - Not supported
     - Supported
     - Not supported
   * - secp160r2 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R2_160`
     - Not supported
     - Not supported
     - Not supported
   * - sect163k1 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_163`
     - Not supported
     - Not supported
     - Not supported
   * - sect233k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_233`
     - Not supported
     - Not supported
     - Not supported
   * - sect239k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_239`
     - Not supported
     - Not supported
     - Not supported
   * - sect283k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_283`
     - Not supported
     - Not supported
     - Not supported
   * - sect409k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_409`
     - Not supported
     - Not supported
     - Not supported
   * - sect571k1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_571`
     - Not supported
     - Not supported
     - Not supported
   * - sect163r1 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_163`
     - Not supported
     - Not supported
     - Not supported
   * - sect233r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_233`
     - Not supported
     - Not supported
     - Not supported
   * - sect283r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_283`
     - Not supported
     - Not supported
     - Not supported
   * - sect409r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_409`
     - Not supported
     - Not supported
     - Not supported
   * - sect571r1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_571`
     - Not supported
     - Not supported
     - Not supported
   * - sect163r2 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R2_163`
     - Not supported
     - Not supported
     - Not supported
   * - FRP256v1
     - :kconfig:option:`CONFIG_PSA_WANT_ECC_FRP_V1_256`
     - Not supported
     - Not supported
     - Not supported

ECC curve driver
----------------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for key management using the wanted ECC curve types.

.. list-table:: ECC curve driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported ECC curve types
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
     - BrainpoolP256r1, Curve25519 (X25519), Edwards25519 (Ed25519), secp192k1, secp256k1, secp192r1, secp224r1, secp256r1, secp384r1
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_MANAGEMENT_DRIVER`
     - BrainpoolP192r1, BrainpoolP224r1, BrainpoolP256r1, BrainpoolP320r1, BrainpoolP384r1, BrainpoolP512r1, Curve25519 (X25519), Edwards25519 (Ed25519), secp192k1, secp256k1, secp192r1, secp224r1, secp256r1, secp384r1, secp521r1
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled ECC curve types. Acts as fallback for the other drivers.
     - Curve25519 (X25519), Edwards25519 (Ed25519), Edwards448 (Ed448), secp224r1, secp256r1, secp384r1

RNG algorithms
==============

RNG uses PRNG seeded by entropy (also known as TRNG).
The following table shows the Kconfig options for enabling RNG and requesting Oberon PSA Crypto to use specific PRNG algorithms, as well as their support for each driver:

.. list-table:: PRNG algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - PRNG algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - RNG support
     - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
     - Supported
     - Supported
     - Supported
   * - CTR-DRBG
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
     - Supported
     - Supported
     - Supported
   * - HMAC-DRBG
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
     - Supported
     - Not supported
     - Supported

.. note::
   * Both PRNG algorithms are NIST-qualified Cryptographically Secure Pseudo Random Number Generators (CSPRNG).
   * :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG` and :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` are custom configurations not described by the PSA Crypto specification.
   * If multiple PRNG algorithms are enabled at the same time, CTR-DRBG will be prioritized for random number generation through the front-end APIs for PSA Crypto.

RNG driver
----------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted RNG algorithms.

.. list-table:: RNG driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported RNG algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - Enabled by default for nRF52840, nRF91 Series, and nRF5340 devices
     - CTR-DRBG, HMAC-DRBG (limited to 1024 bytes per request)
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_CTR_DRBG_DRIVER`
     - CTR-DRBG
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled RNG algorithms. Acts as fallback for the other drivers.
     - CTR-DRBG, HMAC-DRBG (software implementation, entropy provided by nRF RNG peripheral)

Hash algorithms
===============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific hash algorithms, as well as their support for each driver:

.. list-table:: Hash algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - Hash algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - SHA-1 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
     - Supported
     - Supported
     - Supported
   * - SHA-224
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
     - Supported
     - Supported
     - Supported
   * - SHA-256
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
     - Supported
     - Supported
     - Supported
   * - SHA-384
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
     - Not supported
     - Supported
     - Supported
   * - SHA-512
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
     - Not supported
     - Supported
     - Supported
   * - SHA-512/224
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512_224`
     - Not supported
     - Not supported
     - Not supported
   * - SHA-512/256
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512_256`
     - Not supported
     - Not supported
     - Not supported
   * - SHA3-224
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
     - Not supported
     - Supported
     - Not supported
   * - SHA3-256
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
     - Not supported
     - Supported
     - Not supported
   * - SHA3-384
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
     - Not supported
     - Supported
     - Not supported
   * - SHA3-512
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
     - Not supported
     - Supported
     - Not supported
   * - SM3
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SM3`
     - Not supported
     - Not supported
     - Not supported
   * - SHAKE256 512 bits
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
     - Not supported
     - Not supported
     - Not supported
   * - MD2 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_MD2`
     - Not supported
     - Not supported
     - Not supported
   * - MD4 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_MD4`
     - Not supported
     - Not supported
     - Not supported
   * - MD5 (weak)
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_MD5`
     - Not supported
     - Not supported
     - Not supported
   * - RIPEMD-160
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_RIPEMD160`
     - Not supported
     - Not supported
     - Not supported

.. note::
   * The SHA-1 hash is weak and deprecated and is only recommended for use in legacy protocols.
   * The MD5 hash is weak and deprecated and is only recommended for use in legacy protocols.

Hash driver
-----------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted hash algorithms.

.. list-table:: Hash driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported hash algorithms
   * - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :kconfig:option:`CONFIG_PSA_USE_CC3XX_HASH_DRIVER`
     - SHA-1 (weak), SHA-224, SHA-256
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_HASH_DRIVER`
     - SHA-1 (weak), SHA-224, SHA-256, SHA-384, SHA-512, SHA3-224, SHA3-256, SHA3-384, SHA3-512
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled hash algorithms. Acts as fallback for the other drivers.
     - SHA-1 (weak), SHA-224, SHA-256, SHA-384, SHA-512

PAKE algorithms
===============

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific password-authenticated key exchange (PAKE) algorithms, as well as their support for each driver:

.. note::
   The support for PAKE algorithms is :ref:`experimental <software_maturity>`.

.. list-table:: PAKE algorithm configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - PAKE algorithm
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - EC J-PAKE
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
     - Not supported
     - Supported
     - Supported
   * - SPAKE2+ with HMAC
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
     - Not supported
     - Supported
     - Supported
   * - SPAKE2+ with CMAC
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
     - Not supported
     - Supported
     - Supported
   * - SPAKE2+ for Matter
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
     - Not supported
     - Supported
     - Supported
   * - SRP-6
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
     - Not supported
     - Supported
     - Supported
   * - SRP password hashing
     - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`
     - Not supported
     - Supported
     - Supported

PAKE driver
-----------

The following table shows the Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted PAKE algorithms.

.. list-table:: PAKE driver support
   :header-rows: 1
   :widths: auto

   * - Driver
     - Kconfig option
     - Supported PAKE algorithms
   * - :ref:`CRACEN <crypto_drivers_cracen>`
     - :kconfig:option:`CONFIG_PSA_USE_CRACEN_PAKE_DRIVER`
     - EC J-PAKE, SPAKE2+ with HMAC, SPAKE2+ with CMAC, SPAKE2+ for Matter, SRP-6, SRP password hashing
   * - :ref:`nrf_oberon <crypto_drivers_oberon>`
     - Configuration automatically generated based on the enabled PAKE algorithms. Acts as fallback for the other drivers.
     - EC J-PAKE, SPAKE2+ with HMAC, SPAKE2+ with CMAC, SPAKE2+ for Matter, SRP-6, SRP password hashing

Key pair operations
===================

The following sections list the supported key pair operation Kconfig options for different key types.
The Kconfig options follow the ``CONFIG_PSA_WANT_`` configuration scheme, which is described in detail on the :ref:`crypto_drivers` page.

RSA key pair operations
-----------------------

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific RSA key pair operations, as well as their support for each driver:

.. list-table:: RSA key pair operation configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - RSA key pair operation
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - Import
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
     - Supported
     - Supported
     - Supported
   * - Export
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
     - Supported
     - Supported
     - Supported
   * - Generate
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
     - Supported
     - Supported
     - Not supported
   * - Derive
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
     - Supported
     - Supported
     - Supported

SRP key pair operations
-----------------------

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific SRP key pair operations, as well as their support for each driver:

.. list-table:: SRP key pair operation configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - SRP key pair operation
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - Import
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_IMPORT`
     - Not supported
     - Supported
     - Supported
   * - Export
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_EXPORT`
     - Not supported
     - Supported
     - Supported
   * - Generate
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_GENERATE`
     - Not supported
     - Supported
     - Not supported
   * - Derive
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_DERIVE`
     - Not supported
     - Supported
     - Supported

SPAKE2P key pair operations
---------------------------

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific SPAKE2P key pair operations, as well as their support for each driver:

.. list-table:: SPAKE2P key pair operation configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - SPAKE2P key pair operation
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - Import
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT`
     - Not supported
     - Supported
     - Supported
   * - Export
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT`
     - Not supported
     - Supported
     - Supported
   * - Generate
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE`
     - Not supported
     - Supported
     - Supported
   * - Derive
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`
     - Not supported
     - Supported
     - Supported

ECC key pair operations
-----------------------

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific ECC key pair operations, as well as their support for each driver:

.. list-table:: ECC key pair operation configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - ECC key pair operation
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - Generate
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
     - Supported
     - Supported
     - Supported
   * - Import
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
     - Supported
     - Supported
     - Supported
   * - Export
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
     - Supported
     - Supported
     - Supported
   * - Derive
     - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
     - Supported
     - Supported
     - Supported

.. _ug_crypto_supported_features_key_size:

Key size configurations
=======================

The following sections list the supported AES and RSA key size Kconfig options.
The Kconfig options follow the ``CONFIG_PSA_WANT_`` configuration scheme, which is described in detail on the :ref:`crypto_drivers` page.

AES key size configuration
--------------------------

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific AES key sizes, as well as their support for each driver:

.. list-table:: AES key size configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - AES key size
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - 128 bits
     - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
     - Supported
     - Supported
     - Supported
   * - 192 bits
     - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
     - Not supported (nrf_cc310)
     - Supported
     - Supported
   * - 256 bits
     - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
     - Not supported (nrf_cc310)
     - Supported
     - Supported

.. note::
   CRACEN only supports a 96-bit IV for AES GCM.

RSA key size configuration
--------------------------

The following table shows the Kconfig options for requesting Oberon PSA Crypto to use specific RSA key sizes, as well as their support for each driver:

.. list-table:: RSA key size configuration options and support per driver
   :header-rows: 1
   :widths: auto

   * - RSA key size
     - Configuration option
     - :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`
     - :ref:`CRACEN <crypto_drivers_cracen>`
     - :ref:`nrf_oberon <crypto_drivers_oberon>`
   * - 1024 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
     - Supported
     - Not supported
     - Supported
   * - 1536 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
     - Supported
     - Not supported
     - Supported
   * - 2048 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
     - Supported
     - Supported
     - Supported
   * - 3072 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
     - Supported (nrf_cc312 only)
     - Supported
     - Supported
   * - 4096 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
     - Not supported
     - Supported
     - Supported
   * - 6144 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
     - Not supported
     - Not supported
     - Supported
   * - 8192 bits
     - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
     - Not supported
     - Not supported
     - Supported
