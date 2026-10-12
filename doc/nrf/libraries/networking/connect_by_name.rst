.. _lib_connect_by_name:

Host connection helper
######################

.. contents::
   :local:
   :depth: 2

The host connection helper library resolves a host name and connects to it, handling dual-stack connectivity changes.

Overview
********

The library resolves the host for the address families that are currently up, and tries the resolved addresses in resolver order until one connects.
Each address is passed to a connection callback provided by the application, which creates, configures, and connects the socket.
This lets the application set socket options, such as TLS credentials, before connecting.

The host is resolved again when the set of address families that are up changes, or when all addresses have been tried.
A numeric address is used as is, without DNS.

Connecting
==========

The :c:func:`connect_by_name` function makes one round of attempts and does not wait for connectivity.
It returns the socket descriptor from the connection callback on success, or one of the following errors:

* ``-ENETDOWN`` - No wanted address family is up, or no DNS server is available for a host name.
* ``-EHOSTUNREACH`` - The host could not be resolved to a usable address.
* Other negative error code - The last error returned by the connection callback.

The application retries, using the :c:func:`connect_by_name_wait_change` function to wait until connectivity changes or a retry delay elapses:

.. code-block:: c

   static int connect_cb(const struct net_sockaddr *addr, net_socklen_t addrlen, void *user_data)
   {
           int fd = zsock_socket(addr->sa_family, NET_SOCK_STREAM, NET_IPPROTO_TCP);

           if (fd < 0) {
                   return -errno;
           }
           if (zsock_connect(fd, addr, addrlen) < 0) {
                   int err = -errno;

                   zsock_close(fd);
                   return err;
           }
           return fd;
   }

   static const struct connect_by_name_cfg cfg = {
           .host = "example.com",
           .port = 80,
           .socktype = NET_SOCK_STREAM,
           .family = NET_AF_UNSPEC,
           .connect_cb = connect_cb,
   };
   static struct connect_by_name ctx;

   connect_by_name_init(&ctx, &cfg);
   while ((fd = connect_by_name(&ctx)) < 0) {
           connect_by_name_wait_change(&ctx, K_SECONDS(10));
   }

If the connection turns out to be unusable after connecting, for example because of a request timeout, call the :c:func:`connect_by_name_failed` function and close the socket.
The next call to :c:func:`connect_by_name` then tries the next address.
Use the :c:func:`connect_by_name_get_peer` function to get the address of the current connection.

Connectivity loss
=================

If the application sets the ``lost_cb`` callback in the :c:struct:`connect_by_name_cfg` structure, the library calls it when the address family of the current connection goes down.
The callback is called from the network management thread and must not block.
A typical use is to abort the connection so that the application reconnects over another address family.

Readiness tracking
==================

The library supports the following ways of tracking connectivity:

* Connection manager events (:kconfig:option:`CONFIG_CONNECT_BY_NAME_READINESS`) - The library tracks IPv4, IPv6, and DNS readiness through the :ref:`zephyr:conn_mgr_overview`.
  This is the default when the connection manager is enabled, except on nRF91 Series devices without the modem network interface.
* Modem polling (:kconfig:option:`CONFIG_CONNECT_BY_NAME_MODEM_READINESS`) - On nRF91 Series devices without the modem network interface, the library queries the IP addresses of the default PDN with the ``AT+CGPADDR`` command.
  The :c:func:`connect_by_name_wait_change` function polls the modem.
  The connectivity loss callback is not supported in this mode.
* None - The network is assumed ready, and the :c:func:`connect_by_name_wait_change` function waits for the full timeout.

Configuration
*************

To use the library, enable the :kconfig:option:`CONFIG_CONNECT_BY_NAME` Kconfig option.

You can also configure the following options:

* :kconfig:option:`CONFIG_CONNECT_BY_NAME_READINESS`
* :kconfig:option:`CONFIG_CONNECT_BY_NAME_MODEM_READINESS`
* :kconfig:option:`CONFIG_CONNECT_BY_NAME_MODEM_POLL_INTERVAL_MS`
* :kconfig:option:`CONFIG_CONNECT_BY_NAME_MAX_ADDRS`

Limitations
***********

A context must be used from a single thread.
Contexts are independent of each other.

API documentation
*****************

| Header file: :file:`include/net/connect_by_name.h`
| Source files: :file:`subsys/net/lib/connect_by_name`

.. doxygengroup:: connect_by_name
