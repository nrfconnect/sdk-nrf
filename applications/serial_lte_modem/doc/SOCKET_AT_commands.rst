.. _SLM_AT_SOCKET:

Socket AT commands
******************

.. contents::
   :local:
   :depth: 2

The following commands list contains socket-related AT commands.
The application can open up to 8 sockets and select the active one among them.

For more information on the networking services, see the `Zephyr Network APIs`_.

Socket #XSOCKET
===============

The ``#XSOCKET`` command allows you to open or close a socket and to check the socket handle.

Set command
-----------

The set command allows you to open or close a socket.

Syntax
~~~~~~

::

   #XSOCKET=<op>[,<type>,<role>[,<cid>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Close a socket.
  * ``1`` - Open a socket for IP protocol family version 4.
  * ``2`` - Open a socket for IP protocol family version 6.

  When ``0``, the highest-ranked socket is made active after the current one is closed.

* The ``<type>`` parameter can accept one of the following values:

  * ``1`` - Set ``SOCK_STREAM`` for the stream socket type using the TCP protocol.
  * ``2`` - Set ``SOCK_DGRAM`` for the datagram socket type using the UDP protocol.
  * ``3`` - Set ``SOCK_RAW`` for the raw socket type using a generic IP protocol.

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client.
  * ``1`` - Server.

* The ``<cid>`` parameter is an integer.
  It represents ``cid`` in the ``+CGDCONT`` command.
  Its default value is ``0``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKET: <handle>,<type>,<protocol>
   #XSOCKET: <result>,"closed"

* The ``<handle>`` value is an integer and can be interpreted as follows:

  * Positive or ``0`` - The socket opened successfully.
  * Negative - The socket failed to open.

* The ``<type>`` value can be one of the following integers:

  * ``1`` - Set ``SOCK_STREAM`` for the stream socket type using the TCP protocol.
  * ``2`` - Set ``SOCK_DGRAM`` for the datagram socket type using the UDP protocol.
  * ``3`` - Set ``SOCK_RAW`` for the raw socket type using a generic IP protocol.

* The ``<protocol>`` value can be one of the following integers:

  * ``0`` - IPPROTO_IP.
  * ``6`` - IPPROTO_TCP.
  * ``17`` - IPPROTO_UDP.

* The ``<result>`` value indicates the result of closing the socket.
  When ``0``, the socket closed successfully.

Examples
~~~~~~~~

::

   AT#XSOCKET=1,1,0
   #XSOCKET: 3,1,6
   OK
   AT#XSOCKET=1,2,0
   #XSOCKET: 1,2,17
   OK
   AT#XSOCKET=2,1,0
   #XSOCKET: 1,1,6
   OK
   AT#XSOCKET=1,3,0
   #XSOCKET: 1,3,0
   OK
   AT#XSOCKET=0
   #XSOCKET: 0,"closed"
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

   #XSOCKET: <handle>,<family>,<role>,<type>,<cid>

* The ``<handle>`` value is an integer.
  When positive or ``0``, the socket is valid.

* The ``<family>`` value is present only in the response to a request to open the socket.
  It can return one of the following values:

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.
  * ``3`` - Packet family.

* The ``<role>`` value can be one of the following integers:

  * ``0`` - Client.
  * ``1`` - Server.

* The ``<type>`` value can be one of the following integers:

  * ``1`` - Set ``SOCK_STREAM`` for the stream socket type using the TCP protocol.
  * ``2`` - Set ``SOCK_DGRAM`` for the datagram socket type using the UDP protocol.
  * ``3`` - Set ``SOCK_RAW`` for the raw socket type using a generic IP protocol.

* The ``<cid>`` parameter is an integer.
  It represents ``cid`` in the ``+CGDCONT`` command.

Example
~~~~~~~~

::

   AT#XSOCKET?
   #XSOCKET: 3,1,0,1,0
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

   #XSOCKET: <list of ops>,<list of types>,<list of roles>,<cid>

Example
~~~~~~~~

::

   AT#XSOCKET=?
   #XSOCKET: (0,1,2),(1,2,3),(0,1),<cid>
   OK

Secure socket #XSSOCKET
=======================

The ``#XSSOCKET`` command allows you to open or close a secure socket, and to check the socket handle.

.. note::
   TLS and DTLS servers are currently not supported.

Set command
-----------

The set command allows you to open or close a secure socket.

Syntax
~~~~~~

::

   #XSSOCKET=<op>[,<type>,<role>,<sec_tag>[,<peer_verify>[,<cid>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Close a socket.
  * ``1`` - Open a socket for IP protocol family version 4.
  * ``2`` - Open a socket for IP protocol family version 6.

  When ``0``, the highest-ranked socket is made active after the current one is closed.

* The ``<type>`` parameter can accept one of the following values:

  * ``1`` - Set ``SOCK_STREAM`` for the stream socket type using the TLS 1.2 protocol.
  * ``2`` - Set ``SOCK_DGRAM`` for the datagram socket type using the DTLS 1.2 protocol.

* The ``<role>`` parameter can accept one of the following values:

  * ``0`` - Client.
  * ``1`` - Server.

* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag to be used for establishing a secure connection.
  It is associated with a credential, that is, a certificate or PSK. The credential should be stored on the modem side beforehand.

* The ``<peer_verify>`` parameter can accept one of the following values:

  * ``0`` - None (default for server role).
  * ``1`` - Optional.
  * ``2`` - Required (default for client role).

* The ``<cid>`` parameter is an integer.
  It represents ``cid`` in the ``+CGDCONT`` command.
  Its default value is ``0``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSSOCKET: <handle>,<type>,<protocol>
   #XSOCKET: <result>,"closed"

* The ``<handle>`` value is an integer and can be interpreted as follows:

  * Positive or ``0`` - The socket opened successfully.
  * Negative - The socket failed to open.

* The ``<type>`` value can be one of the following integers:

  * ``1`` - ``SOCK_STREAM`` for the stream socket type using the TLS 1.2 protocol.
  * ``2`` - ``SOCK_DGRAM`` for the datagram socket type using the DTLS 1.2 protocol.

* The ``<protocol>`` value can be one of the following integers:

  * ``258`` - IPPROTO_TLS_1_2.
  * ``273`` - IPPROTO_DTLS_1_2.

* The ``<result>`` value indicates the result of closing the socket.
  When ``0``, the socket closed successfully.

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

The read command allows you to check the secure socket handle.

Syntax
~~~~~~

::

   #XSSOCKET?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSSOCKET: <handle>,<family>,<role>,<type>,<sec_tag>,<cid>

* The ``<handle>`` value is an integer.
  When positive or ``0``, the socket is valid.

* The ``<family>`` value can be one of the following integers:

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.

* The ``<role>`` value can be one of the following integers:

  * ``0`` - Client
  * ``1`` - Server

* The ``<type>`` value can be one of the following integers:

  * ``1`` - ``SOCK_STREAM`` for the stream socket type using the TLS 1.2 protocol.
  * ``2`` - ``SOCK_DGRAM`` for the datagram socket type using the DTLS 1.2 protocol.

* The ``<sec_tag>`` value is an integer.
  It indicates to the modem the credential of the security tag to be used for establishing a secure connection.

* The ``<cid>`` value is an integer.
  It represents ``cid`` in the ``+CGDCONT`` command.

Example
~~~~~~~~

::

   AT#XSSOCKET?
   #XSSOCKET: 2,1,0,1,16842753,0
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

   #XSSOCKET: <list of ops>,<list of types>,<list of roles>,<sec_tag>,<peer_verify>,<cid>

Example
~~~~~~~~

::

   AT#XSSOCKET=?
   #XSSOCKET: (0,1,2),(1,2),(0,1),<sec_tag>,<peer_verify>,<cid>
   OK

Select Socket #XSOCKETSELECT
============================

The ``#XSOCKETSELECT`` command allows you to select an active socket among multiple opened ones.

Set command
-----------

The set command allows you to select an active socket.

Syntax
~~~~~~

::

   #XSOCKETSELECT=<handle>

* The ``<handle>`` parameter is the handle value returned from the #XSOCKET or #XSSOCKET commands.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKETSELECT: <handle>

* The ``<handle>`` value is an integer.
  When positive or ``0``, the socket is valid.

Example
~~~~~~~~

::

   AT#XSOCKETSELECT=4
   #XSOCKETSELECT: 4
   OK

Read command
------------

The read command allows you to list all sockets that have been opened and the active socket.

Syntax
~~~~~~

::

   #XSOCKETSELECT?

Response syntax
~~~~~~~~~~~~~~~

::

   #XSOCKETSELECT: <handle>,<family>,<role>,<type>,<sec_tag>,<ranking>,<cid>
   #XSOCKETSELECT: <handle_active>

* The ``<handle>`` value is an integer that indicates the handle of the socket.

* The ``<family>`` value can be one of the following integers:

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.

* The ``<role>`` value can be one of the following integers:

  * ``0`` - Client.
  * ``1`` - Server.

* The ``<type>`` value can return one of the following:

  * ``1`` - Set ``SOCK_STREAM`` for the stream socket type using the TLS 1.2 protocol.
  * ``2`` - Set ``SOCK_DGRAM`` for the datagram socket type using the DTLS 1.2 protocol.

* The ``<sec_tag>`` value is an integer.
  It indicates to the modem the credential of the security tag to be used for establishing a secure connection.
  For a non-secure socket, it returns the value of -1.

* The ``<ranking>`` value is an integer.
  It indicates the ranking value of this socket, where the largest value means the highest ranking.

* The ``<cid>`` value is an integer.
  It represents ``cid`` in the ``+CGDCONT`` command.

* The ``<handle_active>`` value is an integer that indicates the handle of the active socket.

Examples
~~~~~~~~

::

  AT#XSOCKETSELECT?
  #XSOCKETSELECT: 0,1,0,1,-1,2,0
  #XSOCKETSELECT: 1,1,0,2,-1,3,0
  #XSOCKETSELECT: 2,1,0,1,16842755,4,0
  #XSOCKETSELECT: 3,1,0,2,16842755,5,0
  #XSOCKETSELECT: 4,1,1,1,-1,6,0
  #XSOCKETSELECT: 5,1,1,2,-1,7,0
  #XSOCKETSELECT: 6,1,1,1,16842755,8,0
  #XSOCKETSELECT: 7,1,0,1,-1,9,0
  #XSOCKETSELECT: 7
  OK

  AT#XSOCKETSELECT=4
  #XSOCKETSELECT: 4,1,1
  OK

  AT#XSOCKETSELECT?
  #XSOCKETSELECT: 0,1,0,1,-1,2,0
  #XSOCKETSELECT: 1,1,0,2,-1,3,0
  #XSOCKETSELECT: 2,1,0,1,16842755,4,0
  #XSOCKETSELECT: 3,1,0,2,16842755,5,0
  #XSOCKETSELECT: 4,1,1,1,-1,6,0
  #XSOCKETSELECT: 5,1,1,2,-1,7,0
  #XSOCKETSELECT: 6,1,1,1,16842755,8,0
  #XSOCKETSELECT: 7,1,0,1,-1,9,0
  #XSOCKETSELECT: 4
  OK

Test command
------------

The test command is not supported.

Socket options #XSOCKETOPT
==========================

The ``#XSOCKETOPT`` command allows you to get and set socket options.

Set command
-----------

The set command allows you to get and set socket options.

Syntax
~~~~~~

::

   #XSOCKETOPT=<op>,<name>[,<value>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Get
  * ``1`` - Set

