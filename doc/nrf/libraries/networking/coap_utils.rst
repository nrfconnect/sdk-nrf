.. _coap_utils_readme:

CoAP utils
##########

.. contents::
   :local:
   :depth: 2

The CoAP utils library is a simple module that enables communication with devices that support the CoAP protocol.
It allows sending and receiving non-confirmable CoAP requests.

Overview
********

The library uses :ref:`CoAP <zephyr:coap_sock_interface>` and :ref:`BSD socket API <bsd_sockets_interface>`.

After calling :c:func:`coap_init`, the library opens a socket for receiving UDP packets for IPv4 or IPv6 connections, depending on the ``ip_family`` parameter.
At this point, you can start sending CoAP non-confirmable requests, to which you will receive answers depending on the server configuration.

Limitations
***********

Currently, the library only supports the User Datagram Protocol (UDP) protocol.

Configuration
*************

To enable the CoAP utils library, set the :kconfig:option:`CONFIG_COAP` and :kconfig:option:`CONFIG_COAP_UTILS` Kconfig options.

API documentation
*****************

| Header file: :file:`include/net/coap_utils.h`
| Source files: :file:`subsys/net/lib/coap_utils/`

.. doxygengroup:: coap_utils
   :project: nrf
   :members:
