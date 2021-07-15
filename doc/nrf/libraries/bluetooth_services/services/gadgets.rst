.. _gadgets_service_readme:

Alexa Gadgets Service
#####################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE GATT Alexa Gadgets Service is a custom service that manages Alexa Gadgets stream transactions.
For details about the stream format, see Alexa Gadgets Bluetooth LE Packet `Alexa Gadgets Bluetooth LE Packet Format`_.

The Alexa Gadgets Service is used in the :ref:`peripheral_alexa_gadgets` sample.

Service UUID
************

The 128-bit vendor-specific service UUID is 0000FE03-0000-1000-8000-00805F9B34FB.

Characteristics
***************

This service has two characteristics.

RX Characteristic (2BEEA05B-1879-4BB4-8A2F-72641F82420B)
========================================================

Notify
   Enable notifications for the RX Characteristic to send events and respond to commands.

TX Characteristic (F04EB177-3005-43A7-AC61-A390DDF83076)
========================================================

Write or Write Without Response
   Commands and directives are received as Write Without Response.


API documentation
*****************

| Header file: :file:`include/bluetooth/services/gadgets.h`
| Source file: :file:`subsys/bluetooth/services/gadgets.c`

.. doxygengroup:: bt_gadgets
   :project: nrf
   :members:
