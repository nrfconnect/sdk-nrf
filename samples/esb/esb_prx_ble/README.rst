.. _esb_prx_ble:

Enhanced ShockBurst: Receiver with Bluetooth LE
################################################

.. contents::
   :local:
   :depth: 2

The sample shows how to use the :ref:`ug_esb` protocol in receiver mode concurrently with Bluetooth® LE protocol.
It demonstrates how to configure the Enhanced ShockBurst protocol to receive packets while simultaneously running Bluetooth LE services.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, if you want to test the Enhanced ShockBurst Transmitter functionality, you need to build and run the :ref:`esb_ptx` sample.
You can use any two of the listed development kits and mix different development kits.

Overview
********

The sample consists of one Receiver that uses the :ref:`esb_README` library in combination with Bluetooth LE functionality.
After building and programming the sample on a development kit, you can test that packets that are sent by the kit that runs the :ref:`Transmitter <esb_ptx>` sample are picked up by the kit that runs the Receiver sample.
Successful communication is indicated by LED changes.

The Receiver sample listens for packets and sends an ACK when a packet is received.
If packets are successfully received from the Transmitter, the LED pattern changes every time a packet is received.

The sample demonstrates cooperative operation between ESB and Bluetooth LE protocols using the MPSL (Multiprotocol Service Layer) time slot mechanism.
This mechanism allows both protocols to share radio time without interference.
The sample runs the :ref:`lbs_readme` alongside the ESB receiver functionality, enabling simultaneous wireless communication using both protocols.

User interface
***************

All LEDs:
   Indicate that packets are sent or received.
   The first four packets turn on the LEDs sequentially.
   The next four packets turn them off again in the same order.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Lit when the development kit is connected.

      LED 2:
         Lit when the development kit is controlled remotely from the connected device.

      LED 3 and LED 4:
         Indicate that packets are received.

      Button 1:
         Send a notification with the button state: "pressed" or "released".

   .. group-tab:: nRF54 DKs

      LED 0:
         Lit when the development kit is connected.

      LED 1:
         Lit when the development kit is controlled remotely from the connected device.

      LED 2 and LED 3:
         Indicate that packets are received.

      Button 0:
         Send a notification with the button state: "pressed" or "released".

Configuration
*************

|config|

Building and running
********************

The Receiver sample can be found under :file:`samples/esb/esb_prx_ble` in the |NCS| folder structure.

See :ref:`building` and :ref:`programming` for information about how to build and program the application, respectively.

.. include:: /includes/nRF54H20_erase_UICR.txt

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Testing
=======

This sample combines ESB receiver functionality with Bluetooth LE LBS service, and both protocols can be tested independently.

Testing ESB functionality
-------------------------

To test the ESB receiver functionality, follow the testing procedure described in the :ref:`Testing section of the Enhanced ShockBurst: Receiver <esb_prx_testing>` sample documentation.
In brief, you need to program the Transmitter sample (:ref:`esb_ptx`) on another development kit and observe that the LEDs change synchronously on both kits as packets are transmitted and received.

Testing LBS service
-------------------

To test the Bluetooth LE LBS service functionality, follow the testing procedure described in the :ref:`Testing section of LBS <peripheral_lbs_testing>` sample documentation.
You can use a smartphone or tablet with the `nRF Connect for Mobile`_ or `nRF Blinky`_ application to connect to the device (advertising as ``Nordic_LBS``), control the LED remotely, and receive button press notifications.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`esb_readme`
* :ref:`ug_ble`
* :ref:`mpsl`
* :ref:`lbs_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`
  * :file:`include/irq.h`

* :ref:`zephyr:api_peripherals`:

   * :file:`include/gpio.h`
