.. _nrf_security_config:

Enabling nRF Security
#####################

.. contents::
   :local:
   :depth: 2

To enable nRF Security, set the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option.

You can use nRF Security with the PSA Crypto APIs or the Legacy crypto APIs.

PSA Crypto APIs
   .. ncs-include:: driver_config.rst
      :start-after: psa_crypto_support_def_start
      :end-before: psa_crypto_support_def_end

   For more configuration options, see :ref:`psa_crypto_support`.

Legacy crypto APIs
   .. ncs-include:: backend_config.rst
      :start-after: legacy_crypto_support_def_start
      :end-before: legacy_crypto_support_def_end

   For more configuration options, see :ref:`nrf_security_legacy_config`.

Building with TF-M
******************

If :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled together with :kconfig:option:`CONFIG_NRF_SECURITY`, the TF-M secure image will enable the use of the hardware acceleration of Arm CryptoCell.
In this case, the Kconfig configurations in the nRF Security subsystem control the features enabled in TF-M.

.. ncs-include:: driver_config.rst
   :start-after: psa_crypto_support_tfm_build_start
   :end-before: psa_crypto_support_tfm_build_end
