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

Alternatively, you can use this sample to control the color of the RGB LED on the nRF52840 Dongle or Thingy:53.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuapp_ns, nrf52840dk_nrf52840, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52dk_nrf52832, nrf52dk_nrf52810, nrf52840dongle_nrf52840, thingy53_nrf5340_cpuapp_and_cpuapp_ns

The sample also requires a smartphone or tablet running a compatible application.
The `Testing`_ instructions refer to `nRF Connect for Mobile`_, but you can also use other similar applications (for example, `nRF Blinky`_ or `nRF Toolbox`_).

.. note::
   |thingy53_sample_note|

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

nRF52840 Dongle
===============

Green LED:
   When the main loop is running (that is, the device is advertising), the LED blinks with a period of 2 seconds, duty cycle 50%.

RGB LED:
   The RGB LED channels are used independently to display the following information:

   * Red - if Dongle is connected.
   * Green - if user set the LED using Nordic LED Button Service.

Button 1:
   Send a notification with the button state: "pressed" or "released".

Thingy:53
=========

RGB LED:
   The RGB LED channels are used independently to display the following information:

   * Red - if the main loop is running (that is, the device is advertising).
     The LED blinks with a period of 2 seconds, duty cycle 50%.
   * Green - if the device is connected.
   * Blue - if user set the LED using Nordic LED Button Service.

   For example, if Thingy:53 is connected over Bluetooth, the LED color toggles between green and yellow.
   The green LED channel is kept on and the red LED channel is blinking.

Button 1:
   Send a notification with the button state: "pressed" or "released".

Development kits
================

LED 1:
   Blinks when the main loop is running (that is, the device is advertising) with a period of 2 seconds, duty cycle 50%.

LED 2:
   On when the development kit is connected.

LED 3:
   On when the development kit is controlled remotely from the connected device.

Button 1:
   Send a notification with the button state: "pressed" or "released".

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_lbs`

.. include:: /includes/build_and_run.txt

Minimal build
=============

You can build the sample with a minimum configuration as a demonstration of how to reduce code size and RAM usage, using the ``-DCONF_FILE='prj_minimal.conf'`` flag in your build.

See :ref:`cmake_options` for instructions on how to add this option to your build.
For example, when building on the command line, you can do so as follows:

.. code-block:: console

   west build samples/bluetooth/peripheral_lbs -- -DCONF_FILE='prj_minimal.conf'

.. _peripheral_lbs_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps:

1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
#. Power on the development kit or insert your dongle into the USB port.
#. Connect to the device from the nRF Connect application.
   The device is advertising as ``Nordic_LBS``.
   The services of the connected device are shown.
#. In :guilabel:`Nordic LED Button Service`, enable notifications for the :guilabel:`Button` characteristic.
#. Press **Button 1** on the device.
#. Observe that notifications with the following values are displayed:

   * ``Button released`` when **Button 1** is released.
   * ``Button pressed`` when **Button 1** is pressed.

#. Write the following values to the LED characteristic in the :guilabel:`Nordic LED Button Service`.
   Depending on the hardware platform, this produces results described in the table.

+------------------------+---------+----------------------------------------------+
| Hardware platform      | Value   | Effect                                       |
+========================+=========+==============================================+
| Development kit        | ``OFF`` | Switch the **LED3** off.                     |
+                        +---------+----------------------------------------------+
|                        | ``ON``  | Switch the **LED3** on.                      |
+------------------------+---------+----------------------------------------------+
| nRF52840 Dongle        | ``OFF`` | Switch the green channel of the RGB LED off. |
+                        +---------+----------------------------------------------+
|                        | ``ON``  | Switch the green channel of the RGB LED on.  |
+------------------------+---------+----------------------------------------------+
| Thingy:53              | ``OFF`` | Switch the blue channel of the RGB LED off.  |
+                        +---------+----------------------------------------------+
|                        | ``ON``  | Switch the blue channel of the RGB LED on.   |
+------------------------+---------+----------------------------------------------+

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
