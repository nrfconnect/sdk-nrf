.. _ug_tfm_building:

Building and configuring TF-M
#############################

.. contents::
   :local:
   :depth: 2

TF-M is one of the images that are built as part of a :ref:`multi-image application <ug_tfm_building_secure_services>`.

To add TF-M to your build, enable the :kconfig:option:`CONFIG_BUILD_WITH_TFM` configuration option by adding it to your :file:`prj.conf` file.

.. note::
   If you use menuconfig to enable :kconfig:option:`CONFIG_BUILD_WITH_TFM`, you must also enable its dependencies.

By default, TF-M is configured to build the :ref:`minimal version <tfm_minimal_build>`.
To use the full TF-M, you must disable the :kconfig:option:`CONFIG_TFM_PROFILE_TYPE_MINIMAL` option.

.. _ug_tfm_building_board_targets:

Board targets supported by TF-M
*******************************

The boards supported by the SDK distinguish entries according to which CPU is to be targeted (for multi-core SoCs) and whether the :ref:`security by separation <ug_tfm_security_by_separation>` is to be used or not (addition of the ``*/ns`` :ref:`variant <app_boards_names>` if it is used).
To build with TF-M in the |NCS|, you must use a board target with the ``*/ns`` variant.

The following table lists the board targets that you can use to build with TF-M.
See :ref:`app_boards_names` for the complete list of boards and board targets supported by the SDK.

.. list-table:: Board targets supported by TF-M
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Board name
     - TF-M board target
   * - nRF9161 DK
     - PCA10153
     - :zephyr:board:`nrf9161dk <nrf9161dk>`
     - ``nrf9161dk/nrf9161/ns``
   * - nRF9160 DK
     - PCA10090
     - :ref:`nrf9160dk <zephyr:nrf9160dk_nrf9160>`
     - ``nrf9160dk/nrf9160/ns``
   * - nRF9151 DK
     - PCA10171
     - :zephyr:board:`nrf9151dk <nrf9151dk>`
     - ``nrf9151dk/nrf9151/ns``
   * - nRF9131 EK
     - PCA10165
     - :zephyr:board:`nrf9131ek <nrf9131ek>`
     - ``nrf9131ek/nrf9131/ns``
   * - nRF54LM20 DK
     - PCA10184
     - :zephyr:board:`nrf54lm20dk <nrf54lm20dk>`
     - ``nrf54lm20dk/nrf54lm20a/cpuapp/ns``
   * - nRF54L15 DK
     - PCA10156
     - :zephyr:board:`nrf54l15dk <nrf54l15dk>`
     - ``nrf54l15dk/nrf54l15/cpuapp/ns``
   * - nRF54L10 emulated on the nRF54L15 DK
     - PCA10156
     - :ref:`nrf54l10dk/nrf54l10 <zephyr:nrf54l15dk_nrf54l10>`
     - ``nrf54l15dk/nrf54l10/cpuapp/ns``
   * - nRF5340 DK
     - PCA10095
     - :zephyr:board:`nrf5340dk <nrf5340dk>`
     - ``nrf5340dk/nrf5340/cpuapp/ns``
   * - Thingy:53
     - PCA20053
     - :zephyr:board:`thingy53 <thingy53>`
     - ``thingy53/nrf5340/cpuapp/ns``
   * - nRF7002 DK
     - PCA10143
     - :zephyr:board:`nrf7002dk <nrf7002dk>`
     - ``nrf7002dk/nrf5340/cpuapp/ns``
   * - Thingy:91
     - PCA20035
     - :ref:`thingy91 <ug_thingy91>`
     - ``thingy91/nrf9160/ns``
   * - Thingy:91 X
     - PCA20065
     - :ref:`thingy91x <ug_thingy91x>`
     - ``thingy91x/nrf9151/ns``
   * - Thingy:91 X
     - PCA20065
     - :ref:`thingy91x <ug_thingy91x>`
     - ``thingy91x/nrf5340/cpuapp/ns``

.. _ug_tfm_building_secure_services:

Enabling secure services
************************

To enable the secure services in TF-M, you must use the :ref:`TF-M Crypto Service PSA Crypto API implementation <ug_crypto_architecture_implementation_standards_tfm>`.

Complete the following steps to enable the secure services in TF-M:

1. Enable :kconfig:option:`CONFIG_NRF_SECURITY` to use the PSA Crypto APIs through :ref:`nRF Security <nrf_security>`.
#. Enable the :kconfig:option:`CONFIG_BUILD_WITH_TFM` configuration option by adding it to your :file:`prj.conf` file.
#. :ref:`Configure PSA Crypto API <psa_crypto_support>` Kconfig options.
#. Build the application with :ref:`tfm_minimal_build` or :ref:`tfm_configurable_build`.

After building the application, the TF-M secure image enables the use of the hardware acceleration, while the Kconfig configurations in the nRF Security subsystem control the features enabled in TF-M.

See :ref:`tfm_partition_crypto` for more information about the TF-M Crypto partition.

.. note::
   Depending on the implementation you are using, the |NCS| build system uses different versions of the PSA Crypto API.

   .. ncs-include:: ../psa_certified_api_overview.rst
      :start-after: psa_crypto_support_tfm_build_start
      :end-before: psa_crypto_support_tfm_build_end

.. _tfm_minimal_build:

Minimal build
*************

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
******************

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
==============================================

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

To control the number of logging messages, set the :kconfig:option:`CONFIG_TFM_SPM_LOG_LEVEL` Kconfig option.
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
*************************************

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
Since ``tfm_secure`` spans the ``mcuboot_pad`` and ``tfm`` partitions, you can decrease the size of any of them by 0x200 to fix the alignment issue.
Decrease the size of the (optional) ``mcuboot_pad`` partition, and thus the size of the ``tfm_secure`` partition, as follows:

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
***************************************

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
=====================================================

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
