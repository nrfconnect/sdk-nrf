.. _enocean_sample:

Bluetooth: EnOcean
##################

The Bluetooth EnOcean sample demonstrates the basic usage of the :ref:`bt_enocean_readme` library.

Overview
********

The EnOcean sample sets up a basic Bluetooth observer for both EnOcean switches and sensors.

The observer device forwards incoming advertisements to the EnOcean library for processing.
The application receives events from the EnOcean library through callbacks, and prints the outcome to console.
The LEDs of the board also respond to button presses from an EnOcean switch.


Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

* At least one :ref:`supported EnOcean device <bt_enocean_devices>`.

.. note::
    The sample supports up to four devices at a time that work with one board.

User interface
**************

The following board LEDs are used by this sample:

LED 1 and LED 2:
   At the end of commissioning, blink four times to indicate a new EnOcean device has been commissioned.

   After commissioning, indicate whether a button is pressed on a commissioned light switch device.

   .. note::
        If you are using EnOcean switches with a single rocker, toggling the rocker is indicated on LEDs 2 and 4.

LED 3 and LED 4:
   Indicate the On/Off state of each button channel.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/enocean`

.. include:: /includes/build_and_run.txt

.. _enocean_sample_testing:

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. :ref:`Commission one or more EnOcean devices <bt_enocean_commissioning>`.
   The board LEDs will blink when each of the devices has been successfully commissioned.
#. Connect the board to the computer with an USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal_specific|
#. Depending on the EnOcean devices you commissioned:

    * If you commissioned a light switch, press any of its buttons or toggle the rocker to control the board LEDs.
      The board LEDs light on and off as detailed in `User interface`_, and the received values are printed to the console.
    * Sensor devices will automatically start reporting their sensor values to the application.
      The values are printed to the console.

The following code sample shows the light switch output when Button 4 on the EnOcean device was pressed and released:

.. code-block:: c
   :caption: Light switch output

   EnOcean button RX: ba:15:00:00:15:e2: pressed:  0 0 0 1
   EnOcean button RX: ba:15:00:00:15:e2: released: 0 0 0 1

The following code sample shows the sensor device output:

.. code-block:: c
   :caption: Sensor device output

   EnOcean sensor RX: bb:00:00:00:00:e5:
        Occupancy:          true
        Light (sensor):     85 lx
        Energy level:       92 %
        Light (solar cell): 365 lx


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_enocean_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
