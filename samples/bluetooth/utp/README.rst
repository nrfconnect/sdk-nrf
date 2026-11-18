.. _bluetooth_utp:

.. ncs-sample::
   :title: Bluetooth: UTP

   This sample builds a Bluetooth® LE HCI controller that is accessed over UART and that supports the Unified Test Protocol (UTP), the Nordic UTP extensions and Direct Test Mode (DTM) over HCI.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample requires a host that sends HCI commands to the controller over UART.
This can for example be a Bluetooth RFPHY tester.

Overview
********

The sample extends the Zephyr :zephyr:code-sample:`bluetooth_hci_uart_async` sample.
It runs the Bluetooth LE controller on the device and exposes it to the host as an HCI H4 transport over UART.
The controller supports the following test features:

* UTP
* Nordic UTP extensions
* DTM over HCI

Implementation
==============

The sample reuses the sources of the Zephyr :zephyr:code-sample:`bluetooth_hci_uart_async` sample.
The controller is configured in :file:`prj.conf`, and the UART used for HCI is selected in a devicetree overlay.

UART configuration
==================

The UART that carries the HCI traffic is set as the ``zephyr,bt-c2h-uart`` chosen node and is configured to run at 1000000 baud with hardware flow control.
The sample removes the console, shell, MCUmgr and Bluetooth monitor assignments from the chosen nodes, so that they do not interfere with the HCI UART.

Each supported board has its own overlay in the :file:`boards` directory.
The following table lists the UART instance used on each board.

.. list-table::
   :header-rows: 1

   * - Board target
     - HCI UART
   * - ``nrf54h20dk/nrf54h20/cpurad``
     - ``uart135``
   * - ``nrf54l15dk/nrf54l05/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``
     - ``uart20``
   * - ``nrf54lc10dk/nrf54lc10a/cpuapp``
     - ``uart30``
   * - ``nrf54lm20dk/nrf54lm20b/cpuapp``
     - ``uart20``
   * - ``nrf54ls05dk/nrf54ls05a/cpuapp``, ``nrf54ls05dk/nrf54ls05b/cpuapp``
     - ``uart20``
   * - ``nrf54lv10dk/nrf54lv10a/cpuapp``
     - ``uart20``
   * - ``nrf7120dk/nrf7120/cpuapp``
     - ``uart20``

Configuration
*************
|config|

Configuration options
=====================

The sample configuration is in the :file:`prj.conf` file.
It enables the following Kconfig options:

* :kconfig:option:`CONFIG_BT_CTLR_SDC_UTP_HCI` - Enables UTP over HCI support in the controller.
* :kconfig:option:`CONFIG_BT_CTLR_SDC_UTP_VS_NORDIC` - Enables the Nordic vendor-specific UTP additions.
* :kconfig:option:`CONFIG_BT_CTLR_DTM_HCI` - Enables Direct Test Mode over HCI.
* :kconfig:option:`CONFIG_BT_HCI_RAW` and :kconfig:option:`CONFIG_BT_HCI_RAW_H4` - Expose the controller as a raw HCI H:4 interface.
* :kconfig:option:`CONFIG_UART_ASYNC_API` - Enables the asynchronous UART API used by the HCI transport.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/utp`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, connect the host to the serial port of the HCI UART.
Use a baud rate of 1000000 with hardware flow control enabled.
Then send HCI commands to the controller.

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`
* :ref:`zephyr:uart_api`
