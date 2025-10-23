.. _slm_shell_sample:

Cellular: SLM Shell
###################

.. contents::
   :local:
   :depth: 2

The SLM Shell sample demonstrates how to send AT commands to modem through the :ref:`Serial LTE Modem <slm_description>` application running on nRF91 Series SiP.
This sample enables an external MCU to send modem or SLM proprietary AT commands for LTE connection and IP services.
See more information on the functionality of this sample from the :ref:`lib_modem_slm` library, which provides the core functionality for this sample.

Requirements
************

The SLM application should be configured to use UART2 on the nRF91 Series DK side with hardware flow control.

The sample supports the following development kits:

.. table-from-sample-yaml::

Connect the DK with an nRF91 Series DK based on the pin configuration in DTS overlay files of both sides.

The following table shows how to connect UART1 of the DK to the nRF91 Series DK's UART2 for communication through UART:

.. tabs::

   .. group-tab:: nRF52840 DK

      .. list-table::
         :header-rows: 1

         * - nRF52840 DK
           - nRF91 Series DK
         * - UART TX P1.02
           - UART RX P0.11
         * - UART RX P1.01
           - UART TX P0.10
         * - UART CTS P1.07
           - UART RTS P0.12
         * - UART RTS P1.06
           - UART CTS P0.13
         * - GPIO OUT P0.11 (Button1)
           - GPIO IN P0.31
         * - GPIO IN P0.13 (LED1 optional)
           - GPIO OUT P0.30 (optional)
         * - GPIO GND
           - GPIO GND

      .. note::
         The GPIO output level on the nRF91 Series device side must be 3 V to work with the nRF52 Series DK.

         * For nRF91x1 DK, you can set the VDD voltage with the `Board Configurator app`_.
         * For nRF9160 DK, you can set the VDD voltage with the **VDD IO** switch (**SW9**).
           See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information related to nRF9160 DK.

   .. group-tab:: nRF5340 DK

      .. list-table::
         :header-rows: 1

         * - nRF5340 DK
           - nRF91 Series DK
         * - UART TX P1.04
           - UART RX P0.11
         * - UART RX P1.05
           - UART TX P0.10
         * - UART CTS P1.07
           - UART RTS P0.12
         * - UART RTS P1.06
           - UART CTS P0.13
         * - GPIO OUT P0.23 (Button1)
           - GPIO IN P0.31
         * - GPIO IN P0.28 (LED1 optional)
           - GPIO OUT P0.30 (optional)
         * - GPIO GND
           - GPIO GND

      .. note::
         The GPIO output level on the nRF91 Series device side must be 3 V to work with the nRF53 Series DK.

         * For nRF91x1 DK, you can set the VDD voltage with the `Board Configurator app`_.
         * For nRF9160 DK, you can set the VDD voltage with the **VDD IO** switch (**SW9**).
           See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information related to nRF9160 DK

   .. group-tab:: nRF7002 DK

      .. list-table::
         :header-rows: 1

         * - nRF7002 DK
           - nRF91 Series DK
         * - UART TX P1.04
           - UART RX P0.11
         * - UART RX P1.05
           - UART TX P0.10
         * - UART CTS P1.07
           - UART RTS P0.12
         * - UART RTS P1.06
           - UART CTS P0.13
         * - GPIO OUT P0.31
           - GPIO IN P0.31
         * - GPIO IN P0.30 (optional)
           - GPIO OUT P0.30 (optional)
         * - GPIO GND
           - GPIO GND

      .. note::
         The GPIO output level on the nRF91 Series device side must be 1.8 V to work with the nRF7002 DK.

         * For nRF91x1 DK, you can set the VDD voltage with the `Board Configurator app`_.
         * For nRF9160 DK, you can set the VDD voltage with the **VDD IO** switch (**SW9**).
           See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information related to nRF9160 DK.

   .. group-tab:: nRF54L15 DK

      .. list-table::
         :header-rows: 1

         * - nRF54L15 DK
           - nRF91 Series DK
         * - UART TX P1.11
           - UART RX P0.11
         * - UART RX P1.12
           - UART TX P0.10
         * - UART CTS P1.07 (optional)
           - UART RTS P0.12 (optional)
         * - UART RTS P1.06 (optional)
           - UART CTS P0.13 (optional)
         * - GPIO OUT P0.04 (Button 3)
           - GPIO IN P0.31
         * - GPIO IN P0.30 (optional)
           - GPIO OUT P0.03 (optional)
         * - GPIO GND
           - GPIO GND

      .. note::
         The GPIO output level on the nRF91 Series device side must be 1.8 V to work with the nRF54L15 DK.

         * For nRF91x1 DK, you can set the VDD voltage with the `Board Configurator app`_.
         * For nRF9160 DK, you can set the VDD voltage with the **VDD IO** switch (**SW9**).
           See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information related to nRF9160 DK.

References
**********

* `nRF91x1 AT Commands Reference Guide`_
* `nRF9160 AT Commands Reference Guide`_
* :ref:`SLM_AT_commands`

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_modem_slm`
