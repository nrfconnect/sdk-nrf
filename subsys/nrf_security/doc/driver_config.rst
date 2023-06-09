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

To enable a PSA driver, set the following configurations:

+---------------+--------------------------------------------------+-----------------------------------------------------+
| PSA driver    | Configuration option                             | Notes                                               |
+===============+==================================================+=====================================================+
| nrf_cc3xx     | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` | Only on nRF52840, nRF91 Series, and nRF5340 devices |
+---------------+--------------------------------------------------+-----------------------------------------------------+
| nrf_oberon    | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON`|                                                     |
+---------------+--------------------------------------------------+-----------------------------------------------------+

If multiple drivers are enabled, the first ordered item in this table takes precedence for an enabled cryptographic feature, unless the driver does not enable or support it.

Enabling or disabling PSA driver specific configurations controls the support for a given algorithm, per driver.


AES cipher configurations
*************************

To enable AES cipher modes, set one or more of the following Kconfig options:

+----------------+------------------------------------------------------+
| Cipher mode    | Configuration option                                 |
+================+======================================================+
| ECB_NO_PADDING | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING` |
+----------------+------------------------------------------------------+
| CBC_NO_PADDING | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING` |
+----------------+------------------------------------------------------+
| CBC_PKCS7      | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`      |
+----------------+------------------------------------------------------+
| CFB            | :kconfig:option:`CONFIG_PSA_WANT_ALG_CFB`            |
+----------------+------------------------------------------------------+
| CTR            | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`            |
+----------------+------------------------------------------------------+
| OFB            | :kconfig:option:`CONFIG_PSA_WANT_ALG_OFB`            |
+----------------+------------------------------------------------------+
| XTS            | :kconfig:option:`CONFIG_PSA_WANT_ALG_XTS`            |
+----------------+------------------------------------------------------+


AES cipher driver configurations
================================

You can use the following Kconfig options for fine-grained control over which drivers provide AES cipher support:

+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| Cipher mode    | nrf_cc3xx driver support                                            | nrf_oberon driver support                                            |
+================+=====================================================================+======================================================================+
| ECB_NO_PADDING | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_ECB_NO_PADDING_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_ECB_NO_PADDING_OBERON` |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| CBC_NO_PADDING | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CBC_NO_PADDING_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CBC_NO_PADDING_OBERON` |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| CBC_PKCS7      | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CBC_PKCS7_CC3XX`      | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CBC_PKCS7_OBERON`      |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| CFB            | Not supported                                                       | Not supported                                                        |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| CTR            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CTR_CC3XX`            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CTR_OBERON`            |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| OFB            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_OFB_CC3XX`            | Not supported                                                        |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+
| XTS            | Not supported                                                       | Not supported                                                        |
+----------------+---------------------------------------------------------------------+----------------------------------------------------------------------+

.. note::
   * The :ref:`nrf_security_drivers_cc3xx` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell cc310.


Key Derivation Function
***********************

To enable key derivation function (KDF) support, set one or more of the following Kconfig options:

+-------------------+-------------------------------------------------------+
| KDF algorithm     | Configuration option                                  |
+===================+=======================================================+
| HKDF              | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`            |
+-------------------+-------------------------------------------------------+
| TLS 1.2 PRF       | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`       |
+-------------------+-------------------------------------------------------+
| TLS 1.2 PSK to MS | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS` |
+-------------------+-------------------------------------------------------+


Key Derivation Function driver configurations
=============================================

You can use the following Kconfig options for fine-grained control over which drivers provide Key Derivation Function (KDF) support:

+-------------------+--------------------------+-----------------------------------------------------------------------+
| KDF algorithm     | nrf_cc3xx driver support | nrf_oberon driver support                                             |
+===================+==========================+==========================================+============================+
| HKDF              | Not supported            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_HKDF_OBERON`            |
+-------------------+--------------------------+-----------------------------------------------------------------------+
| TLS 1.2 PRF       | Not supported            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_TLS12_PRF_OBERON`       |
+-------------------+--------------------------+-----------------------------------------------------------------------+
| TLS 1.2 PSK to MS | Not supported            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_TLS12_PSK_TO_MS_OBERON` |
+-------------------+--------------------------+-----------------------------------------------------------------------+


MAC configurations
******************

To enable MAC support, set one or more of the following Kconfig options:

+----------------+--------------------------------------------+
| MAC cipher     | Configuration option                       |
+================+============================================+
| CMAC           | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` |
+----------------+--------------------------------------------+
| HMAC           | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` |
+----------------+--------------------------------------------+


MAC driver configurations
=========================

You can use the following Kconfig options for fine-grained control over which drivers provide MAC support:


+----------------+-----------------------------------------------------------+------------------------------------------------------------+
| MAC cipher     | nrf_cc3xx driver support                                  | nrf_oberon driver support                                  |
+================+===========================================================+============================================================+
| CMAC           | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CMAC_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CMAC_OBERON` |
+----------------+-----------------------------------------------------------+------------------------------------------------------------+
| HMAC           | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_HMAC_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_HMAC_OBERON` |
+----------------+-----------------------------------------------------------+------------------------------------------------------------+

.. note::
   * The :ref:`nrf_security_drivers_cc3xx` is limited to AES CMAC key sizes of 128 bits on devices with Arm CryptoCell cc310.
   * The :ref:`nrf_security_drivers_cc3xx` is limited to HMAC using SHA-1, SHA-224, and SHA-256 on devices with Arm CryptoCell.


AEAD configurations
*******************

To enable Authenticated Encryption with Associated Data (AEAD), set one or more of the following Kconfig options:

+----------------+---------------------------------------------------------+
| AEAD cipher    | Configuration option                                    |
+================+=========================================================+
| AES CCM        | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`               |
+----------------+---------------------------------------------------------+
| AES GCM        | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`               |
+----------------+---------------------------------------------------------+
| ChaCha/Poly    | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305` |
+----------------+---------------------------------------------------------+


AEAD driver configurations
==========================

You can use the following Kconfig options for fine-grained control over which drivers provide AEAD support:

+----------------+------------------------------------------------------------------------+-------------------------------------------------------------------------+
| AEAD cipher    | nrf_cc3xx driver support                                               | nrf_oberon driver support                                               |
+================+========================================================================+=========================================================================+
| AES CCM        | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CCM_CC3XX`               | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CCM_OBERON`               |
+----------------+------------------------------------------------------------------------+-------------------------------------------------------------------------+
| AES GCM        | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_GCM_CC3XX`               | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_GCM_OBERON`               |
+----------------+------------------------------------------------------------------------+-------------------------------------------------------------------------+
| ChaCha/Poly    | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CHACHA20_POLY1305_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_CHACHA20_POLY1305_OBERON` |
+----------------+------------------------------------------------------------------------+-------------------------------------------------------------------------+

.. note::
   * The :ref:`nrf_security_drivers_cc3xx` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell cc310.
   * The :ref:`nrf_security_drivers_cc3xx` does not provide hardware support for AES GCM on devices with Arm CryptoCell cc310.


ECC configurations
******************

To enable Elliptic Curve Cryptography (ECC), set one or more of the following Kconfig options:

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

You can use the following Kconfig options for fine-grained control over which drivers provide ECC support:

+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| ECC algorithm         | nrf_cc3xx driver support                                                 | nrf_oberon driver support                                                 |
+=======================+==========================================================================+===========================================================================+
| ECDH                  | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_ECDH_CC3XX`                | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_ECDSA_OBERON`               |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| ECDSA                 | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_ECDSA_CC3XX`               | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_ECDSA_OBERON`               |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| ECDSA (deterministic) | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_DETERMINISTIC_ECDSA_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_DETERMINISTIC_ECDSA_OBERON` |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+

.. note::
   * The :ref:`nrf_security_drivers_oberon` is currently limited to curve types secp224r1, secp256r1, and secp384r1 for ECDH and ECDSA.
   * The :ref:`nrf_security_drivers_oberon` is currently limited to X25519 (using Curve25519) and Ed25519 for EdDSA.


ECC curve configurations
************************

To configure elliptic curve support, set one or more of the following Kconfig options:

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
| Ed25519               | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255` |
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

.. note::
   * :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521` is only supported by :ref:`nrf_security_drivers_builtin`.


ECC curve driver configurations
===============================

You can use the following Kconfig options for fine-grained control over which drivers provide elliptic curve support:

+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| ECC curve type        | nrf_cc3xx driver support                                                 | nrf_oberon driver support                                                 |
+=======================+==========================================================================+===========================================================================+
| Brainpool256r1        | Not supported                                                            | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| Brainpool384r1        | Not supported                                                            | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| Brainpool512r1        | Not supported                                                            | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| Curve25519            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_MONTGOMERY_255_CC3XX`      | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_MONTGOMERY_255_OBERON`      |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| Curve448              | Not supported                                                            | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| Ed25519               | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_TWISTED_EDWARDS_255_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_TWISTED_EDWARDS_255_OBERON` |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp192k1             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_K1_192_CC3XX`         | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp256k1             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_K1_256_CC3XX`         | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp192r1             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_192_CC3XX`         | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp224r1             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_224_CC3XX`         | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_224_OBERON`         |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp256r1             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_256_CC3XX`         | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_256_OBERON`         |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp384r1             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_384_CC3XX`         | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ECC_SECP_R1_384_OBERON`         |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+
| secp521r1             | Not supported                                                            | Not supported                                                             |
+-----------------------+--------------------------------------------------------------------------+---------------------------------------------------------------------------+


