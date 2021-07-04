.. _SLM_AT_SOCKET:

Socket AT commands
******************

.. contents::
   :local:
   :depth: 2

The following commands list contains socket related AT commands.

For more information on the networking services, visit the `BSD Networking Services Spec Reference`_.

Socket #XSOCKET
===============

The ``#XSOCKET`` command allows to open or close a socket, and to check the socket handle.

Set command
-----------

The set command allows to open or close a socket.

Syntax
~~~~~~

::

   #XSOCKET=<op>[,<type>,<role>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Close a socket
  * ``1`` - Open a socket for IP protocol family version 4
  * ``2`` - Open a socket for IP protocol family version 6

* The ``<type>`` parameter can accept one of the following values:

  * 1: SOCK_STREAM for TCP
  * 2: SOCK_DGRAM for UDP

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client
  * ``1`` - Server

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKET: <handle>[,<type>,<protocol>]

* The ``<handle>`` value is an integer, which can be interpreted as follows:

  * Positive - The socket opened successfully.
  * Negative - The socket failed to open.

* The ``<type>`` parameter value is an integer, which can be interpreted as follows:

  * 1: SOCK_STREAM for TCP
  * 2: SOCK_DGRAM for UDP

* The ``<protocol>`` value is value is an integer, which can be interpreted as follows:

  * ``6`` - IPPROTO_TCP
  * ``17`` - IPPROTO_UDP

Examples
~~~~~~~~

::

   AT#XSOCKET=1,1,0
   #XSOCKET: 3,1,6
   OK
   AT#XSOCKET=1,2,0
   #XSOCKET: 3,2,17
   OK
   AT#XSOCKET=2,1,0
   #XSOCKET: 1,1,6
   OK
   AT#XSOCKET=0
   #XSOCKET: 0,"closed"
   OK

Read command
------------

The read command allows to check the socket handle.

Syntax
~~~~~~

::

   #XSOCKET?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKET: <handle>[,<family>,<role>]

* The ``<handle>`` value is an integer, which can be interpreted as follows:

  * Positive - The socket is valid.
  * ``0`` - The socket is closed.

* The ``<family>`` value is present only in the response to a request to open the socket.
  It can be one of the following:

  * ``1`` - IP protocol family version 4
  * ``2`` - IP protocol family version 6

* The ``<role>`` parameter can be one of the following values:

  * ``0`` - Client
  * ``1`` - Server

Examples
~~~~~~~~

::

   AT#XSOCKET?
   #XSOCKET: 3,1,0
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

   #XSOCKET: <list of op>,<list of types>,<list of roles>


* The ``<list of op>`` parameter has following values:

  * ``0`` - Close a socket
  * ``1`` - Open a socket for IP protocol family version 4
  * ``2`` - Open a socket for IP protocol family version 6

* The ``<list of types>`` parameter has following values:

  * 1: SOCK_STREAM for TCP
  * 2: SOCK_DGRAM for UDP

* The ``<list of roles>`` parameter has following values:

  * ``0`` - Client
  * ``1`` - Server


Examples
~~~~~~~~

::

   AT#XSOCKET=?
   #XSOCKET: (0,1,2),(1,2),(0,1)
   OK

Secure Socket #XSSOCKET
=======================

The ``#XSSOCKET`` command allows to open or close a secure socket, and to check the socket handle.

NOTE TLS and DTLS server role is not supported as of now.

Set command
-----------

The set command allows to open or close a secure socket.

Syntax
~~~~~~

::

   #XSSOCKET=<op>[,<type>,<role>,<sec_tag>[,<peer_verify>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Close a socket
  * ``1`` - Open a socket for IP protocol family version 4
  * ``2`` - Open a socket for IP protocol family version 6

* The ``<type>`` parameter can accept one of the following values:

  * 1: SOCK_STREAM for TLS
  * 2: SOCK_DGRAM for DTLS

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client
  * ``1`` - Server

* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag to be used for establishing a secure connection.
  It is associated with a credential, i.e. certificate or PSK. The credential should be stored on the modem side beforehand.

* The ``<peer_verify>`` parameter can accept one of the following values:

  * ``0`` - None (default for server role)
  * ``1`` - Optional
  * ``2`` - Required (default for client role)

Response syntax
~~~~~~~~~~~~~~~

::

   #XSSOCKET: <handle>[,<type>,<protocol>]

* The ``<handle>`` value is an integer, which can be interpreted as follows:

  * Positive - The socket opened successfully.
  * Negative - The socket failed to open.

* The ``<type>`` parameter is an integer, which can be interpreted as follows:

  * 1: SOCK_STREAM for TLS
  * 2: SOCK_DGRAM for DTLS

* The ``<protocol>`` value is value is an integer, which can be interpreted as follows:

  * ``258`` - IPPROTO_TLS_1_2
  * ``273`` - IPPROTO_DTLS_1_2

Examples
~~~~~~~~

