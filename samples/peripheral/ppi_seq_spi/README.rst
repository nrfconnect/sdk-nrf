.. _ppi_seq_spi_sample:

PPI Sequencer with SPI
######################

.. contents::
   :local:
   :depth: 2

The PPI Sequencer with SPI sample presents how to use :ref:`ppi_seq_i2c_spi` driver.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

Sample is using :ref:`ppi_seq_i2c_spi` to perform periodic SPI transfers which are triggered by PPI.
CPU is waken up after completion of every 16 transfers.
Sample runs the sequence of SPI transfers using various periods, ranging from 1 ms to 100 ms.

Configuration
*************

|config|

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe reports about number of completed transfers and the CPU load.
#. Connect the logic analyzer to the SPI pins, to confirm SPI activity.
#. Optionally, measure the current to get average current for various periods.

Dependencies
************

This sample uses the following libraries:

* :ref:`ppi_seq`
* :ref:`ppi_seq_i2c_spi`
