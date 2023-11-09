.. _nrf_security_driver_config:

Feature configurations and driver support
#########################################

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

Key type configurations
***********************

To enable key types for cryptographic algorithms, set one or more of the Kconfig options in the following table:

+-----------------------+-------------------------------------------------------------+
| Key type              | Configuration option                                        |
+=======================+=============================================================+
| AES                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`              |
+-----------------------+-------------------------------------------------------------+
| ARIA                  | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ARIA`             |
+-----------------------+-------------------------------------------------------------+
| DES (weak)            | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_DES`              |
+-----------------------+-------------------------------------------------------------+
| CAMELLIA              | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CAMELLIA`         |
+-----------------------+-------------------------------------------------------------+
| SM4                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SM4`              |
+-----------------------+-------------------------------------------------------------+
| ARC4 (weak)           | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ARC4`             |
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
| ARIA                  | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| DES (weak)            | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| CAMELLIA              | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| SM4                   | Not supported             | Not supported              |
+-----------------------+---------------------------+----------------------------+
| ARC4 (weak)           | Not supported             | Not supported              |
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

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported key types.

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
| CCM* no tag           | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`|
+-----------------------+------------------------------------------------------+
| XTS                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XTS`            |
+-----------------------+------------------------------------------------------+
| Stream cipher         | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`  |
+-----------------------+------------------------------------------------------+

Cipher support
==============

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
| CCM* no tag           | Not supported             | Supported                  |
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

Key agreement configurations
****************************

To enable key agreement support, set one or more of the Kconfig options in the following table:

+-------------------------+-----------------------------------------------------------+
| Key agreement algorithm | Configuration option                                      |
+=========================+===========================================================+
| ECDH                    | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`                |
+-------------------------+-----------------------------------------------------------+
| FFDH                    | :kconfig:option:`CONFIG_PSA_WANT_ALG_FFDH`                |
+-------------------------+-----------------------------------------------------------+

Key agreement support
=====================

The following table shows Key agreement support for each driver:

+-------------------------+---------------------------+----------------------------+
| Key agreement algorithm | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=========================+===========================+============================+
| ECDH                    | Supported                 | Supported                  |
+-------------------------+---------------------------+----------------------------+
| FFDH                    | Not supported             | Not supported              |
+-------------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_AGREEMENT_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported algorithms.

The algorithm support when using ECC key types is dependent on one or more Kconfig options enabling curve support according to `ECC curve configurations`_.

.. note::
   The :ref:`nrf_security_drivers_oberon` is currently limited to curve types secp224r1, secp256r1, secp384r1, and Curve25519 for ECDH.

Key derivation function configurations
**************************************

To enable key derivation function (KDF) support, set one or more of the Kconfig options in the following table:

+--------------------------+---------------------------------------------------------------+
| KDF algorithm            | Configuration option                                          |
+==========================+===============================================================+
| HKDF                     | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`                    |
+--------------------------+---------------------------------------------------------------+
| HKDF-Extract             | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`            |
+--------------------------+---------------------------------------------------------------+
| HKDF-Expand              | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`             |
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
+--------------------------+---------------------------------------------------------------+

.. note::
   PBKDF2 algorithms are not supported with TF-M.

Key derivation function support
===============================

The following table shows key derivation function (KDF) support for each driver:

+--------------------------+--------------------------+----------------------------+
| KDF algorithm            | nrf_cc3xx driver support | nrf_oberon driver support  |
+==========================+==========================+============================+
| HKDF                     | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| HKDF-Extract             | Not supported            | Supported                  |
+--------------------------+--------------------------+----------------------------+
| HKDF-Expand              | Not supported            | Supported                  |
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

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `Key derivation function configurations`_.

MAC configurations
******************

To enable MAC support, set one or more of the Kconfig options in the following table:

+----------------+----------------------------------------------+
| MAC cipher     | Configuration option                         |
+================+==============================================+
| CMAC           | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`   |
+----------------+----------------------------------------------+
| HMAC           | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`   |
+----------------+----------------------------------------------+
| CBC-MAC        | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_MAC`|
+----------------+----------------------------------------------+

MAC support
===========

The following table shows MAC algorithm support for each driver:

