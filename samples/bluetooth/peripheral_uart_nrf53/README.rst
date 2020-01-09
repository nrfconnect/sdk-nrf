.. _peripheral_uart_nrf53:

Bluetooth: Peripheral UART nRF5340
##################################

The Peripheral UART sample demonstrates how to use the :ref:`nus_service_readme` on a dual core device.
It uses the NUS service to send data back and forth between a UART connection and a Bluetooth LE connection, emulating a serial port over Bluetooth LE.

Overview
********

When connected, the sample forwards any data received on the RX pin of the application core UART 0 peripheral to the network core that runs the Bluetooth LE radio and stack.
On Nordic Semiconductor's development kits, the UART 0 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from Bluetooth LE on the network core is sent out of the UART 0 peripheral's TX pin on the application core.

Network core
============

The network core runs the whole Bluetooth stack with the GATT NUS service.
It decodes commands from the application core and issues a corresponding call to the Bluetooth stack and the NUS service.
Some events from the Bluetooth stack and the NUS service, for example ``connection established``, ``disconnection``, or ``data received by NUS``, are encoded by a codec in `TinyCBOR`_ format and transmitted to the application core using `RPMsg`_ (part of `OpenAMP`_).

Application core
================

The application core runs a serialized application, where the Bluetooth commands and events are replaced by encoders and decoders.
For the needs of the NUS service, this application uses only :cpp:func:`bt_nus_transmit` to send data over the NUS service.

Requirements
************

* The following development board:

  * |nRF5340DK|

* A phone or tablet running a compatible application.
  The :ref:`peripheral_uart_nrf53_testing` instructions refer to nRF Connect for Mobile, but similar applications (for example, nRF Toolbox) can be used as well.

  You can also test the application with the :ref:`central_uart` sample.
  See the documentation for that sample for detailed instructions.

User interface
**************

LED 1:
   * Blinks with a period of 2 seconds, duty cycle 50%, when the device is advertising.

LED 2:
   * On when connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_uart_nrf53`

.. include:: /includes/build_and_run.txt

.. _peripheral_uart_nrf53_testing:

Testing
=======

This sample consists of two sample applications, one for the network core, and one for the application core:

   * Application core sample: :file:`peripheral_uart_nrf53/cpuapp`
   * Network core sample: :file:`peripheral_uart_nrf53/cpunet`

Both of these sample applications must be built and flashed before testing.
For details on building samples for a dual core device, see :ref:`ug_nrf5340_building`.

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the computer using a USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the board.
#. Observe that LED 1 is blinking and that the device is advertising with the device name that is configured on the network core application in :option:`CONFIG_BT_DEVICE_NAME <zephyr:CONFIG_BT_DEVICE_NAME>`.
#. Observe that the text "Starting Nordic UART service example[APP CORE]" is printed on the COM listener running on the computer.
#. Connect to the device using `nRF Connect for Mobile`_.
   Observe that LED 2 is on.
#. Optionally, pair or bond with the device.
   The device supports only the Just Works method.
#. In the nRF Connect for Mobile, observe that the services are shown in the connected device.
#. Select the UART RX characteristic value in nRF Connect for Mobile.
   You can write hexadecimal ASCII values to the UART RX and get the text displayed on the COM listener.
#. Type '30 31 32 33 34 35 36 37 38 39' (the hexadecimal value for the string "0123456789") and tap **Write**.
   Verify that the text "0123456789" is displayed on the COM listener.
#. To send data from the device to your phone or tablet, enter any text, for example, "Hello", and press Enter to see it on the COM listener.
   Observe that a notification with the corresponding ASCII values is sent to the peer on handle 0x12.
   For the string "Hello", the notification is '48 65 6C 6C 6F'.
#. Disconnect the device in nRF Connect for Mobile.
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
