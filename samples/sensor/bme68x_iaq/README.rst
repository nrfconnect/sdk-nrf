.. _bme68x:

BME68X: Gas Sensor
##################

.. contents::
   :local:
   :depth: 2

This sample application sets up the BME68X gas sensor with the Bosch Sensor Environmental Cluster (BSEC) library.

Requirements
************

To use the BME68X IAQ driver, you must manually download the BSEC library.
See the :ref:`bme68x_iaq` documentation for more details.

The sample supports the following devices:

.. table-from-sample-yaml::

Building and running
********************

This project outputs sensor data to the console.
It requires a BME68X sensor.

.. |sample path| replace:: :file:`samples/sensor/bme68x_iaq`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe that output similar to the following is logged on UART:

   .. parsed-literal::
      :class: highlight

      *** Booting Zephyr OS build v3.2.99-ncs1-1531-gaf18f6b63608 ***
      [00:00:01.285,339] <inf> app: App started
      [00:00:07.287,658] <inf> app: temp: 28.240385; press: 100043.570312; humidity: 19.981348; iaq: 50
      [00:00:10.301,391] <inf> app: temp: 28.220613; press: 100039.585937; humidity: 19.983814; iaq: 50
      [00:00:13.315,124] <inf> app: temp: 28.188013; press: 100040.007812; humidity: 20.015941; iaq: 50

.. note::
   BSEC takes about 24 hours to calibrate the indoor air quality (IAQ) output.

References
**********

`BME680`_
