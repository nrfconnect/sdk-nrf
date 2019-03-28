.. _peripheral_hids_keyboard:

Bluetooth: Peripheral HIDS keyboard
###################################

The Peripheral HIDS keyboard sample demonstrates how to use the :ref:`hids_readme` to implement a keyboard input device that you can connect to your computer.

Overview
********

The sample uses the buttons on a development board to simulate keys on a keyboard.
One button simulates the letter keys by generating letter keystrokes for a predefined string.
A second button simulates the Shift button and shows how to modify the letter keystrokes.
An LED displays the Caps Lock state, which can be modified by another connected keyboard.

This sample exposes the HID GATT Service.
It uses a report map for a generic keyboard.


Requirements
************

* One of the following development boards:

  * nRF9160 DK board (PCA10090)
  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)
  * nRF51 Development Kit board (PCA10028)

User interface
**************

Button 1:
   Send one character of the predefined input ("hello\\n") to the computer.

Button 2:
   Simulate the Shift key.

LED 1:
   Indicates if Caps Lock is enabled.



Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hids_keyboard`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, you can test it either by connecting the board as keyboard device to a Microsoft Windows computer or by connecting to it with `nRF Connect for Desktop`_.

Testing with a Microsoft Windows computer
-----------------------------------------

To test with a Microsoft Windows computer that has a Bluetooth radio, complete the following steps:

1. Power on your development board.
#. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS keyboard".
#. Open a text editor (for example, Notepad).
#. Repeatedly push Button 1 on the board.
   Every button press sends one character of the test message "hello" (the test message includes a carriage return) to the computer, and this will be displayed in the text editor.
#. Push Button 2 and hold it while pushing Button 1.
   Observe that the next letter of the "hello" message appears as capital letter.
   This is because Button 2 simulates the Shift key.
#. Turn Caps Lock on on the computer.
   Observe that LED 1 turns on.
   This confirms that the output report characteristic has been written successfully on the HID device.
#. Turn Caps Lock off on the computer.
   Observe that LED 1 turns off.
#. Disconnect the computer from the device by removing the device from the computer's devices list.


Testing with nRF Connect for Desktop
------------------------------------

To test with `nRF Connect for Desktop`_, complete the following steps:

1. Power on your development board.
#. Connect to the device from nRF Connect (the device is advertising as "NCS HIDS keyboard").
#. Optionally, bond to the device.
   To bond, click the settings button for the device in nRF Connect, select **Pair**, check **Perform Bonding**, and click **Pair**.
   Wait until the bond is established before you continue.
#. Observe that the services of the connected device are shown.
#. Push Button 1 on the board.
   Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.

   The first notification has the value ``00000B0000000000``, the second has the value ``0000000000000000``.
   The received values correspond to press and release of character "h".
   The format used for keyboard reports is the following byte array: ``[modifier, reserved, Key1, Key2, Key3, Key4, Key6]``.

   Similarly, further press and release events will result in press and release notifications of subsequent characters of the test string.
   Therefore, pushing Button 1 again will result in notification of press and release reports for character "e".
#. Push Button 2 and hold it while pushing Button 1.
   Pushing Button 2 changes the modifier bit to 02, which simulates pushing the Shift key on a keyboard.

   Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.
   The first one has the value ``02000F0000000000``, the second has the value ``0200000000000000``.
   These values correspond to press and release of character "l" with the Shift key pressed.
#. In nRF Connect, select the HID Report (which has UUID 0x2A4D and the properties Read, WriteWithoutResponse, and Write).
   Enter ``02`` in the text box and click the **Write** button.
   This sets the modifier bit of the Output Report to 02, which simulates turning Caps Lock ON.

   Observe that LED 1 turns on.
#. Select the same HID Report again.
   Enter ``00`` in the text box and click the **Write** button.
   This sets the modifier bit to 00, which simulates turning Caps Lock OFF.

   Observe that LED 1 turns off.
#. Disconnect the device in nRF Connect.
   Observe that no new notifications are received and the device is advertising.
#. As bond information is preserved by nRF Connect, you can immediately reconnect to the device by clicking the Connect button again.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`hids_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/misc/printk.h``
* ``include/misc/byteorder.h``
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
