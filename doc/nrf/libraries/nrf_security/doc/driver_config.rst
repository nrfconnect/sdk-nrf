.. _nrf_security_driver_config:

Driver configurations and supported features
############################################

.. contents::
   :local:
   :depth: 2

This section covers the configurations available when using PSA drivers.

.. _nrf_security_drivers_config_multiple:

Configuring multiple drivers
****************************

Multiple PSA drivers can be enabled at the same time, with added support for fine-grained control of which drivers implement support for cryptographic features.

To enable a PSA driver, set the configurations in the following table:

+---------------+--------------------------------------------------+-----------------------------------------------------+
| PSA driver    | Configuration option                             | Notes                                               |
+===============+==================================================+=====================================================+
| nrf_cc3xx     | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` | Only on nRF52840, nRF91 Series, and nRF5340 devices |
+---------------+--------------------------------------------------+-----------------------------------------------------+
| nrf_oberon    | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON`|                                                     |
+---------------+--------------------------------------------------+-----------------------------------------------------+

If multiple drivers are enabled, the first ordered item in this table takes precedence for an enabled cryptographic feature, unless the driver does not enable or support it.

The driver :ref:`nrf_security_drivers_cc3xx` allows enabling or disabling of specific PSA APIs (such as psa_cipher_encrypt, psa_sign_hash), but not individual algorithms.

The driver :ref:`nrf_security_drivers_oberon` allows finer configuration granularity, allowing you to enable or disable individual algorithms as well.

When multiple enabled drivers support the same cryptographic feature, the configuration system attempts to include only one implementation to minimize code size.

Key type configuration
**********************

To enable key types for cryptographic algorithms, set one or more of the Kconfig options in the following table:

+-----------------------+-------------------------------------------------------------+
| Key type              | Configuration option                                        |
+=======================+=============================================================+
| AES                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`              |
+-----------------------+-------------------------------------------------------------+
| Chacha20              | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`         |
+-----------------------+-------------------------------------------------------------+
| ECC Key Pair          | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR`     |
+-----------------------+-------------------------------------------------------------+
| ECC Public Key        | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`   |
+-----------------------+-------------------------------------------------------------+
| RSA Key Pair          | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR`     |
+-----------------------+-------------------------------------------------------------+
| RSA Public Key        | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`   |
+-----------------------+-------------------------------------------------------------+
| DH Key Pair           | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_DH_KEY_PAIR`      |
+-----------------------+-------------------------------------------------------------+
| DH Public key         | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_DH_PUBLIC_KEY`    |
+-----------------------+-------------------------------------------------------------+

Key type support
================

The following table shows key type support for each driver:

+-----------------------+---------------------------+----------------------------+
| Key type              | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=======================+===========================+============================+
| AES                   | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| Chacha20              | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| ECC Key Pair          | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| ECC Public Key        | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| RSA Key Pair          | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| RSA Public Key        | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| DH Key Pair           | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| DH Public key         | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+

Cipher configurations
*********************

To enable cipher modes, set one or more of the Kconfig options in the following table:

+-----------------------+------------------------------------------------------+
| Cipher mode           | Configuration option                                 |
+=======================+======================================================+
| ECB no padding        | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING` |
+-----------------------+------------------------------------------------------+
| CBC no padding        | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING` |
+-----------------------+------------------------------------------------------+
| CBC PKCS#7 padding    | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`      |
+-----------------------+------------------------------------------------------+
| CFB                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CFB`            |
+-----------------------+------------------------------------------------------+
| CTR                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`            |
+-----------------------+------------------------------------------------------+
| OFB                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_OFB`            |
+-----------------------+------------------------------------------------------+
| XTS                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XTS`            |
+-----------------------+------------------------------------------------------+
| Stream cipher         | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`  |
+-----------------------+------------------------------------------------------+

Cipher driver configurations
============================

The following table shows Cipher algorithm support for each driver:

+-----------------------+---------------------------+----------------------------+
| Cipher mode           | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=======================+===========================+============================+
| ECB no padding        | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| CBC no padding        | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| CBC PKCS#7 padding    | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| CFB                   | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| CTR                   | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| OFB                   | Supported                 | Not supported              |
+-----------------------+---------------------------+----------------------------+
| XTS                   | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| Stream cipher         | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_CIPHER_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported algorithms.

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `Cipher configurations`_.

Key size configuration is supported as described in `AES key size configuration`_, for all algorithms except the stream cipher.

.. note::
   The :ref:`nrf_security_drivers_cc3xx` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell cc310.

Key Derivation Function
***********************

To enable key derivation function (KDF) support, set one or more of the Kconfig options in the following table:

+--------------------------+---------------------------------------------------------------+
| KDF algorithm            | Configuration option                                          |
+==========================+===============================================================+
| HKDF                     | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`                    |
+--------------------------+---------------------------------------------------------------+
| PBKDF2-HMAC              | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`             |
+--------------------------+---------------------------------------------------------------+
| PBKDF2-AES-CMAC-PRF-128  | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128` |
+--------------------------+---------------------------------------------------------------+
| TLS 1.2 PRF              | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`               |
+--------------------------+---------------------------------------------------------------+
| TLS 1.2 PSK to MS        | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`         |
+--------------------------+---------------------------------------------------------------+
| TLS 1.2 EC J-PAKE to PMS | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`    |
+-------------------------+----------------------------------------------------------------+

.. note::
   PBKDF2 algorithms are not supported with TF-M.

Key Derivation Function driver configurations
=============================================

The following table shows Key Derivation Function (KDF) support for each driver:

+--------------------------+--------------------------+----------------------------+
| KDF algorithm            | nrf_cc3xx driver support | nrf_oberon driver support  |
+==========================+==========================+============================+
| HKDF                     | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| PBKDF2-HMAC              | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| PBKDF2-AES-CMAC-PRF-128  | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| TLS 1.2 PRF              | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| TLS 1.2 PSK to MS        | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| TLS 1.2 EC J-PAKE to PMS | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `Key Derivation Function`_.

MAC configurations
******************

To enable MAC support, set one or more of the Kconfig options in the following table:

+----------------+--------------------------------------------+
| MAC cipher     | Configuration option                       |
+================+============================================+
| CMAC           | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` |
+----------------+--------------------------------------------+
| HMAC           | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` |
+----------------+--------------------------------------------+

MAC driver configurations
=========================

The following table shows MAC algorithm support for each driver:

+----------------+--------------------------+----------------------------+
| MAC cipher     | nrf_cc3xx driver support | nrf_oberon driver support  |
+================+==========================+============================+
| CMAC           | Supported                | Supported                  |
+----------------+--------------------------+----------------------------+
| HMAC           | Supported                | Supported                  |
+----------------+--------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_MAC_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported algorithms.

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `MAC configurations`_.

Key size configuration for CMAC is supported as described in `AES key size configuration`_.

.. note::
   * The :ref:`nrf_security_drivers_cc3xx` is limited to CMAC using AES key sizes of 128 bits on devices with Arm CryptoCell cc310.
   * The :ref:`nrf_security_drivers_cc3xx` is limited to HMAC using SHA-1, SHA-224, and SHA-256.

AEAD configurations
*******************

To enable Authenticated Encryption with Associated Data (AEAD), set one or more of the Kconfig options in the following table:

+-----------------------+---------------------------------------------------------+
| AEAD cipher           | Configuration option                                    |
+=======================+=========================================================+
| CCM                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`               |
+-----------------------+---------------------------------------------------------+
| CCM star with no tag  | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`   |
+-----------------------+---------------------------------------------------------+
| GCM                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`               |
+-----------------------+---------------------------------------------------------+
| ChaCha20-Poly1305     | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305` |
+-----------------------+---------------------------------------------------------+

AEAD driver configurations
==========================

The following table shows AEAD algorithm support for each driver:

+-----------------------+---------------------------+---------------------------+
| AEAD cipher           | nrf_cc3xx driver support  | nrf_oberon driver support |
+=======================+===========================+===========================+
| CCM                   | Supported                 | Supported                 |
+-----------------------+---------------------------+---------------------------+
| CCM star with no tag  | Not supported             | Supported                 |
+-----------------------+---------------------------+---------------------------+
| GCM                   | Supported                 | Supported                 |
+-----------------------+---------------------------+---------------------------+
| ChaCha20-Poly1305     | Supported                 | Supported                 |
+-----------------------+---------------------------+---------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_AEAD_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported algorithms.

Configuration of the :ref:`nrf_security_drivers_oberon` driver is automatically generated based on the user-enabled algorithms in `AEAD configurations`_.

Key size configuration for CCM and GCM is supported as described in `AES key size configuration`_.

.. note::
   * The :ref:`nrf_security_drivers_cc3xx` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell cc310.
   * The :ref:`nrf_security_drivers_cc3xx` does not provide hardware support for GCM on devices with Arm CryptoCell cc310.

ECC configurations
******************

To enable Elliptic Curve Cryptography (ECC), set one or more of the Kconfig options in the following table:

+-----------------------+-----------------------------------------------------------+
| ECC algorithm         | Configuration option                                      |
+=======================+===========================================================+
| ECDH                  | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`                |
+-----------------------+-----------------------------------------------------------+
| ECDSA                 | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`               |
+-----------------------+-----------------------------------------------------------+
| ECDSA (deterministic) | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA` |
+-----------------------+-----------------------------------------------------------+