* The ``<name>`` parameter can accept one of the following values:

  * ``2`` - :c:macro:`SO_REUSEADDR` (set-only).

    * ``<value>`` is an integer that indicates whether the reuse of local addresses is enabled.
      It is ``0`` for disabled or ``1`` for enabled.

  * ``20`` - :c:macro:`SO_RCVTIMEO`.

    * ``<value>`` is an integer that indicates the receive timeout in seconds.

  * ``21`` - :c:macro:`SO_SNDTIMEO`.

    * ``<value>`` is an integer that indicates the send timeout in seconds.

  * ``25`` - :c:macro:`SO_BINDTODEVICE` (set-only).

    * ``<value>`` is an integer that indicates the PDP context ID to bind to.

  * ``30`` - :c:macro:`SO_SILENCE_ALL`.

    * ``<value>`` is an integer that indicates whether ICMP echo replies for IPv4 and IPv6 are disabled.
      It is ``0`` for allowing ICMP echo replies or ``1`` for disabling them.

  * ``31`` - :c:macro:`SO_IP_ECHO_REPLY`.

    * ``<value>`` is an integer that indicates whether ICMP echo replies for IPv4 are enabled.
      It is ``0`` for disabled or ``1`` for enabled.

  * ``32`` - :c:macro:`SO_IPV6_ECHO_REPLY`.

    * ``<value>`` is an integer that indicates whether ICMP echo replies for IPv6 are enabled.
      It is ``0`` for disabled or ``1`` for enabled.

  * ``50`` - :c:macro:`SO_RAI_NO_DATA` (set-only).
    Immediately release the RRC.

    * ``<value>`` is ignored.

  * ``51`` - :c:macro:`SO_RAI_LAST` (set-only).
    Enter Radio Resource Control (RRC) idle immediately after the next send operation.

    * ``<value>`` is ignored.

  * ``52`` - :c:macro:`SO_RAI_ONE_RESP` (set-only).
    Wait for one incoming packet after the next send operation, before entering RRC idle mode.

    * ``<value>`` is ignored.

  * ``53`` - :c:macro:`SO_RAI_ONGOING` (set-only).
    Keep RRC in connected mode after the next send operation (client).

    * ``<value>`` is ignored.

  * ``54`` - :c:macro:`SO_RAI_WAIT_MORE` (set-only).
    Keep RRC in connected mode after the next send operation (server).

    * ``<value>`` is ignored.

See `nRF socket options`_ for explanation of the supported options.

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

   #XSOCKETOPT: <list of ops>,<name>,<value>

Example
~~~~~~~~

::

   AT#XSOCKETOPT=?
   #XSOCKETOPT: (0,1),<name>,<value>
   OK

.. _SLM_AT_SSOCKETOPT:

Secure socket options #XSSOCKETOPT
==================================

The ``#XSSOCKETOPT`` command allows you to set secure socket options.

Set command
-----------

The set command allows you to set secure socket options.

Syntax
~~~~~~

::

   #XSSOCKETOPT=<op>,<name>[,<value>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Get.
  * ``1`` - Set.

* The ``<name>`` parameter can accept one of the following values:

  * ``2`` - :c:macro:`TLS_HOSTNAME` (set-only).

    * ``<value>`` is a string that indicates the hostname to check against during TLS handshakes.
      It can be ``NULL`` to disable hostname verification.

  * ``4`` - :c:macro:`TLS_CIPHERSUITE_USED` (get-only).
    The TLS cipher suite chosen during the TLS handshake.
    This option is only supported with modem firmware 2.0.0 and newer.

  * ``5`` - :c:macro:`TLS_PEER_VERIFY`.

    * ``<value>`` is an integer that indicates what peer verification level should be used.
      It is ``0`` for none, ``1`` for optional or ``2`` for required.

  * ``12`` - :c:macro:`TLS_SESSION_CACHE`.

    * ``<value>`` is an integer that indicates whether TLS session caching should be used.
      It is ``0`` for disabled or ``1`` for enabled.

  * ``13`` - :c:macro:`TLS_SESSION_CACHE_PURGE` (set-only).
    Indicates that the TLS session cache should be deleted.

    * ``<value>`` can be any integer value.

  * ``14`` - :c:macro:`TLS_DTLS_HANDSHAKE_TIMEO`.

    * ``<value>`` is an integer that indicates the DTLS handshake timeout in seconds.
      It can be one of the following values: ``1``, ``3``, ``7``, ``15``, ``31``, ``63``, ``123``.

  * ``17`` - :c:macro:`TLS_DTLS_CID`.

    * ``<value>`` is an integer that indicates the DTLS connection identifier setting.
      It can be one of the following values:

      * ``0`` - :c:macro:`TLS_DTLS_CID_DISABLED`.
      * ``1`` - :c:macro:`TLS_DTLS_CID_SUPPORTED`.
      * ``2`` - :c:macro:`TLS_DTLS_CID_ENABLED`.

    This option is only supported with modem firmware 1.3.5 and newer.
    See :ref:`nrfxlib:dtls_cid_setting` for more details regarding the allowed values.

  * ``18`` - :c:macro:`TLS_DTLS_CID_STATUS` (get-only).
    It is the DTLS connection identifier status.
    It can be retrieved after the DTLS handshake.
    This option is only supported with modem firmware 1.3.5 and newer.
    See :ref:`nrfxlib:dtls_cid_status` for more details regarding the returned values.

