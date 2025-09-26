.. _secure_storage_in_ncs:

Secure storage in the |NCS|
###########################

.. contents::
   :local:
   :depth: 2

The |NCS| implements secure storage through the :ref:`PSA Certified Secure Storage API <ug_psa_certified_api_overview_secstorage>`.
The implementation is designed to securely store and manage sensitive data, such as cryptographic keys, device credentials, and configuration data.

.. secure_storage_options_table_start

The following implementations of the PSA Secure Storage API are available:

* TF-M's :ref:`ug_tfm_services_its` and :ref:`tfm_partition_ps` services - This option can only be used when :ref:`building with TF-M <ug_tfm_building>` and if the `ARM TrustZone`_ technology and hardware-accelerated firmware isolation are supported by the hardware platform in use.
  Using this option, the Internal Trusted Storage and the Protected Storage are working :ref:`with security by separation <ug_tfm_security_by_separation>`.

  .. note::
     In the |NCS|, the PSA Protected Storage implementation is optional for use alongside other :ref:`data storage options <data_storage_psa_protected>`.
     It does not support storing data to external flash.
     Instead, you can configure your application to encrypt data stored to the external flash, for example using the :ref:`ug_psa_certified_api_overview_crypto`.

* The :ref:`Trusted storage library <trusted_storage_readme>` - This option enables you to provide features like integrity, confidentiality, and authenticity of the stored data without using the TF-M Platform Root of Trust (PRoT) (building without TF-M).
  Using this option, the library can implement the PSA Secure Storage APIs with device-protected storage, without isolation.

.. secure_storage_options_table_end

SoCs from Nordic Semiconductor that come with dedicated hardware components for security (such as `nRF9160's System Protection Unit <nRF9160 System Protection Unit_>`_ or `nRF54L Series' security components <nRF54L15 Security_>`_) have the functionality for `flash <nRF9160 flash access control_>`_ (nRF9160) or `feature <nRF54L15 feature access control_>`_ (nRF54L15) access control, making it possible to configure different features as secure.

The table below gives an overview of the secure storage support for the products and their features.
The table is based on the :ref:`PSA Certified Secure Storage API <ug_psa_certified_api_overview_secstorage>`.

.. list-table:: Secure storage product support
   :widths: auto
   :header-rows: 1

   * - Product
     - Backend
     - Confidentiality
     - Integrity
     - Authenticity
     - Isolation
   * - nRF91 Series with TF-M
     - TF-M's :ref:`ug_tfm_services_its` and :ref:`tfm_partition_ps` services
     - Yes
     - Yes
     - Yes
     - Yes
   * - nRF91 Series without TF-M
     - Trusted storage library
     - Partial [1]_
     - Yes
     - Yes
     - No
   * - nRF54L15 with TF-M
     - TF-M's :ref:`ug_tfm_services_its` and :ref:`tfm_partition_ps` services
     - Yes
     - Yes
     - Yes
     - Yes
   * - nRF54L15 without TF-M
     - Trusted storage library
     - Partial [1]_
     - Yes
     - Yes
     - Yes
   * - nRF5340 with TF-M
     - TF-M's :ref:`ug_tfm_services_its` and :ref:`tfm_partition_ps` services
     - Yes
     - Yes
     - Yes
     - Yes
   * - nRF5340 without TF-M
     - Trusted storage library
     - Partial [1]_
     - Yes
     - Yes
     - No
   * - nRF52840
     - Trusted storage library
     - Partial [1]_
     - Yes
     - Yes
     - No
   * - nRF52833
     - Trusted storage library
     - Partial [2]_
     - Yes
     - Yes
     - No
   * - nRF52832
     - Trusted storage library
     - Partial [2]_
     - Yes
     - Yes
     - No

Notes for confidentiality partial support
  .. [1] On systems without the isolation feature, the confidentiality is limited to protection of data at rest in a non-volatile internal or external memory.
         This partial confidentiality is based on a CPU-inaccessible master key used for data encryption.
         When the data is decrypted for usage, there is no mechanism providing access control and protecting its visibility.
         Use of a TrustZone-enabled system provides stronger protection, and is recommended if available.
  .. [2] The use of Hardware Unique Key (HUK) to provide the AEAD key is not available on nRF52833 and nRF52832 devices.
         The trusted storage library offers the use of SHA-256 to generate the key, which does not provide guarantees of security beyond the integrity check of the encrypted data.
