.. _security:

Security
########

.. contents::
   :local:
   :depth: 2

The following sections give a brief introduction to core security features available in Nordic Semiconductor products.
The features are made available either as built-ins in modules, drivers, and subsystems, or are shown in samples or applications in |NCS|.

Secure boot
***********

The |NCS| supports secure boot of application images enforced by the bootloader.
The secure boot utilizes signature validation and security hardware features to establish a root-of-trust during the boot process, and to ensure the validity of the firmware that is booted.

There are two available bootloaders in |NCS|:

* :ref:`nRF Secure Immutable Bootloader (NSIB) <bootloader>`, enabled by the configuration option :kconfig:option:`CONFIG_SECURE_BOOT`.
* A customized version of `sdk-mcuboot`_, enabled by the configuration option :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`.

When enabling the NSIB, the MCUboot can serve as an upgradable second-stage bootloader.
For more information about the bootloaders, see :ref:`app_bootloaders`.

Trusted Firmware-M
******************

The Trusted Firmware-M project (TF-M) is a reference design of the Arm `Platform Security Architecture (PSA)`_.
Through TF-M, |NCS| utilizes the security features of the Arm TrustZone technology to configure secure peripherals and memory, and to provide PSA functional APIs as secure services.

TF-M enables hardware supported separation of a Secure Processing Environment (SPE) and a Non-Secure Processing Environment (NSPE) that constitutes the Zephyr RTOS, protocol stacks, and the application.
Enable TF-M in a project by enabling the :kconfig:option:`CONFIG_BUILD_WITH_TFM` option.

For more information about the TF-M, see the `TF-M documentation`_ and :ref:`ug_tfm`.
For more information about SPE and NSPE in the |NCS|, see :ref:`app_boards_spe_nspe`.

Hardware unique key
*******************

Nordic Semiconductor devices featuring the CryptoCell cryptographic accelerator allow the usage of a hardware unique key (HUK) for key derivation.
A HUK is a unique symmetric cryptographic key which is loaded in special hardware registers allowing the application to use the key by reference, without any access to the key material.
To enable the HUK in an application, enable the :kconfig:option:`CONFIG_HW_UNIQUE_KEY` option.

For more information, see the hardware unique key :ref:`library <lib_hw_unique_key>` and :ref:`sample <hw_unique_key_usage>`.

.. _trusted_storage_in_ncs:

Trusted storage in the |NCS|
****************************

There are several options for storing keys and other important data persistently when developing applications with the |NCS|.
Different storage options have different features.
One of the options is to use the :ref:`trusted_storage_readme` library.

The trusted storage library enables the users to provide features like integrity, confidentiality and authenticity of the stored data, without using the TF-M Platform Root of Trust (PRoT).
The library implements the PSA Certified Secure Storage API.
It consists of PSA Internal Trusted Storage API and PSA Protected Storage API.

The Internal Trusted Storage and the Protected Storage are designed to work in :ref:`environments both with and without security by separation <app_boards_spe_nspe>`.
The two APIs used in the trusted storage library are also offered as secure services by TF-M.
While TF-M enables security by separation, building and isolating security-critical functions in SPE and applications in NSPE, the trusted storage can be used in environments with no TF-M and separation of firmware.

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

The Secure External Storage SFR is not covered by the trusted storage library by default, but this can be realized by implementing a custom storage backend.

Device firmware upgrade (DFU)
*****************************

The |NCS| supports firmware upgrade using over-the-air (OTA) and serial firmware upgrades, depending on the capabilities of the device.
For more information about the firmware upgrades, see the available :ref:`DFU libraries <lib_dfu>`.

The |NCS| can be configured to enforce secure DFU mechanisms, including validating the digital signature of an image and checking for the version to prevent downgrade attacks.
The secure DFU mechanisms are handled by the MCUboot bootloader.
For more information, see the :doc:`MCUboot documentation <mcuboot:design>`.

.. _cryptographic_operations_in_ncs:

Cryptographic operations in |NCS|
*********************************

Cryptographic operations in |NCS| are handled by the :ref:`nrf_security`, which is configurable through Kconfig options.
The module can be enabled through the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option, and it allows the usage of `Mbed TLS`_ and `PSA Cryptography API 1.0.1`_ for cryptographic operations and random number generation in the application.

The :ref:`nrf_security` acts as an orchestrator for the different cryptographic libraries available in the system.
These libraries include the binary versions of accelerated cryptographic libraries listed in :ref:`nrfxlib:crypto`, and the open source Mbed TLS implementation in |NCS| located in `sdk-mbedtls`_.

HW accelerated libraries are prioritized over SW libraries when both are enabled.
For more information about the configuration and usage of the :ref:`nrf_security`, see the :ref:`nrf_security_config` page.

See also :ref:`crypto_samples` for examples of how to configure and perform different cryptographic operations in the |NCS|.

Access port protection mechanism
********************************

.. include:: ap_protect.rst
   :start-after: app_approtect_info_start
   :end-before: app_approtect_info_end

For more information, see :ref:`app_approtect`.