+----------------+--------------------------+----------------------------+
| MAC cipher     | nrf_cc3xx driver support | nrf_oberon driver support  |
+================+==========================+============================+
| CMAC           | Supported                | Supported                  |
+----------------+--------------------------+----------------------------+
| HMAC           | Supported                | Supported                  |
+----------------+--------------------------+----------------------------+
| CBC-MAC        | Not supported            | Not supported              |
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
| GCM                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`               |
+-----------------------+---------------------------------------------------------+
| ChaCha20-Poly1305     | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305` |
+-----------------------+---------------------------------------------------------+

AEAD support
============

The following table shows AEAD algorithm support for each driver:

+-----------------------+---------------------------+---------------------------+
| AEAD cipher           | nrf_cc3xx driver support  | nrf_oberon driver support |
+=======================+===========================+===========================+
| CCM                   | Supported                 | Supported                 |
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


Asymmetric signature configurations
***********************************

To enable asymmetric signature support, set one or more of the Kconfig options in the following table:

+---------------------------------+--------------------------------------------------------------+
| Asymmetric signature algorithms | Configuration option                                         |
+=================================+==============================================================+
| ECDSA                           | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`                  |
+---------------------------------+--------------------------------------------------------------+
| ECDSA without hashing           | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`              |
+---------------------------------+--------------------------------------------------------------+
| ECDSA (deterministic)           | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`    |
+---------------------------------+--------------------------------------------------------------+
| PureEdDSA                       | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`             |
+---------------------------------+--------------------------------------------------------------+
| HashEdDSA Edwards25519          | :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`              |
+---------------------------------+--------------------------------------------------------------+
| HashEdDSA Edwards448            | :kconfig:option:`CONFIG_PSA_WANT_ALG_ED448PH`                |
+---------------------------------+--------------------------------------------------------------+
| RSA PKCS#1 v1.5 sign            | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`      |
+---------------------------------+--------------------------------------------------------------+
| RSA raw PKCS#1 v1.5 sign        | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`  |
+---------------------------------+--------------------------------------------------------------+
| RSA PSS                         | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`                |
+---------------------------------+--------------------------------------------------------------+
| RSA PSS any salt                | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`       |
+---------------------------------+--------------------------------------------------------------+

Asymmetric signature support
============================

The following table shows asymmetric signature algorithm support for each driver:

+---------------------------------+---------------------------+----------------------------+
| Asymmetric signature algorithms | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=================================+===========================+============================+
| ECDSA                           | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| ECDSA without hashing           | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| ECDSA (deterministic)           | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| PureEdDSA                       | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| HashEdDSA Edwards25519          | Not supported             | Not supported              |
+---------------------------------+---------------------------+----------------------------+
| HashEdDSA Edwards448            | Not supported             | Not supported              |
+---------------------------------+---------------------------+----------------------------+
| RSA PKCS#1 v1.5 sign            | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| RSA raw PKCS#1 v1.5 sign        | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| RSA PSS                         | Not supported             | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| RSA PSS any salt                | Not supported             | Supported                  |
+---------------------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported algorithms.

Configuration of the :ref:`nrf_security_drivers_oberon` driver is automatically generated based on the user-enabled algorithms in `Asymmetric signature configurations`_.

The algorithm support when using ECC key types is dependent on one or more Kconfig options enabling curve support according to `ECC curve configurations`_.

RSA key size configuration is supported as described in `RSA key size configuration`_.

.. note::
   * :ref:`nrf_security_drivers_cc3xx` is limited to RSA key sizes less than or equal to 2048 bits.
   * :ref:`nrf_security_drivers_oberon` does not support RSA key pair generation.
   * :ref:`nrf_security_drivers_oberon` is currently limited to ECC curve types secp224r1, secp256r1, and secp384r1 for ECDSA.
   * :ref:`nrf_security_drivers_oberon` is currently limited to ECC curve type Ed25519 for EdDSA.

Asymmetric encryption configurations
************************************

To enable asymmetric encryption, set one or more of the Kconfig options in the following table:

+---------------------------------+-----------------------------------------------------------+
| Asymmetric encryption algorithm | Configuration option                                      |
+=================================+===========================================================+
| RSA OAEP                        | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`            |
+---------------------------------+-----------------------------------------------------------+
| RSA PKCS#1 v1.5 crypt           | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`  |
+---------------------------------+-----------------------------------------------------------+

Asymmetric encryption support
=============================

The following table shows asymmetric encryption algorithm support for each driver:

