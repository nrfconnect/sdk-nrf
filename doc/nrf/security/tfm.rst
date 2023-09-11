.. _ug_tfm:

Running applications with Trusted Firmware-M
############################################

.. contents::
   :local:
   :depth: 2

On nRF5340 and nRF9160, Trusted Firmware-M (TF-M) is used to configure and boot an application as non-secure.

Overview
********

TF-M is the reference implementation of `Platform Security Architecture (PSA)`_.

It provides a highly configurable set of software components to create a Trusted Execution Environment.
This is achieved by a set of secure run time services such as Secure Storage, Cryptography, Audit Logs, and Attestation.
Additionally, secure boot through MCUboot in TF-M ensures integrity of runtime software and supports firmware upgrade.

.. note::
   Support for TF-M with :ref:`minimal version <tfm_minimal_build>` disabled in |NCS| is currently :ref:`experimental <software_maturity>`.

For official documentation, see the `TF-M documentation`_.

The TF-M implementation in |NCS| is currently demonstrated in the following samples:

- All :ref:`tfm_samples` in this SDK
- All :ref:`cryptography samples <crypto_samples>` in this SDK
- A series of :ref:`TF-M integration samples <zephyr:tfm_integration-samples>` available in Zephyr
- The :ref:`https_client` sample for nRF9160 in this SDK
- The :ref:`openthread_samples` that support the ``nrf5340dk_nrf5340_cpuapp_ns`` build target in this SDK

Building
********

TF-M is one of the images that are built as part of a multi-image application.
For more information about multi-image builds, see :ref:`ug_multi_image`.

To add TF-M to your build, enable the :kconfig:option:`CONFIG_BUILD_WITH_TFM` configuration option by adding it to your :file:`prj.conf` file.

.. note::
   If you use menuconfig to enable :kconfig:option:`CONFIG_BUILD_WITH_TFM`, you must also enable its dependencies.

By default, TF-M is configured to build the :ref:`minimal version <tfm_minimal_build>`.
To use the full TF-M, you must disable the :kconfig:option:`CONFIG_TFM_MINIMAL` option.

You must build TF-M using a non-secure build target.
The following platforms are currently supported:

* nRF5340
* nRF9160

TF-M uses UART1 for logging from the secure application.
To disable logging, enable the :kconfig:option:`TFM_LOG_LEVEL_SILENCE` option.
When building TF-M with logging enabled, UART1 must be disabled in the non-secure application, otherwise the non-secure application will fail to run.
The recommended way to do this is to copy the .overlay file from the :ref:`tfm_hello_world` sample.

Enabling secure services
========================

When using the :ref:`nrf_security`, if :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled together with :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND`, the TF-M secure image will enable the use of the hardware acceleration of Arm CryptoCell.
In such case, the Kconfig configurations in the Nordic Security Backend control the features enabled through TF-M.

You can configure what crypto modules to include in TF-M by using the ``TFM_CRYPTO_`` Kconfig options found in file :file:`zephyr/modules/trusted-firmware-m/Kconfig.tfm.crypto_modules`.

TF-M utilizes :ref:`hardware unique keys <lib_hw_unique_key>` when the PSA Crypto key derivation APIs are used, and ``psa_key_derivation_setup`` is called with the algorithm ``TFM_CRYPTO_ALG_HUK_DERIVATION``.
For more information about the PSA cryptography and the API, see `PSA Cryptography API 1.0.1`_.

.. _tfm_minimal_build:

Minimal build
=============

The default configuration of TF-M has all supported features enabled, which results in a significant memory footprint.
A minimal version of the TF-M secure application is provided in |NCS| to show how to configure a reduced version of TF-M.

The secure services supported by this minimal version allow for generating random numbers, and the platform services.

The minimal version of TF-M is disabled by setting the :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` option or one of the other build profiles.

