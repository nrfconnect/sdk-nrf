.. _nrf_security_readme:
.. _nrf_security:

nRF Security
############

The nRF Security subsystem (``nrf_security``) integrates cryptographic services for SoCs from Nordic Semiconductor.

Overview
********

The nRF Security subsystem provides:

* The PSA Crypto API through :ref:`Oberon PSA Crypto<ug_psa_certified_api_overview_crypto>`.
* TLS, DTLS, and X.509 support through `Mbed TLS`_.
* Hardware acceleration through dedicated cryptographic libraries on selected SoCs (``nrf_cc3xx``, CRACEN), with binary versions of the libraries listed in :ref:`nrfxlib:crypto`.
* Software fallbacks when hardware acceleration is unavailable (``nrf_oberon``).
* A PSA driver abstraction layer that enables simultaneous use of hardware and software implementations.
* Compatibility with the specific Mbed TLS version included in the |NCS| through `sdk-mbedtls`_.
* Integration logic for the Oberon PSA Crypto core (`sdk-oberon-psa-crypto`_).
* Source code for the CRACEN driver.
* Integration with the |NCS| build system.

The nRF Security subsystem can interface with the :ref:`nrf_cc3xx_mbedcrypto_readme`.
This library conforms to the specific revision of Mbed TLS that is supplied through the |NCS|.

.. _nrf_security_config:

Configuration
*************

To enable nRF Security, enable the :kconfig:option:`CONFIG_PSA_CRYPTO` Kconfig option.
On Nordic Semiconductor's Arm cores, this automatically enables the nRF Security subsystem (:kconfig:option:`CONFIG_NRF_SECURITY`).

PSA Crypto
==========

Cryptographic operations are enabled with the :kconfig:option:`CONFIG_PSA_CRYPTO` Kconfig option.

.. ncs-include:: ../../security/crypto/driver_config.rst
   :start-after: psa_crypto_support_def_start
   :end-before: psa_crypto_support_def_end

For more information, see :ref:`psa_crypto_support`.
For the list of supported crypto features, see :ref:`ug_crypto_supported_features`.

Depending on the implementation you are using, the |NCS| builds nRF Security using different versions of the PSA Crypto API.

.. ncs-include:: ../../security/psa_certified_api_overview.rst
   :start-after: psa_crypto_support_tfm_build_start
   :end-before: psa_crypto_support_tfm_build_end

.. _nrf_security_tls_x509_config:

Mbed TLS (TLS and X.509)
========================

Enable Mbed TLS only when your application needs TLS, DTLS, or X.509 certificate handling.
To enable TLS and X.509 support, set the :kconfig:option:`CONFIG_MBEDTLS` Kconfig option.

.. note::
   For all other cryptographic operations, use the `PSA Crypto`_ API by enabling :kconfig:option:`CONFIG_PSA_CRYPTO`.
   nRF Security uses the Mbed TLS integration from Zephyr, but replaces the TF-PSA-Crypto repository used in Zephyr with Oberon PSA Crypto.

   Some TF-PSA-Crypto Kconfig options still use the ``MBEDTLS_`` prefix even when :kconfig:option:`CONFIG_MBEDTLS` is disabled.
   These options configure the crypto stack, not Mbed TLS.

For the Mbed TLS version included in the |NCS|, see :ref:`security_index`.

Configuring the PSA Crypto/Mbed TLS heap
========================================

When using either PSA Crypto or Mbed TLS, you can enable the heap for both with the :kconfig:option:`CONFIG_MBEDTLS_ENABLE_HEAP` Kconfig option and adjust the heap size for your workload using the :kconfig:option:`CONFIG_MBEDTLS_HEAP_SIZE` Kconfig option.

Dependencies
************

The nRF Security subsystem uses the following |NCS| modules:

* `sdk-oberon-psa-crypto`_ - Oberon PSA Crypto (PSA Crypto Core and TF-PSA-Crypto implementation; replaces Zephyr's TF-PSA-Crypto when nRF Security is enabled)
* `sdk-nrfxlib`_ - Prebuilt cryptographic libraries (CryptoCell, Oberon software libraries)
* `sdk-mbedtls`_ - TLS, DTLS, and X.509 support (only when :kconfig:option:`CONFIG_MBEDTLS` is enabled)
* `sdk-trusted-firmware-m`_ - Security by separation with ARM TrustZone and TF-M Crypto Service (only when TF-M is enabled by building for an ``*/ns`` board target)

The subsystem uses :ref:`cryptographic drivers <crypto_drivers>` based on Kconfig options such as :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX`, :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN`, and :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON`.
For driver details and selection rules, see :ref:`crypto_drivers`.

.. _nrf_security_api:

API documentation
*****************

For Doxygen API reference for nRF Security driver structures and the CRACEN driver, see :ref:`crypto_drivers_api_documentation`.