::

   AT#XSSOCKET=1,1,0,16842753,2
   #XSSOCKET: 2,1,258
   OK
   AT#XSOCKET=0
   #XSOCKET: 0,"closed"
   OK

   AT#XSSOCKET=1,2,0,16842753
   #XSSOCKET: 2,2,273
   OK
   AT#XSOCKET=0
   #XSOCKET: 0,"closed"
   OK

Read command
------------

The read command allows to check the secure socket handle.

Syntax
~~~~~~

::

   #XSSOCKET?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSSOCKET: <handle>[,<family>,<role>]

* The ``<handle>`` value is an integer, which can be interpreted as follows:

  * Positive - The socket is valid.
  * ``0`` - The socket is closed.

* The ``<family>`` value is is an integer.
  It can be one of the following:

  * ``1`` - IP protocol family version 4
  * ``2`` - IP protocol family version 6

* The ``<role>`` value is is an integer.
  It can be one of the following:

  * ``0`` - Client
  * ``1`` - Server

Examples
~~~~~~~~

::

   AT#XSSOCKET?
   #XSSOCKET: 2,1,0
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XSSOCKET=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSSOCKET: <list of op>,<list of types>,<list of roles>,<sec-tag>,<peer_verify>


* The ``<list of op>`` parameter has following values:

  * ``0`` - Close a secure socket
  * ``1`` - Open a secure socket for IP protocol family version 4
  * ``2`` - Open a secure socket for IP protocol family version 6

* The ``<list of types>>`` parameter has following values.

  * 1: SOCK_STREAM for TLS
  * 2: SOCK_DGRAM for DTLS

* The ``<list of roles>`` parameter has following values:

  * ``0`` - Client
  * ``1`` - Server

Examples
~~~~~~~~

::

   AT#XSSOCKET=?
   #XSSOCKET: (0,1,2),(1,2),<sec_tag>,<peer_verify>,<hostname_verify>
   OK

Socket options #XSOCKETOPT
==========================

The ``#XSOCKETOPT`` command allows to get and set socket options.

Set command
-----------

The set command allows to get and set socket options.

Syntax
~~~~~~

::

   #XSOCKETOPT=<op>,<name>[,<value>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Get
  * ``1`` - Set

For a complete list of the supported SET ``<name>`` accepted parameters, refer to the `SETSOCKETOPT Service Spec Reference`_.

For a complete list of the supported GET ``<name>`` accepted parameters, refer to the `GETSOCKETOPT Service Spec Reference`_.

Examples
~~~~~~~~

::

   AT#XSOCKETOPT=1,20,30
   OK

::

   AT#XSOCKETOPT=0,20
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

   #XSOCKETOPT: <list of op>,<name>,<value>

Examples
~~~~~~~~

::

   AT#XSOCKETOPT=?
   #XSOCKETOPT: (0,1),<name>,<value>
   OK

Secure Socket options #XSOCKETOPT
=================================

The ``#XSSOCKETOPT`` command allows to set secure socket options.

Set command
-----------

The set command allows to set secure socket options.

Syntax
~~~~~~

::

   #XSSOCKETOPT=<op>,<name>[,<value>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Get
  * ``1`` - Set

* The ``<name>`` parameter can accept one of the following values:

  * ``2`` - TLS_HOSTNAME, ``<value>`` is a string.
  * ``4`` - TLS_CIPHERSUITE_USED, get-only, return an IANA assigned ciphersuite identifier of chosen ciphersuite.
  * ``5`` - TLS_PEER_VERIFY, ``<value>`` is an integer, 0 or 1.
  * ``10`` - TLS_SESSION_CACHE, ``<value>`` is an integer, 0 or 1.
  * ``11`` - TLS_SESSION_CACHE_PURGE, ``<value>`` is an integer with any value.
  * ``12`` - TLS_DTLS_HANDSHAKE_TIMEO, ``<value>`` is an integer with a value in (1, 3, 7, 15, 31, 63, 123). Timeout in seconds.

For a complete list of the supported ``<name>`` accepted parameters, refer to the `SETSOCKETOPT Service Spec Reference`_.


Examples
~~~~~~~~

::

   AT#XSSOCKETOPT=1,5,2
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

   #XSSOCKETOPT=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSSOCKETOPT: <list of op>,<name>,<value>

Examples
~~~~~~~~

::

   AT#XSSOCKETOPT=?
   #XSSOCKETOPT: (1),<name>,<value>
   OK


Socket binding #XBIND
=====================

The ``#XBIND`` command allows to bind a socket with a local port.

This command is for TCP Server role, or UDP Server/Client role.

Set command
-----------

The set command allows to bind a socket with a local port.

Syntax
~~~~~~

::

   #XBIND=<port>

* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
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

Connection #XCONNECT
====================

The ``#XCONNECT`` command allows to connect to a server and to check the connection status.

This command is for TCP or UDP client role.

Set command
-----------

The set command allows to connect to a TCP or UDP server.

Syntax
~~~~~~

