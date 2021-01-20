.. _central_uart:

Bluetooth: Central UART
#######################

.. contents::
   :local:
   :depth: 2

The Central UART sample demonstrates how to use the :ref:`nus_client_readme`.
It uses the NUS Client to send data back and forth between a UART connection and a Bluetooth LE connection, emulating a serial port over Bluetooth LE.


Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 1 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 1 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 1 peripheral's TX pin.


.. _central_uart_debug:

Debugging
*********

In this sample, a UART console is used to send and read data over the NUS Client.
Debug messages are not displayed in the UART console, but are printed by the RTT logger instead.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52833dk_nrf52820

The sample also requires another development kit running a compatible application (see :ref:`peripheral_uart`).

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_uart`

.. include:: /includes/build_and_run.txt


.. _central_uart_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal_specific|
#. Optionally, connect the RTT console to display debug messages. See :ref:`central_uart_debug`.
#. Reset the kit.
#. Observe that the text "Starting NUS Client example" is printed on the COM listener running on the computer and the device starts scanning for Peripherals with NUS.
#. Program the :ref:`peripheral_uart` sample to the second development kit.
   See the documentation for that sample for detailed instructions.
#. Observe that the kits connect.
   When service discovery is completed, the event logs are printed on the Central's terminal.
#. Now you can send data between the two kits.
   To do so, type some characters in the terminal of one of the kits and hit Enter.
   Observe that the data is displayed on the UART on the other kit.
#. Disconnect the devices by, for example, pressing the Reset button on the Central.
   Observe that the kits automatically reconnect and that it is again possible to send data between the two kits.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
