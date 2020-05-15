.. _lpuart_sample:

Low Power UART
##############

The Low Power UART sample demonstrates the capabilities of the :ref:`uart_nrf_sw_lpuart` module.

Overview
********

The sample implements a simple loopback using a single UART instance.
The sample has console and logging disabled by default, to demonstrate low power consumption while having UART active.

Requirements
************

* One of the following development boards:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set22_start
   :end-before: set22_end

* The following pins shorted:

  * TX (Arduino Digital Pin 10) with RX (Arduino Digital Pin 11)
  * Request Pin (Arduino Digital Pin 12) with Response Pin (Arduino Digital Pin 13)

* A logic analyzer

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
