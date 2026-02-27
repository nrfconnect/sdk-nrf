.. _central_uart:

Bluetooth: Central UART
#######################

.. contents::
   :local:
   :depth: 2

The Central UART sample demonstrates how to use the :ref:`nus_client_readme`.
It uses the NUS Client to send data back and forth between a UART connection and a Bluetooth® LE connection, emulating a serial port over Bluetooth LE.

.. _central_uart_requirements:

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires another development kit running a compatible application (see :ref:`peripheral_uart`).

Overview
********

When connected to a Bluetooth LE device implementing the :ref:`nus_service_readme`, the sample relays data between a UART peripheral and the Bluetooth LE connection.
On Nordic Semiconductor's development kits, the UART peripheral is typically gated through the SEGGER chip to a USB CDC ACM virtual serial port.

.. _central_uart_nrf54lm20dongle_usb_cdc_acm_interfaces:

nRF54LM20 Dongle USB CDC ACM interfaces
=======================================

The nRF54LM20 Dongle does not have a built-in SEGGER chip, so on this board, the sample implements the USB CDC ACM device class with several interfaces for different purposes.
When you plug the dongle into a USB port, the following interfaces are available:

.. list-table::
   :header-rows: 1

   * - Index
     - Label
     - Purpose
   * - 0
     - (no label)
     - Zephyr console (logging)
   * - 1
     - NUS
     - Nordic UART Service

.. _central_uart_debug:

Debugging
*********

Depending on the board, this sample uses either the SEGGER J-Link RTT or a USB CDC ACM interface to display debug messages.

.. tabs::

   .. group-tab:: nRF54LM20 Dongle

      The sample uses the CDC ACM interface with index 0 for debug logging (see :ref:`central_uart_nrf54lm20dongle_usb_cdc_acm_interfaces`).
      Open the interface in a terminal emulator to view debug messages.

   .. group-tab:: Other supported boards

      See the list of supported boards in :ref:`central_uart_requirements`.

      The sample uses the SEGGER J-Link RTT for debug logging.
      To view debug messages, follow the procedure in :ref:`testing_rtt_connect`.


FEM support
***********

.. include:: /includes/sample_fem_support.txt


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_uart`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. _central_uart_testing:

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal_specific|

   .. note::
      On the nRF54LM20 Dongle, the sample uses the USB CDC ACM interface with index 1 for NUS data (see :ref:`central_uart_nrf54lm20dongle_usb_cdc_acm_interfaces`).

#. Optionally, view debug messages as described in :ref:`central_uart_debug`.
#. Reset the kit.
#. Observe that the text "Starting Bluetooth Central UART sample" is printed on the COM listener running on the computer and the device starts scanning for Peripherals with NUS.
#. Program the :ref:`peripheral_uart` sample to the second development kit.
   See the documentation for that sample for detailed instructions.
#. Observe that the kits connect.

   When service discovery is completed, the event logs are printed on the Central's terminal.
   Now you can send data between the two kits.
#. To send data, type some characters in the terminal of one of the kits and press Enter.
   Observe that the data is displayed on the UART on the other kit.
#. Disconnect the devices by, for example, pressing the Reset button on the Central.
   Observe that the kits automatically reconnect and that it is again possible to send data between the two kits.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`
* :ref:`lib_uart_async_adapter`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`boards/arm/nrf*/board.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:api_peripherals`:

   * :file:`include/uart.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
