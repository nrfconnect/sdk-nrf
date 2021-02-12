.. _SLM_AT_ICMP:

ICMP AT commands
****************

.. contents::
   :local:
   :depth: 2

The following commands list contains ICMP related AT commands.

ICMP echo request #XPING
========================

The ``#XPING`` command sends an ICMP Echo Request, also known as *Ping*.

Set command
-----------

The set command allows you to send an ICMP Echo Request.

Syntax
~~~~~~

::

   #XPING=<url>,<length>,<timeout>[,<count>[,<interval>]]

* The ``<url>`` parameter is a string.
  It represents the hostname or the IPv4 address of the target host.
* The ``<length>`` parameter is an integer.
  It represents the length of the buffer size.
  Its maximum value is ``548``.
* The ``<timeout>`` parameter is an integer.
  It represents the time to wait for each reply, in milliseconds.
* The ``<count>`` parameter is an integer.
  It represents the number of echo requests to send.
  Its default value is ``1``.
* The ``<interval>`` parameter is an integer.
  It represents the time to wait for sending the next echo request, in milliseconds.
  Its default value is ``1000``.

Response syntax
~~~~~~~~~~~~~~~

This is the response syntax when an echo reply is received:

::

   #XPING: <response time>

The ``<response time>`` value is a *float*.
It represents the elapsed time, in seconds, between the echo requests and the echo replies.

This is the response syntax when an echo reply is not received:

::

   #XPING: "timeout"

Example
~~~~~~~

::

   AT#XPING="5.189.130.26",45,5000,5,1000
   #XPING: 0.386
   #XPING: 0.341
   #XPING: 0.353
   #XPING: 0.313
   #XPING: 0.313
   #XPING: "average 0.341"
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
