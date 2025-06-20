.. _nrf_security_config:

Enabling nRF Security
#####################

.. contents::
   :local:
   :depth: 2

To enable nRF Security, set the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option.

You can use nRF Security with the PSA Crypto APIs or the Legacy crypto APIs.

PSA Crypto APIs
   .. ncs-include:: ../../../../security/crypto/driver_config.rst
      :start-after: psa_crypto_support_def_start
      :end-before: psa_crypto_support_def_end

   The PSA Crypto API is enabled by default when you enable nRF Security.
   For more information, see :ref:`psa_crypto_support`.
   For the list of supported crypto features, see :ref:`ug_crypto_supported_features`.

   Depending on the implementation you are using, the |NCS| builds nRF Security using different versions of the PSA Crypto API.

   .. ncs-include:: ../../../../security/psa_certified_api_overview.rst
      :start-after: psa_crypto_support_tfm_build_start
      :end-before: psa_crypto_support_tfm_build_end

Legacy crypto APIs
   .. ncs-include:: backend_config.rst
      :start-after: legacy_crypto_support_def_start
      :end-before: legacy_crypto_support_def_end

   For more configuration options, see :ref:`nrf_security_legacy_config`.
