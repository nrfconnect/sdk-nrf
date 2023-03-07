.. _slm_shell_sample:

nRF9160: SLM Shell
##################

.. contents::
   :local:
   :depth: 2

The SLM Shell sample demonstrates how to send AT commands to modem through the :ref:`Serial LTE Modem <slm_description>` application running on nRF9160 SiP.
This sample enables an external MCU to send modem or SLM proprietary AT commands for LTE connection and IP services.

Requirements
************

SLM application should be configured to use UART_2 on nRF9160 side.

The sample supports the following development kits:

.. table-from-sample-yaml::

Connect the DK with a nRF9160 DK based on the pin configuration in DTS overlay files of both sides.

The following table shows how to connect PCA10056 UART_1 to nRF9160 UART_2 for communication through UART:

.. list-table::
   :align: center
   :header-rows: 1

   * - nRF52840 DK
     - nRF9160 DK
   * - UART TX P1.02
     - UART RX P0.11
   * - UART RX P1.01
     - UART TX P0.10
   * - GPIO OUT P0.11 (Button1)
     - GPIO IN P0.31
   * - GPIO IN P0.13 (LED1 optional)
     - GPIO OUT P0.30 (optional)
   * - GPIO GND
     - GPIO GND

The following table shows how to connect PCA10095 UART_2 to nRF9160 UART_2 for communication through UART:

.. list-table::
   :align: center
   :header-rows: 1

   * - nRF5340 DK
     - nRF9160 DK
   * - UART TX P1.04
     - UART RX P0.11
   * - UART RX P1.05
     - UART TX P0.10
   * - GPIO OUT P0.23 (Button1)
     - GPIO IN P0.31
   * - GPIO IN P0.28 (LED1 optional)
     - GPIO OUT P0.30 (optional)
   * - GPIO GND
     - GPIO GND

.. note::
   SW9 on nRF9160 DK (VDD IO) should be set to 3V to work with the nRF52 and nRF53 series DK.

References
**********

`AT Commands Reference Guide`_

:ref:`SLM_AT_intro`

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_modem_slm`
