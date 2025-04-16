.. _peripheral_hids_keyboard:

Bluetooth: Peripheral HIDS keyboard
###################################

.. contents::
   :local:
   :depth: 2

The Peripheral HIDS keyboard sample demonstrates how to use the :ref:`hids_readme` to implement a keyboard input device that you can connect to your computer.

The sample also shows how to perform LE Secure Connections Out-of-Band pairing using NFC.

.. note::
   |nrf_desktop_HID_ref|

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

If the NFC_OOB_PAIRING feature is enabled, the sample requires a smartphone or a tablet with Android v8.0.0 or newer.

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

When the application starts, it initializes and starts the NFCT peripheral that is used for pairing.
The application does not start advertising immediately, but only when the NFC tag is read by an NFC polling device, for example a smartphone or a tablet with NFC support.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      To trigger advertising without NFC, press **Button 4**.
      The NDEF message that the tag sends to the NFC device contains data required to initiate pairing.
      To start the NFC data transfer, the NFC device must touch the NFC antenna that is connected to the nRF52 device.

   .. group-tab:: nRF54 DKs

      To trigger advertising without NFC, press **Button 3**.
      The NDEF message that the tag sends to the NFC device contains data required to initiate pairing.
      To start the NFC data transfer, the NFC device must touch the NFC antenna that is connected to the nRF52 device.

After reading the tag, the device can pair with the nRF52 device which is advertising.
After connecting, the sample application behaves in the same way as the original HID Keyboard sample.
Reading the NFC tag again when the application is in a connected state causes disconnection.
When the connection is lost, advertising does not restart automatically.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Sends one character of the predefined input ("hello\\n") to the computer.

         When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 2:
         Simulates the Shift key.

         When pairing, press this button to reject the passkey value which is printed on the COM listener to prevent pairing with the other device.

      LED 1:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 2:
         Lit when at least one device is connected.

      LED 3:
         Indicates if Caps Lock is enabled.

      If the `NFC_OOB_PAIRING` feature is enabled:

      Button 4:
         Starts advertising.

      LED 4:
         Indicates if an NFC field is present.

   .. group-tab:: nRF54 DKs

      Button 0:
         Sends one character of the predefined input ("hello\\n") to the computer.

         When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 1:
         Simulates the Shift key.

         When pairing, press this button to reject the passkey value which is printed on the COM listener to prevent pairing with the other device.

      LED 0:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 1:
         Lit when at least one device is connected.

      LED 2:
         Indicates if Caps Lock is enabled.

      If the `NFC_OOB_PAIRING` feature is enabled:

      Button 3:
         Starts advertising.

      LED 3:
         Indicates if an NFC field is present.

Configuration
*************

|config|

Setup
=====

