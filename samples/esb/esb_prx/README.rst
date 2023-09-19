.. _esb_prx:

Enhanced ShockBurst: Receiver
#############################

.. contents::
   :local:
   :depth: 2

The sample shows how to use the :ref:`ug_esb` protocol in receiver mode.
It shows how to configure the Enhanced ShockBurst protocol to receive packets.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, if you want to test the Enhanced ShockBurst Transmitter functionality, you need to build and run the :ref:`esb_ptx` sample.
You can use any two of the listed development kits and mix different development kits.

Overview
********

The sample consists of one Receiver that uses the :ref:`esb_README` library.
After building and programming each sample on an nRF52 Series development kit, you can test that packets that are sent by the kit that runs the :ref:`Transmitter <esb_ptx>` sample are picked up by the kit that runs the Receiver sample.
Successful communication is indicated by LED changes, which should be in sync on both kits.

The Receiver sample listens for packets and sends an ACK when a packet is received.
If packets are successfully received from the Transmitter, the LED pattern changes every time a packet is received.

User interface
***************

LED 1-4:
   Indicate that packets are sent or received.
   The first four packets turn on **LED 1**, **2**, **3**, and **4**.
   The next four packets turn them off again in the same order.

Configuration
*************

|config|

Building and running
********************

The Receiver sample can be found under :file:`samples/esb/esb_prx` in the |NCS| folder structure.

See :ref:`programming` for information about how to build and program the application.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Testing
=======

After programming the Transmitter sample on one of the development kits and the Receiver sample on the other kit, you can test their functionality.

Complete the following steps to test both the Transmitter and Receiver samples:

1. Power on both kits.
#. Observe that the LEDs change synchronously on both kits.
#. Optionally, connect to the kits with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Observe the logging output for both kits.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`esb_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`
  * :file:`include/irq.h`

* :ref:`zephyr:api_peripherals`:

   * :file:`include/gpio.h`
