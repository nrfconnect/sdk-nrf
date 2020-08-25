.. _serial_lte_modem:

nRF9160: Serial LTE Modem
#########################

The Serial LTE Modem (SLM) application demostrates sending AT commands between a host and a client device.
An nRF9160 DK is used as the host, while the client can be simulated using either a PC or an nRF52 DK.

This application is an enhancement to the :ref:`at_client_sample` sample.
It provides the following features:

* Support for generic proprietary AT commands
* Support for BSD Socket proprietary AT commands
* Support for TCP/UDP proxy proprietary AT commands (optional)
* Support for ICMP proprietary AT commands
* Support for GPS proprietary AT commands
* Support for MQTT client proprietary AT commands
* Support for FTP client proprietary AT commands
* Support for HTTP client proprietary AT commands
* Support for communication to external MCU over UART

All nRF91 modem AT commands are also supported.

Requirements
************

The application supports the following development kit:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set5_start
   :end-before: set5_end

If the client is a PC, the application requires any terminal software, such as TeraTerm.

If the client is an nRF52 device, the application supports the following development kits:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set2_start
   :end-before: set2_end

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

To work with the external MCU (nRF52), you must set the configuration options ``CONFIG_SLM_GPIO_WAKEUP`` and ``CONFIG_SLM_CONNECT_UART_2``.

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
     - UART CTS P0.13
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

Generic commands
****************

The following proprietary generic AT commands are used in this application:

* AT#XSLMVER
* AT#XSLMUART=<baudrate>
* AT#XSLEEP[=<shutdown_mode>]
* AT#XCLAC

If the client sets new UART baudrate by AT#XSLMUART, the client should wait at least 100ms to send command in new baudrate.

BSD Socket AT commands
**********************

The following proprietary BSD socket AT commands are used in this application:

* AT#XSOCKET=<op>[,<type>,<role>[,<sec_tag>]]
* AT#XSOCKETOPT=<op>,<name>[,<value>]
* AT#XBIND=<port>
* AT#XLISTEN
* AT#XACCEPT
* AT#XCONNECT=<url>,<port>
* AT#XSEND=<datatype>,<data>
* AT#XRECV[=<length>]
* AT#XSENDTO=<url>,<port>,<datatype>,<data>
* AT#XRECVFROM[=<length>]
* AT#XGETADDRINFO=<url>

If the configuration option ``CONFIG_SLM_TCP_PROXY`` is defined, the following AT commands are available to use the TCP proxy service:

* AT#XTCPSVR=<op>[,<port>[,<sec_tag>]]
* AT#XTCPCLI=<op>[,<url>,<port>[,<sec_tag>]
* AT#XTCPSEND=<datatype>,<data>
* AT#XTCPRECV[=<length>]

If the configuration option ``CONFIG_SLM_UDP_PROXY`` is defined, the following AT commands are available to use the UDP proxy service:

* AT#XUDPSVR=<op>[,<port>]
* AT#XUDPCLI=<op>[,<url>,<port>[,<sec_tag>]
* AT#XUDPSEND=<datatype>,<data>

ICMP AT commands
****************

The following proprietary ICMP AT commands are used in this application:

* AT#XPING=<addr>,<length>,<timeout>[,<count>[,<interval>]]

GPS AT Commands
***************

The following proprietary GPS AT commands are used in this application:

* AT#XGPS=<op>[,<mask>]

If the configuration option ``CONFIG_SUPL_CLIENT_LIB`` is defined, SUPL A-GPS is enabled.
Default SUPL server is as below:
CONFIG_SLM_SUPL_SERVER="supl.google.com"
CONFIG_SLM_SUPL_PORT=7276

MQTT AT Commands
****************

The following proprietary MQTT AT commands are used in this application:

* AT#XMQTTCON=<op>[,<cid>,<username>,<password>,<url>,<port>[,<sec_tag>]]
* AT#XMQTTPUB=<topic>,<datatype>,<msg>,<qos>,<retain>
* AT#XMQTTSUB=<topic>,<qos>
* AT#XMQTTUNSUB=<topic>

The following unsolicited notification indicates that an MQTT event occurred:

* #XMQTTEVT=<type>,<result>

The following unsolicited notification indicates that a publish message was
received and reports the topic and the message:

* #XMQTTMSG=<datatype>,<topic_length>,<message_length><CR><LF><topic><CR><LF><message>

FTP AT Commands
***************

The following proprietary FTP AT commands are used in this application:

* AT#XFTP=<cmd>[,<param1>[<param2]..]]

The different command options for <cmd> are listed below:

* AT#XFTP="open",<username>,<password>,<hostname>[,<port>[,<sec_tag>]]
* AT#XFTP="status"
* AT#XFTP="ascii"
* AT#XFTP="binary"
* AT#XFTP="close"
* AT#XFTP="pwd"
* AT#XFTP="cd",<folder>
* AT#XFTP="ls"[,<options>[,<folder or file>]]
* AT#XFTP="mkdir",<folder>
* AT#XFTP="rmdir",<folder>
* AT#XFTP="rename",<filename_old>,<filename_new>
* AT#XFTP="delete",<file>
* AT#XFTP="get",<file>
* AT#XFTP="put",<file>[<datatype>,<data>]

HTTP Client AT Commands
***********************

The following proprietary HTTP Client AT commands and unsolicited notifications are used in this application:

.. list-table::
   :align: left
   :header-rows: 1

   * - HTTP Client AT Commands
     - Description
   * - AT#XHTTPCCON=<op>[,<host>,<port>[,<sec_tag>]]
     - Connect to/disconnect from an HTTP server.
   * - AT#XHTTPCREQ=<method>,<resource>,<header>[,<payload_length>]
     - Send an HTTP request to the HTTP server.
   * - #XHTTPCRSP=<byte_received>,<state><CR><LF><response>
     - Indicate that a part of HTTP response was received.

AT#XHTTPCCON
============

The AT#XHTTPCCON command connects to/disconnects from an HTTP server.

.. code-block:: none

   AT#XHTTPCCON=<op>[,<host>,<port>[,<sec_tag>]]

.. csv-table::
    :header: Command Parameters, Data Type, Description

    op, integer, "| 1: Connect to HTTP host
    | 0: Disconnect from HTTP host"
    host, string, Domain name of HTTP server.
    port, integer, TCP port number on which HTTP server is listening.
    sec_tag, integer, Security tag to be used for secure session.

The #XHTTPCCON unsolicited notification indicates the state of connection.

.. code-block:: none

   #XHTTPCCON=<state>

.. csv-table::
    :header: Notification Parameters, Data Type, Description

    state, integer, "| 1: Connected to HTTP host
    | 0: Disconnected from HTTP host"

The example below connects to an HTTP server:

.. code-block:: none

   AT#XHTTPCCON=1,"postman-echo.com",80
   #XHTTPCCON:1
   OK

AT#XHTTPCREQ
============

The AT#XHTTPCREQ sends an HTTP request to the HTTP server.

.. code-block:: none

   AT#XHTTPCREQ=<method>,<resource>,<header>[,<payload_length>]

.. csv-table::
    :header: Command Parameters, Data Type, Description

    method, string, "Request method string."
    resource, string, "Target resource to handle the request."
    header, string, "Header field of request. Each header field should be terminated with <CR><LF>."
    payload_length, integer, "| Length of payload. If payload_length is greater than 0, the SLM will enter pass-through mode
    | and expect the upcoming UART input data as payload. SLM will then send the payload to the HTTP server
    | until *payload_length* bytes are sent.
    |
    | To abort sending payload, use *AT#XHTTPCCON=0* to disconnect from server."

The #XHTTPCREQ unsolicited notification indicates the state of request.

