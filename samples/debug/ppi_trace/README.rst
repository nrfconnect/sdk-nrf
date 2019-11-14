.. _ppi_trace_sample:

PPI trace
#########

The PPI trace sample shows how to trace hardware events on GPIO pins.
It uses the :ref:`ppi_trace` module.

Overview
********

The sample initializes trace pins to observe the following hardware events:

* RTC Compare event (``NRF_RTC_EVENT_COMPARE_0``)
* RTC Tick event (``NRF_RTC_EVENT_TICK``)
* Low frequency clock (LFCLK) Started event (``NRF_CLOCK_EVENT_LFCLKSTARTED``)
* Radio activity during *Bluetooth* advertising (available only for Bluetooth capable devices)

The sample sets up a :ref:`zephyr:counter_interface` to generate an ``NRF_RTC_EVENT_COMPARE_0`` event every 50 ms.
Initially, RTC runs on RC low frequency (lower precision) as clock source.
When the crystal is ready, it switches seamlessly to crystal (precise) as clock source.
When the low-frequency crystal is ready, an ``NRF_CLOCK_EVENT_LFCLKSTARTED`` event is generated.


Requirements
************

* One of the following development boards:

  * nRF9160 DK board (PCA10090)
  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)
* Logic analyzer


Building and running
********************
.. |sample path| replace:: :file:`samples/debug/ppi_trace`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect a logic analyzer to the pins that are used for tracing.
   Check the sample configuration for information about which pins are used.
   To do so in |SES|, select **Project** > **Configure nRF Connect SDK Project** and navigate to **PPI trace pins configuration**.
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

* :ref:`zephyr:counter_interface`
* :ref:`zephyr:device_drivers`
* :ref:`zephyr:logger`
* ``ext/hal/nordic/nrfx/hal/nrf_rtc.h``
* ``ext/hal/nordic/nrfx/hal/nrf_clock.h``