::

   #XCONNECT=<url>,<port>

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to. The max size of hostname is 128 bytes.
  For IP address, it supports both IPv4 and IPv6.

* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the port of the TCP or UDP service on the remote server.

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

::

   AT#XCONNECT="2a02:c207:2051:8976::1",4567
   #XCONNECT: 1
   OK

Read command
------------

The read command is not supported.


Test command
------------

The test command is not supported.

Set listen mode #XLISTEN
========================

The ``#XLISTEN`` command allows to put the TCP socket in listening mode for incoming connections.

This command is for TCP Server role.

Set command
-----------

The set command allows to put the TCP socket in listening mode for incoming connections.

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

Accept connection #XACCEPT
==========================

The ``#XACCEPT`` command allows to accept an incoming connection from a TCP client.

This command is for TCP Server role.

Set command
-----------

The set command allows to wait for the TCP client to connect.

Syntax
~~~~~~

::

   #XACCEPT=<timeout>

* The ``<timeout>`` value sets the timeout value in seconds.
  ``0`` means no timeout and this request becomes blocking.

Response syntax
~~~~~~~~~~~~~~~

::

   #XACCEPT: <handle>,<ip_addr>

* The ``<handle>`` value is an integer.
  It represents the socket handle of the accepted connection.
* The ``<ip_addr>`` value indicates the IP address of the peer host.

Examples
~~~~~~~~

::

   AT#XACCEPT=60
   #XACCEPT: 2,"192.168.0.2"
   OK

Read command
------------

The read command allows to check socket handle of the accepted connection.

Syntax
~~~~~~

::

   #XACCEPT?

Response syntax
~~~~~~~~~~~~~~~

::

   #XACCEPT: <handle>

* The ``<handle>`` value is an integer, which can be interpreted as follows:

  * Positive - The incoming socket is valid.
  * ``0`` - There is no active incoming connection.

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

The ``#XSEND`` command allows to send data over TCP or UDP connection.

Set command
-----------

The set command allows to send data over the connection.

Syntax
~~~~~~

::

   #XSEND[=<data>]

* The ``<data>`` parameter is a string that contains the data to be sent.
  The maximum size of data is 1252 bytes.
  If it's not specified, SLM enters ``slm_data_mode``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSEND: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of bytes that has been sent.

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

The ``#XRECV`` command allows to receive data over TCP or UDP connection.

Set command
-----------

The set command allows to receive data over the connection.

Syntax
~~~~~~

::

   #XRECV=<timeout>

* The ``<timeout>`` value sets the timeout value in seconds.
  ``0`` means no timeout and this request becomes blocking.

Response syntax
~~~~~~~~~~~~~~~

::

   <data>
   #XRECV: <size>

* The ``<data>`` value is a string that contains the data being received.
* The ``<size>`` value is an integer that represents the actual number of bytes received.

Examples
~~~~~~~~

::

   AT#XRECV=10
   Test OK
   #XRECV: 7
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

UDP send data #XSENDTO
======================

The ``#XSENDTO`` command allows to send data over UDP.

Set command
-----------

The set command allows to send data over UDP.

Syntax
~~~~~~

::

   #XSENDTO=<url>,<port>[,<data>]

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to send data to. The max size of hostname is 128 bytes.
  For IP address, it supports both IPv4 and IPv6.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the port of UDP service on remote peer.
* The ``<data>`` parameter is a string that contains the data to be sent.
  The maximum size of data is 1252 bytes.
  If it's not specified, SLM enters ``slm_data_mode``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSENDTO: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of bytes that has been sent.

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

The ``#XRECVFROM`` command allows to receive data over UDP.

Set command
-----------

The set command allows to receive data over UDP.

Syntax
~~~~~~

::

   #XRECVFROM=<timeout>

* The ``<timeout>`` value sets the timeout value in seconds.
  ``0`` means no timeout and this request becomes blocking.

Response syntax
~~~~~~~~~~~~~~~

::

   <data>
   #XRECVFROM: <size>,<ip_addr>

* The ``<data>`` value is a string that contains the data being received.
* The ``<size>`` value is an integer that represents the actual number of bytes received.
* The ``<ip_addr>`` value is an string that represents the IPv4 or IPv6 address of remote peer.

Examples
~~~~~~~~

::

   AT#XRECVFROM=10
   Test OK
   #XRECVFROM: 7,"192.168.1.100"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Resolve hostname #XGETADDRINFO
==============================

The ``#XGETADDRINFO`` command allows to resolve hostnames to IPv4 and/or IPv6 addresses.

Set command
-----------

The set command allows to resolve hostnames to IPv4 and/or IPv6 addresses.

Syntax
~~~~~~

::

   #XGETADDRINFO=<hostname>

* The ``<hostname>`` parameter is a string.

Response syntax
~~~~~~~~~~~~~~~

::

   #XGETADDRINFO: "<ip_addresses>"

* The ``<ip_addresses>`` value is a string.
  It indicates the IPv4 and/or IPv6 address of the resolved hostname.

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
