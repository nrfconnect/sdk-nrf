.. _ppi_trace_sample:

PPI trace
#########

.. contents::
   :local:
   :depth: 2

The PPI trace sample shows how to trace hardware events on GPIO pins.
It uses the :ref:`ppi_trace` module.

Overview
********

The sample initializes trace pins to observe the following hardware events:

* RTC Compare event (:c:enumerator:`NRF_RTC_EVENT_COMPARE_0`)
* RTC Tick event (:c:enumerator:`NRF_RTC_EVENT_TICK`)
* Low frequency clock (LFCLK) Started event (:c:enumerator:`NRF_CLOCK_EVENT_LFCLKSTARTED`)
* Radio activity during *Bluetooth* advertising (available only for Bluetooth capable devices)

The sample sets up a :ref:`zephyr:counter_api` to generate an :c:enumerator:`NRF_RTC_EVENT_COMPARE_0` event every 50 ms.
Initially, RTC runs on RC low frequency (lower precision) as clock source.
When the crystal is ready, it switches seamlessly to crystal (precise) as clock source.
When the low-frequency crystal is ready, an :c:enumerator:`NRF_CLOCK_EVENT_LFCLKSTARTED` event is generated.

The sample uses Zephyr's Link Layer instead of the SoftDevice Link Layer.
This is because the SoftDevice Link Layer is blocked during the initialization until the low-frequency crystal is started and the clock is stable.
For this reason, the SoftDevice Link Layer cannot be used to show the :c:enumerator:`NRF_CLOCK_EVENT_LFCLKSTARTED` event on pin.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52dk_nrf52832, nrf52840dk_nrf52840, nrf9160dk_nrf9160

The sample also requires a logic analyzer.


Building and running
********************
.. |sample path| replace:: :file:`samples/debug/ppi_trace`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect a logic analyzer to the pins that are used for tracing.
   Check the sample configuration for information about which pins are used.
   To do so in |SES|, select :guilabel:`Project` -> :guilabel:`Configure nRF Connect SDK Project` and navigate to :guilabel:`PPI trace pins configuration`.
#. Observe that:

   * The pin that is tracing the RTC Tick event is toggling with a frequency of approximately 32 kHz.
   * The pin that is tracing the RTC Compare event is toggling approximately every 50 ms.
   * The pin that is tracing the LFCLK Started event is set at some point.

#. Measure the typical time between two consecutive toggles of the pin that is tracing the RTC Compare event, before and after the LFCLK Started event is generated.
   Observe that the precision increases when the low-frequency crystal is started.
#. Observe periodical radio activity during Bluetooth advertising.


Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`ppi_trace`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:counter_api`
* :ref:`zephyr:device_model_api`
* :ref:`zephyr:logging_api`
* ``ext/hal/nordic/nrfx/hal/nrf_rtc.h``
* ``ext/hal/nordic/nrfx/hal/nrf_clock.h``
