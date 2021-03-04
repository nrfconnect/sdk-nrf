.. _peripheral_uart:

Bluetooth: Peripheral UART
##########################

.. contents::
   :local:
   :depth: 2

The Peripheral UART sample demonstrates how to use the :ref:`nus_service_readme`.
It uses the NUS service to send data back and forth between a UART connection and a Bluetooth LE connection, emulating a serial port over Bluetooth LE.


Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 1 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 1 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 1 peripheral's TX pin.


.. _peripheral_uart_debug:

Debugging
*********

In this sample, the UART console is used to send and read data over the NUS service.
Debug messages are not displayed in this UART console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52833dk_nrf52820, nrf52dk_nrf52832, nrf52dk_nrf52810


The sample also requires a phone or tablet running a compatible application.
The `Testing`_ instructions refer to nRF Connect for Mobile, but similar applications (for example, nRF Toolbox) can be used as well.

You can also test the application with the :ref:`central_uart` sample.
See the documentation for that sample for detailed instructions.

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

Minimal build
=============

You can build the sample with a minimum configuration as a demonstration of how to reduce code size and RAM usage.

.. code-block:: console

   west build samples/bluetooth/peripheral_uart -- -DCONF_FILE='prj_minimal.conf'

.. _peripheral_uart_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Optionally, connect the RTT console to display debug messages. See :ref:`peripheral_uart_debug`.
#. Reset the kit.
#. Observe that **LED 1** is blinking and that the device is advertising with the device name that is configured in :option:`CONFIG_BT_DEVICE_NAME`.
#. Observe that the text "Starting Nordic UART service example" is printed on the COM listener running on the computer.
#. Connect to the device using nRF Connect for Mobile.
   Observe that **LED 2** is on.
#. Optionally, pair or bond with the device with MITM protection. This requires :ref:`RTT connection <testing_rtt_connect>`.
   To confirm pairing or bonding, press **Button 1** on the device and accept the passkey value on the smartphone.
#. In the application, observe that the services are shown in the connected device.
#. Select the UART RX characteristic value in nRF Connect.
   You can write to the UART RX and get the text displayed on the COM listener.
#. Type '0123456789' and tap :guilabel:`Write`.
   Verify that the text "0123456789" is displayed on the COM listener.
#. To send data from the device to your phone or tablet, enter any text, for example, "Hello", and press Enter to see it on the COM listener.
   Observe that a notification is sent to the peer.
#. Disconnect the device in nRF Connect.
   Observe that **LED 2** turns off.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_service_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``incude/gpio.h``
   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
