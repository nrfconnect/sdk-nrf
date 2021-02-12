.. _SLM_AT_TCP_UDP:

TCP and UDP AT commands
***********************

.. contents::
   :local:
   :depth: 2

The following commands list contains TCP and UDP related AT commands.

For more information on the networking services, visit the `BSD Networking Services Spec Reference`_.

Socket #XSOCKET
===============

The ``#XSOCKET`` command allows you to open or close a socket, or to check the socket handle.

Set command
-----------

The set command allows you to open or close a socket.

Syntax
~~~~~~

::

   #XSOCKET=<op>[,<type>,<role>[,<sec_tag>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Close
  * ``1`` - Open

* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.
  It is associated with the certificate or PSK.
  Specifying the ``<sec_tag>`` is mandatory when opening a socket.
  When TLS/DTLS is expected, the credentials should be stored on the modem side by ``AT%XCMNG`` or by the Nordic nRF Connect/LTE Link Monitor tool.
  The modem needs to be in the offline state.
  The DTLS server is not supported.

* The ``<type>`` parameter value depends on the presence of the <sec_tag> parameter.
  When the ``<sec_tag>`` is not specified:

  * 1: SOCK_STREAM for TCP
  * 2: SOCK_DGRAM for UDP

  When the ``<sec_tag>`` is specified:

  * 1: SOCK_STREAM for TLS
  * 2: SOCK_DGRAM for DTLS

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client
  * ``1`` - Server

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKET: <handle>[,<type>,<protocol>]

* The ``<handle>`` value is an integer.
  It can be interpreted as follows:

  * Positive - The socket opened successfully.
  * Negative - The socket failed to open.
  * ``0`` - The socket closed successfully.

* The ``<type>`` parameter value depends on the presence of the <sec_tag> parameter.
  When the ``<sec_tag>`` is not specified:

  * 1: SOCK_STREAM for TCP
  * 2: SOCK_DGRAM for UDP

  When the ``<sec_tag>`` is specified:

  * 1: SOCK_STREAM for TLS
  * 2: SOCK_DGRAM for DTLS

* The ``<protocol>`` value is present only in the response to a request to open the socket.
  It can be one of the following:

  * ``6`` - IPPROTO_TCP
  * ``17`` - IPPROTO_UDP
  * ``258`` - IPPROTO_TLS_1_2
  * ``273`` - IPPROTO_DTLS_1_2

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XSOCKET: <error>,"closed"

The ``<error>`` value is a negative integer.
It represents the error value according to the standard POSIX *errorno*.

Examples
~~~~~~~~

::

   AT#XSOCKET=1,1,0
   #XSOCKET: 3,6,0
   OK
   AT#XSOCKET=1,2,0
   #XSOCKET: 3,17,0
   OK
   AT#XSOCKET=0
   #XSOCKET: 0,"closed"
   OK
   at#xsocket=1,1,0,16842753
   #XSOCKET: 2,1,0,258
   OK
   at#xsocket=1,2,0,16842753
   #XSOCKET: 2,2,0,273
   OK

Read command
------------

The read command allows you to check the socket handle.

Syntax
~~~~~~

::

   #XSOCKET?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKET: <handle>[,<protocol>,<role>]

* The ``<handle>`` value is an integer.
  It can be interpreted as follows:

  * Positive - The socket is valid.
  * ``0`` - The socket is closed.

* The ``<protocol>`` value is present only in the response to a request to open the socket.
  It can be one of the following:

  * ``6`` - IPPROTO_TCP
  * ``17`` - IPPROTO_UDP
  * ``258`` - IPPROTO_TLS_1_2
  * ``273`` - IPPROTO_DTLS_1_2

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client
  * ``1`` - Server

Examples
~~~~~~~~

::

   AT#XSOCKET?
   #XSOCKET: 3,6,0
   OK

::

   AT#XSOCKET?
   #XSOCKET: 3,17,0
   OK

::

   at#xsocket?
   #XSOCKET: 2,258,0
   OK

::

   at#xsocket?
   #XSOCKET: 2,273,0
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XSOCKET=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKET: <list of op value>,<list of type value>,<list of roles>,<sec-tag>


* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Open
  * ``1`` - Close

