.. _bluetooth-hci-lpuart-sample:

Bluetooth: HCI low power UART
#############################

.. contents::
   :local:
   :depth: 2

The HCI low power UART sample is based on the :ref:`zephyr:bluetooth-hci-uart-sample` but is using the :ref:`uart_nrf_sw_lpuart` for HCI UART communication.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf52840

Overview
********

The sample implements the Bluetooth HCI controller using the :ref:`uart_nrf_sw_lpuart` for UART communication.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/hci_lpuart`

.. include:: /includes/build_and_run.txt

Programming the sample
======================

To program the nRF9160 development kit with the sample:

1. Set the **SW5** switch, marked as debug/prog, in the **NRF52** position.
#. Build the :ref:`bluetooth-hci-lpuart-sample` sample for the nrf9160dk_nrf52840 build target and program the development kit with it.

Testing
=======

The methodology to use to test this sample depends on the host application.

Dependencies
************

This sample uses the following |NCS| driver:

* :ref:`uart_nrf_sw_lpuart`
