.. _gzll_ack_payload_host:

Gazell ACK Payload Host
#######################

.. contents::
   :local:
   :depth: 2

This sample shows the Host role for basic Gazell communication.
As a single sample, the Host receives packets from the Device and transmits acknowledgements with the payload.
Follow the instructions and use the files from this page, and the :ref:`gzll_ack_payload_device` sample.

Requirements
************

.. note::
   Follow the steps from and include the :ref:`gzll_ack_payload_device` sample with this sample.

.. gzll_ack_sample_requirements_start

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any two of the development kits listed above and mix different development kits.

.. gzll_ack_sample_requirements_end


.. gzll_ack_sample_overview_start

Overview
********

The sample uses the :ref:`gzll` library to exchange packets with payload in two directions.
It consists of two applications, a Device and a Host.

* Device
   A Device sends a packet and adds a new packet to the TX queue every time it receives an ACK from Host.
   Before adding a packet to the TX queue, the contents of the buttons are copied to the first payload byte.
   When the Device receives an ACK, the contents of the first payload byte of the ACK are output to the LEDs.

* Host
   A Host listens for a packet and sends an ACK when it has received the packet.
   The contents of the first payload byte of the received packet is output to LEDs.
   The contents of buttons are sent in the first payload byte of the ACK packet.

.. msc::
   hscale = "1.3";
   Device,Host;
   Host note Host      [label="Add button contents to the first payload byte"];
   Host note Host      [label="Add new packet to TX FIFO"];
   Device note Device  [label="Copy button contents to the first payload byte"],Host note Host [label="Listen for a packet"];
   Device note Device  [label="Add new packet to TX FIFO"];
   Device=>Host        [label="Packet"];
   Host note Host      [label="Send ACK"];
   Host=>Device        [label="ACK"];
   Host note Host      [label="Output first payload byte of packet to LEDs"];
   Device note Device  [label="Output first payload byte of ACK to LEDs"],Host note Host [label="Add button contents to the first payload byte"];
   Device note Device  [label="Copy button contents to the first payload byte"],Host note Host [label="Add new packet to TX FIFO"];
   Device note Device  [label="Add new packet to TX FIFO"];
   Device=>Host        [label="Packet"];

The Device transmits packets with its buttons state and the Host acknowledges all successfully received packets from the Device and adds its buttons state in the ACK packets it transmits.
Both devices display the peer's buttons state on their LEDs
You can use this sample to enable simple, bidirectional data exchange over the Gazell protocol between two devices.

.. gzll_ack_sample_overview_end


.. gzll_ack_sample_ui_start

User interface
**************

LED 1-4:
   Indicate that packets are received.
   An LED is turned off when the corresponding button is pressed on the other kit.

Button 1-4:
   The button pressed state bitmask is sent to the other kit.
   A button pressed is sent as 0 and a button released is sent as 1.

.. gzll_ack_sample_ui_end


.. gzll_ack_sample_building_start

Building and running
********************

The Device sample is under :file:`samples/gazell/gzll_ack_payload_device` in the |NCS| folder structure.
The Host sample is under :file:`samples/gazell/gzll_ack_payload_host` in the |NCS| folder structure.

See :ref:`programming` for information about how to build and program the application.

Testing
=======

After programming the Device sample on one of the development kits and the Host sample on the other kit, test them by performing the following steps:

1. Power on both kits.
#. Observe that all the LEDs light up on both kits.
#. Press **Button 1** for the Device sample.
   Observe that the Host sample turns off **LED 1** on the other kit.
#. Press **Button 2** for the Host sample.
   Observe that the Device sample turns off **LED 2** on the other kit.
#. Optionally, connect to the kits with a terminal emulator (for example, nRF Connect Serial Terminal).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Observe the logging output for both kits.

.. gzll_ack_sample_building_end


.. gzll_ack_sample_dependencies_start

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gzll_glue`
* :ref:`dk_buttons_and_leds_readme`

It uses the following :ref:`nrfxlib` library:

* :ref:`nrfxlib:gzll`

It uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``
  * ``include/irq.h``

.. gzll_ack_sample_dependencies_end
