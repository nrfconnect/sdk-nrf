.. _bh1749:

BH1749: Ambient Light Sensor IC
###############################

.. contents::
   :local:
   :depth: 2

This sample application sets up the BH1749 color sensor to provide 8-bit measurement data every time a set threshold value (>50) is reached for the RED color channel.
It also shows how to enable interrupt every time data is ready, instead of threshold trigging.

Requirements
************

The sample supports the following device:

.. table-from-sample-yaml::

Building and running
********************

This project outputs sensor data to the console.
It requires a BH1749 sensor.
It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
The example below uses the Thingy:91.


.. |sample path| replace:: :file:`samples/sensor/bh1749`

.. include:: /includes/build_and_run.txt

Sample output
=============

The following output is displayed in the terminal:

.. code-block:: console

    J-Link RTT Viewer

     device is 0x20022384, name is BH1749

     Threshold trigger

     BH1749 RED: 387

     BH1749 GREEN: 753

     BH1749 BLUE: 397

     BH1749 IR: 81

     (continues when trigger level reached)

References
**********

`BH1749NUC-E`_
