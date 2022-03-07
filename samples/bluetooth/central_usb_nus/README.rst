.. _central_usb_nus:

Bluetooth: Central USB Nordic UART Service
##########################################

.. contents::
   :local:
   :depth: 2

The Central UART sample demonstrates how to use the :ref:`nus_client_readme`.
It uses the NUS Client to send data back and forth between a USB CDC ACM connection and a Bluetooth® LE connection, emulating a serial port over Bluetooth LE.

Overview
********

When USB is connected to a host PC, the board will enumerate as a CDC ACM device. On a Windows PC, it will appear as a COM port. On a Linux PC, it will appear
as /dev/ttyACMxxx.

On Bluetooth LE, it will scan for a device advertising the Nordic UART Service. When a device is found, the board will connect to it.

When connected, the sample forwards any data received via USB to the Bluetooth LE unit. Any data sent from the Bluetooth LE unit is sent out the USB port to the USB Host.


.. _central_usb_nus_debug:

Debugging
*********

This sample uses the board's USB connection for exchanging data with the NUS Client. 

Debug messages are displayed on the UART console. Most DKs have the UART console connected to the JLink CDC port. For boards that don't have
the JLink such as the nRF52840 Dongle, you can connect directly to the UART pins to view the debug messages.

FEM support
***********

.. include:: /includes/sample_fem_support.txt

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf21540dk_nrf52840, nrf52840dongle_nrf52840

The sample also requires another development kit running a compatible application (see :ref:`peripheral_uart`).

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_usb_nus`

.. include:: /includes/build_and_run.txt


.. _central_usb_nus_testing:

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. If using the Dongle, plug into a USB port on the computer. If using a DK, connect a USB cable to the kit's USB connector that is labeled nRF USB. 
   The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal_specific|
#. Optionally, connect to the UART to display debug messages. See :ref:`central_usb_nus_debug`.
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
   * ``include/usb/usb_device.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``