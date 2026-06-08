:orphan:

.. _migration_3.4:

Migration notes for |NCS| v3.4.0
################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.3.0 to |NCS| v3.4.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.4_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Partition Manager deprecation
=============================

.. toggle::

   The :ref:`partition_manager` is a component in the |NCS| and is responsible for handling the memory partitioning at build time.

   This functionality has been deprecated and replaced by Zephyr's default devicetree-based memory partitioning.
   It is recommended that all new designs using Nordic devices are built with DTS instead of Partition Manager.
   Partition Manager will be removed from the |NCS| main branch by the end of 2026.

   Samples, tests, and applications that previously relied on Partition Manager must now provide devicetree overlays that define the required flash and RAM partitions.
   For cellular use cases, you can reuse the partition layouts from :file:`nrf/dts/samples/cellular/*.dtsi` by including the appropriate file in your board overlay.

   For more information on how to configure partitions using DTS and how to migrate your existing configuration to DTS, see the :ref:`migration_partitions` page.

Security
========

Removal of legacy PSA Crypto API
--------------------------------

.. toggle::

   The Mbed TLS module was updated to v4.1.0 (from v3.6.6).

   This change removed support for legacy, deprecated ``mbedcrypto`` APIs and related tests (prefixed with ``mbedtls_``).
   As a consequence, the :ref:`nrf_security` subsystem was updated: Kconfig options related to Mbed TLS were rearranged and the outdated Kconfig options were removed.
   The subsystem now uses the Mbed TLS integration from Zephyr as-is :kconfig:option:`CONFIG_MBEDTLS_BUILTIN`) while replacing upstream TF-PSA-Crypto with Oberon PSA Crypto (:kconfig:option:`CONFIG_TF_PSA_CRYPTO_CUSTOM`).

   From this update onwards:

   * Configure cryptographic features using :ref:`psa_crypto_support` and :ref:`ug_crypto_supported_features` instead of the legacy implementation.
   * Enable :kconfig:option:`CONFIG_MBEDTLS` only if you use TLS or X.509.
   * For cryptographic operations, enable only :kconfig:option:`CONFIG_PSA_CRYPTO`.

   For an overview of the changes brought by this update in Mbed TLS and Zephyr, see the following pages:

    * The Mbed TLS sections of the Zephyr v4.4 :ref:`release notes <zephyr_4.4>` and :ref:`migration guide <migration_4.4>`.
    * Official Mbed TLS' `TF-PSA-Crypto migration guide <Migrating from Mbed TLS 3.x to TF-PSA-Crypto 1.0_>`_ and `Transitioning to the PSA API document <Transitioning to the PSA API_>`_.
    * The release notes from upstream Mbed TLS:

       * `Mbed TLS 4.0.0 release notes`_
       * `TF-PSA-Crypto 1.0.0 release notes`_
       * `Mbed TLS 4.1.0 release notes`_
       * `TF-PSA-Crypto 1.1.0 release notes`_

Build and configuration system
==============================

HAL global define deprecation
-----------------------------

