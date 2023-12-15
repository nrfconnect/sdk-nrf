.. _SLM_AT_ICMP:

ICMP AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes ICMP-related AT commands.

ICMP echo request #XPING
========================

The ``#XPING`` command sends an ICMP Echo Request, also known as *Ping*.

Set command
-----------

The set command allows you to send an ICMP Echo Request.

Syntax
~~~~~~

::

   #XPING=<url>,<length>,<timeout>[,<count>[,<interval>[,<pdn>]]]

* The ``<url>`` parameter is a string.
  It represents the hostname, the IPv4, or the IPv6 address of the target host.
* The ``<length>`` parameter is an integer.
  It represents the length of the buffer size.
* The ``<timeout>`` parameter is an integer.
  It represents the time to wait for each reply, in milliseconds.
* The ``<count>`` parameter is an integer.
  It represents the number of echo requests to send.
  Its default value is ``1``.
* The ``<interval>`` parameter is an integer.
  It represents the time to wait for sending the next echo request, in milliseconds.
  Its default value is ``1000``.
* The ``<pdn>`` parameter is an integer.
  It represents ``cid`` in the ``+CGDCONT`` command.
  Its default value is ``0``.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XPING: <response time> seconds

The ``<response time>`` value is a *float*.
It represents the elapsed time, in seconds, between the echo requests and the echo replies.

Example
~~~~~~~

::

   AT#XPING="5.189.130.26",45,5000,5,1000
   OK
   #XPING: 0.386 seconds
   #XPING: 0.341 seconds
   #XPING: 0.353 seconds
   #XPING: 0.313 seconds
   #XPING: 0.313 seconds
   #XPING: average 0.341 seconds
   AT#XGETADDRINFO="ipv6.google.com"
   #XGETADDRINFO: "2404:6800:4006:80e::200e"
   OK
   AT#XPING="ipv6.google.com",45,5000,5,1000
   OK
   #XPING: 0.286 seconds
   #XPING: 0.077 seconds
   #XPING: 0.110 seconds
   #XPING: 0.037 seconds
   #XPING: 0.106 seconds
   #XPING: average 0.123 seconds
   AT#XPING="5.189.130.26",45,5000,5,1000,1
   OK
   #XPING: 1.612 seconds
   #XPING: 0.349 seconds
   #XPING: 0.334 seconds
   #XPING: 0.278 seconds
   #XPING: 0.278 seconds
   #XPING: average 0.570 seconds

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
