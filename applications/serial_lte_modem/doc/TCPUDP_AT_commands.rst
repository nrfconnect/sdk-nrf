.. _SLM_AT_TCP_UDP:

TCP and UDP AT commands
***********************

.. contents::
   :local:
   :depth: 2

The following commands list contains TCP-related and UDP-related AT commands.
When native TLS is used, you must store the credentials using the ``AT#XCMNG`` AT command.

For more information on the networking services, see the `Zephyr Network APIs`_.

TCP server #XTCPSVR
===================

The ``#XTCPSVR`` command allows you to start and stop the TCP/TLS server.

Set command
-----------

The set command allows you to start and stop the TCP/TLS server.

Syntax
~~~~~~

::

   #XTCPSVR=<op>[<port>[,<sec_tag>]]


* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Stop the server.
  * ``1`` - Start the server using IP protocol family version 4.
  * ``2`` - Start the server using IP protocol family version 6.

* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the TCP service port.
  It is mandatory to set it when starting the server.
* The ``<sec_tag>`` parameter is an integer.
  If it is given, a TLS server will be started.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSVR: <handle>,"started"

   #XTCPSVR: <error>,"not started"

* The ``<handle>`` value is an integer.
  When positive, it indicates the handle of the successfully opened listening socket.
* The ``<error>`` value is an integer.
  It represents the error value according to the standard POSIX *errno*.

Examples
~~~~~~~~

::

   AT#XTCPSVR=1,3442,600
   #XTCPSVR: 2,"started"
   OK

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

   #XTCPSVR: <listen_socket_handle>,<income_socket_handle>,<family>

* The ``<xxx_socket_handle>`` value is an integer.
  When positive, it indicates that the socket opened successfully.
  When negative, it indicates that the socket failed to open or that there is no incoming connection.

* The ``<family>`` value is an integer.

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.

Examples
~~~~~~~~

::

   AT#XTCPSVR?
   #XTCPSVR: 1,2,1
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

   #XTCPSVR: <list of ops>,<port>,<sec_tag>

Examples
~~~~~~~~

::

   AT#XTCPSVR=?
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

  * ``0`` - Disconnect.
  * ``1`` - Connect to the server for IP protocol family version 4.
  * ``2`` - Connect to the server for IP protocol family version 6.

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to.
  Its maximum size is 128 bytes.
  When the parameter is an IP address, it supports both IPv4 and IPv6.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the TCP/TLS service port on the remote server.
* The ``<sec_tag>`` parameter is an integer.
  If it is given, a TLS client will be started.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPCLI: <handle>,"connected"

   #XTCPCLI: <error>,"not connected"

* The ``<handle>`` value is an integer.
  When positive, it indicates the handle of the successfully opened socket.
* The ``<error>`` value is an integer.
  It is either an *errno* code or one of the :c:enum:`dns_resolve_status` values defined in :file:`zephyr/net/dns_resolve.h`.

Examples
~~~~~~~~

::

   AT#XTCPCLI=1,"remote.ip",1234
   #XTCPCLI: 2,"connected"
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

   #XTCPCLI: <handle>,<family>

* The ``<handle>`` value is an integer.
  When positive, it indicates the handle of the successfully opened socket.
  When negative, it indicates that the client socket failed to open.

* The ``<family>`` value is an integer.

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XTCPCLI=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPCLI: <list of ops>,<url>,<port>,<sec_tag>

Examples
~~~~~~~~

::

   AT#XTCPCLI=?
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

   #XTCPSEND[=<data>]

* The ``<data>`` parameter is a string that contains the data to be sent.
  The maximum size of the data is 1024 bytes.
  When the parameter is not specified, SLM enters ``slm_data_mode``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSEND: <size>

* The ``<size>`` value is an integer.
  It represents the actual number of the bytes sent.

Examples
~~~~~~~~

::

   AT#XTCPSEND="Test TLS client"
   #XTCPSEND: 15
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

TCP hang up #XTCPHANGUP
=======================

The ``#XTCPHANGUP`` command allows you to disconnect an incoming connection.

Set command
-----------

The set command allows you to disconnect an incoming connection.
This function is reserved to TCP server role by its nature.

Syntax
~~~~~~

::

   #XTCPHANGUP=<handle>

* The ``<handle>`` parameter is an integer.
  Refer to ``#XTCPSVR?`` command for the ``<income_socket_handle>``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XTCPSVR: <cause>,"disconnected"

* The ``<cause>`` value is an integer of -111 or ECONNREFUSED.

Examples
~~~~~~~~

::

   AT#XTCPSVR?
   #XTCPSVR: 1,2,1
   OK
   AT#XTCPHANGUP=2
   #XTCPSVR: -111,"disconnected"
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

   #TCPHANGUP=?

Response syntax
~~~~~~~~~~~~~~~

::

   #TCPHANGUP: <handle>

