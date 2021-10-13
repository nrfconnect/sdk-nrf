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
  * ``1`` - Connect to the HTTP server for IP protocol family version 4
  * ``2`` - Connect to the HTTP server for IP protocol family version 6

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

   AT#XHTTPCREQ=<method>,<resource>,<headers>[,<payload_length>]

* The ``<method>`` is a string.
  It represents the request method string.
* The ``<resource>`` is a string.
  It represents the target resource to apply the request.
* The ``<headers>`` is a string.
  It represents the header field of the request.
  Each header field should end with ``<CR><LF>``.
  Any occurrence of "\\r\\n" (4 bytes) inside is replaced by ``<CR><LF>`` (2 bytes).
* The ``<payload_length>`` is an integer.
  It represents the length of the payload.
  If ``payload_length`` is greater than ``0``, the SLM will enter data mode and expect the upcoming UART input data as payload.
  The SLM will then send the payload to the HTTP server until the ``payload_length`` bytes are sent.
  To abort sending the payload, terminate data mode by sending the terminator string defined in :kconfig:`CONFIG_SLM_DATAMODE_TERMINATOR`.
  The default pattern string is "+++". Keep in mind that UART silence as configured in :kconfig:`CONFIG_SLM_DATAMODE_SILENCE` is required before and after the pattern string.

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

   #XHTTPCRSP:341,0
   HTTP/1.1 200 OK
   Date: Thu, 11 Mar 2021 04:36:19 GMT
   Content-Type: application/json; charset=utf-8
   Content-Length: 244
   Connection: keep-alive
   ETag: W/"f4-ZKlqfH53aEj3f4zb0kDtYvHD+XU"
   Vary: Accept-Encoding
   set-cookie: sails.sid=s%3AHGcBwpqlDDUZhU16VzuQkfTMhWhA4W1T.%2Bgm1%2BezKGo2JnWxaB5yYDo%2FNh0NbnJzJjEnkMcrfdEI; Path=/; HttpOnly


   #XHTTPCRSP:243,0
   {"args":{"foo1":"bar1","foo2":"bar2"},"headers":{"x-forwarded-proto":"http","x-forwarded-port":"80","host":"postman-echo.com","x-amzn-trace-id":"Root=1-60499e43-67a96f1e18fec45b1db78c25"},"url":"http://postman-echo.com/get?foo1=bar1&foo2=bar2"
   #XHTTPCRSP:1,0
   }
   #XHTTPCRSP:0,1

The following example sends a GET request with some headers that are delimited by "\\r\\n".

::

   AT#XHTTPCREQ="GET","/instruments/150771004/temperature-values/?page=1 HTTP/1.1","Host: demo.abc.com\r\nAccept: application/vnd.api+json\r\nAuthorization: Basic ZGVtbzpkZxxx\r\n"

   OK

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

   #XHTTPCRSP=<byte_received>,<state><CR><LF><response>

* The ``<byte_received>`` is an integer.
  It represents the length of a partially received HTTP response.
* The ``<state>`` value can assume one of the following values:

  * ``0`` - There is more HTTP response data to come.
  * ``1`` - The entire HTTP response has been received.

* The ``<response>`` is the raw data of the HTTP response, including headers and body.

Example
~~~~~~~

The following example sends a PUT request to send data in JSON format to the server, with an optional header.

::

   AT#XHTTPCCON=1,"example.com",80
   #XHTTPCCON: 1

   OK
   AT#XHTTPCREQ="PUT","/iot/v1/device/12345678901","User-Agent: curl/7.58.0
   accept: */*
   CK: DEADBEEFDEADBEEFDE
   Content-Type: application/json
   Content-Length: 224
   ",224
   OK

   #XHTTPCREQ: 1
   {"id":"123456789","name":"iamchanged","desc":"My Hygrometer","type":"general","uri":"http://a.b.c.d/hygrometer","lat":24.95,"lon":121.16,"attributes":[{"key":"label","value":"thermometer"},{"key":"region","value":"Taiwan"}]}
   OK

   #XHTTPCREQ: 0
   #XHTTPCRSP:408,0
   HTTP/1.1 200
   Server: nginx/1.17.3
   Date: Wed, 17 Mar 2021 08:43:56 GMT
   Content-Type: application/json;charset=UTF-8
   Transfer-Encoding: chunked
   Connection: keep-alive
   X-Application-Context: iotapi:pob:80
   Vary: Origin
   X-Content-Type-Options: nosniff
   X-XSS-Protection: 1; mode=block
   Cache-Control: no-cache, no-store, max-age=0, must-revalidate
   Pragma: no-cache
   Expires: 0
   X-Frame-Options: DENY


   #XHTTPCRSP:22,0
   {"id":"12345678901"}

   #XHTTPCRSP:0,1