The HID service specification does not require encryption (:kconfig:option:`CONFIG_BT_HIDS_DEFAULT_PERM_RW_ENCRYPT`), but some systems disconnect from the HID devices that do not support security.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hids_keyboard`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to your development kit, you can test it either by connecting the kit as keyboard device to a Microsoft Windows computer or by connecting to it with `nRF Connect for Desktop`_.

Testing with a Microsoft Windows computer
-----------------------------------------

To test with a Microsoft Windows computer that has a Bluetooth radio, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Power on your development kit.
      #. Press **Button 4** on the kit if the device is not advertising.
         A blinking **LED 1** indicates that the device is advertising.
      #. |connect_terminal|
      #. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS keyboard".
      #. Observe that the connection state is indicated by **LED 2**.
      #. Open a text editor (for example, Notepad).
      #. Repeatedly press **Button 1** on the kit.
         Every button press sends one character of the test message "hello" (the test message includes a carriage return) to the computer, and this will be displayed in the text editor.
      #. Press **Button 2** and hold it while pressing **Button 1**.
         Observe that the next letter of the "hello" message appears as capital letter.
         This is because **Button 2** simulates the Shift key.
      #. Turn on Caps Lock on the computer.
         Observe that **LED 3** turns on.
         This confirms that the output report characteristic has been written successfully on the HID device.
      #. Turn Caps Lock off on the computer.
         Observe that **LED 3** turns off.
      #. Disconnect the computer from the device by removing the device from the computer's devices list.

   .. group-tab:: nRF54 DKs

      1. Power on your development kit.
      #. Press **Button 3** on the kit if the device is not advertising.
         A blinking **LED 0** indicates that the device is advertising.
      #. |connect_terminal|
      #. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS keyboard".
      #. Observe that the connection state is indicated by **LED 1**.
      #. Open a text editor (for example, Notepad).
      #. Repeatedly press **Button 0** on the kit.
         Every button press sends one character of the test message "hello" (the test message includes a carriage return) to the computer, and this will be displayed in the text editor.
      #. Press **Button 1** and hold it while pressing **Button 0**.
         Observe that the next letter of the "hello" message appears as capital letter.
         This is because **Button 1** simulates the Shift key.
      #. Turn on Caps Lock on the computer.
         Observe that **LED 2** turns on.
         This confirms that the output report characteristic has been written successfully on the HID device.
      #. Turn Caps Lock off on the computer.
         Observe that **LED 2** turns off.
      #. Disconnect the computer from the device by removing the device from the computer's devices list.


Testing with Bluetooth Low Energy app
-------------------------------------

To test with the `Bluetooth Low Energy app`_, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Power on your development kit.
      #. Press **Button 4** on the kit if the device is not advertising.
         A blinking **LED 1** indicates that the device is advertising.
      #. |connect_terminal|
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_.
      #. Connect to the device from the app. The device is advertising as "NCS HIDS keyboard".
      #. Pair the devices:

         a. Click the :guilabel:`Settings` button for the device in the app.
         #. Select :guilabel:`Pair`.
         #. Optionally, select :guilabel:`Perform Bonding`.

         Optionally, check :guilabel:`Enable MITM protection` to pair with MITM protection and use a button on the device to confirm or reject the passkey value.

         Wait until the bond is established before you continue.

      #. Observe that the connection state is indicated by **LED 2**.
      #. Observe that the services of the connected device are shown.
      #. Enable notifications for all HID characteristics.
      #. Press **Button 1** on the kit.
         Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.

         The first notification has the value ``00000B0000000000``, the second has the value ``0000000000000000``.
         The received values correspond to press and release of character "h".
         The format used for keyboard reports is the following byte array: ``[modifier, reserved, Key1, Key2, Key3, Key4, Key5, Key6]``.

         Similarly, further press and release events will result in press and release notifications of subsequent characters of the test string.
         Therefore, pressing **Button 1** again will result in notification of press and release reports for character "e".
      #. Press **Button 2** and hold it while pressing **Button 1**.
         Pressing **Button 2** changes the modifier bit to 02, which simulates pressing the Shift key on a keyboard.

         Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.
         The first one has the value ``02000F0000000000``, the second has the value ``0200000000000000``.
         These values correspond to press and release of character "l" with the Shift key pressed.
      #. In the app, select the HID Report (which has UUID ``0x2A4D`` and the properties ``Read``, ``WriteWithoutResponse``, and ``Write``).
         Enter any number that has bit 1 set to ``1`` (for example ``02``) in the text box and click the :guilabel:`tick mark` button.
         This sets the Output Report to the written value, in which setting bit 1 to ``1`` simulates turning Caps Lock ON.
         Other bits are responsible for controlling other Output Report fields that are not reflected in the sample's UI, for example Num Lock.

         Observe that **LED 3** is lit.
      #. Select the same HID Report again.
         Enter any number that has bit 1 set to ``0`` (for example ``00``) in the text box and click :guilabel:`Write`.
         Setting this bit of the Output Report to ``0`` simulates turning Caps Lock OFF.

         Observe that **LED 3** turns off.
      #. Disconnect the device in the app.
         Observe that no new notifications are received.
      #. If the advertising did not start automatically, press **Button 4** to continue advertising.
      #. As bond information is preserved by the app, you can immediately reconnect to the device by clicking the :guilabel:`Connect` button again.

   .. group-tab:: nRF54 DKs

      1. Power on your development kit.
      #. Press **Button 3** on the kit if the device is not advertising.
         A blinking **LED 0** indicates that the device is advertising.
      #. |connect_terminal|
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_.
      #. Connect to the device from the app.
         The device is advertising as "NCS HIDS keyboard".
      #. Pair the devices:

         a. Click the :guilabel:`Settings` button for the device in the app.
         #. Select :guilabel:`Pair`.
         #. Optionally, select :guilabel:`Perform Bonding`.

         Optionally, check :guilabel:`Enable MITM protection` to pair with MITM protection and use a button on the device to confirm or reject the passkey value.

         Wait until the bond is established before you continue.

      #. Observe that the connection state is indicated by **LED 1**.
      #. Observe that the services of the connected device are shown.
      #. Enable notifications for all HID characteristics.
      #. Press **Button 0** on the kit.
         Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.

         The first notification has the value ``00000B0000000000``, the second has the value ``0000000000000000``.
         The received values correspond to press and release of character "h".
         The format used for keyboard reports is the following byte array: ``[modifier, reserved, Key1, Key2, Key3, Key4, Key5, Key6]``.

         Similarly, further press and release events will result in press and release notifications of subsequent characters of the test string.
         Therefore, pressing **Button 0** again will result in notification of press and release reports for character "e".
      #. Press **Button 1** and hold it while pressing **Button 0**.
         Pressing **Button 1** changes the modifier bit to 02, which simulates pressing the Shift key on a keyboard.

         Observe that two notifications are received on one of the HID Report characteristics, denoting press and release for one character of the test message.
         The first one has the value ``02000F0000000000``, the second has the value ``0200000000000000``.
         These values correspond to press and release of character "l" with the Shift key pressed.
      #. In the app, select the HID Report (which has UUID ``0x2A4D`` and the properties ``Read``, ``WriteWithoutResponse``, and ``Write``).
         Enter any number that has bit 1 set to ``1`` (for example ``02``) in the text box and click the :guilabel:`tick mark` button.
         This sets the Output Report to the written value, in which setting bit 1 to ``1`` simulates turning Caps Lock ON.
         Other bits are responsible for controlling other Output Report fields that are not reflected in the sample's UI, for example Num Lock.

         Observe that **LED 2** is lit.
      #. Select the same HID Report again.
         Enter any number that has bit 1 set to ``0`` (for example ``00``) in the text box and click :guilabel:`Write`.
         Setting this bit of the Output Report to ``0`` simulates turning Caps Lock OFF.

         Observe that **LED 2** turns off.
      #. Disconnect the device in the app.
         Observe that no new notifications are received.
      #. If the advertising did not start automatically, press **Button 3** to continue advertising.
      #. As bond information is preserved by the app, you can immediately reconnect to the device by clicking the :guilabel:`Connect` button again.


Testing with Android using NFC for pairing
------------------------------------------

To test with an Android smartphone/tablet, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 4** is lit.
      #. Observe that the device is advertising, as indicated by blinking **LED 1**.
      #. Confirm pairing with 'Nordic_HIDS_keyboard' in a pop-up window on the smartphone/tablet.
      #. Observe that the connection state is indicated by **LED 2**.
      #. Repeatedly press **Button 1** on the kit.
         Every button press sends one character of the test message "hello" to the smartphone (the test message includes a carriage return).
      #. Press **Button 2** and hold it while pressing **Button 1**.
         Observe that the next letter of the "hello" message appears as a capital letter.
         This is because **Button 2** simulates the Shift key.
      #. Touch the NFC antenna with the same central device again.
      #. Observe that devices disconnect.

   .. group-tab:: nRF54 DKs

      1. Touch the NFC antenna with the smartphone or tablet and observe that **LED 3** is lit.
      #. Observe that the device is advertising, as indicated by blinking **LED 0**.
      #. Confirm pairing with 'Nordic_HIDS_keyboard' in a pop-up window on the smartphone/tablet.
      #. Observe that the connection state is indicated by **LED 1**.
      #. Repeatedly press **Button 0** on the kit.
         Every button press sends one character of the test message "hello" to the smartphone (the test message includes a carriage return).
      #. Press **Button 1** and hold it while pressing **Button 0**.
         Observe that the next letter of the "hello" message appears as a capital letter.
         This is because **Button 1** simulates the Shift key.
      #. Touch the NFC antenna with the same central device again.
      #. Observe that devices disconnect.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`hids_readme`
* :ref:`nfc_ndef` (if the `NFC_OOB_PAIRING` option is enabled)
* :ref:`nfc_ndef_le_oob` (if the `NFC_OOB_PAIRING` option is enabled)

When the `NFC_OOB_PAIRING` feature is enabled, it also uses the Type 2 Tag library from `sdk-nrfxlib`_:

* :ref:`nrfxlib:nfc_api_type2`
* :ref:`dk_buttons_and_leds_readme`

The sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:settings_api`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`samples/bluetooth/gatt/bas.h`

References
**********

* `HID Service Specification`_
* `HID usage tables`_
* `Bluetooth Secure Simple Pairing Using NFC`_
* `Bluetooth Core Specification`_ Volume 3 Part H Chapter 2

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
