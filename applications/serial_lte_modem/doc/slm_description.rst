.. _slm_description:

Application description
#######################

.. contents::
   :local:
   :depth: 3

The Serial LTE Modem (SLM) application demonstrates how to use the nRF9160 as a stand-alone LTE modem that can be controlled by AT commands.

The application accepts both the modem-specific AT commands documented in the `nRF91 AT Commands Reference Guide <AT Commands Reference Guide_>`_ and proprietary AT commands documented in :ref:`SLM_AT_intro`.

Overview
********

The nRF9160 SiP integrates both a full LTE modem and an application MCU, enabling you to run your LTE application directly on the nRF9160.

However, you might want to run your application on a different chip and use the nRF9160 only as a modem.
For this use case, the serial LTE modem application provides an interface for controlling the LTE modem through AT commands.

The proprietary AT commands specific to the serial LTE modem application are described in the :ref:`SLM_AT_intro` documentation.
In addition to these, the application also supports the nRF91 AT commands described in the `AT Commands Reference Guide`_.

Communicating with the modem
============================

The nRF9160 DK running the serial LTE modem application serves as the host.
As a client, you can use either a PC or an external MCU.

Connecting with a PC
--------------------

To connect to the nRF9160 DK with a PC, make sure that :ref:`CONFIG_SLM_CONNECT_UART_0 <CONFIG_SLM_CONNECT_UART_0>` is defined in the application.
It is defined in the default configuration.

Use LTE Link Monitor to connect to the nRF9160 DK.
See :ref:`lte_connect` for instructions.
You can then use this connection to send or receive AT commands over UART, and to see the log output of the nRF9160 DK.

Alternatively, you can use a terminal emulator like PuTTY to establish a terminal connection to the nRF9160 DK.
See :ref:`putty` for instructions.

.. note::
   The default AT command terminator is a carriage return followed by a line feed (``\r\n``).
   LTE Link Monitor supports this format.
   When connecting with another terminal emulator, make sure that the configured AT command terminator corresponds to the line terminator of your terminal.
   You can change the termination mode in the :ref:`application configuration <slm_config>`.

Connecting with an external MCU
-------------------------------

If you run your user application on an external MCU (for example, an nRF52 Series DK), you can control the modem on the nRF9160 directly from the application.
See the `nRF52 client for serial LTE modem application`_ repository for a sample implementation of such an application.

To connect with an external MCU, you must set the configuration option :kconfig:`CONFIG_UART_2_NRF_HW_ASYNC_TIMER` and :ref:`CONFIG_SLM_CONNECT_UART_2 <CONFIG_SLM_CONNECT_UART_2>` in the serial LTE modem application configuration.

The following table shows how to connect an nRF52 Series DK to the nRF9160 DK to be able to communicate through UART:

.. list-table::
   :align: center
   :header-rows: 1

   * - nRF52 Series DK
     - nRF9160 DK
   * - UART TX P0.6
     - UART RX P0.11
   * - UART RX P0.8
     - UART TX P0.10
   * - UART CTS P0.7
     - UART RTS P0.12
   * - UART RTS P0.5
     - UART CTS P0.13
   * - GPIO OUT P0.27
     - GPIO IN P0.31

UART instance in use:

* nRF52840 or nRF52832 (UART0)
* nRF9160 (UART2)

UART configuration:

* Hardware flow control: enabled
* Baud rate: 115200
* Parity bit: no
* Operation mode: IRQ

.. note::
   The GPIO output level on the nRF9160 side must be 3 V.
   You can set the VDD voltage with the **VDD IO** switch (**SW9**).
   See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information.

Requirements
************

The application supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

Configuration
*************

|config|

.. _slm_config:

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_SLM_CUSTOMIZED:

CONFIG_SLM_CUSTOMIZED - Flag for customized functionality
   This flag can be used to enable customized functionality.
   To add your own custom logic, enclose the code by ``#if defined(CONFIG_SLM_CUSTOMIZED)`` and enable this flag.

.. _CONFIG_SLM_NATIVE_TLS:

CONFIG_SLM_NATIVE_TLS - Use Zephyr mbedTLS
   This option enables using Zephyr's mbedTLS.
   It requires additional configuration.
   See :ref:`slm_native_tls` for more information.

