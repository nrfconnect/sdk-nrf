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

.. _migration_3.4_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Build and configuration system
==============================

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

|no_changes_yet_note|
