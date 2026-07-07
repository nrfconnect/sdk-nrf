.. _saadct_sample:

.. ncs-sample::
   :title: SAADC + TIMER (SAADCT)

   The SAADC + TIMER (SAADCT) sample demonstrates timer-triggered SAADC sampling using the :ref:`saadct` driver.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires analog inputs connected to the configured SAADC channels.

Overview
********

An external TIMER instance triggers the SAADC sample task through GPPI at the configured sample rate.
The sample configures two single-ended channels and collects series of interleaved channel samples in a memory slab.
For each series, the sample logs the first and the last sample, and reports the total number of series and driver callbacks at the end of the measurement.

Building and running
********************

.. |sample path| replace:: :file:`samples/saadct`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample logs the samples of the consecutive series and the summary of the measurement.

Sample output
=============

The following output is displayed in the console:

.. code-block:: console

   SAADCT sample start
   Stop requested, pending series: 1
   series[0]: first=0x0146 last=0x0148
   series[1]: first=0x0147 last=0x0146
   Measurement finished, series read: 10, callbacks: 11

Dependencies
************

This sample uses the following |NCS| driver:

* :ref:`saadct`
