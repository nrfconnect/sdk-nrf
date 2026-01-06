.. _psa_crypto_support:
.. _nrf_security_driver_config:

Configuring PSA Crypto API
##########################

.. contents::
   :local:
   :depth: 2

.. psa_crypto_support_def_start

The PSA Crypto in the |NCS| provides secure crypto operations through standardized :ref:`Platform Security Architecture <ug_psa_certified_api_overview>`.
Using :ref:`one of the two available implementations of the PSA Crypto API <ug_crypto_architecture_implementation_standards>`, the SDK implements the cryptographic features in software or using hardware accelerators, or both.

.. psa_crypto_support_def_end

.. note::
   If you work with the Mbed TLS legacy crypto toolbox, see :ref:`legacy_crypto_support`.

.. _psa_crypto_support_enable:

Enabling PSA Crypto API
***********************

To use the PSA Crypto API in your application, enable the following Kconfig options depending on your chosen implementation:

* For the :ref:`Oberon PSA Crypto implementation <ug_crypto_architecture_implementation_standards_oberon>`, enable the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option.
* For the :ref:`TF-M Crypto Service implementation <ug_crypto_architecture_implementation_standards_tfm>`, enable the :kconfig:option:`CONFIG_NRF_SECURITY` and the :kconfig:option:`CONFIG_BUILD_WITH_TFM` Kconfig options.
  For more information, see :ref:`ug_tfm_building_secure_services`.
* For the :ref:`IronSide Secure Element implementation <ug_crypto_architecture_implementation_standards_ironside>`, enable the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option on the nRF54H20's :ref:`ug_nrf54h20_architecture_cpu_appcore`.

.. _psa_crypto_support_single_driver:

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

Enabling the CRACEN driver
==========================

To enable the :ref:`nrf_security_drivers_cracen`, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN` Kconfig option.

The nrf_oberon driver may then be disabled by using the Kconfig option :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` (``CONFIG_PSA_CRYPTO_DRIVER_OBERON=n``).

.. note::
   - On nRF54L Series devices, CRACEN is the only source of entropy.
     Therefore, it is not possible to disable the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN` Kconfig option when the Zephyr entropy driver is enabled.
   - On nRF54H20, the IronSide Secure Element firmware relies on the CRACEN driver.
     However, you do not need to enable the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_CRACEN` Kconfig option when the program the firmware bundle onto the :ref:`Secure Domain <ug_nrf54h20_secure_domain>`.
     For more information, see the :ref:`ug_nrf54h20_ironside` page.

.. _psa_crypto_support_enable_nrf_oberon:

Enabling the nrf_oberon driver
==============================

To enable the :ref:`nrf_security_drivers_oberon`, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` Kconfig option to ``y``.
Enabling the nrf_oberon driver automatically enables the :ref:`software fallback mechanism to nrf_oberon <crypto_drivers_software_fallback>`.

.. _psa_crypto_support_disable_software_fallback:

Disabling the software fallback
===============================

To disable the :ref:`software fallback mechanism to nrf_oberon <crypto_drivers_software_fallback>`, set the :kconfig:option:`CONFIG_PSA_CRYPTO_DRIVER_OBERON` Kconfig option to ``n``.

.. _psa_crypto_support_multiple_drivers:

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

You can enable a cryptographic feature or algorithm using `CONFIG_PSA_WANT_*`_ and `CONFIG_PSA_USE_*`_ Kconfig options, which are specific to the :ref:`feature selection mechanism <crypto_drivers_feature_selection>` of the PSA Crypto API.
For a list of supported cryptographic features and algorithms and the Kconfig options to enable them, see :ref:`ug_crypto_supported_features`.

For example, to enable support for the Encrypted key usage scheme (``CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED``), set the following Kconfig options:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
* :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
* :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
* :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
* :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
* :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`

  .. toggle::

     .. code-block:: console

        CONFIG_PSA_WANT_KEY_TYPE_AES=y
        CONFIG_PSA_WANT_AES_KEY_SIZE_256=y
        CONFIG_PSA_WANT_ALG_ECB_NO_PADDING=y
        CONFIG_PSA_WANT_ALG_CMAC=y
        CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC=y
        CONFIG_PSA_WANT_ALG_GCM=y

This configuration enables the key type (AES) and the key size (256 bits) supported by the Encrypted usage scheme, as explained in the :ref:`ug_kmu_guides_supported_key_types` section.
In addition, it enables the following :ref:`cryptographic features <ug_crypto_supported_features>` supported by the CRACEN driver:

* Cipher mode: AES ECB (Electronic CodeBook) mode, no padding
* Message Authentication Code (MAC) cipher: Cipher-based MAC (CMAC) cipher
* Key derivation function (KDF) support: SP800-108 CMAC in counter mode
* Authenticated Encryption with Associated Data (AEAD) cipher: Galois Counter Mode (GCM) cipher

Building PSA Crypto API
***********************

Depending on the implementation you are using, the |NCS| build system uses different versions of the PSA Crypto API.

.. ncs-include:: ../psa_certified_api_overview.rst
   :start-after: psa_crypto_support_tfm_build_start
   :end-before: psa_crypto_support_tfm_build_end
