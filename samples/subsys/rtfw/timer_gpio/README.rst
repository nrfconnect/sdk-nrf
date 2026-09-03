.. _rtfw_timer_gpio_sample:

RTFW TIMER/GPIO source adapter
##############################

The TIMER/GPIO sample demonstrates the control and fast-actuation model
described in :ref:`rtfw_concept`. An owned TIMER enters the source ZLI, where
the sample toggles a GPIO, while a Zephyr shell provides runtime control.

Overview
********

The source adapter starts disabled with a default period of 500 ms. Start,
stop, and period commands software-pend the TIMER IRQ, so they are processed
even while the timer is stopped. An active COMPARE0 event is cleared in the
ZLI, the owned output is toggled, and an optional decimated telemetry event is
published.

Reprogramming stops and clears the timer, clears COMPARE0, writes CC0, and
starts a new phase when requested. This prevents a new compare value from
being stranded below the current counter.

Hardware resources
******************

nRF54LM20 DK
============

* TIMER20 is the direct interrupt source.
* EGU20 is the RT-to-Zephyr doorbell.
* P1.04 is the toggled output.
* P1.05 is the optional ISR debug output.
* TIMER20 and EGU20 are disabled in devicetree so no Zephyr driver claims
  them.
* Bluetooth LBS is enabled to keep the SoftDevice Controller and MPSL active
  during coexistence tests.

nRF54H20 DK
===========

* TIMER130 is the direct interrupt source.
* EGU130 is the RT-to-Zephyr doorbell.
* P0.00 is the toggled output.
* P0.01 is the optional ISR debug output.
* TIMER130 and EGU130 are reserved for the application core.
* Bluetooth is disabled in this app-core-only diagnostic configuration.

The overlays are the ownership contract. Recheck SoC reservations and MPSL
usage before changing a peripheral or pin.

User interface
**************

The sample registers the following shell commands:

.. code-block:: console

   rtfw start
   rtfw stop
   rtfw period <microseconds>
   rtfw status

The accepted period range defaults to 10 us through 1,000,000 us and is
configured by ``CONFIG_SAMPLE_RTFW_TIMER_MIN_PERIOD_US`` and
``CONFIG_SAMPLE_RTFW_TIMER_MAX_PERIOD_US``. The minimum must provide at least
two 1-MHz TIMER ticks, the minimum must not exceed the maximum, and the default
500-ms period must remain inside the configured range. Invalid combinations
are rejected at build time rather than silently clamped by the fast path.

``rtfw status`` reports command state, timer-event count, dropped events,
maximum queue depth, and framework faults. See
:ref:`rtfw_concept` for command-state terminology.

The DK LEDs have these roles:

* LED1 is the Zephyr heartbeat.
* LED2 indicates a Bluetooth connection when Bluetooth is enabled.
* LED3 toggles for processed-command and decimated tick events delivered
  to Zephyr.

Building and flashing
*********************

Build for nRF54LM20 DK:

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20b/cpuapp \
     nrf/samples/subsys/rtfw/timer_gpio
   west flash

Build the nRF54H20 sysbuild configuration:

.. code-block:: console

   west build --sysbuild -b nrf54h20dk/nrf54h20/cpuapp \
     nrf/samples/subsys/rtfw/timer_gpio
   west flash

Use a separate build directory or a pristine build when switching targets or
variants.

Configuration variants
**********************

Tick telemetry is disabled by default so the data plane is not exercised on
every timer interrupt. Enable one event per 16 ticks, for example, with:

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20b/cpuapp \
     nrf/samples/subsys/rtfw/timer_gpio -- \
     -DCONFIG_SAMPLE_RTFW_TIMER_TELEMETRY_DECIMATION=16

Enable the ISR debug pin with ``CONFIG_SAMPLE_RTFW_DEBUG_PIN``. The pin is
asserted around the direct ISR and can be measured together with the toggled
output. ``CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD`` adds a controlled NOP loop for
stress testing; ``CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD_CYCLES`` sets its size.

The sample metadata builds base, data-plane, and instrumentation variants for
both supported boards. Their configurations apply the target-specific ZLI
priority policy described in :ref:`rtfw_concept`.

Validation
**********

Basic control test
==================

1. Connect a logic analyzer or oscilloscope to the output pin.
2. Run ``rtfw status`` and verify that the timer is initially stopped with a
   500-ms period.
3. Run ``rtfw start`` and verify periodic output transitions.
4. Change the period and verify that the next transition belongs to a new
   timer phase.
5. Run ``rtfw stop`` and verify that transitions stop.
6. Change the period while stopped, then start again and inspect requested,
   applied, and pending state.

Period reprogramming regression
===============================

Configure a long period, start the timer, and then request the minimum
period before the old compare expires. The next output transition must occur
within the new-period bound rather than waiting for a 32-bit timer wrap.

On nRF54LM20, keep an encrypted Bluetooth connection active during this test
to exercise coexistence with MPSL.

Data-plane test
===============

Enable telemetry decimation and run ``rtfw status`` during normal operation
and an event storm. Verify that:

* LED3 changes on delivered telemetry;
* ``max_depth`` reflects queue use;
* ``dropped`` remains zero under the intended load; and
* any overflow is visible through counters and fault bits rather than blocking
  the ZLI.

Limitations
***********

This diagnostic source adapter is not a general Zephyr timer replacement. It
does not support system PM and rejects ``CONFIG_PM`` at build time.