The ECC algorithm support is dependent on one or more Kconfig options enabling curve support according to `ECC curve configurations`_.

ECC driver configurations
=========================

The following table shows ECC algorithm support for each driver:

+-----------------------+---------------------------+----------------------------+
| ECC algorithm         | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=======================+===========================+============================+
| ECDH                  | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| ECDSA                 | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| ECDSA (deterministic) | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_SIGNATURE_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for the ECDSA and ECDSA deterministic algorithms.

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_ECDH_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for ECDH.

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `ECC configurations`_.

.. note::
   * The :ref:`nrf_security_drivers_oberon` is currently limited to curve types secp224r1, secp256r1, and secp384r1 for ECDH and ECDSA.
   * The :ref:`nrf_security_drivers_oberon` is currently limited to X25519 (using Curve25519) and Ed25519 for EdDSA.

ECC curve configurations
************************

To configure elliptic curve support, set one or more of the Kconfig options in the following table:

+-----------------------+-----------------------------------------------------------+
| ECC curve type        | Configuration option                                      |
+=======================+===========================================================+
| Brainpool256r1        | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`  |
+-----------------------+-----------------------------------------------------------+
| Brainpool384r1        | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`  |
+-----------------------+-----------------------------------------------------------+
| Brainpool512r1        | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`  |
+-----------------------+-----------------------------------------------------------+
| Curve25519            | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`      |
+-----------------------+-----------------------------------------------------------+
| Curve448              | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_448`      |
+-----------------------+-----------------------------------------------------------+
| Edwards25519          | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255` |
+-----------------------+-----------------------------------------------------------+
| secp192k1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`         |
+-----------------------+-----------------------------------------------------------+
| secp256k1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`         |
+-----------------------+-----------------------------------------------------------+
| secp192r1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`         |
+-----------------------+-----------------------------------------------------------+
| secp224r1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`         |
+-----------------------+-----------------------------------------------------------+
| secp256r1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`         |
+-----------------------+-----------------------------------------------------------+
| secp384r1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`         |
+-----------------------+-----------------------------------------------------------+
| secp521r1             | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`         |
+-----------------------+-----------------------------------------------------------+

ECC curve driver configurations
===============================

The following table shows ECC curve support for each driver:

+-----------------------+---------------------------+----------------------------+
| ECC curve type        | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=======================+===========================+============================+
| Brainpool256r1        | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| Brainpool384r1        | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| Brainpool512r1        | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| Curve25519            | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| Curve448              | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| Edwards25519          | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| secp192k1             | Supported                 | Not supported              |
+-----------------------+---------------------------+----------------------------+
| secp256k1             | Supported                 | Not supported              |
+-----------------------+---------------------------+----------------------------+
| secp192r1             | Supported                 | Not supported              |
+-----------------------+---------------------------+----------------------------+
| secp224r1             | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| secp256r1             | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| secp384r1             | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| secp521r1             | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+

RNG configurations
******************

Enable RNG using the :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` Kconfig option.

