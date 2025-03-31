.. _ug_tfm:

Configuring applications for Trusted Firmware-M
###############################################

.. contents::
   :local:
   :depth: 2

On nRF5340, nRF54L15 and nRF91 Series devices, Trusted Firmware-M (TF-M) is used to configure and boot an application as non-secure.

.. _ug_tfm_building:

Building with TF-M
******************

TF-M is one of the images that are built as part of a multi-image application.

To add TF-M to your build, enable the :kconfig:option:`CONFIG_BUILD_WITH_TFM` configuration option by adding it to your :file:`prj.conf` file.

.. note::
   If you use menuconfig to enable :kconfig:option:`CONFIG_BUILD_WITH_TFM`, you must also enable its dependencies.

By default, TF-M is configured to build the :ref:`minimal version <tfm_minimal_build>`.
To use the full TF-M, you must disable the :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` option.

You must build TF-M using a non-secure board target.
The following platforms are currently supported:

* nRF54L15
* nRF5340
* nRF91 Series

TF-M uses UART1 for logging from the secure application.
To disable logging, enable the :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` option.
When building TF-M with logging enabled, UART1 must be disabled in the non-secure application, otherwise the non-secure application will fail to run.
The recommended way to do this is to copy the .overlay file from the :ref:`tfm_hello_world` sample.

Enabling secure services
========================

When using the :ref:`nrf_security`, if :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled together with :kconfig:option:`CONFIG_NORDIC_SECURITY_BACKEND`, the TF-M secure image will enable the use of the hardware acceleration of Arm CryptoCell.
In such case, the Kconfig configurations in the Nordic Security Backend control the features enabled through TF-M.

See :ref:`tfm_partition_crypto` for more information about the TF-M Crypto partition.

.. _tfm_minimal_build:

Minimal build
=============

.. include:: tfm_supported_services.rst
   :start-after: minimal_build_overview_start
   :end-before: minimal_build_overview_end

The minimal build uses an image of 32 kB.
It is set with the :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` Kconfig option that is enabled by default on the nRF53 and nRF91 Series devices.

With the minimal build, the configuration of TF-M is severely limited.
Hence, it is not possible to modify the TF-M minimal configuration to create your own variant of the minimal configuration.
Instead, the default configuration must be used as a starting point.

.. _tfm_configurable_build:

Configurable build
==================

.. include:: tfm_supported_services.rst
   :start-after: configurable_build_overview_start
   :end-before: configurable_build_overview_end

To enable the configurable, full TF-M build, make sure the following Kconfig options are configured:

* :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled
* :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` is enabled
* :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` is disabled

For description of the build profiles, see :ref:`tf-m_profiles`.
It is not recommended to use predefined TF-M profiles as they might result in a larger memory footprint than necessary.

When the :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_NOT_SET` Kconfig option is enabled, the build process will not set a specific TF-M profile type.
This allows for a more flexible configuration where individual TF-M features can be enabled or disabled as needed.
It also provides more control over the build process and allows for a more fine-grained configuration of the TF-M secure image.

To configure the features of the TF-M secure image, you must choose which TF-M partitions and which secure services to include in the build.

.. note::
     A "TF-M partition" in this context refers to a secure partition within the Trusted Firmware-M architecture.
     These partitions are isolated from each other and from the non-secure application code.
     A service running inside TF-M would typically be implemented within one of these secure partitions.

Each service can be a separate partition, or multiple related services might be grouped into a single partition.
The partition provides the execution environment for the service.
It handles secure function calls and ensures that the service's code and data are protected from unauthorized access.

Following are the available Kconfig options for TF-M partitions:

.. list-table:: Available TF-M Partitions
   :header-rows: 1

   * - Option name
     - Description
     - Default value
     - Dependencies
   * - :kconfig:option:`CONFIG_TFM_PARTITION_PLATFORM`
     - Provides platform services.
     - Enabled
     -
   * - :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO`
     - Provides cryptographic services.
     - Enabled
     - INTERNAL_TRUSTED_STORAGE
   * - :kconfig:option:`CONFIG_TFM_PARTITION_PROTECTED_STORAGE`
     - Provides secure storage services.
     - Enabled
     - PLATFORM, CRYPTO
   * - :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE`
     - Provides internal trusted storage services.
     - Enabled
     -
   * - :kconfig:option:`CONFIG_TFM_PARTITION_INITIAL_ATTESTATION`
     - Provides initial attestation services.
     - Disabled
     - CRYPTO

Secure Partition Manager backend configuration
----------------------------------------------

TF-M's Secure Partition Manager (SPM) backend may also be configured, depending on the isolation requirements of the application.

.. note::
    Do not confuse TF-M's Secure Partition Manager with Secure Partition Manager that was removed in the |NCS| v2.1.0.
    See also `Migrating from Secure Partition Manager to Trusted Firmware-M`_ in the |NCS| v2.0.0 documentation.

.. list-table:: SPM backends
   :header-rows: 1

   * - Option
     - Description
     - Allowed isolation levels
   * - :kconfig:option:`CONFIG_TFM_SFN`
     - With SFN, the Secure Partition is made up of a collection of callback functions that implement secure services.
     - Level 1
   * - :kconfig:option:`CONFIG_TFM_IPC`
     - With IPC, each Secure Partition processes signals in any order, and can defer responding to a message while continuing to process other signals.
     - Levels 1, 2 and 3

To control the number of logging messages, set the :kconfig:option:`CONFIG_TFM_LOG_LEVEL` Kconfig option.
To disable logging, set the :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` option.

The size of TF-M partitions is affected by multiple configuration options and hardware-related options.
The code and memory size of TF-M increases when more services are enabled, but the selected hardware also places limitations on how the separation of secure and non-secure is made.

TF-M is linked as a separate partition in the final binary image.
The reserved sizes of its RAM and flash partitions are configured by the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM` and :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM_SRAM` options.
These configuration options allow you to specify the size allocated for the TF-M partition in the final binary image.
Default partition sizes vary between device families and are not optimized to any specific use case.

To optimize the TF-M size, find the minimal set of features to satisfy the application needs and then minimize the allocated partition sizes while still conforming to the alignment and granularity requirements of given hardware.

.. _ug_tfm_partition_alignment_requirements:

TF-M partition alignment requirements
=====================================

TF-M requires that secure and non-secure partition addresses must be aligned to the flash region size :kconfig:option:`CONFIG_NRF_TRUSTZONE_FLASH_REGION_SIZE`.
The |NCS| ensures that they in fact are aligned and comply with the TF-M requirements.

The following differences apply to the device families:

* On nRF53 and nRF91 Series devices, TF-M uses the SPU to enforce the security policy between the partitions, so the :kconfig:option:`CONFIG_NRF_TRUSTZONE_FLASH_REGION_SIZE` Kconfig option is set to the SPU flash region size.
* On nRF54L15 devices, TF-M uses the MPC to enforce the security policy between the partitions, so the :kconfig:option:`CONFIG_NRF_TRUSTZONE_FLASH_REGION_SIZE` option is set to the MPC region size.

.. list-table:: Region limits on different hardware
   :header-rows: 1

   * - Family
     - RAM granularity
     - ROM granularity
   * - nRF91 Series
     - 8 kB
     - 32 kB
   * - nRF53 Series
     - 8 kB
     - 16 kB
   * - nRF54 Series
     - 4 kB
     - 4 kB

.. figure:: /images/nrf-secure-rom-granularity.svg
   :alt: Partition alignment granularity
   :width: 60em
   :align: left

   Partition alignment granularity on different nRF devices

The imaginary example above shows a worst-case scenario in the nRF91 Series where the flash region size is 32 kB and both the TF-M binary and secure storage are 12 kB.
This leaves a significant amount of unused space in the flash region.
In a real-world scenario, the size of the TF-M binary and secure storage is usually much larger.

When the :ref:`partition_manager` is enabled, it will take into consideration the alignment requirements.
But when the static partitions are used, the user is responsible for following the alignment requirements.

.. figure:: /images/secure-flash-regions.svg
   :alt: Example of aligning partitions with flash regions
   :width: 60em
   :align: left

   Example of aligning partitions with flash regions

.. note::
   If you are experiencing any partition alignment issues when using the Partition Manager, check the `known issues page on the main branch`_.

You need to align the ``tfm_nonsecure``, ``tfm_storage``, and ``nonsecure_storage`` partitions with the TrustZone flash region size.
Both the partition start address and the partition size need to be aligned with the flash region size :kconfig:option:`CONFIG_NRF_TRUSTZONE_FLASH_REGION_SIZE`.

.. note::
   The ``tfm_nonsecure`` partition is placed after the ``tfm_secure`` partition, thus the end address of the ``tfm_secure`` partition is the same as the start address of the ``tfm_nonsecure`` partition.
   As a result, altering the size of the ``tfm_secure`` partition affects the start address of the ``tfm_nonsecure`` partition.

The following static partition snippet shows a non-aligned configuration for nRF5340 which has a TrustZone flash region size :kconfig:option:`CONFIG_NRF_TRUSTZONE_FLASH_REGION_SIZE` of 0x4000.

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

In the above example, the ``tfm_nonsecure`` partition starts at address 0x8200, which is not aligned with the requirement of 0x4000.
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

Analyzing ``tfm_secure`` partition size
=======================================

You can analyze the size of the ``tfm_secure`` partition from the build output:

.. code-block:: console

   [71/75] Linking C executable bin/tfm_s.axf
   Memory region   Used Size  Region Size  %age Used
      FLASH:       31972 B       256 KB     12.20%
      RAM:         4804 B        88 KB      5.33%

The example shows that the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM` Kconfig option for the ``tfm_secure`` partition is set to 256 kB and the TF-M binary uses 32 kB of the available space.
Similarly, the :kconfig:option:`CONFIG_PM_PARTITION_SIZE_TFM_SRAM` option for the ``tfm_secure`` partition is set to 88 kB and the TF-M binary uses 5 kB of the available space.
You can use this information to optimize the size of the TF-M, as long as it is within the alignment requirements explained in the previous section.

Tools for analyzing the ``tfm_secure`` partition size
-----------------------------------------------------

The TF-M build system is compatible with Zephyr's :ref:`zephyr:footprint_tools` tools that let you generate RAM and ROM usage reports (using :ref:`zephyr:sysbuild_dedicated_image_build_targets`).
You can use the reports to analyze the memory usage of the different TF-M partitions and see how changing the Kconfig options affects the memory usage.

Depending on your development environment, you can generate memory reports for TF-M in the following ways:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      You can use the `Memory report`_ feature in the |nRFVSC| to check the size and percentage of memory that each symbol uses on your device for RAM, ROM, and partitions (when applicable).

   .. group-tab:: Command line

       You can use the :ref:`zephyr:sysbuild_dedicated_image_build_targets` ``tfm_ram_report`` and ``tfm_rom_report`` targets for analyzing the memory usage of the TF-M partitions inside the ``tfm_secure`` partition.
       For example, after building the :ref:`tfm_hello_world` sample for the ``nrf9151dk/nrf9151/ns`` board target, you can run the following commands from your application root directory to generate the RAM memory report for TF-M in the terminal:

       .. code-block:: console

          west build -d build/tfm_hello_world -t tfm_ram_report

For more information about the ``tfm_ram_report`` and ``tfm_rom_report`` targets, refer to the :ref:`tfm_build_system` documentation.

.. _ug_tfm_services:

TF-M Services
*************

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

.. _ug_tfm_services_its:

Internal Trusted Storage service
================================