* The ``<type>`` parameter value depends on the presence of the <sec_tag> parameter.
  When the ``<sec_tag>`` is not specified:

  * 1: SOCK_STREAM for TCP
  * 2: SOCK_DGRAM for UDP

  When the ``<sec_tag>`` is specified:

  * 1: SOCK_STREAM for TLS
  * 2: SOCK_DGRAM for DTLS

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client
  * ``1`` - Server

* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.

Examples
~~~~~~~~

::

   at#xsocket=?
   #XSOCKET: (0,1),(1,2),<sec_tag>
   OK

Socket options #XSOCKETOPT
==========================

The ``#XSOCKETOPT`` command allows you to get and set socket options.

Set command
-----------

The set command allows you to get and set socket options.

Syntax
~~~~~~

::

   #XSOCKET=<op>,<name>[,<value>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Get
  * ``1`` - Set

For a complete list of the supported SET ``<name>`` accepted parameters, refer to the `SETSOCKETOPT Service Spec Reference`_.
``SO_RCVTIMEO(20)``, the ``<value>`` parameter is the *Receive Timeout* in seconds.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKETOPT: <value>

For a complete list of the supported GET ``<name>`` accepted parameters, refer to the `GETSOCKETOPT Service Spec Reference`_.
``SO_RCVTIMEO(20)``, the response ``<value>`` is the *Receive Timeout* in seconds.

Unsolicited Notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XSOCKET: <error>, "closed"

``SO_ERROR(4)``, the ``<error>`` response is the *Error Status*.

Examples
~~~~~~~~

::

   at#xsocketopt=1,20,30
   OK

::

   at#xsocketopt=0,20
   #XSOCKETOPT: 30
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XSOCKETOPT=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKETOPT: <list of op value>,<name><value>

Examples
~~~~~~~~

::

   at#xsocketopt=?
   #XSOCKETOPT: (0,1),<name>,<value>
   OK

Socket binding #XBIND
=====================

The ``#XBIND`` command allows you to bind a socket with a local port.

Set command
-----------

The set command allows you to bind a socket with a local port.

Syntax
~~~~~~

::

   #XBIND=<port>

* The ``<port>`` parameter is an integer.
  It represents the specific port to use to bind the socket with.

Examples
~~~~~~~~

::

   AT#XBIND=1234
   OK

Read command
------------

The read command is not supported.


Test command
------------

The test command is not supported.

TCP connection #XCONNECT
========================

The ``#XCONNECT`` command allows you to connect to a TCP server and to check the connection status.

Set command
-----------

The set command allows you to connect to a TCP server.

Syntax
~~~~~~

::

   #XCONNECT=<url>,<port>

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to.
  Its maximum size can be 128 bytes.
  When the parameter is an IP address, it supports IPv4 only, not IPv6.

* The ``<port>`` parameter is an integer.
  It represents the port of the TCP service.

Response syntax
~~~~~~~~~~~~~~~

::

   #XCONNECT: <status>

* The ``<status>`` value is an integer.
  It can assume one of the following values:

* ``1`` - Connected
* ``0`` - Disconnected

Examples
~~~~~~~~

::

   AT#XCONNECT="test.server.com",1234
   #XCONNECT: 1
   OK

::

   AT#XCONNECT="192.168.0.1",1234
   #XCONNECT: 1
   OK

Read command
------------

The read command allows you to check the connection status.

Syntax
~~~~~~

::

   #XCONNECT?

Response syntax
~~~~~~~~~~~~~~~

::

   #XCONNECT: <status>

The ``<status>`` value is an integer.
It can assume one of the following values:

* ``1`` - Connected
* ``0`` - Disconnected

Examples
~~~~~~~~

::

   AT#XCONNECT?
   #XCONNECT: 1
   OK


Test command
------------

The test command is not supported.

TCP set listen mode #XLISTEN
============================

The ``#XLISTEN`` command allows you to put the TCP socket in listening mode for incoming connections.

Set command
-----------

The set command allows you to put the TCP socket in listening mode for incoming connections.

Syntax
~~~~~~

::

   #XLISTEN

Response syntax
~~~~~~~~~~~~~~~

There is no response.

Examples
~~~~~~~~

::

   AT#XLISTEN
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

TCP accept incoming connection #XACCEPT
=======================================

The ``#XACCEPT`` command allows you to wait for the TCP client to connect and to check the IP address of the accepted connection.

Set command
-----------

The set command allows you to wait for the TCP client to connect.

