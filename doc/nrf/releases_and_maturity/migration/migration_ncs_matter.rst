.. _migration_sdk_nrf_to_ncs_matter:

Migrating Matter projects from sdk-nrf to Matter add-on
#######################################################

.. contents::
   :local:
   :depth: 3

The |NCS| v3.4.0 was the last major release with Matter samples, shared sample code, board partition devicetree files, and Matter-specific snippets integrated in the ``sdk-nrf`` repository.
In later releases, the Matter reference applications and their supporting assets are maintained in the separate `Matter add-on <ncs-matter add-on repository_>`_ repository (``ncs-matter``).

This guide describes the changes required when migrating a Matter project that was based on paths under ``sdk-nrf`` to the ``ncs-matter`` add-on structure.
It covers workspace setup, repository layout, build system updates, Kconfig symbol renames, devicetree and snippet changes, and documentation references.

.. note::
   If you only use Matter samples without custom modifications, the simplest approach is to create a new workspace from the ``ncs-matter`` add-on and copy over your application-specific source files, configuration overlays, and factory data.
   Use this guide when you maintain a custom Matter application that was forked from an |NCS| v3.4.0 (or earlier) Matter sample or application.

Overview
********

The ``ncs-matter`` add-on is a Zephyr module that extends the |NCS| with:

* Matter reference samples (formerly under :file:`nrf/samples/matter`)
* Matter bridge and weather station reference applications (formerly under :file:`nrf/applications`, now under :file:`ncs-matter/samples`)
* Shared Matter sample infrastructure (formerly under :file:`nrf/samples/matter/common`, now under :file:`ncs-matter/subsys`)
* Matter partition devicetree include files (formerly under :file:`nrf/dts/samples/matter`, now under :file:`ncs-matter/dts`)
* Matter build and data-model CMake helpers (formerly under :file:`nrf/samples/matter/common/cmake`, now under :file:`ncs-matter/cmake`)
* Matter-specific Zephyr snippets (formerly under :file:`nrf/snippets/matter`, now under :file:`ncs-matter/snippets`)

The add-on manifest (:file:`ncs-matter/west.yml`) imports a fixed |NCS| release together with the matching ``sdk-connectedhomeip`` revision.

Setting up the workspace
************************

To work with Matter samples and applications from the add-on, set up a west workspace that includes the ``ncs-matter`` repository.

Initialize from the add-on manifest
===================================

Complete the following steps to create a new workspace with the `Matter Add-on`_ as the manifest repository:

.. parsed-literal::

   west init -m https://github.com/nrfconnect/ncs-matter --mr <release>
   cd ncs-matter
   west update

Replace <release> with the add-on release tag that matches your target |NCS| version (see the add-on release notes).

Alternatively, if you already cloned the add-on repository locally:

.. code-block:: console

   west config manifest.path ncs-matter
   west update

To get back to the |NCS| manifest, run:

.. code-block:: console

   west config manifest.path nrf
   west update

Building Matter samples
=======================

Build commands must point to sample paths under the add-on repository.
The board target and sysbuild configuration options remain the same as in the |NCS| Matter samples.

For example, to build the Matter template sample for the ``nrf54l15dk/nrf54l15/cpuapp`` board target:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp ncs-matter/samples/template

Compare this with the former path:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp nrf/samples/matter/template

You can also create a :ref:`workspace application <create_application_types_workspace>` by copying a sample from ``ncs-matter/samples/`` into your workspace application folder.
Follow the instructions in :ref:`creating_add_on_index` for add-on based application creation in |nRFVSC|.

Repository layout changes
*************************

The repository layout has been changed in comparison to the |NCS| repository structure.
See the following sections to learn about the changes.

Sample and application paths
============================

All Matter reference code moves from ``sdk-nrf`` to ``ncs-matter``.
The ``matter`` subdirectory is removed from sample paths, and former applications are relocated under ``ncs-matter/samples/``.

