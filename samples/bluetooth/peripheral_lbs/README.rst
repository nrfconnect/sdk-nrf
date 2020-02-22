.. _peripheral_lbs:

Bluetooth: Peripheral LBS
#########################

The peripheral LBS sample demonstrates how to use the :ref:`lbs_readme`.

Overview
********

When connected, the sample sends the state of Button 1 on the development board to the connected device, such as a phone or tablet.
The mobile application on the device can display the received button state and can control the state of LED 3 on the development board.

Alternatively, you can use this sample to control the color of RGB LED on the nRF52840 Dongle.

Requirements
************

* One of the Nordic development boards:

  * |nRF5340DK|
  * |nRF52840Dongle|
  * |nRF52840DK|
  * |nRF52DK|

* A phone or tablet running a compatible application, for example `nRF Connect for Mobile`_, `nRF Blinky`_, or `nRF Toolbox`_.

User interface
**************

For nRF52840 Dongle:

RGB LED:
   Red:
      * On when connected.
   Green:
      * Controlled remotely from the connected device.

Button 1:
   * Sends a notification with the button state: "pressed" or "released".

For Development Kit boards:

LED 1:
   * When the main loop is running (device is advertising), blinks with a period of 2 seconds, duty cycle 50%.

LED 2:
   * On when connected.

LED 3:
   * Controlled remotely from the connected device.

Button 1:
   * Sends a notification with the button state: "pressed" or "released".

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_lbs`

.. include:: /includes/build_and_run.txt

.. _peripheral_lbs_testing:

Testing
=======

After programming the sample to your dongle or development board, test it by performing the following steps.
This testing procedure assumes that you are using `nRF Connect for Mobile`_.

1. Power on your development board or plug in your dongle.
#. Connect to the device from nRF Connect (the device is advertising as "Nordic_Blinky").
#. Observe that the services of the connected device are shown.
#. In "Nordic LED Button Service", click the **Play** button for the "Button" characteristic.
#. Press Button 1 either on the dongle or on the development board.
#. Observe that notifications with the following values are received:

   * ``00`` when Button 1 is released,
   * ``01`` when Button 1 is pressed.

#. Control the color of RGB LED on the dongle or status of LED 3 on the board by writing the following values to the "LED" characteristic in the "Nordic LED Button Service":

   * ``00`` to switch the LED off on the board or turn on the red RGB LED on the dongle.
   * ``01`` to switch the LED on on the board or turn on the green RGB LED on the dongle.

#. Observe that RGB LED on the dongle or LED 3 on the board corresponds to the value of the "LED" characteristic.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lbs_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``lib/libc/minimal/include/errno.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
