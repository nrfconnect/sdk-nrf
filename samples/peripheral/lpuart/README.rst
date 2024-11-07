.. _lpuart_sample:

Low Power UART
##############

.. contents::
   :local:
   :depth: 2

The Low Power UART sample demonstrates the capabilities of the :ref:`uart_nrf_sw_lpuart` module.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires the following pins to be shorted:

   .. list-table:: Pin connections.
      :widths: auto
      :header-rows: 1

      * - Development Kit
        - nRF52 DK
        - nRF52833 DK
        - nRF52840 DK
        - nRF21540 DK
        - nRF5340 DK pins
        - nRF54L15 DK pins
        - nRF9160 DK pins
      * - Request-Response Pins
        - P0.24-P0.25
        - P0.22-P0.23
        - P1.14-P1.15
        - P1.07-P1.08
        - P1.14-P1.15
        - P1.08-P1.09
        - P0.12-P0.13
      * - UART RX-TX Pins
        - P0.22-P0.23
        - P0.20-P0.21
        - P1.12-P1.13
        - P1.05-P1.06
        - P1.12-P1.13
        - P1.10-P1.11
        - P0.10-P0.11

Additionally, it requires a logic analyzer.

Overview
********

The sample implements a simple loopback using a single UART instance.
By default, the console and logging are disabled to demonstrate low power consumption when UART is active.

Configuration
*************

|config|

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************
.. |sample path| replace:: :file:`samples/peripheral/lpuart`

.. include:: /includes/build_and_run_ns.txt

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

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
