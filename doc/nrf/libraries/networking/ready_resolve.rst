.. _lib_ready_resolve:

Ready resolver
##############

.. contents::
   :local:
   :depth: 2

The ready resolver library provides dual-stack, connectivity-aware host name resolution.
It resolves a host name only for the address families that are up, and iterates over the resulting addresses.

Overview
********

The library resolves a host name with the ``getaddrinfo()`` function and returns the resulting addresses one at a time.
The application connects to each address, and can set socket options, such as TLS credentials, before connecting.

The :c:func:`ready_resolve` function resolves the host for the address families that are currently up.
If only one family is up, only that family is queried.
A numeric address is used as is, without DNS.
The function returns immediately when the network is not ready, with one of the following values:

* ``0`` - Success.
* ``-ENETDOWN`` - No wanted address family is up, or no DNS server is available for a host name.
* ``-EAGAIN`` - Temporary DNS failure.
* ``-EHOSTUNREACH`` - The host could not be resolved to a usable address.

The :c:func:`ready_resolve_next` function returns the resolved addresses in resolver order, skipping addresses of families that have gone down.
It returns ``-ENOENT`` when all addresses have been returned.
The resolved addresses are held until the :c:func:`ready_resolve_free` function is called or the host is resolved again.

Connecting
==========

The application makes rounds of connection attempts, and uses the :c:func:`ready_resolve_wait_change` function to wait between rounds until connectivity is gained or a retry delay elapses:

.. code-block:: c

   const struct zsock_addrinfo hints = { .ai_socktype = NET_SOCK_STREAM };
   struct ready_resolve host = {0};
   const struct net_sockaddr *addr;
   net_socklen_t addrlen;
   int fd = -1;

   while (fd < 0) {
           int err = ready_resolve(&host, "example.com", NULL, &hints, 443);

           while (err == 0 && fd < 0 && ready_resolve_next(&host, &addr, &addrlen) == 0) {
                   fd = server_connect(addr, addrlen);
           }
           if (fd < 0) {
                   ready_resolve_wait_change(&host, K_SECONDS(10));
           }
   }
   ready_resolve_free(&host);

The application detects failed connections, for example through socket errors or request timeouts.
It then closes the socket and connects to the next address returned by the :c:func:`ready_resolve_next` function, or resolves the host again.

Readiness tracking
==================

The library tracks connectivity in one of the following ways, selected automatically:

* Modem polling - On nRF91 Series devices without the modem network interface, the library queries the IP addresses of the default PDN with the ``AT+CGPADDR`` command.
  The :c:func:`ready_resolve_wait_change` function polls the modem.
* Connection manager events - On all other devices, the library tracks IPv4, IPv6, and DNS readiness through the :ref:`zephyr:conn_mgr_overview`, which it enables.

Configuration
*************

To use the library, enable the :kconfig:option:`CONFIG_READY_RESOLVE` Kconfig option.

With modem polling, you can set the poll interval using the :kconfig:option:`CONFIG_READY_RESOLVE_MODEM_POLL_INTERVAL_MS` Kconfig option.

Limitations
***********

A context must be zero-initialized before first use and used from a single thread.

Only the default PDN is tracked.
When ``AI_PDNSERV`` is set in the hints to resolve on another PDN, readiness is not checked, and the :c:func:`ready_resolve_wait_change` function waits for the full timeout.
Contexts are independent of each other.

API documentation
*****************

| Header file: :file:`include/net/ready_resolve.h`
| Source files: :file:`subsys/net/lib/ready_resolve`

.. doxygengroup:: ready_resolve
