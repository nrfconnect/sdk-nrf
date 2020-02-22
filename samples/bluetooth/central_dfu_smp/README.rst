.. _bluetooth_central_dfu_smp:

Bluetooth: Central DFU SMP
##########################

The Central DFU SMP sample demonstrates how to use the :ref:`dfu_smp_c_readme` to connect to an SMP Server and send a simple echo command.
The response, which is received as CBOR-encoded data, is decoded and printed.


Overview
********

After connecting, the sample starts MTU size negotiation, discovers the GATT database of the server, and configures the DFU SMP Client.
When configuration is complete, the sample is ready to send SMP commands.

To send an echo command, press Button 1 on the board.
The string that is sent contains a number that is automatically incremented.
This way, you can easily verify if the correct response is received.
The response is decoded and displayed using the `TinyCBOR`_ library (which is part of Zephyr).


Requirements
************

* One of the following development boards:

  * |nRF5340DK|
  * |nRF52840DK|
  * |nRF52DK|

* A device running `MCUmgr`_ with `SMP over Bluetooth`_, for example, another board running the :ref:`smp_svr_sample`


User interface
**************

Button 1:
   Send an echo command.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/central_dfu_smp`

.. include:: /includes/build_and_run.txt

.. _bluetooth_central_dfu_smp_testing:

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Connect the board to the computer using a USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the board.
#. Observe that the text "Starting DFU SMP Client example" is printed on the COM listener running on the computer and the device starts scanning for Peripheral boards with SMP.
#. Program the :ref:`smp_svr_sample` to another board.
   See the documentation for that sample for more information.
#. Observe that the boards connect.
   When service discovery is completed, the event logs are printed on the Central board's terminal.
   If you connect to the Server board with a terminal emulator, you can observe that it prints "connected".
#. Press Button 1 on the Client board.
   Observe messages similar to the following::

      Echo test: 1
      Echo response part received, size: 28.
      Total response received - decoding
      {_"r": "Echo message: 1"}

#. Disconnect the devices by, for example, pressing the Reset button on the Central board.
   Observe that the boards automatically reconnect and that it is again possible to send data between the two boards.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dfu_smp_c_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

It uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``

In addition, it uses the following external library that is distributed with Zephyr:

* `TinyCBOR`_