Syntax
~~~~~~

::

   #XACCEPT

Response syntax
~~~~~~~~~~~~~~~

::

   #TCPACCEPT: <ip_addr>

The ``<ip_addr>`` value indicates the IPv4 address of the peer host.

Examples
~~~~~~~~

::

   AT#XACCEPT
   #XACCEPT: 192.168.0.2
   OK

Read command
------------

The read command allows you to check the IP address of the accepted connection.

Syntax
~~~~~~

::

   #XACCEPT?

Response syntax
~~~~~~~~~~~~~~~

::

   #TCPACCEPT: <ip_addr>

The ``<ip_addr>`` value indicates the IPv4 address of the peer host.
It is ``0.0.0.0`` if there is no accepted connection yet.

Examples
~~~~~~~~

::

   AT#XACCEPT?
   #XACCEPT: 192.168.0.2
   OK

Test command
------------

The test command is not supported.

Send data #XSEND
================

The ``#XSEND`` command allows you to send data over the connection.

Set command
-----------

The set command allows you to send data over the connection.

Syntax
~~~~~~

::

   #XSEND=<datatype>,<data>

* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<data>`` parameter is a string.
  It contains the data being sent.
  The maximum size for ``NET_IPV4_MTU`` is 576 bytes.
  It should have no ``NULL`` character in the middle.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSEND: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of bytes sent.

Examples
~~~~~~~~

::

   AT#XSEND="Test TCP"
   #XSEND: 8
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Receive data #XRECV
===================

The ``#XRECV`` command allows you to receive data over the connection.

Set command
-----------

The set command allows you to receive data over the connection.

Syntax
~~~~~~

::

   #XRECV[=<size>]

* The ``<size>`` value is an integer.
  It represents the actual number of requested bytes.
  It is set to the value of ``NET_IPV4_MTU`` when not specified.

Response syntax
~~~~~~~~~~~~~~~

::

   <data>
   #XRECV: <datatype>, <size>

* The ``<data>`` value is a string.
  It contains the data being received.
* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<size>`` value is an integer.
  It represents the actual number of bytes received.
  The maximum size for ``NET_IPV4_MTU`` is 576 bytes.
  It must not have any ``NULL`` character in the middle.

Examples
~~~~~~~~

::

   AT#XRECV
   Test OK
   #XRECV: 1,7
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

UDP Send data #XSENDTO
======================

The ``#XSENDTO`` command allows you to send data over the UDP channel.

Set command
-----------

The set command allows you to send data over the UDP channel.

Syntax
~~~~~~

::

   #XSENDTO=<url>,<port>,<datatype>,<data>

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to.
  Its maximum size can be 128 bytes.
  When the parameter is an IP address, it supports IPv4 only, not IPv6.
* The ``<port>`` parameter is an integer.
  It represents the port of the TCP service.
* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<data>`` parameter is a string.
  It contains the data being sent.
  The maximum size for ``NET_IPV4_MTU`` is 576 bytes.
  It must not have any``NULL`` character in the middle.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSENDTO: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of bytes sent.

Examples
~~~~~~~~

::

   AT#XSENDTO="test.server.com",1234,"Test UDP"
   #XSENDTO: 8
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

UDP receive data #XRECVFROM
===========================

The ``#XRECVFROM`` command allows you to receive data through the UDP channel.

Set command
-----------

The set command allows you to receive data through the UDP channel.

Syntax
~~~~~~

::

   #XRECVFROM[=<size>]

The ``<size>`` value is an integer.
It represents the actual number of bytes requested.
It is set to match the ``NET_IPV4_MTU`` when not specified.

Response syntax
~~~~~~~~~~~~~~~

::

   <data>
   #XRECVFROM: <datatype>, <size>


* The ``<data>`` value is a string.
  It contains the data being received.
* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<size>`` value is an integer.
  It represents the actual number of bytes received.

Examples
~~~~~~~~

::

   AT#XRECVFROM
   Test OK
   #XRECVFROM: 1,7
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Resolve hostname #XGETADDRINFO
==============================

The ``#XGETADDRINFO`` command allows you to resolve hostnames to IPv4 addresses.

Set command
-----------

The set command allows you to resolve hostnames to IPv4 addresses.

Syntax
~~~~~~

::

   #XGETADDRINFO=<hostname>

