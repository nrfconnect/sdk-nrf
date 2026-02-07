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

Logging
=======

The logging mode depends on the chosen board target.
The sample supports two distinct modes that are described in the following subsections.

Standard logging
----------------

This logging mode is used by most board targets.
Each core running the CoreMark benchmark has an independent UART instance that is used for logging.

To see all logging information for the multi-core board targets, you must open a terminal for each active core.

The sample configuration sets up the following board targets for standard logging:

* ``nrf52840dk/nrf52840``
* ``nrf52833dk/nrf52833``
* ``nrf52dk/nrf52832``
* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54l15dk/nrf54l05/cpuapp``
* ``nrf54l15dk/nrf54l10/cpuapp``
* ``nrf54l15dk/nrf54l15/cpuapp``
* ``nrf54lm20dk/nrf54lm20a/cpuapp``

Multi-domain logging
--------------------

This logging mode is used by multi-core board targets that support logging using the ARM Coresight STM.
Each core running the CoreMark benchmark writes its logging information to its own set of STM Extended Stimulus Port (STMESP).
One core in the system is designated to collect all logs and to send them to the chosen UART instance.
The sample supports multi-domain logging in the standalone mode.
See :ref:`zephyr:logging_cs_stm` for more details.

To see all logging information in this logging mode, it is enough to open one terminal.
When the core used for sending the logs to UART is running the CoreMark benchmark, the logging activity is blocked until the benchmark has completed.

The sample configuration sets up the following board targets for multi-domain logging:

* ``nrf54h20dk/nrf54h20/cpuapp``

.. _coremark_user_interface:

User interface
**************

Each target CPU has an assigned button responsible for starting the benchmark and LED that indicates the ``test in progress`` state:

.. tabs::

   .. group-tab:: nRF52 DKs

      Application core: **Button 1** and **LED 1**

   .. group-tab:: nRF53 DKs

      Application core: **Button 1** and **LED 1**

      Network core: **Button 2** and **LED 2**.

   .. group-tab:: nRF54L DKs

      Application core: **Button 0** and **LED 0**

      FLPR core: **Button 3** and **LED 3**

        This UI is currently only supported for the following board targets:

        * ``nrf54l15dk/nrf54l15/cpuapp``
        * ``nrf54lm20dk/nrf54lm20a/cpuapp``

   .. group-tab:: nRF54H DKs

      Application core: **Button 0** and **LED 0**

      Radio core: **Button 1** and **LED 1**

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
   The :kconfig:option:`CONFIG_APP_MODE_FLASH_AND_RUN` Kconfig option is always enabled for the PPR and FLPR cores on the ``nrf54h20dk/nrf54h20/cpuapp`` board target.
   These cores on the ``nrf54h20dk/nrf54h20/cpuapp`` board target do not use the on-board buttons and LEDs.

.. _SB_CONFIG_APP_CPUFLPR_RUN:

SB_CONFIG_APP_CPUFLPR_RUN - Enable the benchmark execution also for the FLPR core
   This option is only available for board targets that support the FLPR core (for example, ``nrf54l15dk/nrf54l15/cpuapp``) in this sample.

.. note::
   FLPR code is run from RAM.

   This option is not supported for the following board targets that include an SoC with the FLPR core:

     * ``nrf54l15dk/nrf54l05/cpuapp``
     * ``nrf54l15dk/nrf54l10/cpuapp``

.. _SB_CONFIG_APP_CPUNET_RUN:

SB_CONFIG_APP_CPUNET_RUN - Enable the benchmark execution also for the network core or the radio core
   This option is only available for board targets that support the network core (for example, ``nrf5340dk/nrf5340/cpuapp``) or radio core (for example, ``nrf54h20dk/nrf54h20/cpuapp``) in this sample .

.. _SB_CONFIG_APP_CPUPPR_RUN:

SB_CONFIG_APP_CPUPPR_RUN - Enable the benchmark execution also for the PPR core
   This option is only available for board targets that support the PPR core (for example, ``nrf54h20dk/nrf54h20/cpuapp``) in this sample.

.. note::
   PPR code is run from RAM.