The Internal Trusted Storage (ITS) service is one of :ref:`ug_tfm_architecture_rot_services_platform`.
It implements the PSA Internal Trusted Storage APIs (`PSA Certified Secure Storage API 1.0`_) to achieve confidentiality, authenticity and encryption in rest (optional).

To enable the ITS service, set the :kconfig:option:`CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` Kconfig option.

ITS is meant to be used by other TF-M partitions.
It must not be accessed directly by a user application :ref:`placed in the Non-Secure Processing Environment <app_boards_spe_nspe_cpuapp_ns>`.
If you want the user application to access the contents of the partition, use the :ref:`tfm_partition_ps`.

For more information about the general features of the TF-M ITS service, see `TF-M ITS`_.

.. _tfm_encrypted_its:

Encrypted ITS
-------------

TF-M ITS encryption is a data protection mechanism in Internal Trusted Storage. It provides transparent encryption using a Master Key Encryption Key (MKEK) stored in hardware, with unique encryption keys derived for each file.

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

The crypto service is one of :ref:`ug_tfm_architecture_rot_services_platform`.
It implements the PSA Crypto APIs (`PSA Certified Crypto API`_) and provides cryptographic services to other TF-M partitions and to the non-secure application.

To enable the crypto service, set the :kconfig:option:`CONFIG_TFM_PARTITION_CRYPTO` Kconfig option.

You can configure the service directly using the ``CONFIG_TFM_CRYPTO_*`` Kconfig options found in the :file:`zephyr/modules/trusted-firmware-m/Kconfig.tfm.crypto_modules` file.
However, it is recommended to use the ``CONFIG_PSA_WANT_*`` Kconfig options to enable the required algorithms and key types.
These will enable the required ``CONFIG_TFM_CRYPTO_*`` Kconfig options.

TF-M uses :ref:`hardware unique keys <lib_hw_unique_key>` when the PSA Crypto key derivation APIs are used, and ``psa_key_derivation_setup`` is called with the algorithm ``TFM_CRYPTO_ALG_HUK_DERIVATION``.

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

Provisioning
************

For devices that need provisioning, TF-M implements the following Platform Root of Trust (PRoT) security lifecycle states that conform to the `ARM Platform Security Model 1.1`_:

* Device Assembly and Test
* PRoT Provisioning
* Secured

The device starts in the **Device Assembly and Test** state.
The :ref:`provisioning_image` sample shows how to switch the device from the **Device Assembly and Test** state to the **PRoT Provisioning** state, and how to provision the device with hardware unique keys (HUKs) and an identity key.

To switch the device from the **PRoT Provisioning** state to the **Secured** state, set the :kconfig:option:`CONFIG_TFM_NRF_PROVISIONING` Kconfig option for your application.
On the first boot, TF-M ensures that the keys are stored in the Key Management Unit (KMU) and switches the device to the **Secured** state.
The :ref:`tfm_psa_template` sample shows how to achieve this.

.. _ug_tfm_manual_VCOM_connection:

Logging
*******

TF-M employs two UART interfaces for logging: one for the :ref:`Secure Processing Environment<app_boards_spe_nspe>` (including MCUboot and TF-M), and one for the :ref:`Non-Secure Processing Environment<app_boards_spe_nspe>` (including user application).
By default, the logs arrive on different COM ports on the host PC.
See :ref:`ug_tfm_manual_VCOM_connection` for more details.

Alternatively, you can configure the TF-M to connect to the same UART as the application with the :kconfig:option:`CONFIG_TFM_SECURE_UART0` Kconfig option.
Setting this Kconfig option makes TF-M logs visible on the application's VCOM, without manual connection.

The UART instance used by the application is ``0`` by default, and the TF-M UART instance is ``1``.
To change the TF-M UART instance to the same as that of the application's, use the :kconfig:option:`CONFIG_TFM_SECURE_UART0` Kconfig option.

.. note::

   When the TF-M and the user application use the same UART, the TF-M disables logging after it has booted and re-enables it again only to log a fatal error.

For nRF5340 DK devices, see :ref:`nrf5430_tfm_log`.
