.. _ug_tfm_services:

TF-M Services
#############

.. contents::
   :local:
   :depth: 2

As explained in the :ref:`tfm_configurable_build` section, TF-M is built from a set of services that are isolated from each other.
Services can be enabled or disabled based on the application requirements.
Following sections describe the available TF-M services and their purpose.

.. _ug_tfm_services_platform:

Platform service
================

The platform service is the mandatory implementation of the :ref:`ug_tfm_architecture_rot_services_platform`.
It provides platform-specific services to other TF-M partitions and to a non-secure application.
To enable the platform service, set the :kconfig:option:`CONFIG_TFM_PARTITION_PLATFORM` Kconfig option.

For user applications :ref:`placed in the Non-Secure Processing Environment <app_boards_spe_nspe_cpuapp_ns>`, you can set the :kconfig:option:`CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING` Kconfig option, which enables more detailed error descriptions of faults from the application with the Zephyr fault handler.

The platform service also exposes the following |NCS| specific APIs for the non-secure application:

  .. code-block:: c

    /* Search for the fw_info structure in firmware image located at address. */
    int tfm_platform_firmware_info(uint32_t fw_address, struct fw_info *info);

    /* Check if S0 is the active B1 slot. */
    int tfm_platform_s0_active(uint32_t s0_address, uint32_t s1_address, bool *s0_active);

See :ref:`lib_tfm_ioctl_api` for more information about APIs available for the non-secure application.

For more information about the general features of the TF-M Platform partition, see `TF-M Platform`_.

.. _ug_tfm_services_system_reset:

System Reset service
--------------------

The System Reset service is one of the default TF-M platform services that has a specific implementation for the |NCS|.
It allows to perform a system reset through the TF-M platform service using the :c:func:`tfm_hal_system_reset` function.

This service is enabled when you enable the :kconfig:option:`CONFIG_TFM_PARTITION_PLATFORM` Kconfig option.

.. _ug_tfm_services_system_off:

System OFF service
------------------

The System OFF service is one of the TF-M platform services specific to the |NCS|.
It allows the non-secure application to request the system to enter the System OFF mode using a secure service call.

The System OFF mode is part of the power and clock management system and is available on selected Nordic Semiconductor devices, including the nRF54L Series.
For more details about the System OFF mode, see the device datasheet, for example the `nRF54L15 Power and clock management`_ page.

To enable the System OFF service in the |NCS|, enable the :kconfig:option:`CONFIG_TFM_NRF_SYSTEM_OFF_SERVICE` Kconfig option.

Zephyr's :zephyr:code-sample:`nrf_system_off` sample demonstrates how to use the System OFF service.

.. _ug_tfm_services_its:

Internal Trusted Storage service
================================

The Internal Trusted Storage (ITS) service is one of :ref:`ug_tfm_architecture_rot_services_platform`.
It implements the PSA Internal Trusted Storage APIs (`PSA Certified Secure Storage API 1.0`_) to achieve confidentiality, authenticity and encryption in rest (optional).

