.. _channel_sounding_ras_initiator:

Bluetooth: Channel Sounding Initiator with Ranging Requestor
############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the ranging service to request ranging data from a server.
It also provides a basic distance estimation algorithm to demonstrate the IQ data handling.
The accuracy is not representative for Channel Sounding and should be replaced if accuracy is important.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a device running a Channel Sounding Reflector with Ranging Responder to connect to, such as the :ref:`channel_sounding_ras_reflector` sample.

Overview
********

The sample demonstrates a basic Bluetooth® Low Energy Central role functionality that acts as a GATT Ranging Requestor client and configures the Channel Sounding initiator role.
Regular Channel Sounding procedures are set up, local subevent data is stored, and peer ranging data is fetched.

A basic distance estimation algorithm is included in the sample.
The mathematical representations described in `Distance estimation based on phase and amplitude information`_ and `Distance estimation based on RTT packets`_ are used as the basis for this algorithm.

User interface
**************

The sample does not require user input and will scan for a device advertising with the GATT Ranging Service UUID.
The first LED on the development kit will be lit when a connection has been established.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/channel_sounding_ras_initiator`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by connecting to another device programmed with a Channel Sounding Reflector role with Ranging Responder, such as the :ref:`channel_sounding_ras_reflector` sample.

1. |connect_terminal_specific|
#. Reset both kits.
#. Wait until the scanner detects the Peripheral.
   In the terminal window, check for information similar to the following::

      I: Filters matched. Address: XX:XX:XX:XX:XX:XX (random) connectable: 1
      I: Connecting
      I: Connected to XX:XX:XX:XX:XX:XX (random) (err 0x00)
      I: Security changed: XX:XX:XX:XX:XX:XX (random) level 2
      I: MTU exchange success (498)
      I: The discovery procedure succeeded
      I: CS capability exchange completed.
      I: CS config creation complete. ID: 0
      I: CS security enabled.
      I: CS procedures enabled.
      I: Subevent result callback 0
      I: Ranging data ready 0
      I: Ranging data get completed for ranging counter 0
      I: Estimated distance to reflector:
      I: - Round-Trip Timing method: X.XXXXX meters (derived from X samples)
      I: - Phase-Based Ranging method: X.XXXXX meters (derived on antenna path A from X samples)

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`
* :file:`include/bluetooth/gatt_dm.h`
* :file:`include/bluetooth/services/ras.h`

This sample uses the following Zephyr libraries:

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/cs.h`
