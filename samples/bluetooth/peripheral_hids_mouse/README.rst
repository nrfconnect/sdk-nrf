.. _peripheral_hids_mouse:

Bluetooth: Peripheral HIDS mouse
################################

The Peripheral HIDS mouse sample demonstrates how to use the :ref:`hids_readme` to implement a mouse input device that you can connect to your computer.
This sample also shows how to perform directed advertising.

Overview
********

The sample uses the buttons on a development board to simulate the movement of a mouse.
The four buttons simulate movement to the left, upward, to the right, and downward, respectively.
Mouse clicks are not simulated.

This sample exposes the HID GATT Service.
It uses a report map for a generic mouse.

You can also disable directed advertising feature by clearing the BT_DIRECTED_ADVERTISING flag in the application configuration.
This feature is enabled by default and it changes the way in which advertising works in comparison to the other BLE samples.
When the device wants to advertise, it starts with high duty cycle directed advertising provided that it has bonding information.
If the timeout occurs, then the device starts directed advertising to the next bonded peer.
If all bonding information is used and there is still no connection, then the regular advertising starts.

Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

User interface
**************

Button 1:
   Simulate moving the mouse pointer 5 pixels to the left.

   When pairing/bonding, press this button to confirm the passkey value that is printed on the COM listener to pair/bond with the other device.

Button 2:
   Simulate moving the mouse pointer 5 pixels upward.

   When pairing/bonding, press this button to reject the passkey value that is printed on the COM listener to prevent pairing/bonding with the other device.

Button 3:
   Simulate moving the mouse pointer 5 pixels to the right.

Button 4:
   Simulate moving the mouse pointer 5 pixels downward.



Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hids_mouse`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, you can test it either by connecting the board as a mouse device to a Microsoft Windows computer or by connecting to it with `nRF Connect for Desktop`_.

Testing with a Microsoft Windows computer
-----------------------------------------

To test with a Microsoft Windows computer that has a Bluetooth radio, complete the following steps:

1. Power on your development board.
#. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS mouse".
#. Push Button 1 on the board.
   Observe that the mouse pointer on the computer moves to the left.
#. Push Button 2.
   Observe that the mouse pointer on the computer moves upward.
#. Push Button 3.
   Observe that the mouse pointer on the computer moves to the right.
#. Push Button 4.
   Observe that the mouse pointer on the computer moves downward.
#. Disconnect the computer from the device by removing the device from the computer's devices list.


Testing with nRF Connect for Desktop
------------------------------------

To test with `nRF Connect for Desktop`_, complete the following steps:

1. Power on your development board.
#. Connect to the device from nRF Connect (the device is advertising as "NCS HIDS mouse").
#. Optionally, bond to the device.
   To do so, click the settings button for the device in nRF Connect, select **Pair**, check **Perform Bonding**, and click **Pair**. Optionally check **Enable MITM protection** to pair with MITM protection and use a button on the device to confirm or reject passkey value. Clik Match in nRF Connect app.
   Wait until the bond is established before you continue.
#. Observe that the services of the connected device are shown.
#. Click the **Play** button for all HID Report characteristics.
#. Push Button 1 on the board.
   Observe that a notification is received on one of the HID Report characteristics, containing the value ``FB0F00``.

   Mouse motion reports contain data with an X-translation and a Y-translation.
   These are transmitted as 12-bit signed integers.
   The format used for mouse reports is the following byte array, where LSB/MSB is the least/most significant bit: ``[8 LSB (X), 4 LSB (Y) | 4 MSB(X), 8 MSB(Y)]``.

   Therefore, ``FB0F00`` denotes an X-translation of FFB = -5 (two's complement format), which means a movement of 5 pixels to the left, and a Y-translation of 000 = 0.
#. Push Button 2.
   Observe that the value ``00B0FF`` is received on the same HID Report characteristic.
#. Push Button 3.
   Observe that the value ``050000`` is received on the same HID Report characteristic.
#. Push Button 4.
   Observe that the value ``005000`` is received on the same HID Report characteristic.
#. Disconnect the device in nRF Connect.
   Observe that no new notifications are received and the device is advertising.
#. As bond information is preserved by nRF Connect, you can immediately reconnect to the device by clicking the Connect button again.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`hids_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``lib/libc/minimal/include/assert.h``
* ``lib/libc/minimal/include/errno.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:settings`
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
  * ``samples/bluetooth/gatt/bas.h``

References
**********

* `HID Service Specification`_
* `HID usage tables`_
