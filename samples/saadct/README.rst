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

   [00:00:00.002,392] <inf> saadct_sample: SAADCT sample start
   [00:00:00.102,504] <inf> saadct_sample: Stop requested, pending series: 9
   [00:00:00.102,519] <inf> saadct_sample: series[0]: first=0x03ff last=0x03ff
   [00:00:00.102,527] <inf> saadct_sample: series[1]: first=0x03ff last=0x03ff
   [00:00:00.102,534] <inf> saadct_sample: series[2]: first=0x03ff last=0x03ff
   [00:00:00.102,541] <inf> saadct_sample: series[3]: first=0x03ff last=0x03ff
   [00:00:00.102,548] <inf> saadct_sample: series[4]: first=0x03ff last=0x03ff
   [00:00:00.102,554] <inf> saadct_sample: series[5]: first=0x03ff last=0x03ff
   [00:00:00.102,561] <inf> saadct_sample: series[6]: first=0x03ff last=0x03ff
   [00:00:00.102,568] <inf> saadct_sample: series[7]: first=0x03ff last=0x03ff
   [00:00:00.102,575] <inf> saadct_sample: series[8]: first=0x03ff last=0x03ff
   [00:00:00.106,810] <inf> saadct_sample: series[9]: first=0x03ff last=0x03ff
   [00:00:00.106,817] <inf> saadct_sample: Number of series: observed: 10, expected: 10 - PASS
   [00:00:00.106,821] <inf> saadct_sample: Measurement finished, series read: 10, callbacks: 10

Dependencies
************

This sample uses the following |NCS| driver:

* :ref:`saadct`