RNG uses PRNG seeded by entropy (also known as TRNG).
When RNG is enabled, set at least one of the configurations in the following table:

+---------------------------+-------------------------------------------------+
| PRNG algorithms           | Configuration option                            |
+===========================+=================================================+
| CTR-DRBG                  | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`  |
+---------------------------+-------------------------------------------------+
| HMAC-DRBG                 | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` |
+---------------------------+-------------------------------------------------+

.. note::
   * Both PRNG algorithms are NIST qualified Cryptographically Secure Pseudo Random Number Generators (CSPRNG).
   * :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG` and :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` are custom configurations not described by the PSA Crypto specification.
   * If multiple PRNG algorithms are enabled at the same time, CTR-DRBG will be prioritized for random number generation through the front-end APIs for PSA Crypto.

RNG driver configurations
*************************

There are no public configurations for entropy and PRNG algorithm support and the choice of drivers that provide support is automatic.

The PSA drivers using the Arm CryptoCell peripheral is enabled by default for nRF52840, nRF91 Series, and nRF5340 devices.

For devices without a hardware-accelerated cryptographic engine, entropy is provided by the nRF RNG peripheral. PRNG support is provided by the Oberon PSA driver, which is implemented using software.

RSA configurations
******************

To enable Rivest-Shamir-Adleman (RSA) support, set one or more of the Kconfig options in the following table:

+-----------------------+----------------------------------------------------------+
| RSA algorithms        | Configuration option                                     |
+=======================+==========================================================+
| RSA OAEP              | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`           |
+-----------------------+----------------------------------------------------------+
| RSA PKCS#1 v1.5 crypt | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` |
+-----------------------+----------------------------------------------------------+
| RSA PKCS#1 v1.5 sign  | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`  |
+-----------------------+----------------------------------------------------------+
| RSA PSS               | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`            |
+-----------------------+----------------------------------------------------------+

RSA driver configurations
=========================

The following table shows RSA algorithm support for each driver:

+-----------------------+---------------------------+----------------------------+
| RSA algorithms        | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=======================+===========================+============================+
| RSA OAEP              | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| RSA PKCS#1 v1.5 crypt | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| RSA PKCS#1 v1.5 sign  | Supported                 | Supported                  |
+-----------------------+---------------------------+----------------------------+
| RSA PSS               | Not supported             | Supported                  |
+-----------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_SIGNATURE_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for the RSA PKCS#1 v1.5 signing algorithm.

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for RSA PKCS#1 v1.5 and RSA OAEP encryption.

Configuration of the :ref:`nrf_security_drivers_oberon` driver is automatically generated based on the user-enabled algorithms in  `RSA configurations`_.

RSA key size configuration is supported as described in `RSA key size configuration`_.

.. note::
   * :ref:`nrf_security_drivers_cc3xx` is limited to key sizes less than or equal to 2048 bits.
   * :ref:`nrf_security_drivers_oberon` does not support RSA key pair generation.

Hash configurations
*******************

To configure the Hash algorithms, set one or more of the Kconfig options in the following table:

+-----------------------+---------------------------------------------------+
| Hash algorithm        | Configuration option                              |
+=======================+===================================================+
| SHA-1 (weak)          | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`       |
+-----------------------+---------------------------------------------------+
| SHA-224               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`     |
+-----------------------+---------------------------------------------------+
| SHA-256               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`     |
+-----------------------+---------------------------------------------------+
| SHA-384               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`     |
+-----------------------+---------------------------------------------------+
| SHA-512               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`     |
+-----------------------+---------------------------------------------------+
| MD5 (weak)            | :kconfig:option:`CONFIG_PSA_WANT_ALG_MD5`         |
+-----------------------+---------------------------------------------------+
| RIPEMD-160            | :kconfig:option:`CONFIG_PSA_WANT_ALG_RIPEMD160`   |
+-----------------------+---------------------------------------------------+

