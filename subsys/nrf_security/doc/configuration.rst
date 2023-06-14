.. _nrf_security_config:

Configuration
#############

.. contents::
   :local:
   :depth: 2

You can enable the Nordic Security Module using `PSA crypto support`_ or `Legacy crypto support`_.

PSA crypto support
******************

To enable the Nordic Security Module with PSA crypto support, set the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option along with additional configuration options, as described in :ref:`nrf_security_driver_config`.

PSA crypto support is provided through PSA Crypto APIs and is implemented by PSA core.
PSA core uses PSA drivers to implement the cryptographic features either in software, or using hardware accelerators.

Starting from |NCS| v2.4.0, the Mbed TLS PSA core is deprecated and replaced with nrf_oberon PSA core.
The nrf_oberon PSA core code is an optimized and efficient implementation of PSA core licensed for use on Nordic Semiconductor devices.

+---------------+--------------------------------------------------+------------------------------------------------+
| PSA core      | Configuration option                             | Notes                                          |
+===============+==================================================+================================================+
| nrf_oberon    | :kconfig:option:`CONFIG_PSA_CORE_OBERON`         | Default                                        |
+---------------+--------------------------------------------------+------------------------------------------------+
| Mbed TLS      | :kconfig:option:`CONFIG_PSA_CORE_BUILTIN`        | DEPRECATED                                     |
+---------------+--------------------------------------------------+------------------------------------------------+

.. _legacy_crypto_support:

Legacy crypto support
*********************
To enable the legacy crypto support mode of Nordic Security Module, set the :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND` Kconfig option along with additional configuration options, as described in :ref:`nrf_security_legacy_config`.
The legacy crypto support allows backwards compatibility for software that requires usage of Mbed TLS crypto toolbox functions prefixed with ``mbedtls_``.

Custom Mbed TLS configuration files
***********************************

The Nordic Security Module (nrf_security) Kconfig options are used to generate an Mbed TLS configuration file.

Although not recommended, it is possible to provide a custom Mbed TLS configuration file by disabling :kconfig:option:`CONFIG_GENERATE_MBEDTLS_CFG_FILE`.
See :ref:`nrf_security_tls_header`.

Building with TF-M
******************

If :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled together with :kconfig:option:`CONFIG_NRF_SECURITY`, the TF-M secure image will enable the use of the hardware acceleration of Arm CryptoCell.
In this case, the Kconfig configurations in the Nordic Security Module control the features enabled in TF-M.
