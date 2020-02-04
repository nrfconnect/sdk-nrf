.. _doc_bl_crypto:

Bootloader crypto
#################

The bootloader crypto library is the cryptography library that is used by the :ref:`bootloader`.

The API is public because applications that are booted by the immutable bootloader can call functions from this library via the bootloader's code, through external APIs.
See :ref:`doc_fw_info_ext_api` for more information.

The library provides the following functionality:

* SHA256 hashing
* SECP256R1 signature validation
* Root-of-trust firmware validation, which is the function the bootloader uses to validate a firmware's signature and digest, using the SHA256 and SECP256R1 algorithms

These functions are available as separate external APIs.
The API can be used the same way regardless of which backend is used.

Backends
********

When using the library, you can choose between the following backends:

* Hardware backend :ref:`nrf_cc310_bl_readme` (can only be used if Arm CryptoCell CC310 is available)
* Software backend :ref:`nrf_oberon_readme`
* Another image's instance of the bootloader crypto library, called via external APIs.
  The other image chooses its own backend.

To configure which backend is used for hashing, set one of the following configuration options:

* :option:`CONFIG_SB_CRYPTO_OBERON_SHA256`
* :option:`CONFIG_SB_CRYPTO_CC310_SHA256`
* :option:`CONFIG_SB_CRYPTO_CLIENT_SHA256`

To configure which backend is used for firmware verification, set one of the following configuration options:

* :option:`CONFIG_SB_CRYPTO_CC310_ECDSA_SECP256R1`
* :option:`CONFIG_SB_CRYPTO_OBERON_ECDSA_SECP256R1`
* :option:`CONFIG_SB_CRYPTO_CLIENT_ECDSA_SECP256R1`



API documentation
*****************

| Header file: :file:`include/bl_crypto.h`
| Source files: :file:`subsys/bootloader/bl_crypto/` and :file:`subsys/bootloader/bl_crypto_client/`

.. doxygengroup:: bl_crypto
   :project: nrf
   :members:
