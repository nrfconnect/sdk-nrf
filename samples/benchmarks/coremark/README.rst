.. _coremark_sample:

CoreMark
########

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to run the `CoreMark®`_ benchmark to evaluate the performance of a core.
To get started with CoreMark integration in |NCS|, see :ref:`ug_coremark`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample runs the CoreMark benchmark on the target CPU.
CoreMark evaluates the CPU efficiency by performing different algorithms, such as state machine, CRC calculation, matrix manipulation, and list processing (find and sort).

To run the CoreMark benchmark on the preferred core, press the corresponding button.
For the button assignment, see the :ref:`coremark_user_interface` section.
When the benchmark has completed, you can press the same button to restart it.
If you want to run the sample upon startup, enable the :ref:`CONFIG_APP_MODE_FLASH_AND_RUN <CONFIG_APP_MODE_FLASH_AND_RUN>` Kconfig option.

.. _coremark_user_interface:

User interface
**************

Each target CPU has an assigned button responsible for starting the benchmark and LED that indicates the ``test in progress`` state:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Start the benchmark run on the application core.

      Button 2:
         Start the benchmark run on the network or radio core.

      LED 1:
         Indicates ``test in progress`` on the application core.

      LED 2:
         Indicates ``test in progress`` on the network or radio core.

   .. group-tab:: nRF54 DKs

      Button 0:
         Start the benchmark run on the application core.

      Button 1:
         Start the benchmark run on the network or radio core.

      LED 0:
         Indicates ``test in progress`` on the application core.

      LED 1:
         Indicates ``test in progress`` on the network or radio core.

.. _coremark_configuration:

Configuration
*************

|config|

CoreMark runs tests multiple times.
You can define the number of iterations using the :kconfig:option:`CONFIG_COREMARK_ITERATIONS` Kconfig option.
By default, the iteration quantity is set to the minimum time required for valid results, which is 10 seconds.

Additional configuration
========================

Check and configure the following configuration options that are used by the sample:

Run types and data sizes
------------------------

CoreMark has the following predefined run types and data sizes that are used for data algorithms:

* The :kconfig:option:`CONFIG_COREMARK_RUN_TYPE_PERFORMANCE` Kconfig option - Sets :kconfig:option:`CONFIG_COREMARK_DATA_SIZE` to 2000 bytes.
* The :kconfig:option:`CONFIG_COREMARK_RUN_TYPE_PROFILE` Kconfig option - Sets :kconfig:option:`CONFIG_COREMARK_DATA_SIZE` to 1200 bytes.
* The :kconfig:option:`CONFIG_COREMARK_RUN_TYPE_VALIDATION` Kconfig option - Sets :kconfig:option:`CONFIG_COREMARK_DATA_SIZE` to 500 bytes.

You can also specify a custom :kconfig:option:`CONFIG_COREMARK_DATA_SIZE` value and submit your results by following the rules from the `CoreMark GitHub`_ repository.
Make sure that when setting the :kconfig:option:`CONFIG_COREMARK_DATA_SIZE` Kconfig option the required memory space is available.

Memory allocation methods
-------------------------

You can select the following memory allocation methods:

* The :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_STACK` Kconfig option uses the main thread stack for data allocation and is enabled by default.
  You can change the main stack size by setting the :kconfig:option:`CONFIG_MAIN_STACK_SIZE` Kconfig option.
* The :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_MALLOC` Kconfig option uses Zephyr :c:func:`k_malloc` or :c:func:`k_free` functions to allocate the memory in the heap.
  You can adjust the heap size using the :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option.
* The :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_STATIC` option allows data allocation in the RAM as a regular static variable.
  In this case, you do not need to be aware of the main thread stack size or heap memory pool size.

Multithread mode
----------------

CoreMark can also be executed in multithread mode.
To specify a number of threads, use the :kconfig:option:`CONFIG_COREMARK_THREADS_NUMBER` Kconfig option.
In multithread mode, CoreMark is executed in all threads simultaneously.
Each thread runs number of iterations equal to :kconfig:option:`CONFIG_COREMARK_ITERATIONS`.
By default, the :kconfig:option:`CONFIG_TIMESLICE_SIZE` Kconfig option is set to 10 ms, which imitates RTOS multithread usage.
However, in the final report, the thread execution is combined.
In the multithread mode, the :kconfig:option:`CONFIG_COREMARK_DATA_SIZE` Kconfig option is allocated for each thread separately.
Only the :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_STACK` and :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_MALLOC` memory methods can be used with multiple threads.
In case of the :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_STACK` Kconfig option, the data for all threads is allocated to the main thread stack.

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_APP_MODE_FLASH_AND_RUN:

CONFIG_APP_MODE_FLASH_AND_RUN - Start CoreMark sample automatically after flashing
   If enabled, CoreMark starts execution immediately after the CPU starts up.
   It also disables LEDs and buttons.
   Otherwise, it will wait for the button press.

.. note::
   The :kconfig:option:`CONFIG_APP_MODE_FLASH_AND_RUN` Kconfig option is always enabled for the PPR core.
   This core does not use buttons.

.. _SB_CONFIG_APP_CPUNET_RUN:

SB_CONFIG_APP_CPUNET_RUN - Enable execution for the network core or the radio core
   Enable the benchmark execution for the network core for targets with the nRF53 Series SoCs, and for the radio core on targets with the nRF54H20 SoCs.

.. _SB_CONFIG_APP_CPUPPR_RUN:

SB_CONFIG_APP_CPUPPR_RUN - Enable execution for the PPR core
   Enable the benchmark execution also for the PPR core for targets with the nRF54H20 SoCs.

.. note::
   PPR code is run from RAM.
   You must use the ``nordic-ppr`` snippet for the application core to be able to boot the PPR core.
   Use the build argument ``coremark_SNIPPET=nordic-ppr``.
   To build the sample with the execution for the PPR core enabled, run the following command:

   .. code-block:: console

      west build -b nrf54h20dk/nrf54h20/cpuapp -- -DSB_CONFIG_APP_CPUNET_RUN=n -DSB_CONFIG_APP_CPUPPR_RUN=y -Dcoremark_SNIPPET=nordic-ppr

Building and running
********************

When running the benchmark, an extra build flag (:kconfig:option:`CONFIG_COMPILER_OPT`) is set to ``-O3`` to achieve the best CoreMark results.

.. |sample path| replace:: :file:`samples/benchmarks/coremark`

.. include:: /includes/build_and_run.txt

After flashing, messages describing the benchmark state will appear in the console.

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset your development kit.
#. To start the test, press the button assigned to the respective core.
   For button assignment, refer to the :ref:`coremark_user_interface` section.

   All target cores work independently from each other, and it is possible to run the benchmark on several cores simultaneously.
   Measurements running on a core are indicated by the corresponding LED.
   If the :ref:`CONFIG_APP_MODE_FLASH_AND_RUN <CONFIG_APP_MODE_FLASH_AND_RUN>` Kconfig option is enabled, the measurement will launch automatically on all available cores when starting the application.
   In :ref:`CONFIG_APP_MODE_FLASH_AND_RUN <CONFIG_APP_MODE_FLASH_AND_RUN>` mode LEDs and buttons are not used.

#. Wait for all measurements to complete.
   By default, the test takes approximately 11-13 seconds.
#. Wait for the console output for all tested cores.
   The results will be similar to the following example:

      .. code-block:: console

         *** Booting Zephyr OS build bf606fc00ec1  ***
         [00:00:00.502,166] <inf> app: Coremark sample for nrf52dk/nrf52832. Call address: 00007fa1
         [00:00:00.502,197] <inf> app: Press Push button switch 0 to start the test ...
         [00:00:14.483,764] <inf> app: Push button switch 0 pressed!
         [00:00:14.511,627] <inf> app: Coremark started!
         [00:00:14.511,627] <inf> app: CPU FREQ: 64000000 Hz
         [00:00:14.511,627] <inf> app: (threads: 1, data size: 2000; iterations: 2000)

         2K performance run parameters for coremark.
         CoreMark Size    : 666
         Total ticks      : 401312
         Total time (secs): 12.247070
         Iterations/Sec   : 163.304366
         Iterations       : 2000
         Compiler version : GCC10.3.0
         Compiler flags   : -O3 + see compiler flags added by Zephyr
         Memory location  : STACK
         seedcrc          : 0xe9f5
         [0]crclist       : 0xe714
         [0]crcmatrix     : 0x1fd7
         [0]crcstate      : 0x8e3a
         [0]crcfinal      : 0x4983
         Correct operation validated. See README.md for run and reporting rules.
         CoreMark 1.0 : 163.304366 / GCC10.3.0 -O3 + see compiler flags added by Zephyr / STACK
         [00:00:27.759,582] <inf> app: Coremark finished! Press Push button switch 0 to restart ...