The ``<hostname>`` parameter is a string.
It cannot be an IPv4 address string.

Response syntax
~~~~~~~~~~~~~~~

::

   #XGETADDRINFO: "<ip_addr>"

* The ``<ip_addr>`` value is a string.
  It indicates the IPv4 address of the resolved hostname.

Examples
~~~~~~~~

::

   at#xgetaddrinfo="www.google.com"
   #XGETADDRINFO: "172.217.174.100"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

TCP filtering #XTCPFILTER
=========================

The ``#XTCPFILTER`` command allows you to set or clear an allowlist for the TCP server.
If the allowlist is set, only IPv4 addresses in the list are allowed for connection.

Set command
-----------

The set command allows you to set or clear an allowlist for the TCP server.

Syntax
~~~~~~

::

   #XTCPFILTER=<op>[,<ip_addr1>[,<ip_addr2>[,...]]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - clear list and turn filtering mode off
  * ``1`` - set list and turn filtering mode on

* The ``<ip_addr#>`` value is a string.
  It indicates the IPv4 address of an allowed TCP/TLS client.
  The maximum number of IPv4 addresses that can be specified in the list is six.

Examples
~~~~~~~~

::

   AT#XTCPFILTER=1,"192.168.1.1"
   OK

::

   AT#XTCPFILTER=1,"192.168.1.1","192.168.1.2","192.168.1.3","192.168.1.4","192.168.1.5","192.168.1.6"
   OK

::

   AT#XTCPFILTER=0
   OK

::

   AT#XTCPFILTER=1
   OK

Read command
------------

The read command allows you to check TCP filtering settings.

Syntax
~~~~~~

::

   #XTCPFILTER?

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPFILTER: <filter_mode>[,<ip_addr1>[,<ip_addr2>[,...]]]

* The ``<filter_mode>`` value can assume one of the following values:

  * ``0`` - Disabled
  * ``1`` - Enabled

Examples
~~~~~~~~

::

   AT#XTCPFILTER?
   #XTCPFILTER: 1,"192.168.1.1"
   OK

::

   AT#XTCPFILTER?
   #XTCPFILTER: 1,"192.168.1.1","192.168.1.2","192.168.1.3","192.168.1.4","192.168.1.5","192.168.1.6"
   OK

::

   AT#XTCPFILTER?
   #XTCPFILTER: 0
   OK

::

   AT#XTCPFILTER?
   #XTCPFILTER: 1
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XTCPFILTER=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPFILTER: (list of op value),",<IP_ADDR#1>[,<IP_ADDR#2>[,...]]

Examples
~~~~~~~~

::

   at#XTCPFILTER=?
   #XTCPFILTER: (0,1),<IP_ADDR#1>[,<IP_ADDR#2>[,...]]
   OK

TCP server #XTCPSVR
===================

The ``#XTCPSVR`` command allows you to start and stop the TCP server.

Set command
-----------

The set command allows you to start and stop the TCP server.

Syntax
~~~~~~

::

   #XTCPSVR=<op>[<port>[,<sec_tag>]]


* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Stop the server
  * ``1`` - Start the server
  * ``2`` - Start the server with data mode support

* The ``<port>`` parameter is an integer.
  It represents the TCP service port.
  It is mandatory to set it when starting the server.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSVR: <handle>,"started"

The ``<handle>`` value is an integer.
When positive, it indicates that it opened successfully.
When negative, it indicates that it failed to open.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XTCPSVR: <error>,"stopped"

The ``<error>`` value is a negative integer.
It represents the error value according to the standard POSIX *errorno*.

::

   #XTCPDATA: <datatype>,<size>

* The ``<datatype>`` value can assume one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<size>`` value is the length of RX data received by the SLM waiting to be fetched by the MCU.

Examples
~~~~~~~~

::

   at#xtcpsvr=1,3442,600
   #XTCPSVR: 2,"started"
   OK
   #XTCPSVR: "5.123.123.99","connected"
   #XTCPRECV: 1,13
   Hello, TCP#1!
   #XTCPRECV: 1,13
   Hello, TCP#2!

Read command
------------

The read command allows you to check the TCP server settings.

Syntax
~~~~~~

::

   #XTCPSVR?

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSVR: <listen_socket_handle>,<income_socket_handle>,<data_mode>

The ``<handle>`` value is an integer.
When positive, it indicates that it opened successfully.
When negative, it indicates that it failed to open or that there is no incoming connection.

