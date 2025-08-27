.. _ug_crypto_supported_features:

Supported cryptographic operations in the |NCS|
###############################################

.. contents::
   :local:
   :depth: 2

This reference page lists the supported features and limitations of cryptographic operations in the |NCS|.

.. note::
   Cryptographic features and algorithms that are not listed on this page are not supported by Oberon PSA Crypto in the |NCS|.
   Similarly, if a driver is not available for a device Series for a given feature, it is not included in the corresponding section.

Support definitions
*******************

This page uses the same definitions as the :ref:`software_maturity` page, with the exception of not listing features that are not supported.

.. include:: ../../releases_and_maturity/software_maturity.rst
    :start-after: software_maturity_definitions_start
    :end-before: software_maturity_definitions_end

.. _ug_crypto_supported_features_hardware:

Hardware support for PSA Crypto implementations
***********************************************

The following tables list hardware support for the PSA Crypto implementations in the |NCS|.

.. include:: ../../releases_and_maturity/software_maturity.rst
    :start-after: crypto_support_table_start
    :end-before: crypto_support_table_end

.. _ug_crypto_supported_features_operations:

Cryptographic feature support
*****************************

The following sections list the supported cryptographic features and algorithms.
The lists are organized by device Series and cryptographic drivers: :ref:`nrf_cc3xx <crypto_drivers_cc3xx>`, :ref:`CRACEN <crypto_drivers_cracen>`, and :ref:`nrf_oberon <crypto_drivers_oberon>`.

The listed Kconfig options enable the features and algorithms for the drivers that support them.
The Kconfig options follow the ``CONFIG_PSA_WANT_*`` + ``CONFIG_PSA_USE_*`` configuration scheme, which is described in detail on the :ref:`crypto_drivers` page.

Key types and key management
============================

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific key types.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported key types for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key type support per device (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF52840
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - --
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - --
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - --
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - --
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - --
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - --
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - --
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - --
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - --
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - --
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - --
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - --
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - --
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - --
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - --
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - --
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - --
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - --
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - --
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - --
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Key type support per device (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Supported
                 - Supported
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - Supported
                 - Supported
                 - Supported
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported key types for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key type support per device (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF5340
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - --
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - --
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - --
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - --
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - --
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - --
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - --
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - --
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - --
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - --
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - --
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - --
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - --
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - --
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - --
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - --
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - --
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - --
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - --
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - --
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Key type support per device (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF5340
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - Supported
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - Supported
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - Supported
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - Supported
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - Supported
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - Supported
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - Supported
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - Supported
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - Supported
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - Supported
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - Supported
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - Supported
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - Supported
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - Supported
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - Supported
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - Supported
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - Supported
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - Supported
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - Supported
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - Supported
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported key types for nRF54 Series devices.

      .. note::
         Only some of these key types can be :ref:`stored in the Key Management Unit (KMU) <ug_nrf54l_crypto_kmu_supported_key_types>`.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Key type support per device (CRACEN driver) - nRF54 Series
              :header-rows: 1
              :widths: auto

              * - Key type
                - Configuration option
                - nRF54L05
                - nRF54L10
                - nRF54L15
              * - AES
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                - Supported
                - Supported
                - Supported
              * - Chacha20
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                - Supported
                - Supported
                - Supported
              * - ECC Key Pair Import
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                - Supported
                - Supported
                - Supported
              * - ECC Key Pair Export
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                - Supported
                - Supported
                - Supported
              * - ECC Key Pair Generate
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                - Supported
                - Supported
                - Supported
              * - ECC Key Pair Derive
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                - Supported
                - Supported
                - Supported
              * - ECC Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                - Supported
                - Supported
                - Supported
              * - RSA Key Pair Import
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                - Supported
                - Supported
                - Supported
              * - RSA Key Pair Export
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                - Supported
                - Supported
                - Supported
              * - RSA Key Pair Generate
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                - Supported
                - Supported
                - Supported
              * - RSA Key Pair Derive
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                - Supported
                - Supported
                - Supported
              * - RSA Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                - Supported
                - Supported
                - Supported
              * - XChaCha20
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                - --
                - --
                - --
              * - HSS Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                - --
                - --
                - --
              * - LMS Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                - --
                - --
                - --
              * - XMSS Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                - --
                - --
                - --
              * - XMSS-MT Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                - --
                - --
                - --
              * - ML-DSA-44
                - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                - --
                - --
                - --
              * - ML-DSA-65
                - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                - --
                - --
                - --
              * - ML-DSA-87
                - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                - --
                - --
                - --
              * - ML-DSA Key Pair Import
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                - --
                - --
                - --
              * - ML-DSA Key Pair Export
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                - --
                - --
                - --
              * - ML-DSA Key Pair Generate
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                - --
                - --
                - --
              * - ML-DSA Key Pair Derive
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                - --
                - --
                - --
              * - ML-DSA Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                - --
                - --
                - --
              * - ML-KEM-512
                - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                - --
                - --
                - --
              * - ML-KEM-768
                - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                - --
                - --
                - --
              * - ML-KEM-1024
                - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                - --
                - --
                - --
              * - ML-KEM Key Pair Import
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                - --
                - --
                - --
              * - ML-KEM Key Pair Export
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                - --
                - --
                - --
              * - ML-KEM Key Pair Generate
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                - --
                - --
                - --
              * - ML-KEM Key Pair Derive
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                - --
                - --
                - --
              * - ML-KEM Public Key
                - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                - --
                - --
                - --

         .. tab:: nrf_oberon

            .. list-table:: Key type support per device (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Supported
                 - Supported
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - Supported
                 - Supported
                 - Supported
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported key types for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key type support per device (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - --
                 - --
                 - --
                 - --
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - --
                 - --
                 - --
                 - --
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - --
                 - --
                 - --
                 - --
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - --
                 - --
                 - --
                 - --
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - --
                 - --
                 - --
                 - --
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Key type support per device (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Key type
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - AES
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Chacha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECC Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECC Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - LMS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - XMSS Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - XMSS-MT Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-44
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-65
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA-87
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-DSA Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-768
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM-1024
                 - :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Key Pair Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ML-KEM Public Key
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

Key management
--------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for management of the wanted key types.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the key management support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key management support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`

         .. tab:: nrf_oberon

            .. list-table:: Key management support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - Configuration automatically generated based on the enabled key types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`

   .. tab:: nRF53 Series

      The following tables list the key management support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key management support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`

         .. tab:: nrf_oberon

            .. list-table:: Key management support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - Configuration automatically generated based on the enabled key types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`

   .. tab:: nRF54 Series

      The following tables list the key management support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Key management support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`

         .. tab:: nrf_oberon

            .. list-table:: Key management support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - Configuration automatically generated based on the enabled key types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`

   .. tab:: nRF91 Series

      The following tables list the key management support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key management support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`

         .. tab:: nrf_oberon

            .. list-table:: Key management support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key types
               * - Configuration automatically generated based on the enabled key types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_CHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XCHACHA20`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_HSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_LMS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_XMSS_MT_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_44`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_65`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_DSA_KEY_SIZE_87`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_DSA_PUBLIC_KEY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_768`
                   | :kconfig:option:`CONFIG_PSA_WANT_ML_KEM_KEY_SIZE_1024`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_IMPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_EXPORT`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_GENERATE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_KEY_PAIR_DERIVE`
                   | :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ML_KEM_PUBLIC_KEY`

Cipher modes
============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific cipher modes.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   Key size configuration is supported as described in `AES key size configuration`_, for all algorithms except the stream cipher.

   The :ref:`nrf_security_drivers_cc3xx` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310 (nrf_cc310).

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported cipher modes for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Cipher mode support per device (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF52840
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - --
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Cipher mode support per device (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
                 - Supported
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
                 - Supported
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - Supported
                 - Supported
                 - Supported
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported cipher modes for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Cipher mode support per device (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF5340
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - --
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Cipher mode support per device (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF5340
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - Supported
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported cipher modes for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Cipher mode support per device (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
                 - Supported
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
                 - Supported
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - --
                 - --
                 - --
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Cipher mode support per device (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
                 - Supported
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
                 - Supported
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - Supported
                 - Supported
                 - Supported
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported cipher modes for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Cipher mode support per device (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - --
                 - --
                 - --
                 - --
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Cipher mode support per device (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Cipher mode
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ECB no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - CBC no padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - CBC PKCS#7 padding
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - CTR
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - CCM* no tag
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_STAR_NO_TAG`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Stream cipher
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

Cipher driver
-------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted cipher modes.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the cipher driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Cipher driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_CIPHER_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

         .. tab:: nrf_oberon

            .. list-table:: Cipher driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - Configuration automatically generated based on the enabled cipher modes. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_NO_TAG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

   .. tab:: nRF53 Series

      The following tables list the cipher driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Cipher driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_CIPHER_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

         .. tab:: nrf_oberon

            .. list-table:: Cipher driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - Configuration automatically generated based on the enabled cipher modes. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_NO_TAG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

   .. tab:: nRF54 Series

      The following tables list the cipher driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Cipher driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_CIPHER_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

         .. tab:: nrf_oberon

            .. list-table:: Cipher driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - Configuration automatically generated based on the enabled cipher modes. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_NO_TAG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

   .. tab:: nRF91 Series

      The following tables list the cipher driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Cipher driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_CIPHER_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`

         .. tab:: nrf_oberon

            .. list-table:: Cipher driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported cipher modes
               * - Configuration automatically generated based on the enabled cipher modes. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_NO_PADDING`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CBC_PKCS7`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM_NO_TAG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_STREAM_CIPHER`


Key agreement algorithms
========================

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific key agreement algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   The algorithm support when using ECC key types is dependent on one or more Kconfig options enabling curve support according to `ECC curve types`_.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported key agreement algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key agreement algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF52840
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Key agreement algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported key agreement algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key agreement algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF5340
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Key agreement algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF5340
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported key agreement algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Key agreement algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Key agreement algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported key agreement algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key agreement algorithm support per device (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Experimental
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Key agreement algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Key agreement algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ECDH
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

Key agreement driver
--------------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted key agreement algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the key agreement driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key agreement driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_AGREEMENT_DRIVER`
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`

         .. tab:: nrf_oberon

            .. list-table:: Key agreement driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - Configuration automatically generated based on the enabled key agreement algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH` (limited to curve types secp224r1, secp256r1, secp384r1, and Curve25519)

   .. tab:: nRF53 Series

      The following tables list the key agreement driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key agreement driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_AGREEMENT_DRIVER`
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`

         .. tab:: nrf_oberon

            .. list-table:: Key agreement driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - Configuration automatically generated based on the enabled key agreement algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH` (limited to curve types secp224r1, secp256r1, secp384r1, and Curve25519)

   .. tab:: nRF54 Series

      The following tables list the key agreement driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Key agreement driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_AGREEMENT_DRIVER`
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`

         .. tab:: nrf_oberon

            .. list-table:: Key agreement driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - Configuration automatically generated based on the enabled key agreement algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH` (limited to curve types secp224r1, secp256r1, secp384r1, and Curve25519)

   .. tab:: nRF91 Series

      The following tables list the key agreement driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Key agreement driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_AGREEMENT_DRIVER`
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH`

         .. tab:: nrf_oberon

            .. list-table:: Key agreement driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key agreement algorithms
               * - Configuration automatically generated based on the enabled key agreement algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDH` (limited to curve types secp224r1, secp256r1, secp384r1, and Curve25519)

Key encapsulation algorithms
============================

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific key encapsulation algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported key encapsulation algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Key encapsulation algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - ML-KEM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF53 Series

      The following tables list the supported key encapsulation algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Key encapsulation algorithm
                 - Configuration option
                 - nRF5340
               * - ML-KEM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`
                 - Experimental

   .. tab:: nRF54 Series

      The following tables list the supported key encapsulation algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Key encapsulation algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ML-KEM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF91 Series

      The following tables list the supported key encapsulation algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Key encapsulation algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ML-KEM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental

Key encapsulation driver
------------------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted key encapsulation algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the key encapsulation driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key encapsulation algorithms
               * - Configuration automatically generated based on the enabled key encapsulation algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`

   .. tab:: nRF53 Series

      The following tables list the key encapsulation driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key encapsulation algorithms
               * - Configuration automatically generated based on the enabled key encapsulation algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`

   .. tab:: nRF54 Series

      The following tables list the key encapsulation driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key encapsulation algorithms
               * - Configuration automatically generated based on the enabled key encapsulation algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`

   .. tab:: nRF91 Series

      The following tables list the key encapsulation driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: Key encapsulation driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported key encapsulation algorithms
               * - Configuration automatically generated based on the enabled key encapsulation algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_KEM`


KDF algorithms
==============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific key derivation function (KDF) algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following table lists the supported KDF algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: KDF algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - KDF algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - HKDF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Extract
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Expand
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-AES-CMAC-PRF-128
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PRF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PSK to MS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 EC J-PAKE to PMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108r1 CMAC w/counter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108 HMAC counter mode
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following table lists the supported KDF algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: KDF algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - KDF algorithm
                 - Configuration option
                 - nRF5340
               * - HKDF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                 - Supported
               * - HKDF-Extract
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                 - Supported
               * - HKDF-Expand
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                 - Supported
               * - PBKDF2-HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                 - Supported
               * - PBKDF2-AES-CMAC-PRF-128
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                 - Supported
               * - TLS 1.2 PRF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                 - Supported
               * - TLS 1.2 PSK to MS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                 - Supported
               * - TLS 1.2 EC J-PAKE to PMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                 - Supported
               * - SP 800-108r1 CMAC w/counter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                 - Supported
               * - SP 800-108 HMAC counter mode
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported KDF algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: KDF algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - KDF algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - HKDF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Extract
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Expand
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-AES-CMAC-PRF-128
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PRF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PSK to MS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 EC J-PAKE to PMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108r1 CMAC w/counter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108 HMAC counter mode
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: KDF algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - KDF algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - HKDF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Extract
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Expand
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-AES-CMAC-PRF-128
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PRF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PSK to MS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 EC J-PAKE to PMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108r1 CMAC w/counter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108 HMAC counter mode
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following table lists the supported KDF algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: KDF algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - KDF algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - HKDF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Extract
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HKDF-Expand
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - PBKDF2-AES-CMAC-PRF-128
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PRF
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 PSK to MS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - TLS 1.2 EC J-PAKE to PMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108r1 CMAC w/counter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SP 800-108 HMAC counter mode
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

Key derivation function driver
------------------------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted KDF algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following table lists the KDF driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: KDF driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported KDF algorithms
               * - Configuration automatically generated based on the enabled KDF algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`

   .. tab:: nRF53 Series

      The following table lists the KDF driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: KDF driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported KDF algorithms
               * - Configuration automatically generated based on the enabled KDF algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`

   .. tab:: nRF54 Series

      The following tables list the KDF driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: KDF driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported KDF algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_DERIVATION_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`

         .. tab:: nrf_oberon

            .. list-table:: KDF driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported KDF algorithms
               * - Configuration automatically generated based on the enabled KDF algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`

   .. tab:: nRF91 Series

      The following table lists the KDF driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: KDF driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported KDF algorithms
               * - Configuration automatically generated based on the enabled KDF algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXTRACT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HKDF_EXPAND`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PRF`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_PSK_TO_MS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SP800_108_COUNTER_HMAC`

MAC algorithms
==============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific Message Authentication Code (MAC) algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   Key size configuration for CMAC is supported as described in `AES key size configuration`_.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported MAC algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: MAC algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF52840
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported

            .. note::

               - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` is limited to SHA-1, SHA-224, and SHA-256

         .. tab:: nrf_oberon

            .. list-table:: MAC algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported MAC algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: MAC algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF5340
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported

            .. note::

               - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` is limited to SHA-1, SHA-224, and SHA-256

         .. tab:: nrf_oberon

            .. list-table:: MAC algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF5340
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported MAC algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: MAC algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: MAC algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported MAC algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: MAC algorithm support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

            .. note::

               - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` is limited to SHA-1, SHA-224, and SHA-256

         .. tab:: nrf_oberon

            .. list-table:: MAC algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - MAC algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

MAC driver
----------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted MAC algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the MAC driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: MAC driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_MAC_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` (limited to SHA-1, SHA-224, and SHA-256)

         .. tab:: nrf_oberon

            .. list-table:: MAC driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - Configuration automatically generated based on the enabled MAC algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`

   .. tab:: nRF53 Series

      The following tables list the MAC driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: MAC driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_MAC_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` (limited to SHA-1, SHA-224, and SHA-256)

         .. tab:: nrf_oberon

            .. list-table:: MAC driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - Configuration automatically generated based on the enabled MAC algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`

   .. tab:: nRF54 Series

      The following tables list the MAC driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: MAC driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_MAC_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`

         .. tab:: nrf_oberon

            .. list-table:: MAC driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - Configuration automatically generated based on the enabled MAC algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`

   .. tab:: nRF91 Series

      The following tables list the MAC driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: MAC driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_MAC_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC` (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC` (limited to SHA-1, SHA-224, and SHA-256)

         .. tab:: nrf_oberon

            .. list-table:: MAC driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported MAC algorithms
               * - Configuration automatically generated based on the enabled MAC algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC`

AEAD algorithms
===============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific Authenticated Encryption with Associated Data (AEAD) algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   Key size configuration for CCM and GCM is supported as described in `AES key size configuration`_.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported AEAD algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AEAD algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF52840
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - --

            .. note::

               :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM` is limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310; there is no hardware support for GCM on CC310.

         .. tab:: nrf_oberon

            .. list-table:: AEAD algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
                 - Supported
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
                 - Supported
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF53 Series

      The following tables list the supported AEAD algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AEAD algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF5340
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: AEAD algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF5340
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - Experimental

   .. tab:: nRF54 Series

      The following tables list the supported AEAD algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: AEAD algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
                 - Supported
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
                 - Supported
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - --
                 - --
                 - --

         .. note::

            - CRACEN only supports a 96-bit IV for AES GCM.

         .. tab:: nrf_oberon

            .. list-table:: AEAD algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
                 - Supported
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
                 - Supported
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF91 Series

      The following tables list the supported AEAD algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AEAD algorithm support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: AEAD algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - AEAD algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - CCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - GCM
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - XChaCha20-Poly1305
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental

AEAD driver
-----------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted AEAD algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the AEAD driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AEAD driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_AEAD_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM` (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310, no hardware support for GCM on CC310)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`

         .. tab:: nrf_oberon

            .. list-table:: AEAD driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - Configuration automatically generated based on the enabled AEAD algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`

   .. tab:: nRF53 Series

      The following tables list the AEAD driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AEAD driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_AEAD_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM` (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310, no hardware support for GCM on CC310)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`

         .. tab:: nrf_oberon

            .. list-table:: AEAD driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - Configuration automatically generated based on the enabled AEAD algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`

   .. tab:: nRF54 Series

      The following tables list the AEAD driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: AEAD driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_AEAD_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`

         .. tab:: nrf_oberon

            .. list-table:: AEAD driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - Configuration automatically generated based on the enabled AEAD algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`

   .. tab:: nRF91 Series

      The following tables list the AEAD driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AEAD driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_AEAD_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM` (limited to AES key sizes of 128 bits on devices with Arm CryptoCell CC310, no hardware support for GCM on CC310)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`

         .. tab:: nrf_oberon

            .. list-table:: AEAD driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported AEAD algorithms
               * - Configuration automatically generated based on the enabled AEAD algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_GCM`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_CHACHA20_POLY1305`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XCHACHA20_POLY1305`

Asymmetric signature algorithms
===============================

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific asymmetric signature algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   * The algorithm support when using ECC key types is dependent on one or more Kconfig options enabling curve support according to `ECC curve types`_.
   * RSA key size configuration is supported as described in `RSA key size configuration`_.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported asymmetric signature algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric signature algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF52840
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - --
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - --
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - --
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - --
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
                 - Supported
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
                 - Supported
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
                 - --
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
                 - Supported
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
                 - Supported
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - Experimental
                 - Experimental
                 - Experimental
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - Experimental
                 - Experimental
                 - Experimental
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - Experimental
                 - Experimental
                 - Experimental
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - Experimental
                 - Experimental
                 - Experimental
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF53 Series

      The following tables list the supported asymmetric signature algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric signature algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF5340
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - --
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - --
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - --
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - --
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF5340
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - Experimental
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - Experimental
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - Experimental
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - Experimental
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - Experimental

            .. note::
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` is limited to ECC curve types secp224r1, secp256r1, and secp384r1.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` is limited to ECC curve type Ed25519.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` does not support RSA key pair generation.

   .. tab:: nRF54 Series

      The following tables list the supported asymmetric signature algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Asymmetric signature algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
                 - Supported
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
                 - Supported
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
                 - Supported
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - --
                 - --
                 - --
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - --
                 - --
                 - --
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - --
                 - --
                 - --
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - --
                 - --
                 - --
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - --
                 - --
                 - --
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - --
                 - --
                 - --
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
                 - Supported
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
                 - Supported
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
                 - --
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
                 - Supported
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
                 - Supported
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - Experimental
                 - Experimental
                 - Experimental
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - Experimental
                 - Experimental
                 - Experimental
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - Experimental
                 - Experimental
                 - Experimental
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - Experimental
                 - Experimental
                 - Experimental
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - Experimental
                 - Experimental
                 - Experimental

            .. note::
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` is limited to ECC curve types secp224r1, secp256r1, and secp384r1.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` is limited to ECC curve type Ed25519.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` does not support RSA key pair generation.

   .. tab:: nRF91 Series

      The following tables list the supported asymmetric signature algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric signature algorithm support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
                 - --
                 - --
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - --
                 - --
                 - --
                 - --
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - --
                 - --
                 - --
                 - --
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - --
                 - --
                 - --
                 - --
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - --
                 - --
                 - --
                 - --
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric signature algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - ECDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA without hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - ECDSA (deterministic)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - PureEdDSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HashEdDSA Edwards25519
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                 - --
                 - --
                 - --
                 - --
               * - RSA PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA raw PKCS#1 v1.5 sign
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA PSS any salt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - LMS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - ML-DSA
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - XMSS
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - XMSS-MT
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental

            .. note::
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` is limited to ECC curve types secp224r1, secp256r1, and secp384r1.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` is limited to ECC curve type Ed25519.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` does not support RSA key pair generation.

Asymmetric signature driver
---------------------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted asymmetric signature algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the asymmetric signature driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric signature driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - Configuration automatically generated based on the enabled asymmetric signature algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` (limited to ECC curve types secp224r1, secp256r1, and secp384r1)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` (limited to ECC curve type Ed25519)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` (does not support RSA key pair generation)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`

   .. tab:: nRF53 Series

      The following tables list the asymmetric signature driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric signature driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - Configuration automatically generated based on the enabled asymmetric signature algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` (limited to ECC curve types secp224r1, secp256r1, and secp384r1)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` (limited to ECC curve type Ed25519)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` (does not support RSA key pair generation)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`

   .. tab:: nRF54 Series

      The following tables list the asymmetric signature driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Asymmetric signature driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ED25519PH`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - Configuration automatically generated based on the enabled asymmetric signature algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` (limited to ECC curve types secp224r1, secp256r1, and secp384r1)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` (limited to ECC curve type Ed25519)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` (does not support RSA key pair generation)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`

   .. tab:: nRF91 Series

      The following tables list the asymmetric signature driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric signature driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_SIGNATURE_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric signature driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric signature algorithms
               * - Configuration automatically generated based on the enabled asymmetric signature algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA` (limited to ECC curve types secp224r1, secp256r1, and secp384r1)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ECDSA_ANY`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_DETERMINISTIC_ECDSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_PURE_EDDSA` (limited to ECC curve type Ed25519)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN_RAW`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PSS_ANY_SALT` (does not support RSA key pair generation)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_LMS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_ML_DSA`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_XMSS_MT`

Asymmetric encryption algorithms
================================

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific asymmetric encryption algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   RSA key size configuration is supported as described in `RSA key size configuration`_.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported asymmetric encryption algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric encryption algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF52840
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` is limited to key sizes ≤ 2048 bits.

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` does not support RSA key pair generation.

   .. tab:: nRF53 Series

      The following tables list the supported asymmetric encryption algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric encryption algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF5340
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` is limited to key sizes ≤ 2048 bits.

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF5340
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` does not support RSA key pair generation.

   .. tab:: nRF54 Series

      The following tables list the supported asymmetric encryption algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Asymmetric encryption algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
                 - Supported
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` does not support RSA key pair generation.

   .. tab:: nRF91 Series

      The following tables list the supported asymmetric encryption algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric encryption algorithm support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` is limited to key sizes ≤ 2048 bits.

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Asymmetric encryption algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - RSA OAEP
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - RSA PKCS#1 v1.5 crypt
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` does not support RSA key pair generation.

Asymmetric encryption driver
----------------------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted asymmetric encryption algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the asymmetric encryption driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric encryption driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (limited to key sizes ≤ 2048 bits)

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - Configuration automatically generated based on the enabled asymmetric encryption algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (does not support RSA key pair generation)

   .. tab:: nRF53 Series

      The following tables list the asymmetric encryption driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric encryption driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (limited to key sizes ≤ 2048 bits)

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - Configuration automatically generated based on the enabled asymmetric encryption algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (does not support RSA key pair generation)

   .. tab:: nRF54 Series

      The following tables list the asymmetric encryption driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Asymmetric encryption driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT`

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - Configuration automatically generated based on the enabled asymmetric encryption algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (does not support RSA key pair generation)

   .. tab:: nRF91 Series

      The following tables list the asymmetric encryption driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Asymmetric encryption driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_ASYMMETRIC_ENCRYPTION_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (limited to key sizes ≤ 2048 bits)

         .. tab:: nrf_oberon

            .. list-table:: Asymmetric encryption driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported asymmetric encryption algorithms
               * - Configuration automatically generated based on the enabled asymmetric encryption algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_OAEP`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_CRYPT` (does not support RSA key pair generation)

ECC curve types
===============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific ECC curve types.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported ECC curve types for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC curve type support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF52840
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - Supported
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - --
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - Supported
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - Supported
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - Supported
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: ECC curve type support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
                 - --
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
                 - --
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - --
                 - --
                 - --
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
                 - --
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
                 - --
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
                 - --
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
                 - Supported
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
                 - Supported
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - Supported
                 - Supported
                 - Supported
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - --
                 - --
                 - --
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - --
                 - --
                 - --
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - --
                 - --
                 - --
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
                 - Supported
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
                 - Supported
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
                 - Supported
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --
                 - --
                 - --

   .. tab:: nRF53 Series

      The following tables list the supported ECC curve types for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC curve type support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF5340
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - Supported
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - --
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - Supported
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - Supported
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - Supported
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: ECC curve type support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF5340
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - --
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - Supported
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - --
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - --
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - --
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --

   .. tab:: nRF54 Series

      The following tables list the supported ECC curve types for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: ECC curve type support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - Supported
                 - Supported
                 - Supported
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - Supported
                 - Supported
                 - Supported
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - Supported
                 - Supported
                 - Supported
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - Supported
                 - Supported
                 - Supported
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - Supported
                 - Supported
                 - Supported
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - Supported
                 - Supported
                 - Supported
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
                 - Supported
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
                 - Supported
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - --
                 - --
                 - --
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - Supported
                 - Supported
                 - Supported
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - Supported
                 - Supported
                 - Supported
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - Supported
                 - Supported
                 - Supported
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
                 - Supported
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
                 - Supported
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
                 - Supported
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: ECC curve type support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
                 - --
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
                 - --
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - --
                 - --
                 - --
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
                 - --
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
                 - --
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
                 - --
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
                 - Supported
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
                 - Supported
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - Supported
                 - Supported
                 - Supported
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - --
                 - --
                 - --
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - --
                 - --
                 - --
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - --
                 - --
                 - --
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
                 - Supported
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
                 - Supported
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
                 - Supported
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --
                 - --
                 - --

   .. tab:: nRF91 Series

      The following tables list the supported ECC curve types for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC curve type support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
                 - --
                 - --
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - --
                 - --
                 - --
                 - --
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: ECC curve type support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - ECC curve type
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - BrainpoolP192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP320r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                 - --
                 - --
                 - --
                 - --
               * - BrainpoolP512r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                 - --
                 - --
                 - --
                 - --
               * - Curve25519 (X25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Edwards25519 (Ed25519)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Edwards448 (Ed448)
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp192k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                 - --
                 - --
                 - --
                 - --
               * - secp256k1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                 - --
                 - --
                 - --
                 - --
               * - secp192r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                 - --
                 - --
                 - --
                 - --
               * - secp224r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp256r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp384r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - secp521r1
                 - :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`
                 - --
                 - --
                 - --
                 - --

ECC curve driver
----------------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for key management using the wanted ECC curve types.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the ECC curve driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC curve driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

         .. tab:: nrf_oberon

            .. list-table:: ECC curve driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - Configuration automatically generated based on the enabled ECC curve types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

   .. tab:: nRF53 Series

      The following tables list the ECC curve driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC curve driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

         .. tab:: nrf_oberon

            .. list-table:: ECC curve driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - Configuration automatically generated based on the enabled ECC curve types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

   .. tab:: nRF54 Series

      The following tables list the ECC curve driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: ECC curve driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_320`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_521`

         .. tab:: nrf_oberon

            .. list-table:: ECC curve driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - Configuration automatically generated based on the enabled ECC curve types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

   .. tab:: nRF91 Series

      The following tables list the ECC curve driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC curve driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_KEY_MANAGEMENT_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_BRAINPOOL_P_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_K1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

         .. tab:: nrf_oberon

            .. list-table:: ECC curve driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported ECC curve types
               * - Configuration automatically generated based on the enabled ECC curve types. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ECC_MONTGOMERY_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_255`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_TWISTED_EDWARDS_448`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ECC_SECP_R1_384`

RNG algorithms
==============

RNG uses PRNG seeded by entropy (also known as TRNG).
The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for enabling RNG and requesting Oberon PSA Crypto to use specific PRNG algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   * Both PRNG algorithms are NIST-qualified Cryptographically Secure Pseudo Random Number Generators (CSPRNG).
   * :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG` and :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` are custom configurations not described by the PSA Crypto specification.
   * If multiple PRNG algorithms are enabled at the same time, CTR-DRBG will be prioritized for random number generation through the front-end APIs for PSA Crypto.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported RNG algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RNG algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF52840
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported

            .. note::
               - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` is enabled by default for nRF52840.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is limited to 1024 bytes per request.

         .. tab:: nrf_oberon

            .. list-table:: RNG algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
                 - Supported
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
                 - Supported
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is implemented in software, with entropy provided by the hardware RNG peripheral.

   .. tab:: nRF53 Series

      The following tables list the supported RNG algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RNG algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF5340
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported

            .. note::
               - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` is enabled by default for nRF5340.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is limited to 1024 bytes per request.

         .. tab:: nrf_oberon

            .. list-table:: RNG algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF5340
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is implemented in software, with entropy provided by the hardware RNG peripheral.

   .. tab:: nRF54 Series

      The following tables list the supported RNG algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: RNG algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
                 - Supported
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
                 - Supported
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: RNG algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
                 - Supported
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
                 - Supported
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is implemented in software, with entropy provided by the hardware RNG peripheral.

   .. tab:: nRF91 Series

      The following tables list the supported RNG algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RNG algorithm support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

            .. note::
               - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM` is enabled by default for nRF91 Series devices.
               - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is limited to 1024 bytes per request.

         .. tab:: nrf_oberon

            .. list-table:: RNG algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - PRNG algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - RNG support
                 - :kconfig:option:`CONFIG_PSA_WANT_GENERATE_RANDOM`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - CTR-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - HMAC-DRBG
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

            .. note::
               :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` is implemented in software, with entropy provided by the hardware RNG peripheral.

RNG driver
----------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted RNG algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the RNG driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RNG driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Enabled by default for nRF52840, nRF91 Series, and nRF5340 devices
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (limited to 1024 bytes per request)

         .. tab:: nrf_oberon

            .. list-table:: RNG driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Configuration automatically generated based on the enabled RNG algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (software implementation, entropy provided by the hardware RNG peripheral)

   .. tab:: nRF53 Series

      The following tables list the RNG driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RNG driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Enabled by default for nRF52840, nRF91 Series, and nRF5340 devices
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (limited to 1024 bytes per request)

         .. tab:: nrf_oberon

            .. list-table:: RNG driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Configuration automatically generated based on the enabled RNG algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (software implementation, entropy provided by the hardware RNG peripheral)

   .. tab:: nRF54 Series

      The following tables list the RNG driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: RNG driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_CTR_DRBG_DRIVER`
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`

         .. tab:: nrf_oberon

            .. list-table:: RNG driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Configuration automatically generated based on the enabled RNG algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (software implementation, entropy provided by the hardware RNG peripheral)

   .. tab:: nRF91 Series

      The following tables list the RNG driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RNG driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Enabled by default for nRF52840, nRF91 Series, and nRF5340 devices
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (limited to 1024 bytes per request)

         .. tab:: nrf_oberon

            .. list-table:: RNG driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported RNG algorithms
               * - Configuration automatically generated based on the enabled RNG algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_CTR_DRBG`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_HMAC_DRBG` (software implementation, entropy provided by the hardware RNG peripheral)

Hash algorithms
===============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific hash algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   The SHA-1 hash is weak and deprecated and is only recommended for use in legacy protocols.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported hash algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Hash algorithm support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF52840
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - --
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - --
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - --
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - --
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - --
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - --
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Hash algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
                 - --
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
                 - --
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
                 - --
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
                 - --
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported hash algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Hash algorithm support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF5340
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - --
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - --
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - --
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - --
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - --
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - --
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Hash algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF5340
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - Supported
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - Supported
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - Supported
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - Experimental
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - Supported
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - Supported
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported hash algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Hash algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Hash algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - Supported
                 - Supported
                 - Supported
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
                 - --
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
                 - --
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
                 - --
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
                 - --
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported hash algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Hash algorithm support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - --
                 - --
                 - --
                 - --
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
                 - --
                 - --
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - --
                 - --
                 - --
                 - --
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - --
                 - --
                 - --
                 - --
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - --
                 - --
                 - --
                 - --
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - --
                 - --
                 - --
                 - --
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: Hash algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Hash algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - SHA-1 (weak)
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHA3-224
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-256
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-384
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                 - --
                 - --
                 - --
                 - --
               * - SHA3-512
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`
                 - --
                 - --
                 - --
                 - --
               * - SHA-256/192
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE128 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - SHAKE256 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SHAKE256 512 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

Hash driver
-----------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted hash algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the hash driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Hash driver support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_HASH_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`

         .. tab:: nrf_oberon

            .. list-table:: Hash driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - Configuration automatically generated based on the enabled hash algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`

   .. tab:: nRF53 Series

      The following tables list the hash driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Hash driver support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_HASH_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`

         .. tab:: nrf_oberon

            .. list-table:: Hash driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - Configuration automatically generated based on the enabled hash algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`

   .. tab:: nRF54 Series

      The following tables list the hash driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: Hash driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_HASH_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA3_512`

         .. tab:: nrf_oberon

            .. list-table:: Hash driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - Configuration automatically generated based on the enabled hash algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`

   .. tab:: nRF91 Series

      The following tables list the hash driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: Hash driver support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CC3XX_HASH_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`

         .. tab:: nrf_oberon

            .. list-table:: Hash driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported hash algorithms
               * - Configuration automatically generated based on the enabled hash algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_1` (weak)
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_224`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_384`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_512`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHA_256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_192`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_256`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256_512`

PAKE algorithms
===============

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific password-authenticated key exchange (PAKE) algorithms.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following table lists the supported PAKE algorithms for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: PAKE algorithm support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - PAKE algorithm
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - EC J-PAKE
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ for Matter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP-6
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP password hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF53 Series

      The following table lists the supported PAKE algorithms for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: PAKE algorithm support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - PAKE algorithm
                 - Configuration option
                 - nRF5340
               * - EC J-PAKE
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                 - Supported
               * - SPAKE2+ with HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                 - Supported
               * - SPAKE2+ with CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                 - Supported
               * - SPAKE2+ for Matter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                 - Experimental
               * - SRP-6
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                 - Experimental
               * - SRP password hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`
                 - Experimental

   .. tab:: nRF54 Series

      The following tables list the supported PAKE algorithms for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: PAKE algorithm support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - PAKE algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - EC J-PAKE
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ for Matter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP-6
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP password hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`
                 - Experimental
                 - Experimental
                 - Experimental

         .. tab:: nrf_oberon

            .. list-table:: PAKE algorithm support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - PAKE algorithm
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - EC J-PAKE
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ for Matter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP-6
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP password hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`
                 - Experimental
                 - Experimental
                 - Experimental

   .. tab:: nRF91 Series

      The following table lists the supported PAKE algorithms for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: PAKE algorithm support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - PAKE algorithm
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - EC J-PAKE
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with HMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ with CMAC
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - SPAKE2+ for Matter
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP-6
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental
               * - SRP password hashing
                 - :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`
                 - Experimental
                 - Experimental
                 - Experimental
                 - Experimental

PAKE driver
-----------

The following tables show the ``CONFIG_PSA_USE_*`` Kconfig options for configuring Oberon PSA Crypto to use specific drivers for the wanted PAKE algorithms.

.. tabs::

   .. tab:: nRF52 Series

      The following table lists the PAKE driver support for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: PAKE driver support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported PAKE algorithms
               * - Configuration automatically generated based on the enabled PAKE algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`

   .. tab:: nRF53 Series

      The following table lists the PAKE driver support for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: PAKE driver support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported PAKE algorithms
               * - Configuration automatically generated based on the enabled PAKE algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`

   .. tab:: nRF54 Series

      The following tables list the PAKE driver support for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: PAKE driver support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported PAKE algorithms
               * - :kconfig:option:`CONFIG_PSA_USE_CRACEN_PAKE_DRIVER`
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`

         .. tab:: nrf_oberon

            .. list-table:: PAKE driver support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported PAKE algorithms
               * - Configuration automatically generated based on the enabled PAKE algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`

   .. tab:: nRF91 Series

      The following table lists the PAKE driver support for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: PAKE driver support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - Kconfig option
                 - Supported PAKE algorithms
               * - Configuration automatically generated based on the enabled PAKE algorithms. Acts as :ref:`software fallback <crypto_drivers_software_fallback>` for the other drivers.
                 - | :kconfig:option:`CONFIG_PSA_WANT_ALG_JPAKE`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_HMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_CMAC`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SPAKE2P_MATTER`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_6`
                   | :kconfig:option:`CONFIG_PSA_WANT_ALG_SRP_PASSWORD_HASH`

Key pair operations
===================

The following sections list the supported key pair operation Kconfig options for different key types.
The Kconfig options follow the ``CONFIG_PSA_WANT_*`` configuration scheme, which is described in detail on the :ref:`crypto_drivers` page.

RSA key pair operations
-----------------------

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific RSA key pair operations.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported RSA key pair operations for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RSA key pair operation support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF52840
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: RSA key pair operation support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported RSA key pair operations for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RSA key pair operation support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF5340
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: RSA key pair operation support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF5340
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported RSA key pair operations for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: RSA key pair operation support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: RSA key pair operation support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported RSA key pair operations for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RSA key pair operation support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: RSA key pair operation support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - RSA key pair operation
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

SRP key pair operations
-----------------------

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific SRP key pair operations.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following table lists the supported SRP key pair operations for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: SRP key pair operation support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - SRP key pair operation
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following table lists the supported SRP key pair operations for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: SRP key pair operation support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - SRP key pair operation
                 - Configuration option
                 - nRF5340
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_EXPORT`
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_GENERATE`
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_DERIVE`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported SRP key pair operations for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: SRP key pair operation support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - SRP key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: SRP key pair operation support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - SRP key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following table lists the supported SRP key pair operations for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: SRP key pair operation support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - SRP key pair operation
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_GENERATE`
                 - --
                 - --
                 - --
                 - --
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SRP_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

SPAKE2P key pair operations
---------------------------

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific SPAKE2P key pair operations.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following table lists the supported SPAKE2P key pair operations for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: SPAKE2P key pair operation support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - SPAKE2P key pair operation
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following table lists the supported SPAKE2P key pair operations for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: SPAKE2P key pair operation support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - SPAKE2P key pair operation
                 - Configuration option
                 - nRF5340
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT`
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE`
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported SPAKE2P key pair operations for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: SPAKE2P key pair operation support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - SPAKE2P key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: SPAKE2P key pair operation support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - SPAKE2P key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following table lists the supported SPAKE2P key pair operations for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_oberon

            .. list-table:: SPAKE2P key pair operation support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - SPAKE2P key pair operation
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_SPAKE2P_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

ECC key pair operations
-----------------------

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific ECC key pair operations.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported ECC key pair operations for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC key pair operation support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF52840
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: ECC key pair operation support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported ECC key pair operations for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC key pair operation support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF5340
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: ECC key pair operation support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF5340
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported ECC key pair operations for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: ECC key pair operation support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: ECC key pair operation support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported ECC key pair operations for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: ECC key pair operation support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: ECC key pair operation support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - ECC key pair operation
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - Generate
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Import
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Export
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - Derive
                 - :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

.. _ug_crypto_supported_features_key_size:

Key size configurations
=======================

The following sections list the supported AES and RSA key size Kconfig options.
The Kconfig options follow the ``CONFIG_PSA_WANT_*`` configuration scheme, which is described in detail on the :ref:`crypto_drivers` page.

AES key size configuration
--------------------------

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific AES key sizes.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. note::
   CRACEN only supports a 96-bit IV for AES GCM.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported AES key sizes for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AES key size support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF52840
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - --
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: AES key size support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
                 - Supported
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - Supported
                 - Supported
                 - Supported
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported AES key sizes for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AES key size support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF5340
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - Supported
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: AES key size support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF5340
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - Supported
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported AES key sizes for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: AES key size support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
                 - Supported
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - Supported
                 - Supported
                 - Supported
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - Supported
                 - Supported
                 - Supported

         .. tab:: nrf_oberon

            .. list-table:: AES key size support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
                 - Supported
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - Supported
                 - Supported
                 - Supported
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported AES key sizes for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: AES key size support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - --
                 - --
                 - --
                 - --
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: AES key size support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - AES key size
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - 128 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_128`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_192`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 256 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_AES_KEY_SIZE_256`
                 - Supported
                 - Supported
                 - Supported
                 - Supported

RSA key size configuration
--------------------------

The following tables show the ``CONFIG_PSA_WANT_*`` Kconfig options for requesting Oberon PSA Crypto to use specific RSA key sizes.
The options are grouped by Series and drivers available for the device Series, and support level for each device is listed.

.. tabs::

   .. tab:: nRF52 Series

      The following tables list the supported RSA key sizes for nRF52 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RSA key size support (nrf_cc3xx driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF52840
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - --
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - --
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - --
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: RSA key size support (nrf_oberon driver) - nRF52 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF52832
                 - nRF52833
                 - nRF52840
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
                 - Supported
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
                 - Supported
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - Supported
                 - Supported
                 - Supported
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - Supported
                 - Supported
                 - Supported
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - Supported
                 - Supported
                 - Supported
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF53 Series

      The following tables list the supported RSA key sizes for nRF53 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RSA key size support (nrf_cc3xx driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF5340
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - Supported
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - --
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - --
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - --

         .. tab:: nrf_oberon

            .. list-table:: RSA key size support (nrf_oberon driver) - nRF53 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF5340
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - Supported
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - Supported
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - Supported
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - Supported

   .. tab:: nRF54 Series

      The following tables list the supported RSA key sizes for nRF54 Series devices.

      .. tabs::

         .. tab:: CRACEN

            .. list-table:: RSA key size support (CRACEN driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - --
                 - --
                 - --
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - --
                 - --
                 - --
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
                 - Supported
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - Supported
                 - Supported
                 - Supported
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - Supported
                 - Supported
                 - Supported
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - --
                 - --
                 - --
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: RSA key size support (nrf_oberon driver) - nRF54 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF54L05
                 - nRF54L10
                 - nRF54L15
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
                 - Supported
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
                 - Supported
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - Supported
                 - Supported
                 - Supported
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - Supported
                 - Supported
                 - Supported
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - Supported
                 - Supported
                 - Supported
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - Supported
                 - Supported
                 - Supported

   .. tab:: nRF91 Series

      The following tables list the supported RSA key sizes for nRF91 Series devices.

      .. tabs::

         .. tab:: nrf_cc3xx

            .. list-table:: RSA key size support (nrf_cc3xx driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - --
                 - --
                 - --
                 - --
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - --
                 - --
                 - --
                 - --
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - --
                 - --
                 - --
                 - --
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - --
                 - --
                 - --
                 - --

         .. tab:: nrf_oberon

            .. list-table:: RSA key size support (nrf_oberon driver) - nRF91 Series
               :header-rows: 1
               :widths: auto

               * - RSA key size
                 - Configuration option
                 - nRF9131
                 - nRF9151
                 - nRF9160
                 - nRF9161
               * - 1024 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1024`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 1536 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_1536`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 2048 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_2048`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 3072 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_3072`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 4096 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_4096`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 6144 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_6144`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
               * - 8192 bits
                 - :kconfig:option:`CONFIG_PSA_WANT_RSA_KEY_SIZE_8192`
                 - Supported
                 - Supported
                 - Supported
                 - Supported