+---------------------------------+---------------------------+----------------------------+
| Asymmetric encryption algorithm | nrf_cc3xx driver support  | nrf_oberon driver support  |
+=================================+===========================+============================+
| RSA OAEP                        | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+
| RSA PKCS#1 v1.5 crypt           | Supported                 | Supported                  |
+---------------------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all supported algorithms.

Configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `Asymmetric encryption configurations`_.

RSA key size configuration is supported as described in `RSA key size configuration`_.

.. note::
   * :ref:`nrf_security_drivers_cc3xx` is limited to key sizes less than or equal to 2048 bits.
   * :ref:`nrf_security_drivers_oberon` does not support RSA key pair generation.

ECC curve configurations
************************

To configure elliptic curve support, set one or more of the Kconfig options in the following table:

+--------------------------+--------------------------------------------------------------+
| ECC curve type           | Configuration option                                         |
+==========================+==============================================================+
| BrainpoolP160r1 (weak)   | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_160`     |
+--------------------------+--------------------------------------------------------------+
| BrainpoolP192r1          | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`     |
+--------------------------+--------------------------------------------------------------+
| BrainpoolP224r1          | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`     |
+--------------------------+--------------------------------------------------------------+
| BrainpoolP256r1          | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`     |
+--------------------------+--------------------------------------------------------------+
| BrainpoolP320r1          | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`     |
+--------------------------+--------------------------------------------------------------+
| BrainpoolP384r1          | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`     |
+--------------------------+--------------------------------------------------------------+
| BrainpoolP512r1          | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`     |
+--------------------------+--------------------------------------------------------------+
| Curve25519 (X25519)      | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`         |
+--------------------------+--------------------------------------------------------------+
| Curve448 (X448)          | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_448`         |
+--------------------------+--------------------------------------------------------------+
| Edwards25519 (Ed25519)   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`    |
+--------------------------+--------------------------------------------------------------+
| Edwards448 (Ed448)       | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`    |
+--------------------------+--------------------------------------------------------------+
| secp192k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`            |
+--------------------------+--------------------------------------------------------------+
| secp224k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_224`            |
+--------------------------+--------------------------------------------------------------+
| secp256k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`            |
+--------------------------+--------------------------------------------------------------+
| secp192r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`            |
+--------------------------+--------------------------------------------------------------+
| secp224r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`            |
+--------------------------+--------------------------------------------------------------+
| secp256r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`            |
+--------------------------+--------------------------------------------------------------+
| secp384r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`            |
+--------------------------+--------------------------------------------------------------+
| secp521r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`            |
+--------------------------+--------------------------------------------------------------+
| secp160r2 (weak)         | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R2_160`            |
+--------------------------+--------------------------------------------------------------+
| sect163k1 (weak)         | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_163`            |
+--------------------------+--------------------------------------------------------------+
| sect233k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_233`            |
+--------------------------+--------------------------------------------------------------+
| sect239k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_239`            |
+--------------------------+--------------------------------------------------------------+
| sect283k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_283`            |
+--------------------------+--------------------------------------------------------------+
| sect409k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_409`            |
+--------------------------+--------------------------------------------------------------+
| sect571k1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_K1_571`            |
+--------------------------+--------------------------------------------------------------+
| sect163r1 (weak)         | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_163`            |
+--------------------------+--------------------------------------------------------------+
| sect233r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_233`            |
+--------------------------+--------------------------------------------------------------+
| sect283r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_283`            |
+--------------------------+--------------------------------------------------------------+
| sect409r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_409`            |
+--------------------------+--------------------------------------------------------------+
| sect571r1                | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R1_571`            |
+--------------------------+--------------------------------------------------------------+
| sect163r2 (weak)         | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECT_R2_163`            |
+--------------------------+--------------------------------------------------------------+
| FRP256v1                 | :kconfig:option:`CONFIG_PSA_WANT_ECC_FRP_V1_256`             |
+--------------------------+--------------------------------------------------------------+

ECC curve support
=================

The following table shows ECC curve support for each driver:

