.. _central_uart:

Bluetooth: Central UART
#######################

The Central UART sample demonstrates how to use the :ref:`nus_c_readme`.
It uses the NUS Client to send data back and forth between a UART connection and a Bluetooth LE connection, emulating a serial port over Bluetooth LE.


Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 1 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 1 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 1 peripheral's TX pin.

Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

* Another development board running a compatible application (see :ref:`peripheral_uart`).

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_uart`

.. include:: /includes/build_and_run.txt


.. _central_uart_testing:

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the computer using a USB cable. The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal_specific|
#. Reset the board.
#. Observe that the text "Starting NUS Client example" is printed on the COM listener running on the computer and the device starts scanning for Peripheral boards with NUS.
#. Program the :ref:`peripheral_uart` sample to the second board.
   See the documentation for that sample for detailed instructions.
#. Observe that the boards connect.
   When service discovery is completed, the event logs are printed on the Central board's terminal.
#. Now you can send data between the two boards.
   To do so, type some characters in the terminal of one of the boards and hit Enter.
   Observe that the data is displayed on the UART on the other board.
#. Disconnect the devices by, for example, pressing the Reset button on the Central board.
   Observe that the boards automatically reconnect and that it is again possible to send data between the two boards.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_c_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
