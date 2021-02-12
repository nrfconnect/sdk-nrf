.. _bluetooth_central_dfu_smp:

Bluetooth: Central DFU SMP
##########################

.. contents::
   :local:
   :depth: 2

The Central DFU SMP sample demonstrates how to use the :ref:`dfu_smp_readme` to connect to an SMP Server and send a simple echo command.
The response, which is received as CBOR-encoded data, is decoded and printed.


Overview
********

After connecting, the sample starts MTU size negotiation, discovers the GATT database of the server, and configures the DFU SMP Client.
When configuration is complete, the sample is ready to send SMP commands.

To send an echo command, press Button 1 on the development kit
The string that is sent contains a number that is automatically incremented.
This way, you can easily verify if the correct response is received.
The response is decoded and displayed using the `TinyCBOR`_ library (which is part of Zephyr).


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns , nrf52840dk_nrf52840, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52dk_nrf52832, nrf52dk_nrf52810

The sample also requires a device running `mcumgr`_ with transport protocol over Bluetooth Low Energy, for example, another development kit running the :ref:`smp_svr_sample`.

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

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the kit to the computer using a USB cable.
   The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Reset the kit.
#. Observe that the text "Starting DFU SMP Client example" is printed on the COM listener running on the computer and the device starts scanning for Peripherals with SMP.
#. Program the :ref:`smp_svr_sample` to another development kit.
   See the documentation for that sample for more information.
#. Observe that the kits connect.
   When service discovery is completed, the event logs are printed on the Central's terminal.
   If you connect to the Server with a terminal emulator, you can observe that it prints "connected".
#. Press Button 1 on the Client.
   Observe messages similar to the following::

      Echo test: 1
      Echo response part received, size: 28.
      Total response received - decoding
      {_"r": "Echo message: 1"}

#. Disconnect the devices by, for example, pressing the Reset button on the Central.
   Observe that the kits automatically reconnect and that it is again possible to send data between the two kits.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dfu_smp_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

It uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``

In addition, it uses the following external library that is distributed with Zephyr:

* `TinyCBOR`_
