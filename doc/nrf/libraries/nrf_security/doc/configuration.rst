.. _nrf_security_config:

Configuration
#############

.. contents::
   :local:
   :depth: 2

You can enable the nRF Security subsystem using `PSA crypto support`_ or `Legacy crypto support`_.

To enable nRF Security, set the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option along with additional configuration options, as described in :ref:`nrf_security_driver_config`.
This includes PSA crypto support by default.

PSA crypto support
******************

PSA crypto support is included by default when you enable nRF Security through the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option.
PSA crypto support is provided through PSA Crypto APIs and is implemented by PSA core.
PSA core uses PSA drivers to implement the cryptographic features either in software, or using hardware accelerators.

.. caution::
   The PSA Crypto APIs are only thread safe when provided by TF-M.

.. _legacy_crypto_support:

Legacy crypto support
*********************

To enable the legacy crypto support mode of nRF Security, set both the :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND` and :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig options along with additional configuration options, as described in :ref:`nrf_security_legacy_config`.
The legacy crypto support allows backwards compatibility for software that requires usage of Mbed TLS crypto toolbox functions prefixed with ``mbedtls_``.

Custom Mbed TLS configuration files
***********************************

The nRF Security Kconfig options are used to generate an Mbed TLS configuration file.

Although not recommended, it is possible to provide a custom Mbed TLS configuration file by disabling :kconfig:option:`CONFIG_GENERATE_MBEDTLS_CFG_FILE`.
See :ref:`nrf_security_tls_header`.

Building with TF-M
******************

If :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled together with :kconfig:option:`CONFIG_NRF_SECURITY`, the TF-M secure image will enable the use of the hardware acceleration of Arm CryptoCell.
In this case, the Kconfig configurations in the nRF Security subsystem control the features enabled in TF-M.
