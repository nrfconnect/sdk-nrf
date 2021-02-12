.. _peripheral_hids_keyboard:

Bluetooth: Peripheral HIDS keyboard
###################################

.. contents::
   :local:
   :depth: 2

The Peripheral HIDS keyboard sample demonstrates how to use the :ref:`hids_readme` to implement a keyboard input device that you can connect to your computer.

The sample also shows how to perform LE Secure Connections Out-of-Band pairing using NFC.

Overview
********

The sample uses the buttons on a development kit to simulate keys on a keyboard.
One button simulates the letter keys by generating letter keystrokes for a predefined string.
A second button simulates the Shift button and shows how to modify the letter keystrokes.
An LED displays the Caps Lock state, which can be modified by another connected keyboard.

This sample exposes the HID GATT Service.
It uses a report map for a generic keyboard.

LE Secure Out-of-Band pairing with NFC is enabled by default in this sample.
You can disable this feature by clearing the `NFC_OOB_PAIRING` flag in the application configuration.
The following paragraphs describe the application behavior when NFC pairing is enabled.

When the application starts, it initializes and starts the NFCT peripheral, which is used for pairing.
The application does not start advertising immediately, but only when the NFC tag is read by an NFC polling device, for example a smartphone or a tablet with NFC support.
To trigger advertising without NFC, you can press **Button 4**.
The NDEF message that the tag sends to the NFC device contains data required to initiate pairing.
To start the NFC data transfer, the NFC device must touch the NFC antenna that is connected to the nRF52 device.

After reading the tag, the device can pair with the nRF52 device which is advertising.
After connecting, the sample application behaves in the same way as the original HID Keyboard sample.
Reading the NFC tag again when the application is in a connected state causes disconnection.
When the connection is lost, advertising does not restart automatically.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52833dk_nrf52820

.. include:: /includes/hci_rpmsg_overlay.txt

If the `NFC_OOB_PAIRING` feature is enabled, the sample requires  a smartphone or a tablet with Android v8.0.0 or newer.

User interface
**************

Button 1:
   Sends one character of the predefined input ("hello\\n") to the computer.

   When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

Button 2:
   Simulates the Shift key.

   When pairing, press this button to reject the passkey value which is printed on the COM listener to prevent pairing with the other device.

LED 1:
   Blinks with a period of 2 seconds (duty cycle 50%) when the device is advertising.

LED 2:
   On when at least one device is connected.

LED 3:
   Indicates if Caps Lock is enabled.

If the `NFC_OOB_PAIRING` feature is enabled:

Button 4:
   Starts advertising.