See `nRF socket options`_ for explanation of the supported options.


Example
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

   #XSSOCKETOPT: <list of ops>,<name>,<value>

Example
~~~~~~~~

::

   AT#XSSOCKETOPT=?
   #XSSOCKETOPT: (0,1),<name>,<value>
   OK


Socket binding #XBIND
=====================

The ``#XBIND`` command allows you to bind a socket with a local port.

This command can be used with TCP servers and both UDP clients and servers.

Set command
-----------

The set command allows you to bind a socket with a local port.

Syntax
~~~~~~

::

   #XBIND=<port>

* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the specific port to use for binding the socket.

Example
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

The ``#XCONNECT`` command allows you to connect to a server and to check the connection status.

This command is for TCP and UDP clients.

Set command
-----------

The set command allows you to connect to a TCP or UDP server.

Syntax
~~~~~~

::

   #XCONNECT=<url>,<port>

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address of the server.
  The maximum supported size of the hostname is 128 bytes.
  When using IP addresses, it supports both IPv4 and IPv6.

* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the port of the TCP or UDP service on the remote server.

Response syntax
~~~~~~~~~~~~~~~

::

   #XCONNECT: <status>

* The ``<status>`` value is an integer.
  It can return one of the following values:

* ``1`` - Connected.
* ``0`` - Disconnected.

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

The ``#XLISTEN`` command allows you to put the TCP socket in listening mode for incoming connections.

This command is for TCP servers.

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

Example
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

The ``#XACCEPT`` command allows you to accept an incoming connection from a TCP client.

This command is for TCP servers.

Set command
-----------

The set command allows you to wait for the TCP client to connect.

Syntax
~~~~~~

::

   #XACCEPT=<timeout>

* The ``<timeout>`` value sets the timeout value in seconds.
  ``0`` means no timeout, and it makes this request become blocking.

Response syntax
~~~~~~~~~~~~~~~

::

   #XACCEPT: <handle>,<ip_addr>

* The ``<handle>`` value is an integer.
  It represents the socket handle of the accepted connection.
* The ``<ip_addr>`` value indicates the IP address of the peer host.

Example
~~~~~~~~

::

   AT#XACCEPT=60
   #XACCEPT: 2,"192.168.0.2"
   OK

Read command
------------

The read command allows you to check socket handle of the accepted connection.

Syntax
~~~~~~

::

   #XACCEPT?

Response syntax
~~~~~~~~~~~~~~~

::

   #XACCEPT: <handle>

* The ``<handle>`` value is an integer and can be interpreted as follows:

  * Positive - The incoming socket is valid.
  * ``0`` - There is no active incoming connection.

Example
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

The ``#XSEND`` command allows you to send data over TCP and UDP connections.

Set command
-----------

The set command allows you to send data over the connection.

Syntax
~~~~~~

::

   #XSEND[=<data>]

* The ``<data>`` parameter is a string that contains the data to be sent.
  The maximum size of the data is 1024 bytes.
  When the parameter is not specified, SLM enters ``slm_data_mode``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSEND: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of bytes that has been sent.

Example
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

The ``#XRECV`` command allows you to receive data over TCP or UDP connections.

Set command
-----------

The set command allows you to receive data over the connection.

Syntax
~~~~~~

::

   #XRECV=<timeout>[,<flags>]

The ``<timeout>`` value sets the timeout value in seconds.
When ``0``, it means no timeout, and it makes this request become blocking.

The ``<flags>`` value sets the receiving behavior based on the BSD socket definition.
It can be set to one of the following values:

* ``2`` means reading data without removing it from the socket input queue.
* ``64`` means overriding the operation to non-blocking.
* ``256`` (TCP only) means blocking until the full amount of data can be returned.

