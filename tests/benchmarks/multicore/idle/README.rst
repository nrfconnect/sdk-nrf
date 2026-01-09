.. _multicore_idle_test:

Multicore idle test
###################

.. contents::
   :local:
   :depth: 2

The test benchmarks the idle behavior of an application that runs on multiple cores.
It uses a system timer as a wake-up source.

Requirements
************

The test supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf54h20dk_nrf54h20_cpuapp

Overview
********

The test demonstrates how to build a multicore idle application with :ref:`configuration_system_overview_sysbuild`.

When building with sysbuild, the build system adds images based on the options selected in the project's additional configuration and build files.
This test shows how to inform the build system about dedicated sources for additional images.
The test comes with the following additional files:

* :file:`Kconfig.sysbuild` - This file is used to add :ref:`sysbuild Kconfig options <configuration_system_overview_sysbuild>` that are passed to all the images.
* :file:`sysbuild.cmake` - The CMake file adds additional images using the :c:macro:`ExternalZephyrProject_Add` macro.
  You can also add the dependencies for the images if required.

Both the application and remote cores use the same :file:`main.c` that prints the name of the DK on which the application is programmed.

Building and running
********************

.. |test path| replace:: :file:`tests/benchmarks/multicore/idle`

.. include:: /includes/build_and_run_test.txt

To build the test, use configuration setups from :file:`testcase.yaml` using the ``-T`` option.
See the following examples:

nRF5340 DK
  You can build the test for application and network cores as follows:

  .. code-block:: console

     west build -p -b nrf5340dk/nrf5340/cpuapp -T benchmarks.multicore.idle.nrf5340dk_cpuapp_cpunet .

nRF54H20 DK
  You can build the test for application and radio cores as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T benchmarks.multicore.idle.nrf54h20dk_cpuapp_cpurad .

  To achieve the lowest power consumption, use the ``benchmarks.multicore.idle.nrf54h20dk_cpuapp_cpurad.s2ram`` configuration.
  This configuration does not provide console output.
  Use the following build command:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T benchmarks.multicore.idle.nrf54h20dk_cpuapp_cpurad.s2ram .

  You can build the test for application and PPR cores as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T benchmarks.multicore.idle.nrf54h20dk_cpuapp_cpuppr .

  .. note::

     :ref:`zephyr:nordic-ppr` is used in the configuration above to automatically launch PPR core from the application core.

  An additional configuration setup is provided to execute code directly from the non-volatile memory (MRAM) on the PPR core.
  This configuration uses :ref:`zephyr:nordic-ppr-xip` and enables :kconfig:option:`CONFIG_XIP` on the PPR core.
  You can build the sample as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T benchmarks.multicore.idle.nrf54h20dk_cpuapp_cpuppr_xip .

Testing
=======

After programming the test to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output for both cores:

   * For the application core, the output should be as follows:

     .. code-block:: console

       *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
       Multi-core idle test on nrf5340dk/nrf5340/cpuapp
       Multi-core idle test iteration 0
       Multi-core idle test iteration 1
       ...

   * For the remote core, the output should be as follows:

     .. code-block:: console

        *** Booting nRF Connect SDK zephyr-v3.5.0-3517-g9458a1aaf744 ***
        Multi-core idle test on nrf5340dk/nrf5340/cpunet
        Multi-core idle test iteration 0
        Multi-core idle test iteration 1
        ...
