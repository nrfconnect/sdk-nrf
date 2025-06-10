.. _psa_crypto_support:
.. _nrf_security_driver_config:

Configuring nRF Security with PSA Crypto APIs
#############################################

.. contents::
   :local:
   :depth: 2

.. psa_crypto_support_def_start

The PSA Crypto in the |NCS| provides secure crypto operations through standardized :ref:`Platform Security Architecture <ug_psa_certified_api_overview>`.
It implements the cryptographic features either in software, or using hardware accelerators.

The PSA Crypto API is enabled by default when you enable nRF Security with the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option.

.. psa_crypto_support_def_end

.. psa_crypto_support_tfm_build_start

When you :ref:`build with TF-M<ug_tfm>`, `PSA Certified Crypto API`_ v1.0 is implemented.
Otherwise, when you build without TF-M, v1.2 of the API is used.

.. psa_crypto_support_tfm_build_end

This page covers the configurations available when using the :ref:`nrf_security_drivers` compatible with the PSA Crypto API.
If you work with the legacy crypto toolbox, see :ref:`legacy_crypto_support`.

.. _nrf_security_drivers_config_single:

Configuring single drivers
**************************

The nRF Security subsystem allows you to configure individual drivers for cryptographic operations.
Each driver can be enabled or disabled independently through Kconfig options.

Enabling the Arm CryptoCell nrf_cc3xx driver
============================================

To enable the :ref:`Arm CryptoCell nrf_cc3xx driver <nrf_security_drivers_cc3xx>`, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX` Kconfig option.

Using the Arm CryptoCell nrf_cc3xx driver
-----------------------------------------

To use the :ref:`nrf_cc3xx_mbedcrypto_readme` PSA driver, the Arm CryptoCell CC310/CC312 hardware must be first initialized.

The Arm CryptoCell hardware compatible with nrf_cc3xx is initialized in the :file:`hw_cc3xx.c` file, located under :file:`nrf/drivers/hw_cc3xx/`, and is controlled with the :kconfig:option:`CONFIG_HW_CC3XX` Kconfig option.
The Kconfig option has a default value of ``y`` when nrf_cc3xx is available in the SoC.

Enabling the nrf_oberon driver
==============================

To enable the :ref:`nrf_security_drivers_oberon`, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` Kconfig option.

Enabling the CRACEN driver
==========================

To enable the :ref:`nrf_security_drivers_cracen`, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN` Kconfig option.

The nrf_oberon driver may then be disabled by using the Kconfig option :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` (``CONFIG_PSA_CRYPTO_DRIVER_OBERON=n``).

.. note::
   On nRF54L Series devices, CRACEN is the only source of entropy.
   Therefore, it is not possible to disable the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN` Kconfig option when the Zephyr entropy driver is enabled.

.. _nrf_security_drivers_config_multiple:

Configuring multiple drivers
****************************

The nRF Security subsystem supports multiple enabled PSA Crypto API drivers at the same time.
If you do, you can fine-tune which drivers implement support for cryptographic features.
This mechanism is intended to extend the available feature set of hardware-accelerated cryptography or to provide alternative implementations of the PSA Crypto APIs.

Enabling more than one PSA driver might add support for additional key sizes or modes of operation.

You can disable specific features on the PSA driver level to optimize the code size.

To enable a specific PSA Crypto API driver, set the respective Kconfig option, as listed in the following table:

+-----------------------+---------------------------------------------------+-----------------------------------------------------+
| PSA Crypto API driver |               Configuration option                |                        Notes                        |
+=======================+===================================================+=====================================================+
| nrf_cc3xx             | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CC3XX`  | Only on nRF52840, nRF91 Series, and nRF5340 devices |
+-----------------------+---------------------------------------------------+-----------------------------------------------------+
| CRACEN                | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN` | Only on nRF54L Series devices                       |
+-----------------------+---------------------------------------------------+-----------------------------------------------------+
| nrf_oberon            | :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` |                                                     |
+-----------------------+---------------------------------------------------+-----------------------------------------------------+

If you enable multiple drivers, the item higher in the list takes precedence for an enabled cryptographic feature, unless the driver does not enable or support it.

The :ref:`nrf_security_drivers_cc3xx` allows enabling or disabling of specific PSA APIs (such as psa_cipher_encrypt, psa_sign_hash), but not individual algorithms.

The :ref:`nrf_security_drivers_oberon` allows finer configuration granularity, allowing you to enable or disable individual algorithms as well.

When multiple enabled drivers support the same cryptographic feature, the configuration system attempts to include only one implementation to minimize code size.

.. _nrf_security_drivers_config_features:

Configuring cryptographic features
**********************************

You can enable a cryptographic feature or algorithm using `CONFIG_PSA_WANT_*`_ Kconfig options, which are specific for PSA Crypto API configurations.
For example, to enable the AES algorithm, set the :kconfig:option:`CONFIG_PSA_WANT_ALG_AES` Kconfig option.

For a list of supported cryptographic features and algorithms and the Kconfig options to enable them, see :ref:`ug_crypto_supported_features`.
