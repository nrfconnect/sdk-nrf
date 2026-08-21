.. _pulse_meas:

Pulse width measurement
#######################

.. contents::
   :local:
   :depth: 2

The Pulse width measurement sample demonstrates pulse width measurement using the ``pulse_meas`` driver.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

It also requires a pulse source connected to the GPIO pins configured in the board overlay.

Overview
********

A GPIOTE instance detects pulse edges, a TIMER instance measures the elapsed time, and GPPI connects the peripherals without CPU intervention.

Each reported value represents the pulse width in microseconds.

Building and running
********************

.. |sample path| replace:: :file:`samples/pulse_meas`

.. include:: /includes/build_and_run.txt

Sample output
=============

The following output is displayed in the console:

.. code-block:: console

   Pulse width measurement sample start
   Stop requested, pending series: 0
   series[0]: first=1000 us last=1000 us
   Measurement finished, series read: 1, callbacks: 1

Dependencies
************

The module uses the following ``nrfx`` drivers:

* ``nrfx_gpiote``
* ``nrfx_timer``
* ``nrfx_gppi``