LED 4:
   Indicates if an NFC field is present.


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hids_keyboard`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it either by connecting the kit as keyboard device to a Microsoft Windows computer or by connecting to it with `nRF Connect for Desktop`_.

Testing with a Microsoft Windows computer
-----------------------------------------

To test with a Microsoft Windows computer that has a Bluetooth radio, complete the following steps:

1. Power on your development kit.
#. Press Button 4 on the kit if the device is not advertising.
   Advertising is indicated by blinking **LED 1**.
#. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS keyboard".
#. Observe that the connection state is indicated by **LED 2**.
#. Open a text editor (for example, Notepad).
#. Repeatedly press **Button 1** on the kit.
   Every button press sends one character of the test message "hello" (the test message includes a carriage return) to the computer, and this will be displayed in the text editor.
#. Press **Button 2** and hold it while pressing **Button 1**.
   Observe that the next letter of the "hello" message appears as capital letter.
   This is because **Button 2** simulates the Shift key.
#. Turn Caps Lock on on the computer.
   Observe that **LED 3** turns on.
   This confirms that the output report characteristic has been written successfully on the HID device.
#. Turn Caps Lock off on the computer.
   Observe that **LED 3** turns off.
#. Disconnect the computer from the device by removing the device from the computer's devices list.


Testing with nRF Connect for Desktop
------------------------------------

To test with `nRF Connect for Desktop`_, complete the following steps:

1. Power on your development kit.
#. Press **Button 4** on the kit if the device is not advertising.
   Advertising is indicated by blinking **LED 1**.
#. Connect to the device from nRF Connect (the device is advertising as "NCS HIDS keyboard").
#. Optionally, bond to the device.
   To bond, click the settings button for the device in nRF Connect, select :guilabel:`Pair`, check :guilabel:`Perform Bonding`, and click :guilabel:`Pair`.
   Optionally, check :guilabel:`Enable MITM protection` to pair with MITM protection and use a button on the device to confirm or reject the passkey value.
#. Click :guilabel:`Match` in the nRF Connect app.
   Wait until the bond is established before you continue.
#. Observe that the connection state is indicated by **LED 2**.
#. Observe that the services of the connected device are shown.
#. Enable notifications for all HID characteristics.
#. Press **Button 1** on the kit.
   Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.

   The first notification has the value ``00000B0000000000``, the second has the value ``0000000000000000``.
   The received values correspond to press and release of character "h".
   The format used for keyboard reports is the following byte array: ``[modifier, reserved, Key1, Key2, Key3, Key4, Key6]``.

   Similarly, further press and release events will result in press and release notifications of subsequent characters of the test string.
   Therefore, pressing **Button 1** again will result in notification of press and release reports for character "e".
#. Press **Button 2** and hold it while pressing **Button 1**.
   Pressing **Button 2** changes the modifier bit to 02, which simulates pressing the Shift key on a keyboard.

   Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.
   The first one has the value ``02000F0000000000``, the second has the value ``0200000000000000``.
   These values correspond to press and release of character "l" with the Shift key pressed.
#. In nRF Connect, select the HID Report (which has UUID 0x2A4D and the properties Read, WriteWithoutResponse, and Write).
   Enter ``02`` in the text box and click the :guilabel:`Write` button.
   This sets the modifier bit of the Output Report to 02, which simulates turning Caps Lock ON.

   Observe that **LED 3** turns on.
#. Select the same HID Report again.
   Enter ``00`` in the text box and click the :guilabel:`Write` button.
   This sets the modifier bit to 00, which simulates turning Caps Lock OFF.

   Observe that **LED 3** turns off.
#. Disconnect the device in nRF Connect.
   Observe that no new notifications are received and the device is advertising.
#. As bond information is preserved by nRF Connect, you can immediately reconnect to the device by clicking the Connect button again.


Testing with Android using NFC for pairing
------------------------------------------

To test with an Android smartphone/tablet, complete the following steps:

1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 4** is lit.
#. Observe that the device is advertising, as indicated by blinking **LED 1**.
#. Confirm pairing with 'Nordic_Mouse_NFC' in a pop-up window on the smartphone/tablet.
#. Observe that the connection state is indicated by **LED 2**.
#. Repeatedly press **Button 1** on the kit.
   Every button press sends one character of the test message "hello" to the smartphone (the test message includes a carriage return).
#. Press **Button 2** and hold it while pressing **Button 1**.
   Observe that the next letter of the "hello" message appears as a capital letter.
   This is because **Button 2** simulates the Shift key.
#. Touch the NFC antenna with the same central device again.
#. Observe that devices disconnect.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`hids_readme`
* :ref:`nfc_ndef` (if the `NFC_OOB_PAIRING` option is enabled)
* :ref:`nfc_ndef_le_oob` (if the `NFC_OOB_PAIRING` option is enabled)

When the `NFC_OOB_PAIRING` feature is enabled, it also uses the Type 2 Tag library from nrfxlib:

* :ref:`nrfxlib:nfc_api_type2`
* :ref:`dk_buttons_and_leds_readme`

The sample uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:settings_api`
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
* `Bluetooth Secure Simple Pairing Using NFC`_
* `Bluetooth Core Specification`_ Volume 3 Part H Chapter 2