.. list-table:: Matter sample and application path mapping
   :widths: 50 50
   :header-rows: 1

   * - Former path in ``sdk-nrf``
     - New path in ``ncs-matter``
   * - :file:`nrf/samples/matter/template`
     - :file:`ncs-matter/samples/template`
   * - :file:`nrf/samples/matter/light_bulb`
     - :file:`ncs-matter/samples/light_bulb`
   * - :file:`nrf/samples/matter/light_switch`
     - :file:`ncs-matter/samples/light_switch`
   * - :file:`nrf/samples/matter/lock`
     - :file:`ncs-matter/samples/lock`
   * - :file:`nrf/samples/matter/window_covering`
     - :file:`ncs-matter/samples/window_covering`
   * - :file:`nrf/samples/matter/thermostat`
     - :file:`ncs-matter/samples/thermostat`
   * - :file:`nrf/samples/matter/smoke_co_alarm`
     - :file:`ncs-matter/samples/smoke_co_alarm`
   * - :file:`nrf/samples/matter/temperature_sensor`
     - :file:`ncs-matter/samples/temperature_sensor`
   * - :file:`nrf/samples/matter/contact_sensor`
     - :file:`ncs-matter/samples/contact_sensor`
   * - :file:`nrf/samples/matter/closure`
     - :file:`ncs-matter/samples/closure`
   * - :file:`nrf/samples/matter/manufacturer_specific`
     - :file:`ncs-matter/samples/manufacturer_specific`
   * - :file:`nrf/applications/matter_bridge`
     - :file:`ncs-matter/samples/matter_bridge`
   * - :file:`nrf/applications/matter_weather_station`
     - :file:`ncs-matter/samples/matter_weather_station`

Shared sample infrastructure
============================

The shared code, Kconfig definitions, and CMake logic that were previously located under :file:`nrf/samples/matter/common` are reorganized in the add-on module:

.. list-table:: Shared Matter infrastructure path mapping
   :widths: 45 55
   :header-rows: 1

   * - Former path in ``sdk-nrf``
     - New path in ``ncs-matter``
   * - :file:`nrf/samples/matter/common/src/`
     - :file:`ncs-matter/subsys/`
   * - :file:`nrf/samples/matter/common/cmake/source_common.cmake`
     - Automatic via :file:`ncs-matter/subsys/CMakeLists.txt` (linked when ``CONFIG_CHIP=y``)
   * - :file:`nrf/samples/matter/common/cmake/data_model.cmake`
     - :file:`ncs-matter/cmake/data_model.cmake`
   * - :file:`nrf/samples/matter/common/cmake/zap_helpers.cmake`
     - :file:`ncs-matter/cmake/zap_helpers.cmake`
   * - :file:`nrf/samples/matter/common/cmake/source_common.cmake`, :file:`data_model.cmake`, and :file:`zap_helpers.cmake` (combined)
     - :file:`ncs-matter/cmake/sample.cmake`
   * - :file:`nrf/samples/matter/common/src/Kconfig`
     - :file:`ncs-matter/subsys/Kconfig` (included from :file:`ncs-matter/Kconfig`)

Devicetree partition files
==========================

Base Matter partition layouts move from :file:`nrf/dts/samples/matter/` to :file:`ncs-matter/dts/`.
The add-on registers this directory as a devicetree root (``dts_root`` in :file:`zephyr/module.yml`), so partition files are included by filename only.

.. list-table:: Matter partition devicetree file mapping
   :widths: 50 50
   :header-rows: 1

   * - Former include in board overlay
     - New include in board overlay
   * - ``#include <samples/matter/nrf52840_partitions.dtsi>``
     - ``#include <nrf52840_partitions.dtsi>``
   * - ``#include <samples/matter/nrf5340_cpuapp_partitions.dtsi>``
     - ``#include <nrf5340_cpuapp_partitions.dtsi>``
   * - ``#include <samples/matter/nrf5340_cpunet_partitions.dtsi>``
     - ``#include <nrf5340_cpunet_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54l15_cpuapp_partitions.dtsi>``
     - ``#include <nrf54l15_cpuapp_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54l15_cpuapp_internal_partitions.dtsi>``
     - ``#include <nrf54l15_cpuapp_internal_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54l15_cpuapp_tfm_application_partitions.dtsi>``
     - ``#include <nrf54l15_cpuapp_tfm_application_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54l15_cpuapp_tfm_base_partitions.dtsi>``
     - ``#include <nrf54l15_cpuapp_tfm_base_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54l10_cpuapp_partitions.dtsi>``
     - ``#include <nrf54l10_cpuapp_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54lm20_cpuapp_partitions.dtsi>``
     - ``#include <nrf54lm20_cpuapp_partitions.dtsi>``
   * - ``#include <samples/matter/nrf54lm20_cpuapp_internal_partitions.dtsi>``
     - ``#include <nrf54lm20_cpuapp_internal_partitions.dtsi>``

