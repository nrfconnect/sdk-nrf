.. _snippet-coverage-support:

Coverage support snippet (coverage_support)
###########################################

.. contents::
   :local:
   :depth: 2

The ``coverage_support`` snippet enables GCOV code coverage collection and applies the board-specific stack sizes that an instrumented build requires.

Overview
********

Code coverage instrumentation increases both the code size and the stack usage of every instrumented function, which makes an otherwise working application overflow its stacks.
For each supported board target, the snippet applies the following configuration:

* Enables coverage (:kconfig:option:`CONFIG_COVERAGE_GCOV`) and forces it on for the build (:kconfig:option:`CONFIG_FORCE_COVERAGE`), with the GCOV heap taken from the system heap (:kconfig:option:`CONFIG_COVERAGE_GCOV_HEAP_SIZE` set to ``0``).
* Increases the main, ISR, idle, privileged, shell, test, and Ztest stack sizes.
* Enables size optimizations (:kconfig:option:`CONFIG_SIZE_OPTIMIZATIONS`) to offset the code size that the instrumentation adds.
* Adds a devicetree overlay that reserves the additional RAM the instrumented image needs.

Supported board targets
***********************

The snippet supports the following board targets:

* ``nrf52840dk/nrf52840``
* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf54h20dk/nrf54h20/cpuapp`` and ``nrf54h20dk/nrf54h20/cpurad``
* ``nrf54l15dk/nrf54l15/cpuapp``
* ``nrf54lm20dk/nrf54lm20a/cpuapp`` and ``nrf54lm20dk/nrf54lm20b/cpuapp``
* ``nrf54ls05dk/nrf54ls05b/cpuapp``
* ``nrf54lv10dk/nrf54lv10a/cpuapp``
* ``nrf7120dk/nrf7120/cpuapp``

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -S coverage_support [...]
