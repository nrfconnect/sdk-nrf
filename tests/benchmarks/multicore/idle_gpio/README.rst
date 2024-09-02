.. _multicore_idle_gpio_test:

Multicore idle GPIO test
########################

.. contents::
   :local:
   :depth: 2

The test benchmarks the idle behavior of an application that runs on multiple cores.
It uses a pin as a wake-up source.

Requirements
************

The test supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

Overview
********

The test demonstrates how to build a multicore idle application with :ref:`configuration_system_overview_sysbuild`.

When building with sysbuild, the build system adds child images based on the options selected in the project's additional configuration and build files.
This test shows how to inform the build system about dedicated sources for additional images.
The test comes with the following additional files:

* :file:`Kconfig.sysbuild` - This file is used to add :ref:`sysbuild Kconfig options <configuration_system_overview_sysbuild>` that are passed to all the images.
* :file:`sysbuild.cmake` - The CMake file adds additional images using the :c:macro:`ExternalZephyrProject_Add` macro.
  You can also add the dependencies for the images if required.

Both the application and remote cores use the same :file:`main.c` that prints the name of the DK on which the application is programmed.

Building and running
********************

.. |test path| replace:: :file:`tests/benchmarks/multicore/idle_gpio`

.. include:: /includes/build_and_run_test.txt

The remote board must be specified using ``SB_CONFIG_REMOTE_BOARD``.
To build the test, use configuration setups from :file:`testcase.yaml` using the ``-T`` option.
See the following examples:

nRF54H20 DK
  You can build the test for application and radio cores as follows:

  .. code-block:: console

     west build -p -b nrf54h20dk/nrf54h20/cpuapp -T benchmarks.multicore.idle_gpio.nrf54h20dk_cpuapp_cpurad.s2ram .

Testing
=======

After programming the test to your development kit, complete the following steps to test it:

#. Connect the PPK2 Power Profiler Kit or other current measurement device.
#. Reset the kit.
#. Observe the low current consumption as all cores are suspended.
#. Press a button to execute the user callback.
   The device will remain in active state for 1 second.

   * **Button 0** - Wakes up the application core.
   * **Button 1** - Wakes up the radio core.
