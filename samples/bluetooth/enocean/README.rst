.. _enocean_sample:

Bluetooth: EnOcean
##################

.. contents::
   :local:
   :depth: 2

The Bluetooth® EnOcean sample demonstrates the basic usage of the :ref:`bt_enocean_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires at least one :ref:`supported EnOcean device <bt_enocean_devices>`.

.. note::
    The sample supports up to four devices at a time with one development kit.

Overview
********

The EnOcean sample sets up a basic Bluetooth observer for both EnOcean switches and sensors.

The observer device forwards incoming advertisements to the EnOcean library for processing.
The application receives events from the EnOcean library through callbacks, and prints the outcome to the console.
The LEDs of the kit also respond to button presses from an EnOcean switch.

User interface
**************

The following LEDs are used by this sample:

LED 1 and LED 2:
   At the end of commissioning, blink four times to indicate that a new EnOcean device has been commissioned.

   After commissioning, indicate whether a button is pressed on a commissioned light switch device.

   .. note::
        If you are using EnOcean switches with a single rocker, toggling the rocker is indicated on **LEDs 2** and **4**.

LED 3 and LED 4:
   Indicate the On/Off state of each button channel.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/enocean`

.. include:: /includes/build_and_run.txt

.. _enocean_sample_testing:

Testing
=======

|test_sample|

1. :ref:`Commission one or more EnOcean devices <bt_enocean_commissioning>`.
   The LEDs will blink when each of the devices has been successfully commissioned.
#. Connect the kit to the computer with a USB cable.
   The kit is assigned a serial port.
   |serial_port_number_list|
#. |connect_terminal_specific|
#. Depending on the EnOcean devices you commissioned:

   * If you commissioned a light switch, press any of its buttons or toggle the rocker to control the LEDs.
     The LEDs turn on and off as detailed in `User interface`_, and the received values are printed to the console.
   * Sensor devices will automatically start reporting their sensor values to the application.
     The values are printed to the console.

The following code sample shows the light switch output when **Button 4** on the EnOcean device was pressed and released:

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

* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
