.. _rtfw_timer_gpio_sample:

.. ncs-sample::
   :title: RTFW TIMER/GPIO source adapter

   The TIMER/GPIO sample demonstrates the control and low-latency hardware response model described in :ref:`lib_rtfw`.
   An owned TIMER triggers the source ZLI, where the sample toggles a GPIO, while a Zephyr shell provides runtime control.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

To observe the GPIO output timing, use a logic analyzer or oscilloscope.
The nRF54LM20 DK coexistence test also requires a Bluetooth LE-capable host.

Board-specific resources
========================

The sample uses the following hardware resources on each supported development kit.

nRF54LM20 DK
-------------

* TIMER20 is the direct interrupt source.
* EGU20 is the RT-to-Zephyr doorbell.
* **P1.04** is the toggled output.
* **P1.05** is the optional ISR debug output.
* TIMER20 and EGU20 are disabled in devicetree so no Zephyr driver claims them.
* Bluetooth and LBS are enabled so coexistence tests can keep the SoftDevice Controller and MPSL active.

nRF54H20 DK
------------

* TIMER130 is the direct interrupt source.
* EGU130 is the RT-to-Zephyr doorbell.
* **P0.00** is the toggled output.
* **P0.01** is the optional ISR debug output.
* TIMER130 and EGU130 are reserved for the application core.
* Bluetooth is disabled in this app-core-only diagnostic configuration.

The overlays define the peripheral ownership.
Check the SoC reservations and MPSL usage before changing a peripheral or pin.

Overview
********

The source adapter starts disabled with a default period of 500 ms.
Start, stop, and period commands software-pend the TIMER IRQ, so they are processed even while the timer is stopped.
An active COMPARE0 event is cleared in the ZLI, the owned output is toggled, and an optional decimated telemetry event is published.
The configured period is the interval between output transitions, so one complete output cycle takes twice the configured period.

When the fast path processes a valid configuration command, it stops and clears the timer, clears COMPARE0, and writes CC0.
If the resulting configuration is enabled, the timer starts a new phase, including when an additional start command is issued while it is already running.
If the resulting configuration is disabled, the output is driven low.
Stopping while the output is high can therefore create a final transition.
This prevents a new compare value from being stranded below the current counter until the 32-bit timer wraps.

Limitations
===========

This diagnostic source adapter is not a general Zephyr timer replacement.
It does not support system PM and rejects :kconfig:option:`CONFIG_PM` at build time.

User interface
**************

The sample registers the following shell commands:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Command
     - Description
   * - ``rtfw start``
     - Starts the TIMER/GPIO source adapter
   * - ``rtfw stop``
     - Stops the TIMER/GPIO source adapter
   * - ``rtfw period <microseconds>``
     - Sets the timer period in microseconds
   * - ``rtfw status``
     - Displays the requested and applied command state, pending state, timer-event count, dropped events, maximum queue depth, and framework faults

The ``rtfw status`` command distinguishes requested, applied, and pending command state and reports cumulative timer-event and queue diagnostics.
See :ref:`lib_rtfw_command_status` for command-state terminology.

On the nRF54LM20 DK, the peripheral advertises as ``Nordic_RTFW_Timer`` and includes the LBS UUID.
The LBS service is included for coexistence testing and does not control the timer or telemetry.

The development kit LEDs have the following roles:

**LED 1**:
   Blinks as the Zephyr heartbeat.

**LED 2**:
   Lights when a Bluetooth connection is active on the nRF54LM20 DK.

**LED 3**:
   Toggles when a processed-command event or decimated tick event is delivered to Zephyr.

Configuration
*************

|config|

.. options-from-kconfig::
   :show-type:

Timer and instrumentation options
=================================

The accepted period range defaults to 10 µs through 1,000,000 µs and is configured by :option:`CONFIG_SAMPLE_RTFW_TIMER_MIN_PERIOD_US` and :option:`CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US`.
The minimum period must provide at least two 1 MHz TIMER ticks, the minimum must not exceed the maximum, and the default 500 ms period must remain inside the configured range.
Invalid combinations are rejected at build time rather than silently clamped by the fast path.
The shell rejects period requests outside the configured range at runtime.

Tick telemetry is disabled by default so the data plane is not exercised on every timer interrupt.
Set :option:`CONFIG_SAMPLE_RTFW_TIMER_TELEMETRY_DECIMATION` to the required interval, such as ``16`` to publish one event for every 16 timer ticks.

Enable :option:`CONFIG_SAMPLE_RTFW_DEBUG_PIN` to assert the board-specific debug output around the direct source ISR.
Measure the toggled output and debug output together to distinguish output timing from ISR execution.
The pulse measures direct ISR execution time but excludes TIMER-event-to-ISR-entry latency.

The :option:`CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD` option adds a controlled NOP loop to the source ISR.
Use the :option:`CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD_CYCLES` option to select its size.
This option is for stress testing and should remain disabled for baseline measurements.
The board configurations select ZLI priorities according to the requirements in :ref:`lib_rtfw_interrupt_priority`.

Building and running
********************

.. |sample path| replace:: :file:`samples/subsys/rtfw/timer_gpio`

.. include:: /includes/build_and_run.txt

Command-line builds
===================

Use the tab for your development kit:

.. tabs::

   .. group-tab:: nRF54LM20 DK

      Run the following commands:

      .. code-block:: console

         west build -b nrf54lm20dk/nrf54lm20b/cpuapp \
           nrf/samples/subsys/rtfw/timer_gpio
         west flash

   .. group-tab:: nRF54H20 DK

      Run the following commands to build the sysbuild configuration:

      .. code-block:: console

         west build --sysbuild -b nrf54h20dk/nrf54h20/cpuapp \
           nrf/samples/subsys/rtfw/timer_gpio
         west flash

Use a separate build directory or a pristine build when switching targets or variants.

Testing
========

|test_sample|

Timer control
-------------

#. |connect_terminal|
#. Connect a logic analyzer or oscilloscope to the output pin.
#. Run ``rtfw status`` and verify that the timer is initially stopped with a 500 ms period.
#. Run ``rtfw start`` and verify that output transitions occur every 500 ms, resulting in a complete output cycle every second.
#. Change the period and verify that the next transition belongs to a new timer phase.
#. Run ``rtfw stop`` and verify that transitions stop and the output is driven low.
#. Change the period while stopped, then start again and inspect the requested, applied, and pending state.

Period reconfiguration
----------------------

Configure a long period, start the timer, and then request the minimum period before the old compare expires.
Verify that the timer starts a new phase and does not wait for a 32-bit timer wrap.

On the nRF54LM20 DK, connect a Bluetooth LE central to ``Nordic_RTFW_Timer`` and keep the connection active during this test to exercise coexistence with MPSL.

Telemetry delivery
-------------------

Enable telemetry decimation and run ``rtfw status`` during normal operation and an event storm.
Verify the following:

* **LED 3** changes on delivered telemetry.
* ``max_depth`` reflects queue use.
* ``dropped`` remains zero under the intended load.
* Any overflow is visible through counters and fault bits rather than blocking the ZLI.

Dependencies
************

This sample uses the following |NCS| components:

* :ref:`lib_rtfw`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`lbs_readme` on the **nRF54LM20 DK**

On the nRF54LM20 DK, it also uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:softdevice_controller`
* :ref:`nrfxlib:mpsl`

It also uses the following Zephyr components:

* :ref:`zephyr:kernel_api`
* :ref:`zephyr:shell_api`
* :ref:`zephyr:bluetooth_api` on the **nRF54LM20 DK**

The sample also uses hardware abstraction layers from `nrfx`_.
