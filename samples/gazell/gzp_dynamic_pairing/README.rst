.. _gzp_dynamic_pairing:

Gazell Dynamic Pairing
######################

.. contents::
   :local:
   :depth: 2

Overview
********

This sample demonstrates the functionality of the :ref:`gzp` subsystem.
It consists of two applications, one running on the device and one running on the host.

Device
======

The application sends packets continuously.
If a packet transmission fails (either times out or encryption fails), the Device makes an attempt to pair with a Host by sending a pairing request, consisting of an "address request" and a "Host ID" request.

If the Device is paired with a Host, the pairing data is stored into the non-volatile memory.

Before adding a packet to the TX queue, the content of the buttons is copied to the first payload byte (byte 0).

The application alternates between sending the packets encrypted through the pairing library or directly as plaintext.

Host
====

The application listens for packets continuously, monitoring for pairing requests as well as normal user data.

The Gazell pairing library uses pipe 0 and pipe 1 for encrypted communication.
The application grants any request for a Host ID, thus granting pairing.
Unencrypted packets can be received on pipe 2.

When the Host has received data, the content of the first payload byte is output to LEDs.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf52dk_nrf52832

You can use any two of the development kits listed above and mix different development kits.

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

Building and running
********************

The Device sample is under :file:`samples/gazell/gzp_dynamic_pairing/device` in the |NCS| folder structure.
The Host sample is under :file:`samples/gazell/gzp_dynamic_pairing/host` in the |NCS| folder structure.

See :ref:`gs_programming` for information about how to build and program the application.

Testing
=======

After programming the Device sample on one of the development kits and the Host sample on the other kit, test them by performing the following steps:

1. Power on both kits.
#. Observe that all the LEDs are off on both kits.
#. Place the kits next to each other for Gazell pairing.
#. Observe that the Host sample turns on all LEDs.
   It indicates that the pairing is done.
#. Press **Button 2** for the Device sample.
   Observe that the Host sample turns off **LED 2** on the other kit.

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