To enable the ITS service, set the :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` Kconfig option.

ITS is meant to be used by other TF-M partitions.
It must not be accessed directly by a user application :ref:`placed in the Non-Secure Processing Environment <app_boards_spe_nspe_cpuapp_ns>`.
If you want the user application to access the contents of the partition, use the :ref:`tfm_partition_ps`.

This service is enabled as the default storage mechanism when you enable the :ref:`tfm_partition_crypto` service.
If you are using a device with the :ref:`key_storage_kmu`, you can disable the :ref:`ug_tfm_services_its` and start using KMU instead to save memory.

For more information about the general features of the TF-M ITS service, see `TF-M ITS`_.

.. _tfm_encrypted_its:

Encrypted ITS
-------------

TF-M ITS encryption is a data protection mechanism in Internal Trusted Storage.
It provides transparent encryption using a Master Key Encryption Key (MKEK) stored in hardware, with unique encryption keys derived for each file.

.. note::
   |encrypted_its_not_supported_on_nrf54lm20|

To enable TF-M ITS encryption, set the :kconfig:option:`CONFIG_TFM_ITS_ENCRYPTED` Kconfig option.

On Nordic Semiconductor devices, the hardware-accelerated AEAD scheme ChaChaPoly1305 is used with a 256-bit key.
This key is derived with a key derivation function (KDF) based on NIST SP 800-108 CMAC.
The input key of the KDF is the MKEK, a symmetric key stored in the Key Management Unit (KMU) of Nordic Semiconductor devices.
The MKEK is protected by the KMU peripheral and its key material cannot be read by the software.
It can only be used by reference.

The file ID is used as a derivation label for the KDF.
This means that each file ID uses a different AEAD key.
As long as each file has a unique file ID, the key used for encryption and authentication is unique.

To strengthen data integrity, the metadata of the ITS file (creation flags or size, or both) is used as authenticated data in the encryption process.

The nonce for the AEAD operation is generated by concatenating a random 8-byte seed and an increasing 4-byte counter.
The random seed is generated once in the boot process and stays the same until reset.

.. _tfm_partition_its_sizing:

Sizing the Internal Trusted Storage
-----------------------------------

The RAM and flash usage of the ITS service are included in the ``tfm_secure`` partition.
The storage itself is a separate ``tfm_its`` partition.

When using the :ref:`partition_manager`, you can configure the size of the ``tfm_its`` with the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM_INTERNAL_TRUSTED_STORAGE` Kconfig option.
The resulting partition is visible in the :file:`partitions.yml` file in the build directory:

.. code-block:: console

    EMPTY_2:
      address: 0xea000
      end_address: 0xf0000
      placement:
        after:
        - tfm_its
      region: flash_primary
      size: 0x6000
    tfm_its:
      address: 0xe8000
      end_address: 0xea000
      inside:
      - tfm_storage
      placement:
        align:
          start: 0x8000
        before:
        - tfm_otp_nv_counters
      region: flash_primary
      size: 0x2000

The :ref:`partition_manager` can only align the start address of the ``tfm_its`` partition with the flash region size (see :ref:`ug_tfm_partition_alignment_requirements`).
If the size of the ``tfm_its`` does not equal the flash region size, the Partition Manager allocates an additional empty partition to fill the gap.
See the :ref:`tfm_ps_static_partition` for an example on how to optimize the size of the ``tfm_its`` partition by manual configuration.

TF-M does not guarantee in build time that the ``tfm_its`` partition can hold the assets that are configured with the :kconfig:option:`CONFIG_TFM_ITS_NUM_ASSETS` and :kconfig:option:`CONFIG_TFM_ITS_MAX_ASSET_SIZE` options.
Depending on the available flash size, the ITS can use one or two flash pages (4 KB) for ensuring power failure safe operations.
In addition, ITS stores the bookkeeping information for the assets in the flash memory and the bookkeeping size scales with the configured number of assets.
This can leave a very small amount of space for the actual assets.

It is recommended to test the ITS with the intended assets to ensure that the assets fit in the available space.

.. _tfm_partition_crypto:

Crypto service
==============

The :ref:`TF-M Crypto Service <ug_crypto_architecture_implementation_standards_tfm>` is one of :ref:`ug_tfm_architecture_rot_services_platform`.
It implements the PSA Crypto APIs (`PSA Certified Crypto API`_) and provides cryptographic services to other TF-M partitions and to the non-secure application.

To enable the crypto service, set the :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO` Kconfig option.

You can configure the service directly using the ``CONFIG_TFM_CRYPTO_*`` Kconfig options found in the :file:`zephyr/modules/trusted-firmware-m/Kconfig.tfm.crypto_modules` file.
However, it is recommended to use the ``CONFIG_PSA_WANT_*`` Kconfig options to enable the required algorithms and key types.
These will enable the required ``CONFIG_TFM_CRYPTO_*`` Kconfig options.

TF-M uses :ref:`hardware unique keys <lib_hw_unique_key>` when the PSA Crypto key derivation APIs are used, and ``psa_key_derivation_setup`` is called with the algorithm ``TFM_CRYPTO_ALG_HUK_DERIVATION``.

When enabled, the Crypto service by default uses the :ref:`ug_tfm_services_its` to store the keys and other sensitive data.
If you are using a device with the :ref:`key_storage_kmu`, you can disable the :ref:`ug_tfm_services_its` and start using KMU instead to save memory.

For more information about the general features of the Crypto partition, see `TF-M Crypto`_.

.. _tfm_partition_ps:

Protected Storage service
=========================

The Protected Storage (PS) service is one of possible :ref:`ug_tfm_architecture_rot_services_application`.
It implements the PSA Protected Storage APIs (`PSA Certified Secure Storage API 1.0`_).

To enable the PS service, set the :kconfig:option:`CONFIG_TFM_PARTITION_PROTECTED_STORAGE` Kconfig option.

The PS service uses the ITS service to achieve confidentiality and authenticity.
In addition, it provides encryption, authentication, and rollback protection.

A user application :ref:`placed in the Non-Secure Processing Environment <app_boards_spe_nspe_cpuapp_ns>` should use the PS partition for storing sensitive data.

For more information about the general features of the TF-M PS service, see `TF-M PS`_.

Sizing the Protected Storage partition
--------------------------------------

The RAM and flash usage of the PS service are included in the ``tfm_secure`` partition.
The storage itself is a separate ``tfm_ps`` partition.
Additionally, the PS partition requires non-volatile counters for rollback protection.
Those are stored in the ``tfm_otp_nv_counters`` partition.

When using the :ref:`partition_manager`, the size of the ``tfm_ps`` is configured with the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM_PROTECTED_STORAGE` Kconfig option.
The size of the ``tfm_otp_nv_counters`` is configured with the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM_OTP_NV_COUNTERS` Kconfig option.

Resulting partitions are visible in the :file:`partitions.yml` file in the build directory:

.. code-block:: console

    EMPTY_0:
      address: 0xfc000
      end_address: 0x100000
      placement:
        after:
        - tfm_ps
      region: flash_primary
      size: 0x4000
    EMPTY_1:
      address: 0xf2000
      end_address: 0xf8000
      placement:
        after:
        - tfm_otp_nv_counters
      region: flash_primary
      size: 0x6000
    tfm_otp_nv_counters:
      address: 0xf0000
      end_address: 0xf2000
      inside:
      - tfm_storage
      placement:
        align:
          start: 0x8000
        before:
        - tfm_ps
      region: flash_primary
      size: 0x2000
    tfm_ps:
      address: 0xf8000
      end_address: 0xfc000
      inside:
      - tfm_storage
      placement:
        align:
          start: 0x8000
        before:
        - end
      region: flash_primary
      size: 0x4000

Similarly to :ref:`tfm_partition_its_sizing`, the :ref:`partition_manager` can only align the start addresses of the partitions with the flash region size.
The Partition Manager allocates an additional empty partition to fill the gaps.

See :ref:`tfm_ps_static_partition` for an example on how to optimize the size of the partitions by manual configuration.

TF-M does not guarantee in build time that the ``tfm_ps`` partition can hold the assets that are configured with the :kconfig:option:`CONFIG_TFM_PS_NUM_ASSETS` and :kconfig:option:`CONFIG_TFM_PS_MAX_ASSET_SIZE` options.
The PS partition uses the ITS internally to store the assets in ``tfm_ps``.
This means that some of the flash space is reserved for the ITS functionality.
Additionally, the PS service stores the file metadata in object tables, which also consumes flash space.
The size of the object table scales with the number of configured assets and two object tables (old and new) are required when performing PS operations.
This might leave a very small amount of space for the actual assets.

It is highly recommended to test the PS with the intended assets to ensure that the assets fit in the available space.

.. _tfm_ps_static_partition:

Example of PS sizing with static partitions
-------------------------------------------

With devices where ROM granularity is higher than the flash page size (nRF53 Series and nRF91 Series), it might be useful to configure the ``tfm_its``, ``tfm_ps`` and ``tfm_otp_nv_counters`` partitions as static partitions.
For example, when these three partitions are combined into a single ``tfm_storage`` partition, only the ``tfm_storage`` partition needs to be aligned with the flash region size.
This allows potential optimizations in the flash memory usage.

You can start by copying the default configuration from the :file:`partitions.yml` file in the build directory as the :file:`pm_static.yml` file in the application directory.
The following snippet shows the meaningful parts of the default configuration for the ``tfm_its``, ``tfm_ps`` and ``tfm_otp_nv_counters`` partitions in the nRF9151 SoC:

.. code-block:: console

    EMPTY_0:
      address: 0xfc000
      end_address: 0x100000
      placement:
        after:
        - tfm_ps
      region: flash_primary
      size: 0x4000
    EMPTY_1:
      address: 0xf2000
      end_address: 0xf8000
      placement:
        after:
        - tfm_otp_nv_counters
      region: flash_primary
      size: 0x6000
    EMPTY_2:
      address: 0xea000
      end_address: 0xf0000
      placement:
        after:
        - tfm_its
      region: flash_primary
      size: 0x6000
    app:
      address: 0x40000
      end_address: 0xe8000
      region: flash_primary
      size: 0xa8000
    tfm_nonsecure:
      address: 0x40000
      end_address: 0xe8000
      orig_span: &id004
      - app
      region: flash_primary
      size: 0xa8000
      span: *id004
    tfm_its:
      address: 0xe8000
      end_address: 0xea000
      inside:
      - tfm_storage
      placement:
        align:
          start: 0x8000
        before:
        - tfm_otp_nv_counters
      region: flash_primary
      size: 0x2000
    tfm_otp_nv_counters:
      address: 0xf0000
      end_address: 0xf2000
      inside:
      - tfm_storage
      placement:
        align:
          start: 0x8000
        before:
        - tfm_ps
      region: flash_primary
      size: 0x2000
    tfm_ps:
      address: 0xf8000
      end_address: 0xfc000
      inside:
      - tfm_storage
      placement:
        align:
          start: 0x8000
        before:
        - end
      region: flash_primary
      size: 0x4000
    tfm_storage:
      address: 0xe8000
      end_address: 0xfc000
      orig_span: &id006
      - tfm_ps
      - tfm_its
      - tfm_otp_nv_counters
      region: flash_primary
      size: 0x14000
      span: *id006

The ``tfm_storage`` partition that holds the ``tfm_its``, ``tfm_ps`` and ``tfm_otp_nv_counters`` partitions must be aligned with the flash region size, so that you can configure it as secure.
After removing the empty partitions, unnecessary alignments and adjusting the sizes of the partitions, the same information in the :file:`pm_static.yml` file should look like this:

.. code-block:: console

    app:
      address: 0x40000
      end_address: 0xf8000
      region: flash_primary
      size: 0xb8000
    tfm_nonsecure:
      address: 0x40000
      end_address: 0xf8000
      orig_span: &id004
      - app
      region: flash_primary
      size: 0xb8000
      span: *id004
    tfm_its:
      address: 0xf8000
      end_address: 0xfa000
      inside:
      - tfm_storage
      placement:
        before:
        - tfm_otp_nv_counters
      region: flash_primary
      size: 0x2000
    tfm_otp_nv_counters:
      address: 0xfa000
      end_address: 0xfc000
      inside:
      - tfm_storage
      placement:
        before:
        - tfm_ps
      region: flash_primary
      size: 0x2000
    tfm_storage:
      address: 0xf8000
      end_address: 0x100000
      orig_span: &id006
      - tfm_ps
      - tfm_its
      - tfm_otp_nv_counters
      region: flash_primary
      size: 0x8000
      span: *id006

The ``tfm_storage`` partition is still aligned with the flash region size and the ``tfm_its``, ``tfm_ps`` and ``tfm_otp_nv_counters`` partitions are placed inside it.
The available space for the non-secure application has increased by 0x10000 bytes.

.. note::

   For devices that are intended for production and meant to be updated in the field, you should always use static partitions to ensure that the partitions are not moved around in the flash memory.

.. _ug_tfm_services_initial_attestation:

Initial Attestation service
===========================

The Initial Attestation service implements the PSA Initial Attestation APIs (`PSA Certified Attestation API 1.0`_).
The service allows the device to prove its identity to a remote entity.

To enable the Initial Attestation service, set the :kconfig:option:`CONFIG_TFM_PARTITION_INITIAL_ATTESTATION` Kconfig option.

The :ref:`tfm_psa_template` sample demonstrates how to use the Initial Attestation service.

The Initial Attestation service is not enabled by default.
Keep it disabled unless you need attestation.

For more information about the general features of the TF-M Initial Attestation service, see `TF-M Attestation`_.
