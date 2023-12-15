.. _SLM_AT_HTTPC:

HTTP client AT commands
***********************

.. contents::
   :local:
   :depth: 2

This page describes the HTTP client AT commands.

HTTP client connection #XHTTPCCON
=================================

The ``#XHTTPCCON`` command allows you to connect to and disconnect from an HTTP server and to show the HTTP connection information.

Set command
-----------

The set command allows you to connect to and disconnect from an HTTP server.

Syntax
~~~~~~

::

   AT#XHTTPCCON=<op>[,<host>,<port>[,<sec_tag>[,peer_verify[,hostname_verify]]]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Disconnect from the HTTP server.
  * ``1`` - Connect to the HTTP server for IP protocol family version 4.
  * ``2`` - Connect to the HTTP server for IP protocol family version 6.

* The ``<host>`` parameter is a string.
  It represents the HTTP server hostname.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the HTTP server port.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.
* The ``<peer_verify>`` parameter accepts the following values:

  * ``0`` - None.
    Do not authenticate the peer.
  * ``1`` - Optional.
    Optionally authenticate the peer.
  * ``2`` - Required (default).
    Authenticate the peer.

* The ``<hostname_verify>`` parameter accepts the following values:

  * ``0`` - Do not verify the hostname against the received certificate.
  * ``1`` - Verify the hostname against the received certificate (default).

See `nRF socket options`_ ``peer_verify`` and ``tls_hostname`` for more information on ``<peer_verify>`` and ``<hostname_verify>``.


Response syntax
~~~~~~~~~~~~~~~

::

   #XHTTPCCON: <state>

* ``<state>`` is one of the following:

  * ``0`` - Disconnected
  * ``1`` - Connected

Example
~~~~~~~

::

   AT#XHTTPCCON=1,"postman-echo.com",80
   #XHTTPCCON:1
   OK

Read command
------------

The read command shows the HTTP connection information.

Syntax
~~~~~~

::

   AT#XHTTPCCON?

Response syntax
~~~~~~~~~~~~~~~

::

   XHTTPCCON: <state>,<host>,<port>[,<sec_tag>]

Example
~~~~~~~

::

   AT#XHTTPCCON?
   #XHTTPCCON: 1,"postman-echo.com",80
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Example
~~~~~~~

::

   AT#XHTTPCCON=?
   #XHTTPCCON: (0,1),<host>,<port>,<sec_tag>,<peer_verify>,<hostname_verify>
   OK

HTTP request #XHTTPCREQ
=======================

The ``#XHTTPCREQ`` command allows you to send an HTTP request to the server.

Set command
-----------

The set command allows you to send an HTTP request to the server.

Syntax
~~~~~~

::

   AT#XHTTPCREQ=<method>,<resource>[,<headers>[,<content_type>,<content_length>[,<chunked_transfer>]]]

* The ``<method>`` is a string.
  It represents the request method string.
* The ``<resource>`` is a string.
  It represents the target resource to apply the request.
* The ``<headers>`` parameter is a string.
  It represents the *optional headers* field of the request.
  Each header field should end with ``<CR><LF>``.
  Any occurrence of "\\r\\n" (4 bytes) inside is replaced by ``<CR><LF>`` (2 bytes).

  .. note::
     ``Host``, ``Content-Type`` and ``Content-Length`` must not be included.

* The ``<content_type>`` is a string.
  It represents the HTTP/1.1 ``Content-Type`` of the payload.
* The ``<content_length>`` is an integer.
  It represents the HTTP/1.1 ``Content-Length`` of the payload.
  This parameter is ignored if ``<chunked_transfer>`` is not ``0``.
* The ``<chunked_transfer>`` is an integer.
  It indicates if the payload will be sent in chunked mode or not.

  * ``0`` - normal mode (default)
  * ``1`` - chunked mode

  If ``<content_length>`` is greater than ``0`` or ``<chunked_transfer>`` is not ``0``, the SLM application enters ``slm_data_mode``.
  The SLM sends the payload to the HTTP server until the terminator string defined in :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` is received.

Response syntax
~~~~~~~~~~~~~~~

::

   #XHTTPCREQ: <state>

``<state>`` is one of the following:

* ``0`` - Request sent successfully
* ``1`` - Wait for payload data
* *Negative integer* - Error code

Example
~~~~~~~

The following example sends a GET request to retrieve data from the server without any optional header:

::

   AT#XHTTPCCON=1,"postman-echo.com",80

   #XHTTPCCON: 1

   OK

   AT#XHTTPCREQ="GET","/get?foo1=bar1&foo2=bar2"

   OK

   #XHTTPCREQ: 0

   HTTP/1.1 200 OK
   Date: Tue, 01 Mar 2022 05:22:27 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 244
   Connection: keep-alive
   ETag: W/"f4-/OfnvALw5zFsaujZvrn62iBBcKo"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3AzTRyDH581ybGp-7K1k78tkBmVLeybFTY.Z7c5iNEaK0hH5hIMsuJpuZEH18d%2FbtSqOuhRAh1GmYM; Path=/; HttpOnly


   #XHTTPCRSP:337,1
   {"args":{"foo1":"bar1","foo2":"bar2"},"headers":{"x-forwarded-proto":"http","x-forwarded-port":"80","host":"postman-echo.com","x-amzn-trace-id":"Root=1-621dad93-79bf415c46aa37f925498d97"},"url":"http://postman-echo.com/get?foo1=bar1&foo2=bar2"}
   #XHTTPCRSP:244,1

The following example sends a POST request, with headers delimited by "\\r\\n", and with a JSON payload:

::

   AT#XHTTPCREQ="POST","/post","User-Agent: slm\r\naccept: */*\r\n","application/json",17

   OK

   #XHTTPCREQ: 1
   {"hello":"world"}+++
   #XHTTPCREQ: 0

   #XDATAMODE: 0

   HTTP/1.1 200 OK
   Date: Tue, 01 Mar 2022 05:22:28 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 359
   Connection: keep-alive
   ETag: W/"167-2YuosrP0ARLW1c5oeDiW7MId014"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3A_b9-1rOslsmoczQUGjv93SicuBw8f6lb.x%2B6xkThAVld5%2FpykDn7trZ9JGh%2Fir3MVU0izYBfB0Kg; Path=/; HttpOnly


   #XHTTPCRSP:342,1
   {"args":{},"data":{"hello":"world"},"files":{},"form":{},"headers":{"x-forwarded-proto":"http","x-forwarded-port":"80","host":"postman-echo.com","x-amzn-trace-id":"Root=1-621dad94-2fcac1637dc28f172c6346e6","content-length":"17","user-agent":"slm","accept":"*/*","content-type":"application/json"},"json":{"hello":"world"},"url":"http://postman-echo.com/post"}
   #XHTTPCRSP:359,1

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

HTTP response #XHTTPCRSP
========================

The ``#XHTTPCRSP`` is an unsolicited notification that indicates that a part of the HTTP response has been received.

Unsolicited notification
------------------------

It indicates that a part of the HTTP response has been received.

Syntax
~~~~~~

::

   <response><CR><LF>#XHTTPCRSP:<received_byte_count>,<state>

* ``<response>`` is the raw data of the HTTP response, including headers and body.
* ``<received_byte_count>`` is an integer.
  It represents the length of a partially received HTTP response.
* ``<state>`` is one of the following:

  * ``0`` - There is more HTTP response data to come.
  * ``1`` - The entire HTTP response has been received.