RNG configurations
******************

To enable PRNG seeded by entropy (also known as TRNG), set one or more of the following configurations:

+---------------------------+-------------------------------------------------+
| PRNG algorithms           | Configuration option                            |
+===========================+=================================================+
| CTR_DRBG                  | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`  |
+---------------------------+-------------------------------------------------+
| HMAC_DRBG                 | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` |
+---------------------------+-------------------------------------------------+

.. note::
   * Both PRNG algorithms are NIST qualified Cryptographically Secure Pseudo Random Number Generators (CSPRNG).
   * :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG` and :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` are custom configurations not described by the PSA Crypto specification.
   * If multiple PRNG algorithms are enabled at the same time, CTR_DRBG will be prioritized for random number generation through the front-end APIs for PSA Crypto.


RNG driver configurations
*************************

There are no public configurations for entropy and PRNG algorithm support and the choice of drivers that provide support is automatic.

The PSA drivers using the Arm CryptoCell peripheral is enabled by default for nRF52840, nRF91 Series, and nRF5340 devices.

For devices without a hardware-accelerated cryptographic engine, entropy is provided by the nRF RNG periperal. PRNG support is provided by the Oberon PSA driver, which is implemented using software.


RSA configurations
******************

To enable Rivest-Shamir-Adleman (RSA) support, set one or more of the following Kconfig options:

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

You can use the following Kconfig options for fine-grained control over which drivers provide RSA support:

+-----------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------+
| RSA algorithms        | nrf_cc3xx driver support                                                 | nrf_oberon driver support                                                |
+=======================+==========================================================================+==========================================================================+
| RSA OAEP              | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_OAEP_CC3XX`            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_OAEP_OBERON`           |
+-----------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------+
| RSA PKCS#1 v1.5 crypt | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_PKCS1V15_CRYPT_CC3XX`  | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_PKCS1V15_CRYPT_OBERON` |
+-----------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------+
| RSA PKCS#1 v1.5 sign  | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_PKCS1V15_SIGN_CC3XX`   | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_PKCS1V15_SIGN_OBERON`  |
+-----------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------+
| RSA PSS               | Not supported                                                            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_RSA_PSS_OBERON`            |
+-----------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------+

.. note::
   * :ref:`nrf_security_drivers_cc3xx` is limited to key sizes less than or equal to 2048 bits.
   * :ref:`nrf_security_drivers_oberon` does not support RSA key pair generation.


Secure Hash configurations
**************************

To configure the Secure Hash algorithms, set one or more of the following Kconfig options:

+-----------------------+-----------------------------------------------+
| Hash algorithm        | Configuration option                          |
+=======================+===============================================+
| SHA-1                 | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`   |
+-----------------------+-----------------------------------------------+
| SHA-224               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224` |
+-----------------------+-----------------------------------------------+
| SHA-256               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256` |
+-----------------------+-----------------------------------------------+
| SHA-384               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384` |
+-----------------------+-----------------------------------------------+
| SHA-512               | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512` |
+-----------------------+-----------------------------------------------+


Secure Hash driver configurations
=================================

You can use the following PSA driver-specific configurations for fine-grained control over which drivers provide the Secure Hash algorithm.

+-----------------------+---------------------------------------------------------------+---------------------------------------------------------------+
| Hash algorithm        |  nrf_cc3xx driver support                                     | nrf_oberon driver support                                     |
+=======================+===============================================================+===============================================================+
| SHA-1                 |  :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_1_CC3XX`   | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_1_OBERON`   |
+-----------------------+---------------------------------------------------------------+---------------------------------------------------------------+
| SHA-224               |  :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_224_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_224_OBERON` |
+-----------------------+---------------------------------------------------------------+---------------------------------------------------------------+
| SHA-256               |  :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_256_CC3XX` | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_256_OBERON` |
+-----------------------+---------------------------------------------------------------+---------------------------------------------------------------+
| SHA-384               |  Not supported                                                | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_384_OBERON` |
+-----------------------+---------------------------------------------------------------+---------------------------------------------------------------+
| SHA-512               |  Not supported                                                | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_ALG_SHA_512_OBERON` |
+-----------------------+---------------------------------------------------------------+---------------------------------------------------------------+
