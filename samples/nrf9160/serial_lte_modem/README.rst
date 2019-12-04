.. _serial_lte_modem:

nRF9160: Serial LTE Modem
#########################

The Serial LTE Modem (SLM) sample demostrates sending AT commands between a host and a client device.
An nRF9160 DK is used as the host, while the client can be simulated using either a PC or an nRF52 DK.

This sample is an enhancement to the :ref:`at_client_sample` sample.
It provides the following features:

 * Support for TCP/IP proprietary AT commands
 * Support for UDP proprietary AT commands
 * Support for GPS proprietary AT commands
 * Support for communication to external MCU over UART

All nRF91 modem AT commands are also supported.

Requirements
************

* The following development board:

    * |nRF9160DK|

* If the client is a PC:

    * Any terminal software, such as TeraTerm.

* If the client is an nRF52 device:

    * |nRF52840DK|
    * |nRF52DK|

.. terminal_config

Terminal connection
===================

By default, configuration option ``CONFIG_SLM_CONNECT_UART_0`` is defined.
This means that you can use the J-Link COM port on the PC side to connect with nRF9160 DK, and send or receive AT commands there.

Terminal serial configuration:

    * Hardware flow control: disabled
    * Baud rate: 115200
    * Parity bit: no

.. note::
   * The default AT command terminator is Carrier Return and Line Feed, i.e. ``\r\n``.
   * nRF91 logs are output to the same terminal port.

External MCU configuration
==========================

To work with external MCU (nRF52), you must set the configuration option ``CONFIG_SLM_CONNECT_UART_2``.
The pin interconnection between nRF91 and nRF52 is presented in the following table:

.. list-table::
   :align: center
   :header-rows: 1

   * - nRF52 DK
     - nRF91 DK
   * - UART TX P0.6
     - UART RX P0.11
   * - UART RX P0.8
     - UART TX P0.10
   * - UART CTS P0.7
     - UART RTS P0.12
   * - UART RTS P0.5
     - UART RTS P0.13
   * - GPIO OUT P0.27
     - GPIO IN P0.31

UART instance in use:

    * nRF52840 and nRF52832 (UART0)
    * nRF9160 (UART2)

UART configuration:

    * Hardware flow control: enabled
    * Baud rate: 115200
    * Parity bit: no
    * Operation mode: IRQ

Note that the GPIO output level on nRF91 side should be 3 V.

TCP/IP AT commands
******************

The following proprietary TCP/IP AT commands are used in this sample:

* AT#XSOCKET=<op>[,<type>]
* AT#XSOCKET?
* AT#XBIND=<port>
* AT#XTCPCONN=<url>,<port>
* AT#XTCPCONN?
* AT#XTCPSEND=<data>
* AT#XTCPRECV=<length>,<timeout>

UDP AT commands
***************

The following proprietary UDP AT commands are used in this sample:

* AT#XUDPSENDTO=<url>,<port>,<data>
* AT#XUDPRECVFROM=<url>,<port>,<length>,<timeout>

GPS AT Commands
***************

The following proprietary GPS AT commands are used in this sample:


* AT#XGPSRUN=<op>[,<mask>]
* AT#XGPSRUN?

Building and Running
********************

.. |sample path| replace:: :file:`samples/nrf9160/serial_lte_modem`

.. include:: /includes/build_and_run_nrf9160.txt

The following configuration files are located in the :file:`samples/nrf9160/serial_lte_modem` directory:

- :file:`prj.conf`
  This is the standard default configuration file.
- :file:`child_secure_partition_manager.conf`
  This is the project-specific Secure Partition Manager configuration file.

Testing
=======

To test the sample with a PC client, open a terminal window and start sending AT commands to the nRF9160 DK.
See `Terminal connection`_ section for the serial connection configuration details.

When testing the sample with an nRF52 client, the DKs go through the following start-up sequence:

    1. nRF91 starts up and enters sleep state.
    #. nRF52 starts up and starts a periodical timer to toggle the GPIO interface.
    #. nRF52 deasserts the GPIO interface.
    #. nRF91 is woken up and sends a ``Ready\r\n`` message to the nRF52.
    #. On receiving the message, nRF52 can proceed to issue AT commands.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * ``nrf/drivers/lte_link_control``
    * ``nrf/drivers/at_cmd``
    * ``nrf/lib/bsd_lib``
    * ``nrf/lib/at_cmd_parser``
    * nRF BSD Socket
    * Zephyr BSD socket

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`

References
**********

* nRF91 `AT Commands Reference Guide`_ in Nordic Infocenter