.. toggle::

   Global HAL defines are deprecated.
   Do not reference the following global defines in code:

   * ``NRF51``
   * ``NRF51422_XXAA``
   * ``NRF51422_XXAB``
   * ``NRF51422_XXAC``
   * ``NRF52805_XXAA``
   * ``NRF52810_XXAA``
   * ``NRF52811_XXAA``
   * ``NRF52820_XXAA``
   * ``NRF52832_XXAA``
   * ``NRF52833_XXAA``
   * ``NRF52840_XXAA``
   * ``NRF5340_XXAA_APPLICATION``
   * ``NRF5340_XXAA_NETWORK``
   * ``NRF54H20_XXAA``
   * ``NRF54L05_XXAA``
   * ``DEVELOP_IN_NRF54L15``
   * ``NRF54L10_XXAA``
   * ``NRF54L15_XXAA``
   * ``DEVELOP_IN_NRF54LM20B``
   * ``NRF54LM20A_XXAA``
   * ``NRF7120_ENGA_XXAA``
   * ``NRF54LM20B_XXAA``
   * ``NRF_FLPR``
   * ``NRF9120_XXAA``
   * ``NRF9160_XXAA``
   * ``NRF_APPLICATION``
   * ``NRF_RADIOCORE``
   * ``NRF_PPR``
   * ``ENABLE_APPROTECT``
   * ``ENABLE_APPROTECT_USER_HANDLING``
   * ``ENABLE_AUTHENTICATED_APPROTECT``
   * ``ENABLE_SECURE_APPROTECT``
   * ``ENABLE_SECUREAPPROTECT``
   * ``ENABLE_SECURE_APPROTECT_USER_HANDLING``
   * ``ENABLE_AUTHENTICATED_SECUREAPPROTECT``
   * ``NRF_SKIP_FICR_NS_COPY_TO_RAM``
   * ``NRF_CONFIG_CPU_FREQ_MHZ``
   * ``NRF_SKIP_CLOCK_CONFIGURATION``
   * ``NRF_DISABLE_FICR_TRIMCNF``
   * ``NRF_SKIP_TAMPC_SETUP``
   * ``NRF_SKIP_GLITCHDETECTOR_DISABLE``
   * ``NRF54L_CONFIGURATION_56_ENABLE``

   Use the corresponding Kconfig symbols instead.

Samples and applications
========================

This section describes the changes related to samples and applications.

.. _nrf_audio_migration_3.4:

nRF Audio (formerly nRF5340 Audio)
----------------------------------

.. toggle::

   * The :file:`buildprog.py` script no longer supports the ``-u`` (user name) option.
     Use the ``-cn`` (custom name) option instead.
     The custom name is now used for both unicast and broadcast.
     Changing this parameter requires a pristine build.
     The option is intended as a convenience argument; set names through configuration options for persistent configuration.

.. _matter_migration_3.4:

Matter
------