Building and running
********************

When running the benchmark, an extra build flag (:kconfig:option:`CONFIG_COMPILER_OPT`) is set to ``-O3`` to achieve the best CoreMark results.

.. |sample path| replace:: :file:`samples/benchmarks/coremark`

.. include:: /includes/build_and_run.txt

After flashing, messages describing the benchmark state will appear in the console.

Alternative build configurations
================================

The default sample configuration uses the following settings for the benchmark execution:

* Memory allocation using the stack memory (see the :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_STACK` Kconfig option)
* Single thread (see the :kconfig:option:`CONFIG_COREMARK_THREADS_NUMBER` Kconfig option)
* Manual benchmark execution on a button press

You can build the default configuration using the following command:

.. parsed-literal::
   :class: highlight

   west build -b *board_target*

The following subsections describe alternative configurations.
The deviation from the default configuration is described.
Each subsection also provides a build command that demonstrates how to build the described alternative configuration.

Flash and run configuration
---------------------------

The flash and run configuration changes the trigger for the benchmark execution.
In this configuration, the benchmark is automatically started on all supported cores after the system boot.

You can build this configuration using the following command:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=flash_and_run

Heap memory configuration
-------------------------

The heap memory configuration changes the method for memory allocation during the benchmark execution.
In this configuration, the benchmark allocates the memory using the heap memory (see the :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_MALLOC` Kconfig option).

You can build this configuration using the following command:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=heap_memory

Static memory configuration
---------------------------

The static memory configuration changes the method for memory allocation during the benchmark execution.
In this configuration, the benchmark allocates the memory using the static memory (see the :kconfig:option:`CONFIG_COREMARK_MEMORY_METHOD_STATIC` Kconfig option).

You can build this configuration using the following command:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=static_memory

Multiple thread configuration
-----------------------------

The multiple thread configuration uses more than one thread during the benchmark execution (see the :kconfig:option:`CONFIG_COREMARK_THREADS_NUMBER` Kconfig option).

You can build this configuration using the following command:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=multiple_threads

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|

   .. note::
      To see all logging information for the multi-core board targets and the standard logging mode, you must open a terminal for each active core.
      The ``nrf5340dk/nrf5340/cpuapp`` is an example of such board target.

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

   .. tabs::

      .. group-tab:: Standard logging

         .. code-block:: console

            *** Booting nRF Connect SDK v2.8.99-bd4a30a3a758 ***
            *** Using Zephyr OS v3.7.99-02718211f9a9 ***
            [00:00:00.261,383] <inf> app: Standard logging mode

            [00:00:00.266,967] <inf> app: CoreMark sample for nrf52840dk/nrf52840
            [00:00:00.274,139] <inf> app: Press Push button switch 0 to start the test ...

            [00:00:01.267,608] <inf> app: Push button switch 0 pressed!
            [00:00:01.273,864] <inf> app: CoreMark started! CPU FREQ: 64000000 Hz, threads: 1, data size: 2000; iterations: 2000

            2K performance run parameters for coremark.
            CoreMark Size    : 666
            Total ticks      : 401215
            Total time (secs): 12.244000
            Iterations/Sec   : 163.345312
            Iterations       : 2000
            Compiler version : GCC12.2.0
            Compiler flags   : -O3 + see compiler flags added by Zephyr
            Memory location  : STACK
            seedcrc          : 0xe9f5
            [0]crclist       : 0xe714
            [0]crcmatrix     : 0x1fd7
            [0]crcstate      : 0x8e3a
            [0]crcfinal      : 0x4983
            Correct operation validated. See README.md for run and reporting rules.
            CoreMark 1.0 : 163.345312 / GCC12.2.0 -O3 + see compiler flags added by Zephyr / STACK
            [00:00:13.597,778] <inf> app: CoreMark finished! Press Push button switch 0 to restart ...

      .. group-tab:: Multi-domain logging

         .. code-block:: console

            *** Booting nRF Connect SDK v2.8.99-f9add8e14565 ***
            *** Using Zephyr OS v3.7.99-02718211f9a9 ***
            [00:00:00.208,166] <inf> app/app: Multi-domain logging mode
            [00:00:00.208,168] <inf> app/app: This core is used to output logs from all cores to terminal over UART

            [00:00:00.208,441] <inf> ppr/app: CoreMark sample for nrf54h20dk@0.9.0/nrf54h20/cpuppr
            [00:00:00.208,496] <inf> ppr/app: CoreMark started! CPU FREQ: 16000000 Hz, threads: 1, data size: 2000; iterations: 500

            [00:00:01.186,256] <inf> rad/app: CoreMark sample for nrf54h20dk@0.9.0/nrf54h20/cpurad
            [00:00:01.186,305] <inf> rad/app: Press Push button 1 to start the test ...

            [00:00:01.285,614] <inf> app/app: CoreMark sample for nrf54h20dk@0.9.0/nrf54h20/cpuapp
            [00:00:01.285,654] <inf> app/app: Press Push button 0 to start the test ...

            [00:00:04.984,744] <inf> app/app: Push button 0 pressed!
            [00:00:04.984,753] <inf> app/app: CoreMark started! CPU FREQ: 320000000 Hz, threads: 1, data size: 2000; iterations: 10000

            [00:00:04.984,755] <inf> app/app: Logging is blocked for all cores until this core finishes the CoreMark benchmark

            [00:00:05.714,470] <inf> rad/app: Push button 1 pressed!
            [00:00:05.714,486] <inf> rad/app: CoreMark started! CPU FREQ: 256000000 Hz, threads: 1, data size: 2000; iterations: 10000

            2K performance run parameters for coremark.
            CoreMark Size    : 666
            Total ticks      : 13471150
            Total time (secs): 13.471000
            Iterations/Sec   : 37.116769
            Iterations       : 500
            Compiler version : GCC12.2.0
            Compiler flags   : -O3 + see compiler flags added by Zephyr
            Memory location  : STACK
            seedcrc          : 0xe9f5
            [0]crclist       : 0xe714
            [0]crcmatrix     : 0x1fd7
            [0]crcstate      : 0x8e3a
            [0]crcfinal      : 0xa14c
            Correct operation validated. See README.md for run and reporting rules.
            CoreMark 1.0 : 37.116769 / GCC12.2.0 -O3 + see compiler flags added by Zephyr / STACK
            [00:00:13.595,072] <inf> ppr/app: CoreMark finished! Press the reset button to restart...

            2K performance run parameters for coremark.
            CoreMark Size    : 666
            Total ticks      : 11436744
            Total time (secs): 11.436000
            Iterations/Sec   : 874.431619
            Iterations       : 10000
            Compiler version : GCC12.2.0
            Compiler flags   : -O3 + see compiler flags added by Zephyr
            Memory location  : STACK
            seedcrc          : 0xe9f5
            [0]crclist       : 0xe714
            [0]crcmatrix     : 0x1fd7
            [0]crcstate      : 0x8e3a
            [0]crcfinal      : 0x988c
            Correct operation validated. See README.md for run and reporting rules.
            CoreMark 1.0 : 874.431619 / GCC12.2.0 -O3 + see compiler flags added by Zephyr / STACK
            [00:00:16.446,916] <inf> app/app: CoreMark finished! Press Push button 0 to restart ...

            2K performance run parameters for coremark.
            CoreMark Size    : 666
            Total ticks      : 14290211
            Total time (secs): 14.290000
            Iterations/Sec   : 699.790063
            Iterations       : 10000
            Compiler version : GCC12.2.0
            Compiler flags   : -O3 + see compiler flags added by Zephyr
            Memory location  : STACK
            seedcrc          : 0xe9f5
            [0]crclist       : 0xe714
            [0]crcmatrix     : 0x1fd7
            [0]crcstate      : 0x8e3a
            [0]crcfinal      : 0x988c
            Correct operation validated. See README.md for run and reporting rules.
            CoreMark 1.0 : 699.790063 / GCC12.2.0 -O3 + see compiler flags added by Zephyr / STACK
            [00:00:19.911,390] <inf> rad/app: CoreMark finished! Press Push button 1 to restart ...
