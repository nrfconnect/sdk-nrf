.. _nrf_cc310_bl_readme:

nrf_cc310_bl crypto library
###########################

.. contents::
   :local:
   :depth: 2

The nrf_cc310_bl library is a software library to interface with Arm CryptoCell CC310 hardware accelerator that is available on the nRF52840 SoC and the nRF91 Series.
The library adds hardware support for cryptographic algorithms to be used in a bootloader-specific use cases.

The nrf_cc310_bl library supports the following cryptographic algorithms:

* ECDSA verify using NIST curve secp256r1
* SHA-256

Initializing the library
========================
The library must be initialized before the APIs can be used.

.. code-block:: c
   :caption: Initializing the nrf_cc310_bl library

   if (nrf_cc310_bl_init() != 0) {
           /** nrf_cc310_bl failed to initialize. */
           return -1;
   }


Enabling/Disabling the CryptoCell hardware
==========================================
The CryptoCell CC310 hardware must be manually enabled/disabled prior to the API calls in the nrf_cc310_bl library.

.. note::
   CryptoCell consumes power when the hardware is enabled even if there is no ongoing operation.

Enabling the CryptoCell hardware
--------------------------------

The hardware is enabled by writing to a specific register.

.. code-block:: c
   :caption: Enabling the CryptoCell hardware

   NRF_CRYPTOCELL->ENABLE=1;

Disabling the CryptoCell hardware
---------------------------------

The hardware is disabled by writing to a specific register.

.. code-block:: c
   :caption: Disabling the CryptoCell hardware

   NRF_CRYPTOCELL->ENABLE=0;

.. note::
   Note that the structure type for the CryptoCell hardware register is called ``NRF_CRYPTOCELL_S`` in nRF91 Series devices, due to the hardware only being accessible from the Secure Processing Environment in the Cortex-M33 architecture.

API documentation
=================

:ref:`crypto_api_nrf_cc310_bl`