Update every board-specific :file:`.overlay` file in your project that references the old ``samples/matter/`` devicetree path.

Snippets
========

Matter snippets move from :file:`nrf/snippets/matter/` to :file:`ncs-matter/snippets/`.
Snippet names are shortened because the add-on registers :file:`ncs-matter/snippets` as a snippet root.

.. list-table:: Matter snippet mapping
   :widths: 35 35 30
   :header-rows: 1

   * - Former snippet path
     - Former snippet name
     - New snippet name
   * - :file:`nrf/snippets/matter/matter-debug/`
     - ``matter-debug``
     - ``debug``
   * - :file:`nrf/snippets/matter/matter-diagnostic-logs/`
     - ``matter-diagnostic-logs``
     - ``diagnostic-logs``

Update build commands and :file:`sample.yaml` definitions that reference the old snippet names.
For example, replace ``template_SNIPPET="matter-diagnostic-logs;matter-debug"`` with ``template_SNIPPET="diagnostic-logs;debug"``.

Build system changes
********************

To align the `Matter Add-on`_ with the |NCS| build system, the CMake, Sysbuild and Kconfig files have been created.
The Zephyr module configuration points to the new files instead of the alternatives in the |NCS| repository.

CMake updates
=============

Replace references to ``ZEPHYR_NRF_MODULE_DIR`` Matter sample paths with ``ZEPHYR_NCS_MATTER_MODULE_DIR`` add-on paths.

In application :file:`CMakeLists.txt` files, make the following changes:

* Remove the explicit ``enable-gnu-std.cmake`` include.
  GNU standard support is enabled globally by the add-on module (:file:`ncs-matter/CMakeLists.txt`).
* Replace separate includes of :file:`source_common.cmake`, :file:`data_model.cmake`, and :file:`zap_helpers.cmake` with a single include of :file:`sample.cmake`:

  .. code-block:: c

     # Before (sdk-nrf)
     include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
     include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)
     include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/data_model.cmake)
     include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/zap_helpers.cmake)

     # After (ncs-matter add-on)
     include(${ZEPHYR_NCS_MATTER_MODULE_DIR}/cmake/sample.cmake)

* Update explicit source file paths.
  For example, in the Matter bridge application, replace:

  .. code-block:: c

     ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/binding/binding_handler.cpp

  with:

  .. code-block:: c

     ${ZEPHYR_NCS_MATTER_MODULE_DIR}/subsys/binding/binding_handler.cpp

The shared Matter sample sources under :file:`ncs-matter/subsys/` are linked automatically for all applications that enable ``CONFIG_CHIP`` through the add-on module CMake entry point.
You no longer need to include :file:`source_common.cmake` manually unless you have a non-standard build layout.

Kconfig updates
===============

Removed the following line from sample :file:`Kconfig` files:

.. code-block:: kconfig

   source "$(ZEPHYR_NRF_MODULE_DIR)/samples/matter/common/src/Kconfig"

The add-on Kconfig tree (:file:`ncs-matter/Kconfig` and :file:`ncs-matter/subsys/Kconfig`) is loaded automatically when the ``ncs-matter`` Zephyr module is present in the workspace.

Kconfig symbol renames
======================

All ``CONFIG_NCS_SAMPLE_MATTER_*`` options are renamed to ``CONFIG_MATTER_*`` in the add-on.
Update every :file:`prj.conf`, :file:`prj_release.conf`, :file:`*.conf` overlay, and :file:`sample.yaml` reference.