.. toggle::

   * The :ref:`partition_manager` has been deprecated, removed from the Matter samples and applications and replaced by Zephyr's default devicetree-based memory partitioning.
     The base :file:`.dtsi` files for all supported SoCs have been created in the :file:`nrf/dts/samples/matter` directory.
     The files from the :file:`nrf/dts/samples/matter` directory are used by default in all Matter samples and applications.
     You can also reuse them as a base for your own custom partitioning.

     To reuse the base :file:`.dtsi` files for your own custom partitioning, find the appropriate base :file:`.dtsi` file for your SoC and include it in your :file:`.overlay` board file.
     For example, to reuse the base file for the ``nrf54lm20dk/nrf54lm20b/cpuapp`` board target, add the following line to your :file:`sample/boards/nrf54lm20dk_nrf54lm20b_cpuapp.overlay` board file:

     .. code-block:: dts

        #include "<samples/matter/nrf54lm20_cpuapp_partitions.dtsi>"

     After that, once the base is ready, you can modify partitions by using the |nRFVSC| with its `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_, or manually edit the :file:`.overlay` file.

     For more information on how to configure partitions using DTS and how to migrate your existing configuration to DTS, see the :ref:`migration_partitions` page.

   * The :ref:`matter_window_covering_sample` sample now uses the Thread Sleepy End Device (SED) device type by default.
     You can enable the Thread Synchronized Sleepy End Device (SSED) device type as an optional feature.
     To enable the Thread SSED support, add the ``-DEXTRA_CONF_FILE=ssed.conf`` extra argument to the build command.

Libraries
=========

This section describes the changes related to libraries.

.. toggle::

   * :ref:`lib_location` library:

     * The library now always uses the chosen ``zephyr,wifi`` node to find the used Wi-Fi device.
       If your application uses the deprecated ``ncs,location-wifi`` node, you need to change it to use the ``zephyr,wifi`` node instead:

       .. code-block:: dts

        chosen {
                zephyr,wifi = &mywifi;
        };

Wi-Fi®
------

.. toggle::

   * The Wi-Fi Enterprise snippet has changed.
     Use the |NCS| ``nordic-wifi-enterprise`` snippet instead of Zephyr's ``wifi-enterprise`` snippet for Wi-Fi Enterprise builds.
     The |NCS| snippet is available in :file:`snippets/nordic-wifi-enterprise`.


.. _migration_3.4_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Build and configuration system
==============================

.. toggle::

   * The Kconfig options :kconfig:option:`CONFIG_SRAM_SIZE` and :kconfig:option:`CONFIG_SRAM_BASE_ADDRESS` have been deprecated.
     Use the devicetree ``zephyr.sram`` chosen node to specify which RAM node is used.
     If you adjust either option manually, :kconfig:option:`CONFIG_SRAM_DEPRECATED_KCONFIG_SET` is set to indicate the deprecation.
     However, applications will continue to build and work, and a deprecation notice will be shown.
     For the majority of cases, you should not change these values as they default to the values of the ``zephyr,sram`` chosen node.
     If the code references these Kconfig options, you should update them.
     No deprecation warning is emitted when these values are referenced, because of how Kconfig defines are generated.

     .. note::
        This is listed in the recommended changes for this |NCS| release.
        In the next |NCS| release, this will be a required change.

Nordic SoC platform symbols (Haltium / Lumos)
---------------------------------------------

.. toggle::

   The internal *Haltium* and *Lumos* platform names have been removed from the Nordic SoC integration in favor of the explicit Zephyr ``SOC_SERIES_*`` symbols.
   Old symbols, headers, and macros are kept as deprecated aliases for one release cycle and emit a deprecation warning at build time.
   Update your application as follows:

   * Kconfig options:

     * Replace ``CONFIG_NRF_PLATFORM_HALTIUM`` with :kconfig:option:`CONFIG_SOC_SERIES_NRF54H` or :kconfig:option:`CONFIG_SOC_SERIES_NRF92` (whichever applies).
     * Replace ``CONFIG_NRF_PLATFORM_LUMOS`` with :kconfig:option:`CONFIG_SOC_SERIES_NRF54L` or :kconfig:option:`CONFIG_SOC_SERIES_NRF71` (whichever applies).
     * Replace ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR`` with :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR` in your :file:`sysbuild.conf` file.

   * C headers (:file:`zephyr/soc/nordic/common/`):

     * Replace ``#include <haltium_power.h>`` with ``#include <soc_power.h>``.
     * Replace ``#include <haltium_pm_s2ram.h>`` with ``#include <soc_pm_s2ram.h>``.

   * C macros:

     * Replace ``HALTIUM_PLATFORM_PSA_KEY_ID(...)`` with ``NRF_PLATFORM_PSA_KEY_ID(...)`` from :file:`include/psa/nrf_platform_key_ids.h`.

   The following table summarizes the renames:

   .. list-table:: Haltium / Lumos platform symbol renames
      :widths: 40 40 20
      :header-rows: 1

      * - Old name
        - New name
        - Type
      * - ``CONFIG_NRF_PLATFORM_HALTIUM``
        - :kconfig:option:`CONFIG_SOC_SERIES_NRF54H` or :kconfig:option:`CONFIG_SOC_SERIES_NRF92`
        - Kconfig
      * - ``CONFIG_NRF_PLATFORM_LUMOS``
        - :kconfig:option:`CONFIG_SOC_SERIES_NRF54L` or :kconfig:option:`CONFIG_SOC_SERIES_NRF71`
        - Kconfig
      * - ``SB_CONFIG_NRF_HALTIUM_GENERATE_UICR``
        - :kconfig:option:`SB_CONFIG_NRF_GENERATE_UICR`
        - Sysbuild Kconfig
      * - :file:`<haltium_power.h>`
        - :file:`<soc_power.h>`
        - Header
      * - :file:`<haltium_pm_s2ram.h>`
        - :file:`<soc_pm_s2ram.h>`
        - Header
      * - ``HALTIUM_PLATFORM_PSA_KEY_ID()``
        - ``NRF_PLATFORM_PSA_KEY_ID()``
        - Macro

   .. note::
      The MDK-defined ``HALTIUM_XXAA`` and ``LUMOS_XXAA`` preprocessor symbols are managed by the MDK release schedule and are not affected by this migration.
      Code that needs to distinguish hardware should use the corresponding ``NRF54H_SERIES``, ``NRF92_SERIES``, ``NRF54L_SERIES``, or ``NRF7120_ENGA_XXAA`` defines instead.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

