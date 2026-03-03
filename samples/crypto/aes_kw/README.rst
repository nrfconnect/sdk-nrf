.. _crypto_aes_kw:

Crypto: AES KW
##############

.. contents::
   :local:
   :depth: 2

The AES KW sample demonstrates how to use the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` to perform key wrapping and unwrapping operations using AES Key-Encryption Key.

The sample can be configured to use either AES-KW (Key Wrap) or AES-KWP (Key Wrap with padding) algorithm.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54l15dk_nrf54l05_cpuapp, nrf54l15dk_nrf54l10_cpuapp_and_cpuapp_ns, nrf54l15dk_nrf54l15_cpuapp_and_cpuapp_ns, nrf54lm20dk_nrf54lm20a_cpuapp, nrf54lm20dk_nrf54lm20a_cpuapp_ns, nrf54lv10dk_nrf54lv10a_cpuapp, nrf54lv10dk_nrf54lv10a_cpuapp_ns

Overview
********

The sample :ref:`enables PSA Crypto API <psa_crypto_support_enable>` and configures the following Kconfig options for the cryptographic features:

* :kconfig:option:`CONFIG_PSA_WANT_KEY_TYPE_AES` - Used to enable support for AES key types from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_types`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_ECB_NO_PADDING` - Used to enable support for the ECB cipher mode without padding from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_cipher_modes`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_AES_KW` - Used to enable AES key-wrapping algorithm based on the NIST Key Wrap (KW) mode of a block cipher from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_wrap_algorithms`.
* :kconfig:option:`CONFIG_PSA_WANT_ALG_AES_KWP` - Used to enable AES key-wrapping algorithm based on the NIST Key Wrap with Padding (KWP) mode of a block cipher from among the supported cryptographic operations for :ref:`ug_crypto_supported_features_key_wrap_algorithms`.
  This algorithm is used when you use the :ref:`AES-KWP overlay<aes_kwp_overlay>` file.

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_overview_driver_selection_start
   :end-before: crypto_sample_overview_driver_selection_end

Once built and run, the sample performs the following operations:

1. Initialization:

   a. The PSA Crypto API is initialized using :c:func:`psa_crypto_init`.
   #. A predefined 128-bit or 192-bit AES key (Key-Encryption Key) is imported using :c:func:`psa_import_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for both wrapping and unwrapping.
   #. A predefined plain key is imported using :c:func:`psa_import_key` and stored in the PSA crypto keystore.
      The key is configured with usage flags for export.
      This is required to be able to execute wrapping operation.

#. Wrapping and unwrapping of a sample plain key:

   a. A wrapping operation is done using :c:func:`psa_wrap_key` with either the ``PSA_ALG_KW`` or ``PSA_ALG_KWP`` algorithm (depending on the sample configuration).
   #. Wrapped key is compared with expected value taken from either RFC3394 or RFC5649.
   #. The plain key is removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.
   #. An unwrapping operation is done using :c:func:`psa_unwrap_key` with key usage configured for export.
      It is required to get the decrypted key value and compare to the original key.
   #. A key export operation is done using :c:func:`psa_export_key`.
   #. The exported key is compared with the original plain ley to verify correctness.

#. Cleanup:

   a. Both Key-Encryption Key and unwrapped plain key are removed from the PSA crypto keystore using :c:func:`psa_destroy_key`.

.. note::
   192-bit AES keys are not supported by the CRACEN driver for nRF54LM20 and nRF54LV10A devices.
   See also :ref:`ug_crypto_supported_features_aes_key_sizes`.

Configuration
*************

|config|

.. _aes_kwp_overlay:

AES-KW algorithm overlay
========================

The sample allows you to use either the AES-KW or the AES-KWP algorithm.
By default, the sample demonstrates AES-KW algorithm usage.

If you want to configure the sample to work with the AES-KWP algorithm, use the :file:`aes_kwp.conf` file in the :file:`overlays` directory to extend the main configuration file.
To provide this extra overlay file, use the :ref:`EXTRA_CONF_FILE CMake option<building_overlay_files>` when building the sample (``-DEXTRA_CONF_FILE=aes_kwp.conf``).

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/aes_kw`

.. include:: /includes/build_and_run.txt

Testing
=======

.. include:: /samples/crypto/aes_cbc/README.rst
   :start-after: crypto_sample_testing_start
   :end-before: crypto_sample_testing_end

.. code-block:: text

   *** Using Zephyr OS v4.3.99-a7c2a12b9134 ***
   [00:00:00.002,512] <inf> aes_kw: Starting AES-KW example...
   [00:00:00.002,526] <inf> aes_kw: Using AES-KWP algorithm
   [00:00:00.002,535] <inf> aes_kw: Importing Key-Encryption Key...
   [00:00:00.002,563] <inf> aes_kw: Key-Encryption Key imported successfully!
   [00:00:00.002,568] <inf> aes_kw: Importing key (to wrap)...
   [00:00:00.002,579] <inf> aes_kw: ---- Source key (len: 20): ----
   [00:00:00.002,589] <inf> aes_kw: Content:
                                    c3 7b 7e 64 92 58 43 40  be d1 22 07 80 89 41 15 |.{~d.XC@ .."...A.
                                    50 68 f7 38                                      |Ph.8
   [00:00:00.002,598] <inf> aes_kw: ---- Source key end  ----
   [00:00:00.002,622] <inf> aes_kw: KEY key imported successfully!
   [00:00:00.002,627] <inf> aes_kw: Wrapping key...
   [00:00:00.012,626] <inf> aes_kw: ---- Wrapped key (len: 32): ----
   [00:00:00.012,637] <inf> aes_kw: Content:
                                    13 8b de aa 9b 8f a7 fc  61 f9 77 42 e7 22 48 ee |........ a.wB."H.
                                    5a e6 ae 53 60 d1 ae 6a  5f 54 f3 73 fa 54 3b 6a |Z..S`..j _T.s.T;j
   [00:00:00.012,647] <inf> aes_kw: ---- Wrapped key end  ----
   [00:00:00.012,660] <inf> aes_kw: Wrapped key size is expected: 32 bytes
   [00:00:00.012,676] <inf> aes_kw: Wrapped key data is correct!
   [00:00:00.012,702] <inf> aes_kw: Unwrapping key...
   [00:00:00.013,875] <inf> aes_kw: ---- Unwrapped key (len: 20): ----
   [00:00:00.013,886] <inf> aes_kw: Content:
                                    c3 7b 7e 64 92 58 43 40  be d1 22 07 80 89 41 15 |.{~d.XC@ .."...A.
                                    50 68 f7 38                                      |Ph.8
   [00:00:00.013,896] <inf> aes_kw: ---- Unwrapped key end  ----
   [00:00:00.013,909] <inf> aes_kw: Unwrapped key size is expected: 20 bytes
   [00:00:00.013,925] <inf> aes_kw: Unwrapped key data is correct!
   [00:00:00.013,963] <inf> aes_kw: Example finished successfully!