.. note::
   * The SHA-1 hash is weak and deprecated and is only recommended for use in legacy protocols.
   * The MD5 hash is weak and deprecated and is only recommended for use in legacy protocols.

Hash driver configurations
==========================

The following table shows Hash algorithm support for each driver:

+-----------------------+----------------------------+---------------------------+
| Hash algorithm        |  nrf_cc3xx driver support  | nrf_oberon driver support |
+=======================+============================+===========================+
| SHA-1 (weak)          | Supported                  | Supported                 |
+-----------------------+----------------------------+---------------------------+
| SHA-224               | Supported                  | Supported                 |
+-----------------------+----------------------------+---------------------------+
| SHA-256               | Supported                  | Supported                 |
+-----------------------+----------------------------+---------------------------+
| SHA-384               | Not supported              | Supported                 |
+-----------------------+----------------------------+---------------------------+
| SHA-512               | Not supported              | Supported                 |
+-----------------------+----------------------------+---------------------------+
| MD5 (weak)            | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| RIPEMD160             | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_HASH_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all the supported algorithms.

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in  `HASH configurations`_.

Password-authenticated key agreement configurations
***************************************************

To enable password-authenticated key agreement (PAKE) support, set one or more of the Kconfig options in the following table:

+-----------------------+-----------------------------------------------+
| PAKE algorithm        | Configuration option                          |
+=======================+===============================================+
| EC J-PAKE             | :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`   |
+-----------------------+-----------------------------------------------+
| SPAKE2+               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P` |
+-----------------------+-----------------------------------------------+
| SRP-6                 | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`   |
+-----------------------+-----------------------------------------------+

.. note::
   * The provided support is experimental.
   * Not supported with TF-M.

Password-authenticated key agreement driver configurations
==========================================================

The following table shows PAKE algorithm support for each driver:

+-----------------------+--------------------------+---------------------------+
| PAKE algorithm        | nrf_cc3xx driver support | nrf_oberon driver support |
+=======================+==========================+===========================+
| EC J-PAKE             | Not supported            | Supported                 |
+-----------------------+--------------------------+---------------------------+
| SPAKE2+               | Not supported            | Supported                 |
+-----------------------+--------------------------+---------------------------+
| SRP-6                 | Not supported            | Supported                 |
+-----------------------+--------------------------+---------------------------+

Configuration of the :ref:`nrf_security_drivers_oberon` driver is automatically generated based on the user-enabled algorithms in  `Password-authenticated key agreement configurations`_.

Key size configurations
***********************

:ref:`nrf_security` supports key size configuration options for AES and RSA keys.

AES key size configuration
==========================

To enable AES key size support, set one or more of the Kconfig options in the following table:

+--------------+----------------------------------------------------+
| AES key size | Configuration option                               |
+==============+====================================================+
| 128 bits     | :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128` |
+--------------+----------------------------------------------------+
| 192 bits     | :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192` |
+--------------+----------------------------------------------------+
| 256 bits     | :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256` |
+--------------+----------------------------------------------------+

.. note::
   All AES key size configurations are introduced by :ref:`nrf_security` and are not described by the PSA Crypto specification.

RSA key size configuration
==========================

To enable RSA key size support, set one or more of the Kconfig options in the following table:

+--------------------+-----------------------------------------------------+
| RSA key size       | Configuration option                                |
+====================+=====================================================+
| 1024 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024` |
+--------------------+-----------------------------------------------------+
| 1536 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536` |
+--------------------+-----------------------------------------------------+
| 2048 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048` |
+--------------------+-----------------------------------------------------+
| 3072 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072` |
+--------------------+-----------------------------------------------------+
| 4096 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096` |
+--------------------+-----------------------------------------------------+
| 6144 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144` |
+--------------------+-----------------------------------------------------+
| 8192 bits          | :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192` |
+--------------------+-----------------------------------------------------+

.. note::
   All RSA key size configurations are introduced by :ref:`nrf_security` and are not described by the PSA Crypto specification.