Examples
~~~~~~~~

::

   AT#TCPHANGUP=?
   #TCPHANGUP: <handle>
   OK


TCP receive data
================

::

   <data>
   #XTCPDATA: <size>

* The ``<data>`` parameter is a string that contains the data received.
* The ``<size>`` parameter is the size of the string, which is present only when SLM is not operating in ``slm_data_mode``.

UDP server #XUDPSVR
===================

The ``#XUDPSVR`` command allows you to start and stop the UDP server.

.. note::
   DTLS server functionality is not supported by nRF91 Series devices.

Set command
-----------

The set command allows you to start and stop the UDP server.

Syntax
~~~~~~

::

   #XUDPSVR=<op>[,<port>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Stop the server.
  * ``1`` - Start the server for IP protocol family version 4.
  * ``2`` - Start the server for IP protocol family version 6.

* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the UDP service port.
  It is mandatory for starting the server.

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPSVR: <handle>,"started"

* The ``<handle>`` value is an integer.
  It indicates the handle of the successfully opened listening socket.

Examples
~~~~~~~~

::

   AT#XUDPSVR=1,3442
   #XUDPSVR: 2,"started"
   OK

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

   #XUDPSVR: <handle>,<family>

* The ``<handle>`` value is an integer.
  When positive, it indicates the handle of the successfully opened socket.
  When negative, it indicates that it failed to open.

* The ``<family>`` value is an integer.

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.

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

   #XUDPSVR: <list of ops>,<port>

Examples
~~~~~~~~

::

   AT#XUDPSVR=?
   #XUDPSVR: (0,1,2),<port>
   OK

UDP/DTLS client #XUDPCLI
========================

The ``#XUDPCLI`` command allows you to create a UDP/DTLS client and to connect to a server.

.. note::
   The UDP/DTLS client always works in a connection-oriented way.

Set command
-----------

The set command allows you to create a UDP/DTLS client and connect to a server.

Syntax
~~~~~~

::

   #XUDPCLI=<op>[,<url>,<port>[,<sec_tag>[,<use_dtls_cid>]]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Disconnect.
  * ``1`` - Connect to the server for IP protocol family version 4.
  * ``2`` - Connect to the server for IP protocol family version 6.

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address to connect to.
  Its maximum size can be 128 bytes.
  When the parameter is an IP address, it supports both IPv4 and IPv6.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the UDP/DTLS service port on the remote server.
* The ``<sec_tag>`` parameter is an integer.
  If it is given, a DTLS client will be started.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.
* The ``<use_dtls_cid>`` parameter is an integer.
  It indicates whether to use DTLS's connection identifier.
  This parameter is only supported with modem firmware 1.3.5 and newer.
  See :ref:`SLM_AT_SSOCKETOPT` for more details regarding the allowed values.
  The command will fail (with ``<error>`` equal to ``-134`` (``-ENOTSUP``) if the requested connection identifier is not accepted by the server.

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPCLI: <handle>,"connected"

   #XUDPCLI: <error>,"not connected"

* The ``<handle>`` value is an integer.
  When positive, it indicates the handle of the successfully opened socket.
* The ``<error>`` value is an integer.
  It is either an *errno* code or one of the :c:enum:`dns_resolve_status` values defined in :file:`zephyr/net/dns_resolve.h`.

Examples
~~~~~~~~

::

   AT#XUDPCLI=1,"remote.host",2442
   #XUDPCLI: 2,"connected"
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

   #XUDPCLI: <handle>,<family>

* The ``<handle>`` value is an integer.
  When positive, it indicates the handle of the successfully opened socket.
  When negative, it indicates that it failed to open.

* The ``<family>`` value is an integer.

  * ``1`` - IP protocol family version 4.
  * ``2`` - IP protocol family version 6.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XUDPCLI: <list of ops>,<url>,<port>,<sec_tag>,<use_dtls_cid>

Examples
~~~~~~~~

::

   AT#XUDPCLI=?
   #XUDPCLI: (0,1,2),<url>,<port>,<sec_tag>,<use_dtls_cid>
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

   #XUDPSEND[=<data>]

* The ``<data>`` parameter is a string that contains the data to be sent.
  The maximum size of the data is 1024 bytes.
  When the parameter is not specified, SLM enters ``slm_data_mode``.

Response syntax
~~~~~~~~~~~~~~~

::

   #XUDPSEND: <size>

* The ``<size>`` value is an integer.
  It indicates the actual number of bytes sent.

Examples
~~~~~~~~

::

   AT#XUDPSEND="Test UDP by hostname"
   #XUDPSEND: 20
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

UDP receive data
================

::

   <data>
   #XUDPDATA: <size>

* The ``<data>`` parameter is a string that contains the data received.
* The ``<size>`` parameter is the size of the string, which is present only when SLM is not operating in ``slm_data_mode``.
