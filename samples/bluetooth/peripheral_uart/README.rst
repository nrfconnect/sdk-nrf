.. _peripheral_uart:

Bluetooth: Peripheral UART
#########################

Overview
********

This sample demonstrates NUS (Nordic UART) GATT Service. This example
uses the NUS service to send data back and forth between an UART
connection and an BLE connection.

When connected the example will forward any data received on the UART 1
peripherals RX pin to the BLE unit. On Nordic devkits, the UART 1
peripheral will typically be gated through the SEGGER chip to a USB
CDC virtual serial port.

Any data sendt from the BLE unit will be sent out of the UART 1 peripherals
TX pin.

Requirements
************

* Any Nordic development board
* A mobile running a compatible application (nRF Connect, nRF Toolbox)

Building and Running
********************

This sample can be found under :file:`samples/bluetooth/peripheral_uart` in the
Nordic Connect SDK tree.
See :ref:`bluetooth setup section <bluetooth_setup>` for details.