* The ``<data_mode>`` value can assume one of the following values:

  * ``0`` - Disabled
  * ``1`` - Enabled

Examples
~~~~~~~~

::

   at#xtcpsvr?
   #XTCPSVR: 1,2,0
   OK
   #XTCPSVR: -110,"disconnected"
   at#xtcpsvr?
   #XTCPSVR: 1,-1
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XTCPSVR=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSVR: (list of op value),<port>,<sec_tag>

Examples
~~~~~~~~

::

   at#xtcpsvr=?
   #XTCPSVR: (0,1,2),<port>,<sec_tag>
   OK

TCP/TLS client #XTCPCLI
=======================

The ``#XTCPCLI`` command allows you to create a TCP/TLS client and to connect to a server.

Set command
-----------

The set command allows you to create a TCP/TLS client and to connect to a server.

Syntax
~~~~~~

::

   #XTCPCLI=<op>[,<url>,<port>[,[sec_tag]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Disconnect
  * ``1`` - Connect to the server
  * ``2`` - Connect to the server with data mode support

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to.
  Its maximum size is 128 bytes.
  When the parameter is an IP address, it supports IPv4 only, not IPv6.
* The ``<port>`` parameter is an integer.
  It represents the TCP/TLS service port.
  It is mandatory for starting the server.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPCLI: <handle>, "connected"

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XTCPCLI: <error>, "disconnected"

The ``<error>`` value is a negative integer.
It represents the error value according to the standard POSIX *errorno*.

When TLS/DTLS is expected, the credentials should be stored on the modem side by ``AT%XCMNG`` or by the Nordic nRF Connect/LTE Link Monitor tool.
The modem needs to be in the offline state.

::

   #XTCPDATA: <datatype>,<size>

* The ``<datatype>`` value can assume one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<size>`` value is the length of RX data received by the SLM waiting to be fetched by the MCU.

Examples
~~~~~~~~

::

   at#xtcpcli=1,"remote.ip",1234
   #XTCPCLI: 2,"connected"
   OK
   #XTCPRECV: 1,31
   PONG: b'Test TCP by IP address'

   at#xtcpcli=0
   OK

Read command
------------

The read command allows you to verify the status of the connection.

Syntax
~~~~~~

::

   #XTCPCLI?

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPCLI: <handle>,<data_mode>

The ``<handle>`` value is an integer.
When positive, it indicates that it opened successfully.
When negative, it indicates that it failed to open.

* The ``<data_mode>`` value can assume one of the following values:

  * ``0`` - Disabled
  * ``1`` - Enabled

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XTCPCLI: (op list),<url>,<port>,<sec_tag>

Examples
~~~~~~~~

::

   at#xtcpcli=?
   #XTCPCLI: (0,1,2),<url>,<port>,<sec_tag>
   OK

TCP send data #XTCPSEND
=======================

The ``#XTCPSEND`` command allows you to send the data over the connection.

Set command
-----------

The set command allows you to send the data over the connection.
When used from a TCP/TLS client, it sends the data to the remote TCP server
When used from a TCP server, it sends data to the remote TCP client

Syntax
~~~~~~

::

   #XTCPSEND=<datatype>,<data>

* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<data>`` parameter is a string.
  It contains the data being sent.
  The maximum size for ``NET_IPV4_MTU`` is 576 bytes.
  It should have no ``NULL`` character in the middle.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSEND: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of the bytes sent.

Examples
~~~~~~~~

::

   at#xtcpsend=1,"Test TLS client"
   #XTCPSEND: 15
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

TCP receive data #XTCPRECV
==========================

The ``#XTCPRECV`` command allows you to receive data over the connection.

Set command
-----------

The set command allows you to receive data over the connection.
It receives data buffered in the Serial LTE Modem.

Syntax
~~~~~~

::

   #XTCPRECV[=<size>]

* The ``<size>`` value is an integer.
  It represents the requested number of bytes.

Response syntax
~~~~~~~~~~~~~~~

::

   <data>
   #XTCPRECV: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of the bytes received in the response.

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

UDP server #XUDPSVR
===================

The ``#XUDPSVR`` command allows you to start and stop the UDP server.

Set command
-----------

The set command allows you to start and stop the UDP server.

Syntax
~~~~~~

::

   #XUDPSVR=<op>[,<port>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Stop the server
  * ``1`` - Start the server
  * ``2`` - Start the server with data mode support

* The ``<port>`` parameter is an integer.
  It represents the UDP service port.
  It is mandatory for starting the server.
  The data mode is enabled when the TCP/TLS server is started.

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPSVR: <handle>,"started"

The ``<handle>`` value is an integer.
When positive, it indicates that it opened successfully.
When negative, it indicates that it failed to open.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XUDPSVR: <error>,"stopped"

The ``<error>`` value is a negative integer.
It represents the error value according to the standard POSIX *errorno*.

The reception of data is automatic.
It is reported to the client as follows:

::

   #XUDPRECV: <datatype>,<size>
   <data>

* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV


Examples
~~~~~~~~

::

   at#xudpsvr=1,3442
   #XUDPSVR: 2,"started"
   OK
   #XUDPRECV: 1,13
   Hello, UDP#1!
   #XUDPRECV: 1,13
   Hello, UDP#2!

Read command
------------

The read command allows you to check the current value of the subparameters.

Syntax
~~~~~~

::

   #XUDPSVR?

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPSVR: <handle>,<data_mode>

The ``<handle>`` value is an integer.
When positive, it indicates that it opened successfully.
When negative, it indicates that it failed to open.

* The ``<data_mode>`` value can assume one of the following values:

  * ``0`` - Disabled
  * ``1`` - Enabled

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XUDPSVR=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPSVR: (list of op value),<port>,<sec_tag>

Examples
~~~~~~~~

::

   at#xudpsvr=?
   #XUDPSVR: (0,1,2),<port>,<sec_tag>
   OK

UDP/DTLS client #XUDPCLI
========================

The ``#XUDPCLI`` command allows you to create a UDP/DTLS client and to connect to a server.

Set command
-----------

The set command allows you to create a UDP/DTLS client and connect to a server.

Syntax
~~~~~~

::

   #XUDPCLI=<op>[,<url>,<port>[,<sec_tag>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Disconnect
  * ``1`` - Connect to the server
  * ``2`` - Connect to the server with data mode support

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to.
  Its maximum size can be 128 bytes.
  When the parameter is an IP address, it supports IPv4 only, not IPv6.
* The ``<port>`` parameter is an integer.
  It represents the UDP/DTLS service port.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPCLI: <handle>,"connected"

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XUDPCLI: <error>,"disconnected"

The ``<error>`` value is a negative integer.
It represents the error value according to the standard POSIX *errorno*.

The reception of data is automatic.
It is reported to the client as follows:

::

   #XTCPCLI: <datatype>,<size>
   <data>

* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

Examples
~~~~~~~~

::

   at#xudpcli=1,"remote.host",2442
   #XUDPCLI: 2,"connected"
   OK
   at#xudpsend=1,"Test UDP by hostname"
   #XUDPSEND: 20
   OK
   #XUDPRECV: 1,26
   PONG: Test UDP by hostname
   at#xudpcli=0
   OK

Read command
------------

The read command allows you to check the current value of the subparameters.

Syntax
~~~~~~

::

   #XUDPCLI?

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPCLI: <handle>,<data_mode>

The ``<handle>`` value is an integer.
When positive, it indicates that it opened successfully.
When negative, it indicates that it failed to open.

* The ``<data_mode>`` value can assume one of the following values:

  * ``0`` - Disabled
  * ``1`` - Enabled

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XUDPCLI: (op list),<url>,<port>,<sec_tag>

Examples
~~~~~~~~

::

   at#xudpcli=?
   #XUDPCLI: (0,1,2),<url>,<port>,<sec_tag>
   OK

UDP send data #XUDPSEND
=======================

The ``#XUDPSEND`` command allows you to send data over the connection.

Set command
-----------

The set command allows you to send data over the connection.

Syntax
~~~~~~

::

   #XUDPSEND=<datatype>,<data>

* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)
  * ``2`` - JSON
  * ``3`` - HTML
  * ``4`` - OMA TLV

* The ``<data>`` parameter is a string type.
  It contains arbitrary data.


Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPSEND: <size>

* The ``<size>`` value is an integer.
  It indicates the actual number of bytes sent.

Examples
~~~~~~~~

::

   at#xudpsend=1,"Test UDP by hostname"
   #XUDPSEND: 20
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