When :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` is set, the configurability of TF-M is severely limited.
Hence, it is not possible to modify the TF-M minimal configuration to create your own variant of the minimal configuration.
Instead, the default configuration must be used as a starting point.


.. _tfm_encrypted_its:

Encrypted ITS
=============

TF-M implements a PSA internal trusted storage (ITS) with encryption and authentication.
For more information about the general features of the TF-M ITS service, see `TF-M ITS`_.

To enable TF-M ITS encryption, use the Kconfig option :kconfig:option:`CONFIG_TFM_ITS_ENCRYPTED`.
The ITS encryption is transparent to the user as long as the Master Key Encryption Key (MKEK) is populated before use.

On Nordic Semiconductor devices, the hardware-accelerated AEAD scheme ChaChaPoly1305 is used with a 256 bits key.
This key is derived with a key derivation function (KDF) based on NIST SP 800-108 CMAC.
The input key of the KDF is the MKEK, a symmetric key stored in the Key Management Unit (KMU) of Nordic Semiconductor devices.
The MKEK is protected by the KMU peripheral and its key material cannot be read by software. It can only be used by reference.

The file ID is used as a derivation label for the KDF.
This means that each file ID uses a different AEAD key.
As long as each file has a unique file ID, the key used for encryption and authentication is unique.

To strengthen data integrity, the metadata of the ITS file (creation flags/size) is used as authenticated data in the encryption process.

The nonce for the AEAD operation is generated by concatenating a random 8-byte seed and an increasing 4-byte counter.
The random seed is generated once in the boot process and stays the same until reset.

Logging
*******

TF-M employs two UART interfaces for logging: one for the secure part (MCUboot and TF-M), and one for the non-secure application.
The logs arrive on different COM ports on the host PC.


Virtual COM ports on the nRF5340 DK
===================================

On the nRF5340 DK v1.0.0, you must connect specific wires on the kit to receive secure logs on the host PC.
Specifically, wire the pins **P0.25** and **P0.26** of the **P2** connector to **RxD** and **TxD** of the **P24** connector respectively.
See :ref:`logging_cpunet` on the Working with nRF5340 DK page for more information.

On the nRF5340 DK v2.0.0, there are only two virtual COM ports available.
By default, one of the ports is used by the non-secure UART0 peripheral from the application and the other by the UART1 peripheral from the network core.

There are several options to get UART output from the secure TF-M:

* Disable the output for the network core and change the pins used by TF-M.
  The network core will usually have an |NCS| child image.
  To configure a child image, see Configuration of the child image section described in :ref:`ug_nrf5340_multi_image`.
  To configure logging in an |NCS| image, see :ref:`ug_logging`.
  To change the pins used by TF-M, the RXD (:kconfig:option:`CONFIG_TFM_UART1_RXD_PIN`) and TXD (:kconfig:option:`CONFIG_TFM_UART1_TXD_PIN`) Kconfig options in the application image can be set to **P1.00** (32) and **P1.01** (33).

* The secure and non-secure UART peripherals can be wired to the same pins.
  Specifically, physically wire together the pins **P0.25** and **P0.26** to **P0.20** and **P0.22**, respectively.

* If the non-secure application, network core and TF-M outputs are all needed simultaneously, additional UART <-> USB hardware is needed.
  A second nRF DK can be used if available.
  Pin **P0.25** needs to be wired to the TXD pin, and **P0.26** to the RXD pin of the external hardware.
  These pins will provide the secure TF-M output, while the two native COM ports of the DK will be used for the non-secure application and the network core output.

Limitations
***********

The following limitations apply to TF-M and its usage:

* Firmware Update service is not supported.
* The following crypto modules or ciphers are not supported:

  * AES output feedback (AES-OFB) mode.
  * AES cipher feedback (AES-CFB) mode.

* Isolation level 3 is not supported.
* In Isolation level 2 or higher, the number of peripherals configured as secure in Application Root of Trust (ARoT) is limited by the number of available MPU regions.
* Nordic Semiconductor devices only support the GCC toolchain for building TF-M.

.. _ug_tfm_partition_alignment_requirements:

TF-M partition alignment requirements
*************************************

TF-M requires that secure and non-secure partition addresses must be aligned to the NRF_SPU flash region size :kconfig:option:`CONFIG_NRF_SPU_FLASH_REGION_SIZE`.
|NCS| ensures that they in fact are aligned and comply with the TF-M requirements.

TF-M requires this alignment because it uses the SPU to enforce the security policy between the partitions.
When the :ref:`partition_manager` is enabled, it will take into consideration the alignment requirements.
But when the static partitions are used, the user is responsible for following the alignment requirements.

If you are experiencing any partition alignment issues when using the Partition Manager, check the :ref:`known_issues` page on the main branch.

The partitions which need to be aligned to the SPU flash region size are partitions ``tfm_nonsecure`` and ``nonsecure_storage``.
Both the partition start address and the partition size need to be aligned with the NRF_SPU flash region size :kconfig:option:`CONFIG_NRF_SPU_FLASH_REGION_SIZE`.

Note that the ``tfm_nonsecure`` partition is placed after the ``tfm_secure`` partition, thus the end address of the ``tfm_secure`` partition is the same as the start address of the ``tfm_nonsecure`` partition.
As a result, altering the size of the ``tfm_secure`` partition affects the start address of the ``tfm_nonsecure`` partition.

The following static partition snippet shows a non-aligned configuration for nRF5340 which has a SPU flash region size :kconfig:option:`CONFIG_NRF_SPU_FLASH_REGION_SIZE` of 0x4000.

.. code-block:: console

    tfm_secure:
      address: 0x4000
      size: 0x4200
      span: [mcuboot_pad, tfm]
    mcuboot_pad:
      address: 0x4000
      size: 0x200
    tfm:
      address: 0x4200
      size: 0x4000
    tfm_nonsecure:
      address: 0x8200
      size: 0x4000
      span: [app]
    app:
      address: 0x8200
      size: 0x4000

In the above example, the ``tfm_nonsecure`` partition starts at address 0x8200, which is not aligned with the SPU requirement of 0x4000.
Since ``tfm_secure`` spans the ``mcuboot_pad`` and ``tfm`` partitions we can decrease the size of any of them by 0x200 to fix the alignment issue.
We will decrease the size of the (optional) ``mcuboot_pad`` partition and thus the size of the ``tfm_secure`` partition as follows:

.. code-block:: console

    tfm_secure:
      address: 0x4000
      size: 0x4000
      span: [mcuboot_pad, tfm]
    mcuboot_pad:
      address: 0x4000
      size: 0x0
    tfm:
      address: 0x4000
      size: 0x4000
    tfm_nonsecure:
      address: 0x8000
      size: 0x4000
      span: [app]
    app:
      address: 0x8000
      size: 0x4000



.. _ug_tfm_migrate:

Migrating from Secure Partition Manager to Trusted Firmware-M
*************************************************************

The interface to TF-M is different from the interface to SPM.
Due to that, the application code that uses the SPM Secure Services needs to be ported to use TF-M instead.

TF-M can replace the following SPM services:

* ``spm_request_system_reboot`` with ``tfm_platform_system_reset``.
* ``spm_request_random_number`` with ``psa_generate_random`` or ``entropy_get_entropy``.
* ``spm_request_read`` with ``tfm_platform_mem_read`` or ``soc_secure_mem_read``.
* ``spm_s0_active`` with ``tfm_platform_s0_active``.
* ``spm_firmware_info`` with ``tfm_firmware_info``.

The following SPM services have no replacement in TF-M:

* ``spm_prevalidate_b1_upgrade``
* ``spm_busy_wait``
* ``spm_set_ns_fatal_error_handler``

.. note::
   By default, TF-M configures memory regions as secure memory, while SPM configures memory regions as non-secure.
   The partitions ``tfm_nonsecure``, ``mcuboot_secondary``, and ``nonsecure_storage`` are configured as non-secure flash memory regions.
   The partition ``sram_nonsecure`` is configured as a non-secure RAM region.

If a static partition file is used for the application, make the following changes:

* Rename the ``spm`` partition to ``tfm``.
* Add a partition called ``tfm_secure`` that spans ``mcuboot_pad`` (if MCUboot is enabled) and ``tfm`` partitions.
* Add a partition called ``tfm_nonsecure`` that spans the application, and other possible application partitions that must be non-secure.
* For non-secure storage partitions, place the partitions inside the ``nonsecure_storage`` partition.
