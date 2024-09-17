.. _bluetooth-hci-lpuart-sample:

Bluetooth: HCI low power UART
#############################

.. contents::
   :local:
   :depth: 2

The HCI low power UART sample is based on the :zephyr:code-sample:`bluetooth_hci_uart` but is using the :ref:`uart_nrf_sw_lpuart` for HCI UART communication.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample implements the Bluetooth® HCI controller using the :ref:`uart_nrf_sw_lpuart` for UART communication.

This sample is also supported on the Thingy:91.
However, it must be programmed using a debugger and a 10-pin SWD cable.
Firmware updates over serial using MCUboot are not supported for either of the MCUs on the Thingy:91 in this configuration.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/hci_lpuart`

.. include:: /includes/build_and_run.txt

Programming the sample
======================

To program the nRF9160 development kit with the sample:

1. Set the **SW10** switch, marked as debug/prog, in the **NRF52** position.
   In nRF9160 DK v0.9.0 and earlier, the switch is called **SW5**
#. Build the :ref:`bluetooth-hci-lpuart-sample` sample for the ``nrf9160dk/nrf52840`` board target and program the development kit with it.

Testing
=======

The methodology to use to test this sample depends on the host application.

Dependencies
************

This sample uses the following |NCS| driver:

* :ref:`uart_nrf_sw_lpuart`