Response syntax
~~~~~~~~~~~~~~~

::

   #XRECV: <size>
   <data>

* The ``<data>`` value is a string that contains the data being received.
* The ``<size>`` value is an integer that represents the actual number of bytes received.

Example
~~~~~~~~

::

   AT#XRECV=10
   #XRECV: 7
   Test OK
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

UDP send data #XSENDTO
======================

The ``#XSENDTO`` command allows you to send data over UDP.

Set command
-----------

The set command allows you to send data over UDP.

Syntax
~~~~~~

::

   #XSENDTO=<url>,<port>[,<data>]

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address of the remote peer.
  The maximum size of the hostname is 128 bytes.
  When using IP addresses, it supports both IPv4 and IPv6.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the port of the UDP service on remote peer.
* The ``<data>`` parameter is a string that contains the data to be sent.
  Its maximum size is 1024 bytes.
  When the parameter is not specified, SLM enters ``slm_data_mode``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XSENDTO: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of bytes that has been sent.

Example
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

The ``#XRECVFROM`` command allows you to receive data over UDP.

Set command
-----------

The set command allows you to receive data over UDP.

Syntax
~~~~~~

::

   #XRECVFROM=<timeout>[,<flags>]

The ``<timeout>`` value sets the timeout value in seconds.
When ``0``, it means no timeout, and it makes this request become blocking.

The ``<flags>`` value sets the receiving behavior based on the BSD socket definition.
It can be set to one of the following values:

* ``2`` means reading data without removing it from the socket input queue.
* ``64`` means overriding the operation to non-blocking.

Response syntax
~~~~~~~~~~~~~~~

::

   #XRECVFROM: <size>,<ip_addr>
   <data>

* The ``<data>`` value is a string that contains the data being received.
* The ``<size>`` value is an integer that represents the actual number of bytes received.
* The ``<ip_addr>`` value is a string that represents the IPv4 or IPv6 address of remote peer.

Example
~~~~~~~~

::

   AT#XRECVFROM=10
   #XRECVFROM: 7,"192.168.1.100"
   Test OK
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Poll sockets #XPOLL
===================

The ``#XPOLL`` command allows you to poll selected or all sockets that have already been opened.

Set command
-----------

The set command allows you to poll a set of sockets to check whether they are ready for I/O.

Syntax
~~~~~~

::

   #XPOLL=<timeout>[,<handle1>[,<handle2> ...<handle8>]

* The ``<timeout>`` value sets the timeout value in milliseconds, and the poll blocks up to this timeout.
  ``0`` means no timeout, and the poll returns without blocking.
  ``-1`` means indefinite, and the poll blocks indefinitely until any events are received.

* The ``<handleN>`` value sets the socket handles to poll.
  The handles values could be obtained by ``AT#XSOCKETSELECT?`` command.
  If no handle values are specified, all opened sockets will be polled.

Response syntax
~~~~~~~~~~~~~~~

::

   #XPOLL: <error>
   #XPOLL: <handle>,<revents>

* The ``<error>`` value is an error code when the poll fails.
* The ``<handle>`` value is an integer. It is the handle of a socket that have events returned, so-called ``revents``.
* The ``<revents>`` value is a hexadecimal string. It represents the returned events, which could be a combination of POLLIN, POLLERR, POLLHUP and POLLNVAL.

Examples
~~~~~~~~

::

   AT#XPOLL=2000,0
   #XPOLL: 0,"0x0001"
   OK

   AT#XPOLL=2000,1
   #XPOLL: 1,"0x0001"
   OK

   AT#XPOLL=2000
   #XPOLL: 0,"0x0001"
   #XPOLL: 1,"0x0001"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Resolve hostname #XGETADDRINFO
==============================

The ``#XGETADDRINFO`` command allows you to resolve hostnames to IPv4 and IPv6 addresses.

Set command
-----------

The set command allows you to resolve hostnames to IPv4 and IPv6 addresses.

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
  It indicates the IPv4 or IPv6 address of the resolved hostname.

Example
~~~~~~~~

::

   AT#XGETADDRINFO="nordicsemi.com"
   #XGETADDRINFO: "104.20.251.111"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
