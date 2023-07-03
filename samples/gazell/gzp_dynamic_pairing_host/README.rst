.. _gzp_dynamic_pairing_host:

Gazell Dynamic Pairing Host
###########################

This sample shows the Host role for the functionality of the :ref:`gzp` subsystem.
As a single sample, the Host receives packets from the Device and transmits acknowledgements with the payload.
Follow the instructions and use the files from this page, and the :ref:`gzp_dynamic_pairing_device` sample.

.. contents::
   :local:
   :depth: 2

Requirements
************

.. note::
   Follow the steps from and include the :ref:`gzp_dynamic_pairing_device` sample with this sample.

.. gzp_dynamic_pairing_requirements_start

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any two of the development kits listed above and mix different development kits.

.. gzp_dynamic_pairing_requirements_end


.. gzp_dynamic_pairing_overview_start

Overview
********

This sample demonstrates the :ref:`ug_gzp` protocol.
It consists of two applications, a Device and a Host.

* Device
   The application sends packets continuously.
   If a packet transmission fails (either times out or encryption fails), the Device makes an attempt to pair with a Host by sending a pairing request, consisting of an "address request" and a "Host ID" request.
   If the Device is paired with a Host, the pairing data is stored into the non-volatile memory.
   Before adding a packet to the TX queue, the content of the buttons is copied to the first payload byte (byte 0).
   The application alternates between sending the packets encrypted through the pairing library or directly as plaintext.

* Host
   The application listens for packets continuously, monitoring for pairing requests as well as normal user data.
   The Gazell pairing library uses pipe 0 and pipe 1 for encrypted communication.
   The application grants any request for a Host ID, thus granting pairing.
   Unencrypted packets can be received on pipe 2.
   When the Host has received data, the content of the first payload byte is output to the LEDs.

The Host automatically accepts the pairing request and receives both types of packets from the Device and displays its buttons state on the LEDs.
You can use this sample to learn how to use the :ref:`gzp` subsystem to establish a secure link between two devices over the Gazell protocol.

.. gzp_dynamic_pairing_overview_end


.. gzp_dynamic_pairing_ui_start

User interface
**************

Device
======

Button 1-4:
   The button pressed state bitmask is sent to the other kit.
   A button pressed is sent as 0 and a button released is sent as 1.

Host
====

LED 1-4:
   Indicate that packets are received.
   A LED is turned off when the corresponding button is pressed on the other kit.

.. gzp_dynamic_pairing_ui_end


.. gzp_dynamic_pairing_building_start

Building and running
********************

The Device sample is under :file:`samples/gazell/gzp_dynamic_pairing_device` in the |NCS| folder structure.
The Host sample is under :file:`samples/gazell/gzp_dynamic_pairing_host` in the |NCS| folder structure.

See :ref:`programming` for information about how to build and program the application.

Testing
=======

After programming the Device sample on one of the development kits and the Host sample on the other kit, complete the following steps to test them:

1. Power on both kits.
#. Observe that all the LEDs are off on both kits.
#. Place the kits next to each other for Gazell pairing.
#. Observe that the Host sample turns on all LEDs.

   It indicates that the pairing is done.
#. Press **Button 2** for the Device sample.

   Observe that the Host sample turns off **LED 2** on the other kit.

.. gzp_dynamic_pairing_building_end


.. gzp_dynamic_pairing_dependencies_start

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gzll_glue`
* :ref:`gzp`
* :ref:`dk_buttons_and_leds_readme`

It uses the following :ref:`nrfxlib` library:

* :ref:`nrfxlib:gzll`

It uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

.. gzp_dynamic_pairing_dependencies_end
