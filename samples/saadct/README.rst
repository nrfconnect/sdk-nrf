.. _saadct_sample:

SAADC + TIMER (SAADCT) sample
#############################

Overview
********

This sample demonstrates timer-triggered SAADC sampling using the SAADCT driver.
An external TIMER instance triggers the SAADC sample task through GPPI at the
configured sample rate.

Requirements
************

* nRF54L15 DK or nRF54H20 DK
* Analog inputs connected to the configured SAADC channels

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/saadct
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: build
   :compact:

Change the board to ``nrf54h20dk/nrf54h20/cpuapp`` to run on nRF54H20.
