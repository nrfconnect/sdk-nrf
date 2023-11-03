.. _nrf_cc310_platform_readme:
.. _nrf_cc3xx_platform_readme:

nrf_cc3xx_platform library
##########################

.. contents::
   :local:
   :depth: 2

The nrf_cc3xx_platform library is software library to interface with the Arm CryptoCell CC310/CC312 hardware accelerator that is available on the nRF52840 SoC, the nRF53 Series, and the nRF91 Series.

The platform library provides low-level functionality needed by the CC310/CC312
mbedcrypto library.

This includes memory, mutex, and entropy APIs needed for the CC310/CC312 mbedcrypto
library.

The library adds hardware support for the True Random Number Generator, TRNG,
available in the CC310/CC312 hardware.

API documentation
=================

:ref:`crypto_api_nrf_cc3xx_platform`
