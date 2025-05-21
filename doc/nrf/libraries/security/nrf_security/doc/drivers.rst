.. _nrf_security_drivers:

nRF Security drivers
####################

.. contents::
   :local:
   :depth: 2

.. ncs-include:: ../../../../security/psa_certified_api_overview.rst
   :start-after: psa_crypto_lib_table_start
   :end-before: psa_crypto_lib_table_end

In addition, the nRF Security subsystem supports the :ref:`legacy crypto toolbox APIs <legacy_crypto_support>`.
It also integrated and configures the PSA Oberon core through `sdk-oberon-psa-crypto`_, which provides the :ref:`nrf_security_drivers_oberon`.

.. _nrf_security_drivers_cc3xx:

Arm CryptoCell cc3xx drivers
****************************

+---------------------------------------------------------------------+----------------------------------------------------------+
|                           Driver library                            |               Supported hardware platforms               |
+=====================================================================+==========================================================+
| :ref:`nrf_cc3xx_platform and nrf_cc3xx_mbedcrypto <nrfxlib:crypto>` | nRF52840, nRF5340, nRF91 Series devices                  |
+---------------------------------------------------------------------+----------------------------------------------------------+

Arm CryptoCell cc3xx drivers, namely ``nrf_cc3xx_platform`` and ``nrf_cc3xx_mbedcrypto``, are closed-source binaries that provide low-level functionalities for hardware-accelerated cryptography using `CryptoCell 310 <nRF9160 CRYPTOCELL - Arm TrustZone CryptoCell 310_>`_ and `CryptoCell 312 <nRF5340 CRYPTOCELL - Arm TrustZone CryptoCell 312_>`_ hardware peripherals.

* The :ref:`nrf_cc3xx_platform_readme` provides low-level functionality needed by the CC310/CC312 mbedcrypto library.
* The :ref:`nrf_cc3xx_mbedcrypto_readme` provides low-level integration with the Mbed TLS version provided in the |NCS|.
  It also includes legacy crypto API functions from the Mbed TLS crypto toolbox (prefixed with ``mbedtls_``).

Both these drivers should not be used directly.
Use them only through nRF Security.

For configuration details, see the following pages:

* :ref:`nrf_security_driver_config` (both drivers)
* :ref:`nrf_security_legacy_backend_config` (:ref:`nrf_cc3xx_mbedcrypto_readme` used as legacy backend)

.. note::
      The :ref:`nrfxlib:crypto` in nrfxlib also include the :ref:`nrf_cc310_bl_readme`.
      This library is not used by the nRF Security subsystem.

.. _nrf_security_drivers_oberon:

nrf_oberon driver
*****************

+---------------------------------------------------------------------+----------------------------------------------------------+
|                           Driver library                            |               Supported hardware platforms               |
+=====================================================================+==========================================================+
| :ref:`nrf_oberon <nrfxlib:nrf_oberon_readme>`                       | nRF devices with Arm Cortex®-M0, -M4, or -M33 processors |
+---------------------------------------------------------------------+----------------------------------------------------------+

The :ref:`nrf_oberon_readme` is a driver provided through `sdk-oberon-psa-crypto`_, a lightweight PSA Crypto API implementation optimized for resource-constrained microcontrollers.
The driver is distributed as a closed-source binary that provides select cryptographic algorithms optimized for use in nRF devices.
This provides faster execution than the original Mbed TLS implementation.

.. note::
   |original_mbedtls_def_note|

The nrf_oberon driver provides support for the following encryption algorithms:

* AES ciphers
* SHA-1
* SHA-256
* SHA-384
* SHA-512
* ECDH and ECDSA using NIST curve secp224r1 and secp256r1
* ECJPAKE using NIST curve secp256r1

The nrf_oberon driver also provides Mbed TLS legacy crypto integration for selected features.

For configuration details, see the following pages:

* :ref:`nrf_security_driver_config`
* :ref:`nrf_security_legacy_backend_config` (nrf_oberon used as legacy backend)

.. _nrf_security_drivers_cracen:

CRACEN driver
*************

+---------------------------------------------------------------------+----------------------------------------------------------+
|                           Driver library                            |               Supported hardware platforms               |
+=====================================================================+==========================================================+
| :ref:`CRACEN <ug_nrf54l_cryptography>`                              | nRF54L Series devices                                    |
+---------------------------------------------------------------------+----------------------------------------------------------+

The CRACEN driver provides entropy and hardware-accelerated cryptography using the Crypto Accelerator Engine (CRACEN) peripheral.
For more information about it, see :ref:`ug_nrf54l_crypto_kmu_cracen_peripherals` on the :ref:`ug_nrf54l_cryptography` page.

For configuration details in the nRF Security subsystem, see :ref:`nrf_security_driver_config`.

API documentation
*****************

| Header files: :file:`subsys/nrf_security/include/psa/crypto_driver_contexts_*.h`

.. doxygengroup:: nrf_security_api_structures
