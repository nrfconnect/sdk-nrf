.. _peripheral_uart:

Bluetooth: Peripheral UART
##########################

The Peripheral UART sample demonstrates how to use the :ref:`nus_service_readme`.
It uses the NUS service to send data back and forth between a UART connection and a Bluetooth LE connection, emulating a serial port over Bluetooth LE.


Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 1 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 1 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 1 peripheral's TX pin.

Requirements
************

* One of the following development boards:

  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)

* A phone or tablet running a compatible application. The `Testing`_ instructions refer to nRF Connect for Mobile, but similar applications (for example, nRF Toolbox) can be used as well.

  You can also test the application with the :ref:`central_uart` sample. See the documentation for that sample for detailed instructions.

User interface
**************

LED 1:
   * Blinks with a period of 2 seconds, duty cycle 50%, when the main loop is running (device is advertising).

LED 2:
   * On when connected.

Button 1:
   * Confirm the passkey value that is printed on the COM listener to pair/bond with the other device.

Button 2:
   * Reject the passkey value that is printed on the COM listener to prevent pairing/bonding with the other device.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_uart`

.. include:: /includes/build_and_run.txt

.. _peripheral_uart_testing:

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the computer using a USB cable. The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the board.
#. Observe that LED 1 is blinking and that the device is advertising with the device name that is configured in :option:`CONFIG_BT_DEVICE_NAME <zephyr:CONFIG_BT_DEVICE_NAME>`.
#. Observe that the text "Starting Nordic UART service example" is printed on the COM listener running on the computer.
#. Connect to the device using nRF Connect for Mobile.
   Observe that LED 2 is on.
#. Optionally, pair/bond with the device with MITM protection.
   To confirm pairing/bonding, press Button 1 on the device and accept the passkey value on the smartphone.
   If tested with :ref:`central_uart` use the Button 1 on the both devices to confirm bonding.
#. In the app, observe that the services are shown in the connected device.
#. Select the UART RX characteristic value in nRF Connect.
   You can write hexadecimal ASCII values to the UART RX and get the text displayed on the COM listener.
#. Type '30 31 32 33 34 35 36 37 38 39' (the hexadecimal value for the string "0123456789") and tap **write**.
   Verify that the text "0123456789" is displayed on the COM listener.
#. To send data from the device to your phone or tablet, enter any text, for example, "Hello", and press Enter to see it on the COM listener.
   Observe that a notification with the corresponding ASCII values is sent to the peer on handle 0x12.
   For the string "Hello", the notification is '48 65 6C 6C 6F'.
#. Disconnect the device in nRF Connect.
   Observe that LED 2 turns off.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_service_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``incude/gpio.h``
   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
