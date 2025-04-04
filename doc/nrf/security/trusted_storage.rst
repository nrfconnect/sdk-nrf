.. _trusted_storage_in_ncs:

Trusted storage in the |NCS|
############################

.. contents::
   :local:
   :depth: 2

There are several options for storing keys and other important data persistently when developing applications with the |NCS|, each with different features.
The implementation of the PSA Certified Secure Storage API can be provided using one of the following options:

* TF-M Platform Root of Trust (PRoT).
  This can only be used if the `ARM TrustZone`_ technology and hardware-accelerated firmware isolation are supported by the platform in use.
* The :ref:`trusted_storage_readme` library, which enables you to provide features like integrity, confidentiality, and authenticity of the stored data without using the TF-M Platform Root of Trust (PRoT).

The trusted storage library implements the PSA Certified Secure Storage API and is designed to securely store and manage sensitive data, such as cryptographic keys, device credentials, and configuration data.
The mechanism is essential for IoT devices, as it allows the implementation of secure communication between devices.

The PSA Certified Secure Storage API consists of PSA Internal Trusted Storage API and PSA Protected Storage API.

The Internal Trusted Storage and the Protected Storage are designed to work in :ref:`environments both with and without security by separation <app_boards_spe_nspe>`.
The two APIs used in the trusted storage library are also offered as secure services by TF-M.
While TF-M enables security by separation, building and isolating security-critical functions in SPE and applications in NSPE, you can use the trusted storage in environments with no TF-M and separation of firmware.

The table below gives an overview of the trusted storage support for the products and their features.

.. list-table:: Trusted storage product support
   :widths: auto
   :header-rows: 1

   * - Product
     - Backend
     - Confidentiality
     - Integrity
     - Authenticity
     - Isolation
   * - nRF91 Series with TF-M
     - TF-M secure storage service
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
     - TF-M secure storage service
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
     - TF-M secure storage service
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
.. [1] On systems without the isolation feature, the confidentiality is limited to protection of data at rest in a non-volatile internal or external memory.
       This partial confidentiality is based on a CPU-inaccessible master key used for data encryption.
       When the data is decrypted for usage, there is no mechanism providing access control and protecting its visibility.
       Use of a TrustZone-enabled system provides stronger protection, and is recommended if available.
.. [2] The use of Hardware Unique Key (HUK) to provide the AEAD key is not available on nRF52833 devices.
       The trusted storage library offers the use of SHA-256 to generate the key, which does not provide guarantees of security beyond the integrity check of the encrypted data.

The trusted storage library addresses two of the PSA Certified Level 2 and Level 3 optional security functional requirements (SFRs):

* Secure Encrypted Storage (internal storage)
* Secure Storage (internal storage)

The Secure External Storage SFR is not covered by the trusted storage library by default, but you can implement a custom storage backend.