.. _migration_3.4_google_fast_pair:

Google Fast Pair
----------------

.. toggle::

   For applications and samples using the :ref:`bt_fast_pair_readme` library:

   * The devicetree (DTS) partition overlays of the Fast Pair samples and Fast Pair-enabled board targets have been migrated from the legacy ``fixed-partitions`` compatible string to the new ``zephyr,mapped-partition`` compatible string introduced in Zephyr.
     The new layout matches the output of the Partition Manager-to-DTS helper script (:file:`scripts/pm_to_dts.py`) and aligns the Fast Pair sample overlays with the partition binding convention adopted by the rest of the Zephyr SoC devicetree.

     If your application uses a Fast Pair DTS partition overlay derived from earlier |NCS| releases, update each :file:`<board_target>.overlay` file as follows:

     * On the ``partitions`` grouping node, replace the ``compatible = "fixed-partitions";`` property with an empty ``ranges;`` property.
     * On every child partition node (for example, ``boot_partition``, ``slot0_partition``, ``slot1_partition``, ``bt_fast_pair_partition``, and ``storage_partition``), add the ``compatible = "zephyr,mapped-partition";`` property.

     For example, replace the following overlay snippet:

     .. code-block:: devicetree

        partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;

            slot0_partition: partition@0 {
                label = "image-0";
                reg = <0x0 DT_SIZE_K(996)>;
            };
        };

     With the following:

     .. code-block:: devicetree

        partitions {
            ranges;
            #address-cells = <1>;
            #size-cells = <1>;

            slot0_partition: partition@0 {
                compatible = "zephyr,mapped-partition";
                label = "image-0";
                reg = <0x0 DT_SIZE_K(996)>;
            };
        };

     Refer to the partition overlays under the :file:`nrf/samples/bluetooth/fast_pair/locator_tag/` and :file:`nrf/samples/bluetooth/fast_pair/input_device/` sample directories for complete examples of the updated layout.

     .. important::
        This migration is mandatory on nRF53 Series board targets.
        Recent Zephyr updates added a ``ranges`` translation to the ``flash1`` SoC node (network core flash), so ``DT_REG_ADDR()`` on partition nodes now returns their bus address rather than their in-flash offset.
        With the legacy ``fixed-partitions`` compatible, the Zephyr linker still uses ``CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET`` to place the network core code partition, which double-counts the ``flash1`` base.
        As a result, the network core image is linked at an address outside the valid flash range and ``nrfutil`` rejects programming with the following message:

        .. code-block:: console

           Device error: Address range 0x02008800..0x020262ac is outside the memory ranges defined for programming through the Network core (Generic)

        Switching to ``zephyr,mapped-partition`` makes the linker derive the network core code partition's load address directly from ``DT_REG_ADDR(zephyr_code_partition)``, which yields the correct bus address and avoids the double-counting.
        On the application core, where ``flash0`` uses identity ``ranges``, both compatibles produce the same addresses, but updating the overlays is still recommended for consistency.

nRF Cloud library
-----------------

.. toggle::

   * The default value of the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC` choice has changed from :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` to the device UUID.
     On an nRF9160 device, the new default is :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_INTERNAL_UUID`.
     On the nRF91x1 and future SoCs, the new default is :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_MDM_DEVICE_UUID`.

     If your application relied on the previous IMEI-based default without setting it explicitly in the :file:`prj.conf` file, the device will connect to nRF Cloud with a different client ID after the upgrade and appear as a new, unprovisioned device.
     To preserve the previous behavior, add the following line to your :file:`prj.conf` file:

     .. code-block:: cfg

        CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI=y

     The :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` and :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_HW_ID` Kconfig options are deprecated but kept functional for fleet operators with IMEI-provisioned devices already in the field.
     New applications should use the UUID default to match the device ID that the nRF Cloud provisioning tools generate.