.. code-block:: none

   #XHTTPCREQ:<state>

.. csv-table::
    :header: Notification Parameters, Data Type, Description

    state, integer, "| 0: Request is sent successfully.
    | 1: Wait for payload data.
    | Negative integer: Error code"

#XHTTPCRSP
==========

The #XHTTPCRSP unsolicited notification indicates that a part of HTTP response was
received.

.. code-block:: none

   #XHTTPCRSP=<byte_received>,<state><CR><LF><response>

.. csv-table::
    :header: Notification Parameters, Data Type, Description

    byte_received, integer, Length of partially received HTTP response.
    state, integer, "| 0. The whole HTTP response is received.
    | 1: There is more HTTP response data to come.
    | Negative integer: Error code"
    response, raw data, The raw data of the HTTP response including headers and body.

The example below sends a GET request to retrieve data from the server without an optional header:

.. code-block:: none

   AT#XHTTPCREQ="GET","/get?foo1=bar1&foo2=bar2",""
   OK
   #XHTTPCREQ:0
   #XHTTPCRSP:576,0
   HTTP/1.1 200 OK
   Date: Wed, 09 Sep 2020 08:08:45 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 244
   Connection: keep-alive
   ETag: W/"f4-8qqGYUH6MF4k5ssZjXy/pQ2Wv2M"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3Awm7Yy6ZHF1L9bhf5GQFyOfskldPnP1AU.3tM0APxqLZLEaHtZMlUi9OJH8AR7OI%2F9qNV8h1NQOj8; Path=/; HttpOnly

The example below sends a POST request to send data to the server with an optional header:

.. code-block:: none

   AT#XHTTPCREQ="POST","/post","User-Agent: SLM/1.2.0
   Accept: */*
   Content-Type: application/x-www-form-urlencoded
   Content-Length: 20
   ",20
   OK
   #XHTTPCREQ:1
   12345678901234567890
   OK
   #XHTTPCREQ:0
   #XHTTPCRSP:576,1
   HTTP/1.1 200 OK
   Date: Wed, 09 Sep 2020 08:21:03 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 405
   Connection: keep-alive
   ETag: W/"195-JTHehAiV7LQRCKihfzcZBX1rgGM"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3AtApCs6p2Ja2on5dYO8QvhQSEEfnvkjOX.31HKOpZcip6MzzUoqPw2WZib0rPpimc5y10Mjczukoc; Path=/; HttpOnly
   {"args":{},"data":"","files":{},"form":{"12345678901234567890":""},"headers":{"x-forwarded-proto":"https","x-forwarded-port":"443","host":"postman-echo.com","x-amzn-trace-id":"Root=1-5f589067-d61d0850c65f3568f9c9e050","content-length":"20",#XHTTPCRSP:165,0
   "user-agent":"SLM/1.2.0","accept":"*/*","content-type":"application/x-www-form-urlencoded"},"json":{"12345678901234567890":""},"url":"https://postman-echo.com/post"}

Building and Running
********************

.. |sample path| replace:: :file:`applications/nrf9160/serial_lte_modem`

.. include:: /includes/build_and_run_nrf9160.txt

The following configuration files are located in the :file:`applications/nrf9160/serial_lte_modem` directory:

- :file:`prj.conf`
  This is the standard default configuration file.
- :file:`child_secure_partition_manager.conf`
  This is the project-specific Secure Partition Manager configuration file.

Testing
=======

To test the application with a PC client, open a terminal window and start sending AT commands to the nRF9160 DK.
See `Terminal connection`_ section for the serial connection configuration details.

When testing the application with an nRF52 client, the DKs go through the following start-up sequence:

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
* ``nrf/lib/at_notif``
* ``nrf/lib/modem_info``
* ``nrf/subsys/net/lib/ftp_client``

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`

References
**********

* nRF91 `AT Commands Reference Guide`_ in Nordic Infocenter
