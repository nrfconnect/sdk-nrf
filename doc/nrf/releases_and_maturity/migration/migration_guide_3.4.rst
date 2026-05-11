.. _migration_3.4:

Migration notes for |NCS| v3.4.0 (Working draft)
################################################

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

Samples and applications
========================

This section describes the changes related to samples and applications.

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

     After that, once the base is ready, you can modify partitions by using the |nRFVSC| with its `Devicetree language support`_ and the `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_, or manually edit the :file:`.overlay` file.

     For more information on how to configure partitions using DTS and how to migrate your existing configuration to DTS, see the :ref:`migration_partitions` page.

   * The :ref:`matter_window_covering_sample` sample now uses the Thread Sleepy End Device (SED) device type by default.
     You can enable the Thread Synchronized Sleepy End Device (SSED) device type as an optional feature.
     To enable the Thread SSED support, add the ``-DEXTRA_CONF_FILE=ssed.conf`` extra argument to the build command.

.. _migration_3.4_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Build and configuration system
==============================

* The Kconfig options :kconfig:option:`CONFIG_SRAM_SIZE` and :kconfig:option:`CONFIG_SRAM_BASE_ADDRESS` have been deprecated.
  Use the devicetree ``zephyr.sram`` chosen node to specify which RAM node is used.
  If you adjust either option manually, :kconfig:option:`CONFIG_SRAM_DEPRECATED_KCONFIG_SET` is set to indicate the deprecation.
  However, applications will continue to build and work with this notice.
  For the majority of cases, you should not change these values as they default to the values of the ``zephyr,sram`` chosen node.
  If the code references these Kconfig options, you should update them.
  No deprecation warning will be emitted when these values are referenced due to the Kconfig define generation process.

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
