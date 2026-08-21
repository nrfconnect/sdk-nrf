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

The sample supports the following development kits:

.. table-from-sample-yaml::

Analog inputs must be connected to the configured SAADC channels.

Building and running
********************

The example below uses the nRF54L15 DK.
To run on nRF54H20 DK, use the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

.. |sample path| replace:: :file:`samples/saadct`

.. include:: /includes/build_and_run.txt