+--------------------------+---------------------------+----------------------------+
| ECC curve type           | nrf_cc3xx driver support  | nrf_oberon driver support  |
+==========================+===========================+============================+
| BrainpoolP160r1 (weak)   | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| BrainpoolP192r1          | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| BrainpoolP224r1          | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| BrainpoolP256r1          | Supported                 | Not supported              |
+--------------------------+---------------------------+----------------------------+
| BrainpoolP320r1          | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| BrainpoolP384r1          | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| BrainpoolP512r1          | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| Curve25519 (X25519)      | Supported                 | Supported                  |
+--------------------------+---------------------------+----------------------------+
| Curve448 (X448)          | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| Edwards25519 (Ed25519)   | Supported                 | Supported                  |
+--------------------------+---------------------------+----------------------------+
| Edwards448 (Ed448)       | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| secp192k1                | Supported                 | Not supported              |
+--------------------------+---------------------------+----------------------------+
| secp224k1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| secp256k1                | Supported                 | Not supported              |
+--------------------------+---------------------------+----------------------------+
| secp192r1                | Supported                 | Not supported              |
+--------------------------+---------------------------+----------------------------+
| secp224r1                | Supported                 | Supported                  |
+--------------------------+---------------------------+----------------------------+
| secp256r1                | Supported                 | Supported                  |
+--------------------------+---------------------------+----------------------------+
| secp384r1                | Supported                 | Supported                  |
+--------------------------+---------------------------+----------------------------+
| secp521r1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| secp160r2 (weak)         | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect163k1 (weak)         | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect233k1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect239k1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect283k1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect409k1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect571k1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect163r1 (weak)         | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect233r1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect283r1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect409r1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect571r1                | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| sect163r2 (weak)         | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+
| FRP256v1                 | Not supported             | Not supported              |
+--------------------------+---------------------------+----------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for key management using ECC curves.

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

RNG support
===========

There are no public configurations for entropy and PRNG algorithm support and the choice of drivers that provide support is automatic.

The PSA drivers using the Arm CryptoCell peripheral is enabled by default for nRF52840, nRF91 Series, and nRF5340 devices.

For devices without a hardware-accelerated cryptographic engine, entropy is provided by the nRF RNG peripheral. PRNG support is provided by the Oberon PSA driver, which is implemented using software.

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
| SHA-512/224           | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512_224` |
+-----------------------+---------------------------------------------------+
| SHA-512/256           | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512_256` |
+-----------------------+---------------------------------------------------+
| SHA3-224              | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`    |
+-----------------------+---------------------------------------------------+
| SHA3-256              | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`    |
+-----------------------+---------------------------------------------------+
| SHA3-384              | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`    |
+-----------------------+---------------------------------------------------+
| SHA3-512              | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`    |
+-----------------------+---------------------------------------------------+
| SM3                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SM3`         |
+-----------------------+---------------------------------------------------+
| SHAKE256 512 bits     | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`|
+-----------------------+---------------------------------------------------+
| MD2 (weak)            | :kconfig:option:`CONFIG_PSA_WANT_ALG_MD2`         |
+-----------------------+---------------------------------------------------+
| MD4 (weak)            | :kconfig:option:`CONFIG_PSA_WANT_ALG_MD4`         |
+-----------------------+---------------------------------------------------+
| MD5 (weak)            | :kconfig:option:`CONFIG_PSA_WANT_ALG_MD5`         |
+-----------------------+---------------------------------------------------+
| RIPEMD-160            | :kconfig:option:`CONFIG_PSA_WANT_ALG_RIPEMD160`   |
+-----------------------+---------------------------------------------------+

.. note::
   * The SHA-1 hash is weak and deprecated and is only recommended for use in legacy protocols.
   * The MD5 hash is weak and deprecated and is only recommended for use in legacy protocols.

Hash support
============

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
| SHA-512/224           | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SHA-512/256           | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SHA3-224              | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SHA3-256              | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SHA3-384              | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SHA3-512              | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SM3                   | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| SHAKE256 512 bits     | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| MD2 (weak)            | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| MD4 (weak)            | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| MD5 (weak)            | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+
| RIPEMD160             | Not supported              | Not supported             |
+-----------------------+----------------------------+---------------------------+

The option :kconfig:option:`CONFIG_PSA_USE_CC3XX_HASH_DRIVER` enables the driver :ref:`nrf_security_drivers_cc3xx` for all the supported algorithms.

The configuration of the :ref:`nrf_security_drivers_oberon` is automatically generated based on the user-enabled algorithms in `Hash configurations`_.

Password-authenticated key exchange configurations
**************************************************

To enable password-authenticated key exchange (PAKE) support, set one or more of the Kconfig options in the following table:

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

Password-authenticated key exchange support
===========================================

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

Configuration of the :ref:`nrf_security_drivers_oberon` driver is automatically generated based on the user-enabled algorithms in  `Password-authenticated key exchange configurations`_.

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
