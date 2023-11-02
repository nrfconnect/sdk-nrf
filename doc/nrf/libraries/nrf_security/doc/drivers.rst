.. _nrf_security_drivers:

nRF Security drivers
####################

.. contents::
   :local:
   :depth: 2

The nRF Security subsystem supports multiple enabled PSA drivers at the same time.
This mechanism is intended to extend the available feature set of hardware-accelerated cryptography or to provide alternative implementations of the PSA Crypto APIs.

You can enable a cryptographic feature or algorithm using PSA Crypto API configurations that follow the format ``PSA_WANT_ALG_XXXX``.

Enabling more than one PSA driver might add support for additional key sizes or modes of operation.

It is possible to disable specific features on the PSA driver level to optimize the code size.

The nRF Security supports the following PSA drivers:

* Arm CryptoCell cc3xx binary
* nrf_oberon binary

.. note::
   Whenever this documentation mentions 'original' Mbed TLS, it refers to the open-source `Arm Mbed TLS project`_, not the customized version available in Zephyr.
   There is an option to utilize a 'built-in' driver, which corresponds to the software-implemented cryptography from the 'original' Mbed TLS deliverables.
   This is provided to ensure that the cryptographic toolbox supports all requested features.

.. _nrf_security_drivers_cc3xx:

Arm CryptoCell cc3xx driver
***************************

The Arm CryptoCell cc3xx driver is a is a closed-source binary that provides hardware-accelerated cryptography using the Arm CryptoCell cc310/cc312 hardware.

The Arm CryptoCell cc3xx driver is only available on the following devices:

* nRF52840
* nRF9160
* nRF5340


Enabling the Arm CryptoCell cc3xx driver
========================================

The Arm CryptoCell cc3xx driver can be enabled by setting the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` Kconfig option.


Using the Arm CryptoCell cc3xx driver
=====================================

To use the :ref:`nrf_cc3xx_mbedcrypto_readme` PSA driver, the Arm CryptoCell cc310/cc312 hardware must be first initialized.

The Arm CryptoCell cc3xx hardware is initialized in the :file:`hw_cc3xx.c` file, located under :file:`nrf/drivers/hw_cc3xx/`, and is controlled with the :kconfig:option:`CONFIG_HW_CC3XX` Kconfig option.
The Kconfig option has a default value of ``y`` when cc3xx is available in the SoC.

.. _nrf_security_drivers_oberon:

nrf_oberon driver
*****************

The :ref:`nrf_oberon_readme` is distributed as a closed-source binary that provides select cryptographic algorithms optimized for use in nRF devices.
This provides faster execution than the original Mbed TLS implementation.

The nrf_oberon driver provides support for the following:

* AES ciphers
* SHA-1
* SHA-256
* SHA-384
* SHA-512
* ECDH and ECDSA using NIST curve secp224r1 and secp256r1
* ECJPAKE using NIST curve secp256r1

Enabling the nrf_oberon driver
==============================

To enable the :ref:`nrf_oberon_readme` PSA driver, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` Kconfig option.

Legacy Mbed TLS
***************

Some legacy Mbed TLS APIs are still supported, for instance for TLS and DTLS support and backwards compatibility.

Enabling legacy APIs requires enabling one of the available PSA drivers.

.. note::
   * The legacy Mbed TLS APIs no longer support the glued functionality.
   * Legacy configurations no longer have an effect on the configurations for the secure image of a TF-M build.

Enabling legacy Mbed TLS support
================================

To configure the legacy Mbed TLS APIs, set the option :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND` instead of :kconfig:option:`CONFIG_NRF_SECURITY`.

Additionally, either :kconfig:option:`CONFIG_CC3XX_BACKEND` or :kconfig:option:`CONFIG_OBERON_BACKEND` must be enabled.

.. note::
   Enabling the CryptoCell by using :kconfig:option:`CONFIG_CC3XX_BACKEND` in a non-secure image of a TF-M build will have no effect.
