.. _SLM_AT_HTTPC:

HTTP client AT commands
***********************

.. contents::
   :local:
   :depth: 2

The following commands list contains the HTTP client AT commands.

HTTP client connection #XHTTPCCON
=================================

The ``#XHTTPCCON`` command allows you to connect to and disconnect from an HTTP server and to show the HTTP connection information.

Set command
-----------

The set command allows you to connect to and disconnect from an HTTP server.

Syntax
~~~~~~

::

   AT#XHTTPCCON=<op>[,<host>,<port>[,<sec_tag>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Disconnect from the HTTP server
  * ``1`` - Connect to the HTTP server

* The ``<host>`` parameter is a string.
  It represents the HTTP server hostname.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the HTTP server port.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection.


Response syntax
~~~~~~~~~~~~~~~

::

   #XHTTPCCON=<state>

* The ``<state>`` value can assume one of the following values:

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

   XHTTPCCON: <state>,<host>,<port>[,<sec_tag>]]

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
   #XHTTPCCON: (0,1),<host>,<port>,<sec_tag>
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

   AT#XHTTPCREQ=<method>,<resource>,<header>[,<payload_length>]

* The ``<method>`` is a string.
  It represents the request method string.
* The ``<resource>`` is a string.
  It represents the target resource to apply the request.
* The ``<header>`` is a string.
  It represents the header field of the request.
  Each header field should end with ``<CR><LF>``.
* The ``<payload_length>`` is an integer.
  It represents the length of the payload.
  If ``payload_length`` is greater than ``0``, the SLM will enter the pass-through mode and expect the upcoming UART input data as payload.
  The SLM will then send the payload to the HTTP server until the ``payload_length`` bytes are sent.
  To abort sending the payload, use ``AT#XHTTPCCON=0`` to disconnect from the server.

Response syntax
~~~~~~~~~~~~~~~

::

   #XHTTPCREQ:<state>

The ``<state>`` value can assume one of the following values:

* ``0`` - Request sent successfully
* ``1`` - Wait for payload data
* *Negative integer* - Error code

Example
~~~~~~~

The following example sends a GET request to retrieve data from the server without any optional header.

::

   AT#XHTTPCREQ="GET","/get?foo1=bar1&foo2=bar2",""

   OK


   #XHTTPCREQ: 0

   #XHTTPCRSP:341,1
   HTTP/1.1 200 OK
   Date: Thu, 11 Mar 2021 04:36:19 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 244
   Connection: keep-alive
   ETag: W/"f4-ZKlqfH53aEj3f4zb0kDtYvHD+XU"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3AHGcBwpqlDDUZhU16VzuQkfTMhWhA4W1T.%2Bgm1%2BezKGo2JnWxaB5yYDo%2FNh0NbnJzJjEnkMcrfdEI; Path=/; HttpOnly


   #XHTTPCRSP:243,1
   {"args":{"foo1":"bar1","foo2":"bar2"},"headers":{"x-forwarded-proto":"http","x-forwarded-port":"80","host":"postman-echo.com","x-amzn-trace-id":"Root=1-60499e43-67a96f1e18fec45b1db78c25"},"url":"http://postman-echo.com/get?foo1=bar1&foo2=bar2"
   #XHTTPCRSP:1,1
   }
   #XHTTPCRSP:0,0

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

The set command allows you to connect to and disconnect from an HTTP server.

Syntax
~~~~~~

::

   #XHTTPCRSP=<byte_received>,<state><CR><LF><response>

* The ``<byte_received>`` is an integer.
  It represents the length of a partially received HTTP response.
* The ``<state>`` value can assume one of the following values:

  * ``0`` - The entire HTTP response has been received.
  * ``1`` - There is more HTTP response data to come.

* The ``<response>`` is the raw data of the HTTP response, including headers and body.

Example
~~~~~~~

The following example sends a POST request to send data to the server with an optional header.

::

   AT#XHTTPCREQ="POST","/post","User-Agent: SLM/1.2.0
   Accept: */*
   Content-Type: application/x-www-form-urlencoded
   Content-Length: 20
   ",20

   OK

   #XHTTPCREQ: 1
   12345678901234567890

   OK

   #XHTTPCREQ: 0

   #XHTTPCRSP:342,1
   HTTP/1.1 200 OK
   Date: Thu, 11 Mar 2021 04:48:40 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 402
   Connection: keep-alive
   ETag: W/"192-M03zprKgFyLUNjiEI1RVdT9hTiQ"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3AnqNP2vvtUzX8Yjty19rZmvDI9MUKLa_V.rMf6q%2BKs%2FTL4cRPwCoHAbdlvEiu0sugyRF%2B3AErgBMs; Path=/; HttpOnly


   #XHTTPCRSP:366,1
   {"args":{},"data":"","files":{},"form":{"12345678901234567890":""},"headers":{"x-forwarded-proto":"http","x-forwarded-port":"80","host":"postman-echo.com","x-amzn-trace-id":"Root=1-6049a11f-187acd3f4ab56df7250ba74f","content-length":"20","user-agent":"SLM/1.2.0","accept":"*/*","content-type":"application/x-www-form-urlencoded"},"json":{"12345678901234567890":""},"
   #XHTTPCRSP:35,1
   url":"http://postman-echo.com/post"
   #XHTTPCRSP:1,1
   }
   #XHTTPCRSP:0,0