.. list-table:: Matter sample Kconfig symbol renames
   :widths: 50 50
   :header-rows: 1

   * - Former symbol (``sdk-nrf``)
     - New symbol (``ncs-matter``)
   * - ``CONFIG_NCS_SAMPLE_MATTER_APP_TASK_QUEUE_SIZE``
     - ``CONFIG_MATTER_APP_TASK_QUEUE_SIZE``
   * - ``CONFIG_NCS_SAMPLE_MATTER_APP_TASK_MAX_SIZE``
     - ``CONFIG_MATTER_APP_TASK_MAX_SIZE``
   * - ``CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING``
     - ``CONFIG_MATTER_CUSTOM_BLUETOOTH_ADVERTISING``
   * - ``CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS``
     - ``CONFIG_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE``
     - ``CONFIG_MATTER_FACTORY_RESET_ON_KEY_MIGRATION_FAILURE``
   * - ``CONFIG_NCS_SAMPLE_MATTER_LEDS``
     - ``CONFIG_MATTER_LEDS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL``
     - ``CONFIG_MATTER_SETTINGS_SHELL``
   * - ``CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS``
     - ``CONFIG_MATTER_TEST_EVENT_TRIGGERS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX``
     - ``CONFIG_MATTER_TEST_EVENT_TRIGGERS_MAX``
   * - ``CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS``
     - ``CONFIG_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX_TRIGGERS_DELEGATES``
     - ``CONFIG_MATTER_TEST_EVENT_TRIGGERS_MAX_TRIGGERS_DELEGATES``
   * - ``CONFIG_NCS_SAMPLE_MATTER_TEST_SHELL``
     - ``CONFIG_MATTER_TEST_SHELL``
   * - ``CONFIG_NCS_SAMPLE_MATTER_PERSISTENT_STORAGE``
     - ``CONFIG_MATTER_PERSISTENT_STORAGE``
   * - ``CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_PATH``
     - ``CONFIG_MATTER_ZAP_FILE_PATH``
   * - ``CONFIG_NCS_SAMPLE_MATTER_CERTIFICATION``
     - ``CONFIG_MATTER_CERTIFICATION``
   * - ``CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER``
     - ``CONFIG_MATTER_USE_DEFAULT_BUTTON_HANDLER``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_MAX_SIMULTANEOUS_SESSIONS``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_MAX_SIMULTANEOUS_SESSIONS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_CRASH_LOGS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_REMOVE_CRASH_AFTER_READ``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_END_USER_LOGS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_NETWORK_LOGS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_TEST``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_TEST``
   * - ``CONFIG_NCS_SAMPLE_MATTER_DIAGNOSTIC_LOGS_REDIRECT``
     - ``CONFIG_MATTER_DIAGNOSTIC_LOGS_REDIRECT``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG``
     - ``CONFIG_MATTER_WATCHDOG``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP``
     - ``CONFIG_MATTER_WATCHDOG_PAUSE_IN_SLEEP``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_ON_DEBUG``
     - ``CONFIG_MATTER_WATCHDOG_PAUSE_ON_DEBUG``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT``
     - ``CONFIG_MATTER_WATCHDOG_DEFAULT``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT``
     - ``CONFIG_MATTER_WATCHDOG_TIMEOUT``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT_FEED_TIME``
     - ``CONFIG_MATTER_WATCHDOG_DEFAULT_FEED_TIME``
   * - ``CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_EVENT_TRIGGERS``
     - ``CONFIG_MATTER_WATCHDOG_EVENT_TRIGGERS``
   * - ``CONFIG_NCS_SAMPLE_MATTER_SETTINGS_STORAGE_BACKEND``
     - ``CONFIG_MATTER_SETTINGS_STORAGE_BACKEND``
   * - ``CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_BACKEND``
     - ``CONFIG_MATTER_SECURE_STORAGE_BACKEND``
   * - ``CONFIG_NCS_SAMPLE_MATTER_STORAGE_MAX_KEY_LEN``
     - ``CONFIG_MATTER_STORAGE_MAX_KEY_LEN``
   * - ``CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_MAX_ENTRY_NUMBER``
     - ``CONFIG_MATTER_SECURE_STORAGE_MAX_ENTRY_NUMBER``
   * - ``CONFIG_NCS_SAMPLE_MATTER_SECURE_STORAGE_PSA_KEY_VALUE_OFFSET``
     - ``CONFIG_MATTER_SECURE_STORAGE_PSA_KEY_VALUE_OFFSET``

Removed Kconfig options
=======================

The following options from :file:`nrf/samples/matter/common/src/Kconfig` are no longer available in the add-on:

* ``CONFIG_NCS_SAMPLE_MATTER_ZAP_GENERATION_STATIC``
* ``CONFIG_NCS_SAMPLE_MATTER_ZAP_GENERATION_BUILD_TIME``

The add-on uses pre-generated ZAP output checked into each sample's :file:`src/default_zap/zap-generated/` directory.
Regenerate ZAP files using the Matter west commands documented in the add-on tooling section.

