.. _esb_ptx_ble:

Enhanced ShockBurst: Transmitter with Bluetooth LE
##################################################

.. contents::
   :local:
   :depth: 2

The sample shows how to use the :ref:`ug_esb` protocol in transmitter mode concurrently with Bluetooth® LE protocol.
It demonstrates how to configure the Enhanced ShockBurst protocol to transmit packets while simultaneously running Bluetooth LE services.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, if you want to fully test the Enhanced ShockBurst Transmitter functionality, you need to build and run the :ref:`esb_prx` sample.
You can use any two of the listed development kits and mix different development kits.

Overview
********

The sample consists of one Transmitter that uses the :ref:`esb_README` library in combination with Bluetooth LE functionality.
After building and programming the sample on a development kit, you can test that packets that are sent by the Transmitter sample are picked up by the Receiver sample.
Successful communication is indicated by LED changes.

The Transmitter sends a packet, waits for a configurable time (100 milliseconds by default), and then sends another packet.
The LEDs indicate that packets were sent and acknowledged.
Therefore, if packets are successfully received and acknowledged by the Receiver, the LED pattern changes every 100 milliseconds (with the default delay).

The sample demonstrates cooperative operation between ESB and Bluetooth LE protocols using the MPSL (Multiprotocol Service Layer) time slot mechanism.
This mechanism allows both protocols to share radio time without interference.
The sample runs the :ref:`lbs_readme` alongside the ESB transmitter functionality, enabling simultaneous wireless communication using both protocols.
This LBS functionality is used to transmit the button state to connected BLE device and remotely control the LED on your development kit from connected BLE device.

User interface
***************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Lit when the development kit is connected to BLE device.

      LED 2:
         Lit when the development kit is controlled remotely from the connected BLE device.

      LED 3 and LED 4:
         Indicate that packets are sent.
         The first two packets turn on the LEDs sequentially.
         The next two packets turn them off again in the same order.

      Button 1:
         Send a notification with the button state: "pressed" or "released".

   .. group-tab:: nRF54 DKs

      LED 0:
         Lit when the development kit is connected.

      LED 1:
         Lit when the development kit is controlled remotely from the connected device.

      LED 2 and LED 3:
         Indicate that packets are sent.
         The first two packets turn on the LEDs sequentially.
         The next two packets turn them off again in the same order.

      Button 0:
         Send a notification with the button state: "pressed" or "released".

Configuration
*************

|config|

Building and running
********************

The Transmitter sample is located under :file:`samples/esb/esb_ptx_ble` in the |NCS| folder structure.

See :ref:`building` and :ref:`programming` for information about how to build and program the application, respectively.

.. include:: /includes/nRF54H20_erase_UICR.txt

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Testing
=======

This sample combines ESB transmitter functionality with Bluetooth LE LBS service, and both protocols can be tested independently.

Testing ESB functionality
-------------------------

To test the ESB transmitter functionality, follow the procedure described in the :ref:`esb_ptx_testing` section.
In brief, you need to program the :ref:`esb_prx` sample on another development kit and observe that the LEDs change synchronously on both kits as packets are transmitted and received.

Testing LBS service
-------------------

To test the Bluetooth LE LBS service functionality, follow the procedure described in the :ref:`peripheral_lbs_testing` section.
You can use a smartphone or tablet with the `nRF Connect for Mobile`_ or `nRF Blinky`_ application to connect to the device (advertising as ``Nordic_LBS``), control the LED remotely, and receive button press notifications.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`esb_readme`
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

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`
