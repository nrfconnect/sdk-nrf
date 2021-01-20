.. _peripheral_lbs:

Bluetooth: Peripheral LBS
#########################

.. contents::
   :local:
   :depth: 2

The peripheral LBS sample demonstrates how to use the :ref:`lbs_readme`.

Overview
********

When connected, the sample sends the state of **Button 1** on the development kit to the connected device, such as a phone or tablet.
The mobile application on the device can display the received button state and can control the state of **LED 3** on the development kit.

Alternatively, you can use this sample to control the color of RGB LED on the nRF52840 Dongle.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52dk_nrf52832, nrf52dk_nrf52810, nrf52840dongle_nrf52840


The sample also requires a phone or tablet running a compatible application, for example `nRF Connect for Mobile`_, `nRF Blinky`_, or `nRF Toolbox`_.

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

For development kits:

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

Minimal Build
=============

The sample can be built with a minimum configuration as a demonstration of how to reduce code size and RAM usage.

.. code-block:: console

   west build samples/bluetooth/peripheral_lbs -- -DCONF_FILE='prj_minimal.conf'


Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps.
This testing procedure assumes that you are using `nRF Connect for Mobile`_.

1. Power on your development kit or plug in your dongle.
#. Connect to the device from nRF Connect (the device is advertising as "Nordic_Blinky").
#. Observe that the services of the connected device are shown.
#. In "Nordic LED Button Service", click the :guilabel:`Play` button for the "Button" characteristic.
#. Press **Button 1** either on the dongle or on the development kit.
#. Observe that notifications with the following values are received:

   * ``00`` when **Button 1** is released,
   * ``01`` when **Button 1** is pressed.

#. Control the color of RGB LED on the dongle or status of **LED 3** on the kit by writing the following values to the "LED" characteristic in the "Nordic LED Button Service":

   * ``00`` to switch the LED off on the kit or turn on the red RGB LED on the dongle.
   * ``01`` to switch the LED on on the kit or turn on the green RGB LED on the dongle.

#. Observe that RGB LED on the dongle or **LED 3** on the kit corresponds to the value of the "LED" characteristic.

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
