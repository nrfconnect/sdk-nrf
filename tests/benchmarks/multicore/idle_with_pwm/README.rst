.. _multicore_idle_with_pwm_test:

PWM in low power states test
############################

.. contents::
   :local:
   :depth: 2

This test benchmarks the idle behavior of an application that runs on multiple cores and uses the PWM driver to light up a LED.

The test scenario repeats forever the following:

* Gradually increase PWM duty cycle from 0% to 50% for one second (the LED lights up), power state active.
* Set PWM duty cycle to 0% for one second (PWM disabled, the LED is OFF), power state low.

Requirements
************

The test supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54h20dk_nrf54h20_cpuapp

Overview
********

The test demonstrates the use of PWM with low power modes.

The code stored in the :file:`main.c` file is compiled for both application and radio cores.
The application core uses pwm130 to generate PWM signal on **LED2**.
The radio core uses pwm131 to generate PWM signal on GPIO port 0, pin 7.

There are three test configurations in the :file:`testcase.yaml`.

* ``benchmarks.multicore.idle_with_pwm.nrf54h20dk_cpuapp_cpurad.s2ram``

  This configuration uses Kconfig options that enable entering low power modes.
  Logging is disabled.
  Core sleeps for time sufficient to enter the ``suspend-to-ram`` power state.

* ``benchmarks.multicore.idle_with_pwm.nrf54h20dk_cpuapp_cpurad.sleep``

  This configuration uses Kconfig options that enable entering low power modes.
  Logging is disabled.
  Core sleeps for time sufficient to enter the ``suspend-to-idle`` power state.

* ``benchmarks.multicore.idle_with_pwm.nrf54h20dk_cpuapp_cpurad.no_sleep``

  You can use this configuration for debug purposes.
  Logging is enabled while power mode is disabled.

Building and running
********************

.. |test path| replace:: :file:`tests/benchmarks/multicore/idle_with_pwm`

.. include:: /includes/build_and_run_test.txt

To build the test, use configuration setups from the :file:`testcase.yaml` file using the ``-T`` option.
See the example:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -T benchmarks.multicore.idle_with_pwm.nrf54h20dk_cpuapp_cpurad.s2ram .

Testing
=======

After programming the test to your development kit, complete the following steps to test it:

1. Connect the PPK2 Power Profiler Kit or other current measurement device.
#. Reset the kit.
#. Observe the **LED2** brightness (or use oscilloscope to collect the PWM signal on port 0, pin 7).
#. When the **LED2** is off, the power profiler indicates low current consumption resulting from both cores being in low power mode.
