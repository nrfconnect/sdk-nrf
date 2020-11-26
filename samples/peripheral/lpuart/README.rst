.. _lpuart_sample:

Low Power UART
##############

.. contents::
   :local:
   :depth: 2

The Low Power UART sample demonstrates the capabilities of the :ref:`uart_nrf_sw_lpuart` module.

Overview
********

The sample implements a simple loopback using a single UART instance.
The sample has console and logging disabled by default, to demonstrate low power consumption while having UART active.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160, nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf52dk_nrf52832, nrf5340dk_nrf5340_cpuapp

The sample also requires the following pins to be shorted:

* TX (Arduino Digital Pin 10) with RX (Arduino Digital Pin 11)
* Request Pin (Arduino Digital Pin 12) with Response Pin (Arduino Digital Pin 13)

Additionally, it requires a logic analyzer.

Building and running
********************
.. |sample path| replace:: :file:`samples/peripheral/lpuart`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Connect the logic analyzer to the shorted pins, to confirm UART activity.
2. Measure the current to confirm that the power consumption indicates that high-frequency clock is disabled during the idle stage.
   During the idle stage, the UART receiver is ready to start reception, as the request pin wakes it up.

Dependencies
************

This sample uses the following |NCS| driver:

* :ref:`uart_nrf_sw_lpuart`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:device_model_api`
* :ref:`zephyr:logging_api`