.. _CONFIG_SLM_EXTERNAL_XTAL:

CONFIG_SLM_EXTERNAL_XTAL - Use external XTAL for UARTE
   This option configures the application to use an external XTAL for UARTE.
   See the `nRF9160 Product Specification`_ (section 6.19 UARTE) for more information.

.. _CONFIG_SLM_CONNECT_UART_0:

CONFIG_SLM_CONNECT_UART_0 - UART 0
   This option selects UART 0 for the UART connection.
   Select this option if you want to test the application with a PC.

.. _CONFIG_SLM_CONNECT_UART_2:

CONFIG_SLM_CONNECT_UART_2 - UART 2
   This option selects UART 2 for the UART connection.
   Select this option if you want to test the application with an external CPU.

.. _CONFIG_SLM_START_SLEEP:

CONFIG_SLM_START_SLEEP - Enter sleep on startup
   This option makes the nRF9160 enter deep sleep after startup.
   It is not selected by default.

.. _CONFIG_SLM_INTERFACE_PIN:

CONFIG_SLM_INTERFACE_PIN - Interface GPIO to wake up from sleep or exit idle
   This option specifies which interface GPIO to use for exiting sleep or idle mode.
   By default, **P0.6** (Button 1 on the nRF9160 DK) is used when :ref:`CONFIG_SLM_CONNECT_UART_0 <CONFIG_SLM_CONNECT_UART_0>` is selected, and **P0.31** is used when :ref:`CONFIG_SLM_CONNECT_UART_2 <CONFIG_SLM_CONNECT_UART_2>` is selected.

.. _CONFIG_SLM_SOCKET_RX_MAX:

CONFIG_SLM_SOCKET_RX_MAX - Maximum RX buffer size for receiving socket data
   This option specifies the maximum buffer size for receiving data through the socket interface.
   By default, this size is set to :c:enumerator:`NET_IPV4_MTU` (576), which is defined in Zephyr.
   The maximum value is 708, which is the maximum segment size (MSS) defined for the modem.

   This option impacts the total RAM usage.

.. _CONFIG_SLM_CR_TERMINATION:

CONFIG_SLM_CR_TERMINATION - CR termination
   This option configures the application to accept AT commands ending with a carriage return.

.. _CONFIG_SLM_LF_TERMINATION:

CONFIG_SLM_LF_TERMINATION - LF termination
   This option configures the application to accept AT commands ending with a line feed.

.. _CONFIG_SLM_CR_LF_TERMINATION:

CONFIG_SLM_CR_LF_TERMINATION - CR+LF termination
   This option configures the application to accept AT commands ending with a carriage return followed by a line feed.

.. _CONFIG_SLM_TCP_POLL_TIME:

CONFIG_SLM_TCP_POLL_TIME - Poll timeout in seconds for TCP connection
   This option specifies the poll timeout for the TCP connection, in seconds.

.. _CONFIG_SLM_SMS:

CONFIG_SLM_SMS - SMS support in SLM
   This option enables additional AT commands for using the SMS service.

.. _CONFIG_SLM_GNSS:

CONFIG_SLM_GNSS - GNSS support in SLM
   This option enables additional AT commands for using GNSS service.

.. _CONFIG_SLM_AGPS:

CONFIG_SLM_AGPS - nRF Cloud A-GPS support in SLM
   This option enables additional AT commands for using the nRF Cloud A-GPS service.
   It is not selected by default.

.. _CONFIG_SLM_PGPS:

CONFIG_SLM_PGPS - nRF Cloud P-GPS support in SLM
   This option enables additional AT commands for using the nRF Cloud P-GPS service.
   It is not selected by default.

.. _CONFIG_SLM_CELL_POS:

CONFIG_SLM_CELL_POS - nRF Cloud cellular positioning support in SLM
   This option enables additional AT commands for using the nRF Cloud Cellular Positioning service.
   It is not selected by default.

.. _CONFIG_SLM_FTPC:

CONFIG_SLM_FTPC - FTP client support in SLM
   This option enables additional AT commands for using the FTP client service.

.. _CONFIG_SLM_MQTTC:

CONFIG_SLM_MQTTC - MQTT client support in SLM
   This option enables additional AT commands for using the MQTT client service.

.. _CONFIG_SLM_HTTPC:

CONFIG_SLM_HTTPC - HTTP client support in SLM
   This option enables additional AT commands for using the HTTP client service.

.. _CONFIG_SLM_TWI:

CONFIG_SLM_TWI - TWI support in SLM
   This option enables additional AT commands for using the TWI service.

Additional configuration
========================

To save power, console and logging output over ``UART_0`` is disabled in this application.
This information is logged to RTT instead.
See :ref:`testing_rtt_connect` for instructions on how to view this information.

To switch to UART output, change the following options in the :file:`prj.conf` file::

   # Segger RTT
   CONFIG_USE_SEGGER_RTT=n
   CONFIG_RTT_CONSOLE=n
   CONFIG_UART_CONSOLE=y
   CONFIG_LOG_BACKEND_RTT=n
   CONFIG_LOG_BACKEND_UART=y


Configuration files
===================

The sample provides predefined configuration files for both the parent image and the child image.
You can find the configuration files in the :file:`applications/nrf9160/serial_lte_modem` directory.

The following files are provided:

* :file:`prj.conf` - This configuration file contains the standard configuration for the serial LTE modem application.

* :file:`overlay-native_tls.conf` - This configuration file contains additional configuration options that are required to use :ref:`slm_native_tls`.
  You can include it by adding ``-DOVERLAY_CONFIG=overlay-native_tls.conf`` to your build command.
  See :ref:`cmake_options`.

* :file:`child_secure_partition_manager.conf` - This configuration file contains the project-specific configuration for the :ref:`secure_partition_manager` child image.

.. _slm_native_tls:

Native TLS sockets
------------------

By default, the secure socket (TLS/DTLS) is offloaded onto the modem.
However, if you require customized TLS/DTLS features that are not supported by the modem firmware, you can use a native TLS socket instead.
The serial LTE modem application will then handle all secure sockets used in TCP/IP, TCP/IP proxy, and MQTT.

If native TLS is enabled, the `Credential storage management %CMNG`_ command is overridden to map the :ref:`security tag <nrfxlib:security_tags>` from the serial LTE modem application to the modem.
You must use the overridden AT%CMNG command to provision the credentials to the modem.
Note that the serial LTE modem application supports security tags in the range of 0 - 214748364.

The configuration options that are required to enable the native TLS socket are defined in the :file:`overlay-native_tls.conf` file.

.. note::

   The following limitations exist for native TLS sockets:

   * PSK, PSK identity, and Public Key are currently not supported.
   * DTLS server is currently not supported.
   * ``AT%CMNG=1`` is not supported.
   * FTP client and HTTP client do currently not support native TLS.

.. _slm_building:

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf9160/serial_lte_modem`

.. include:: /includes/build_and_run_nrf9160.txt

.. _slm_testing_section:

Testing
=======

The testing instructions focus on testing the application with a PC client.
If you have an nRF52 Series DK running a client application, you can also use this DK for testing the different scenarios.

|test_sample|

1. |connect_kit|
#. :ref:`Connect to the kit with LTE Link Monitor <lte_connect>`.
   If you want to use a different terminal emulator, see `Connecting with a PC`_.
#. Reset the kit.
#. Observe that the nRF9160 DK sends a ``Ready\r\n`` message on UART.
#. Enter ``AT+CFUN=1`` to turn on the modem and connect to the network.
#. Enter ``AT+CFUN?`` and observe that the connection indicators in the LTE Link Monitor side panel turn green.
   This indicates that the modem is connected to the network.
#. Send AT commands and observe the responses from the nRF9160 DK.
   See :ref:`slm_testing` for typical test cases.


Dependencies
************

This application uses the following |NCS| libraries:

* :ref:`at_cmd_parser_readme`
* :ref:`at_monitor_readme`
* :ref:`nrf_modem_lib_readme`
* :ref:`lib_modem_jwt`
* :ref:`lib_ftp_client`
* :ref:`sms_readme`
* :ref:`lib_fota_download`
* :ref:`lib_download_client`
* :ref:`cloud_api_readme`
* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_nrf_cloud_cell_pos`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following samples:

* :ref:`secure_partition_manager`
