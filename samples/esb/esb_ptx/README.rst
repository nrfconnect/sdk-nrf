.. _esb_ptx:

Enhanced ShockBurst: Transmitter
################################

.. contents::
   :local:
   :depth: 2

This sample shows the Enhanced ShockBurst Transmitter application.
It includes the basic steps that are needed to transmit and receive packets using :ref:`ug_esb`.
Follow the instructions and use the files from this page, and the :ref:`esb_prx` sample.

Requirements
************

.. note::
   Follow the steps from and include the :ref:`esb_prx` sample with this sample.

.. esb_ptx_sample_requirements_start

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any two of the development kits listed above and mix different development kits.

.. esb_ptx_sample_requirements_end


.. esb_ptx_sample_remaining_start

Overview
********

The sample consists of two applications, one Transmitter and one Receiver, that use the :ref:`esb_README` library.
After programming each application on an nRF5 Series development kit, you can test that packets that are sent by the kit that runs the Transmitter application are picked up by the kit that runs the Receiver application.
Successful communication is indicated by LED changes, which should be in sync on both kits.

Transmitter
===========

The Transmitter example sends a packet, waits for a configurable time (50 milliseconds by default), and then sends another packet.
In each packet, the four least significant bits of the first byte of the payload are incremented (or reset to zero when they reach 16).
The LEDs indicate that packets were sent and acknowledged.
Therefore, if packets are successfully received and acknowledged by the Receiver, the LED pattern will change every 50 milliseconds (with the default delay).

Receiver
========

The Receiver example listens for packets and sends an ACK when a packet is received.
If packets are successfully received from the transmitter, the LED pattern will change every time a packet is received.

User interface
***************

LED 1-4:
   Indicate that packets are sent or received.
   The first four packets turn on LED 1, 2, 3, and 4.
   The next four packets turn them off again in the same order.

Configuration
*************

|config|

Building and running
********************

The Transmitter sample can be found under :file:`samples/esb/esb_ptx` in the |NCS| folder structure.
The Receiver sample can be found under :file:`samples/esb/esb_prx` in the |NCS| folder structure.

See :ref:`gs_programming` for information about how to build and program the application.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Testing
=======

After programming the Transmitter sample on one of the development kits and the Receiver sample on the other kit, test them by performing the following steps:

1. Power on both kits.
#. Observe that the LEDs change synchronously on both kits.
#. (Optional.) |connect_terminal|
#. Observe the logging output for both kits.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`esb_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``
  * ``include/irq.h``

* :ref:`zephyr:api_peripherals`:

   * :file:`incude/gpio.h`

.. esb_ptx_sample_remaining_end