Sysbuild configuration
======================

Matter samples in the add-on use a :file:`sysbuild_internal.conf` file at the sample root to hold sysbuild image options that were previously embedded in sample-specific sysbuild configuration.
If your custom project relied on the same sysbuild defaults (for example, MCUboot overwrite-only mode with compressed image support), copy the relevant options from the corresponding add-on sample's :file:`sysbuild_internal.conf` file.

The add-on sysbuild Kconfig (:file:`ncs-matter/sysbuild/Kconfig.sysbuild`) disables the Partition Manager globally for Matter builds, consistent with the devicetree-based partitioning introduced in |NCS| v3.4.0.
See also :ref:`matter_migration_3.4` for partition migration details that still apply after moving to the add-on.

Documentation and tooling
***************************

Documentation
=============

Matter sample documentation, getting started guides, and protocol documentation that previously lived under :file:`nrf/doc/nrf/protocols/matter/` and :file:`nrf/doc/nrf/samples/matter.rst` are published from the ``ncs-matter`` add-on documentation set.
After migrating your project, use the add-on documentation as the primary reference for building, configuring, and testing Matter samples.

The |NCS| documentation retains high-level Matter integration information and links to the add-on, similar to how :ref:`zigbee_samples` references the Zigbee add-on repositories.

.. note::
   Documentation cross-references that use the ``|sample path|`` substitution now point to paths under :file:`ncs-matter/samples/` instead of :file:`nrf/samples/matter/`.
   Update any custom documentation or internal wikis that hard-code the old paths.

Validation scripts
==================

If you use the Matter sample validation tooling locally, switch to the add-on copy under :file:`ncs-matter/scripts/matter_sample_checker/`.
The checker configuration expects the add-on directory layout - for example, partition ``#include`` directives without the ``samples/matter/`` prefix, and snippet paths under :file:`ncs-matter/snippets/`.

West ZAP tooling
================

The Matter west extension commands (``west zap-gui``, ``west zap-generate``, and related commands) remain part of the Connected Home IP module in the |NCS|.
When working in an add-on workspace, make sure your ``ZAP_INSTALL_PATH`` environment variable is set and run west commands from the workspace root that contains both ``ncs-matter`` and ``modules/lib/matter``.

What remains in sdk-nrf
***********************

The following Matter-related components stay in the core |NCS| (``sdk-nrf`` and imported modules):

* The Connected Home IP stack itself (:file:`modules/lib/matter`, imported through ``sdk-nrf``)
* Matter Kconfig and GN integration under :file:`modules/lib/matter/config/nrfconnect/`
* Shared |NCS| infrastructure used by Matter (OpenThread, MCUboot integration, IPC radio, factory data generation in ``sdk-nrf``)
* Core |NCS| documentation links and software maturity information for Matter platform features

Only the Matter reference samples, their shared sample code, snippets, and partition devicetree assets move to ``ncs-matter``.

Migration checklist
*******************

Use this checklist when migrating a custom Matter application:

#. Set up a west workspace that includes the ``ncs-matter`` add-on at a release matching your target |NCS| version.
#. Move or copy your application sources to a path under your workspace (or use an add-on sample as the new base).
#. Update :file:`CMakeLists.txt`:

   * Include :file:`ncs-matter/cmake/sample.cmake` instead of the former ``samples/matter/common/cmake/*`` files.
   * Replace ``ZEPHYR_NRF_MODULE_DIR`` Matter paths with ``ZEPHYR_NCS_MATTER_MODULE_DIR``.

#. Update :file:`Kconfig` files: remove the ``samples/matter/common/src/Kconfig`` source line.
#. Rename all ``CONFIG_NCS_SAMPLE_MATTER_*`` symbols to ``CONFIG_MATTER_*`` in configuration files.
#. Update devicetree board overlays: replace ``#include <samples/matter/...>`` with ``#include <...>`` using the filenames from :file:`ncs-matter/dts/`.
#. Update snippet names in build commands (``matter-debug`` → ``debug``, ``matter-diagnostic-logs`` → ``diagnostic-logs``).
#. Update ``west build`` paths to point to the new sample location.
#. Copy or merge :file:`sysbuild_internal.conf` defaults from the matching add-on sample if needed.
#. Rebuild from a pristine build directory and verify commissioning, DFU, and factory data workflows.
