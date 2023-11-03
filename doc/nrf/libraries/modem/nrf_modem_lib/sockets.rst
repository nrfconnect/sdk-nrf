.. _nrf_sockets:

Networking sockets
##################

.. contents::
   :local:
   :depth: 2


The Modem library offers an implementation of a subset of the POSIX networking sockets API.
This API allows you to reuse existing programming experience and port existing simple networking applications to nRF91 Series devices.
The implementation is namespaced (``nrf_``) to avoid conflicts with functions that might be part of libc or other POSIX compatibility libraries.

Following is a list of supported functions:

* :c:func:`nrf_socket`
* :c:func:`nrf_close`
* :c:func:`nrf_connect`
* :c:func:`nrf_bind`
* :c:func:`nrf_listen`
* :c:func:`nrf_send`
* :c:func:`nrf_sendto`
* :c:func:`nrf_recv`
* :c:func:`nrf_recvfrom`
* :c:func:`nrf_setsockopt`
* :c:func:`nrf_getsockopt`
* :c:func:`nrf_poll`
* :c:func:`nrf_getaddrinfo`
* :c:func:`nrf_freeaddrinfo`
* :c:func:`nrf_fcntl`
* :c:func:`nrf_getifaddrs`
* :c:func:`nrf_freeifaddrs`
* :c:func:`nrf_inet_pton`
* :c:func:`nrf_inet_ntop`

The library supports up to eight networking sockets.

Socket options
**************

The following table shows all socket options supported by the Modem library.

+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| Option level    | Option                          | Type                   | Operations | Description                                                                                |
+=================+=================================+========================+============+============================================================================================+
| NRF_SOL_SOCKET  | NRF_SO_ERROR                    | ``int``                | get        | Requests and clears pending error information on the socket.                               |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_REUSEADDR                | ``int``                | set        | Non-zero requests reuse of local addresses in :c:func:`nrf_bind` (protocol-specific).      |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_RCVTIMEO                 | ``struct nrf_timeval`` | get/set    | Timeout value for a socket receive operation.                                              |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_SNDTIMEO                 | ``struct nrf_timeval`` | get/set    | Timeout value for a socket send operation.                                                 |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_BINDTOPDN                | ``int``                | set        | Bind this socket to a specific PDN ID.                                                     |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_POLLCB                   | ``struct nrf_pollcb``  | set        | Set callbacks for poll() events on sockets.                                                |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_EXCEPTIONAL_DATA         | ``int``                | get/set    | Send data on socket as part of exceptional event.                                          |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_RAI_LAST                 | ``int``                | set        | Enter Radio Resource Control (RRC) idle immediately after the next send operation.         |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_RAI_ONE_RESP             | ``int``                | set        | Wait for one incoming packet after the next send operation, before entering RRC idle mode. |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_RAI_ONGOING              | ``int``                | set        | Keep RRC in connected mode after the next send operation (client).                         |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_RAI_WAIT_MORE            | ``int``                | set        | Keep RRC in connected mode after the next send operation (server).                         |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SOCKET  | NRF_SO_RAI_NO_DATA              | ``int``                | set        | Immediately release the RRC.                                                               |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_TAG_LIST             | ``nrf_sec_tag_t *``    | get/set    | Set/get the security tag associated with a socket.                                         |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_HOSTNAME             | ``char *``             | get/set    | Set/get the hostname to check against during TLS handshakes.                               |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_CIPHERSUITE_LIST     | ``nrf_sec_cipher_t *`` | get/set    | Set/get allowed cipher suite list.                                                         |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_PEER_VERIFY          | ``nrf_peer_verify_t``  | get/set    | Set/get Peer verification level.                                                           |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_ROLE                 | ``nrf_sec_role_t``     | get/set    | Set/get TLS role.                                                                          |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_SESSION_CACHE        | ``nrf_session_cache_t``| get/set    | Non-zero enables TLS session cache.                                                        |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_SESSION_CACHE_PURGE  | ``int``                | set        | Delete TLS session cache.                                                                  |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_DTLS_HANDSHAKE_TIMEO | ``int``                | get/set    | Set/get DTLS handshake timeout.                                                            |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_HANDSHAKE_STATUS     | ``int``                | get        | Get latest TLS/DTLS completed handshake procedure end status.                              |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_DTLS_CID             | ``int``                | get/set    | Set DTLS Connection ID setting.                                                            |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_DTLS_CID_STATUS      | ``int``                | get        | Get status of DTLS Connection ID.                                                          |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_DTLS_CONN_SAVE       | ``int``                | set        | Save DTLS connection.                                                                      |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_DTLS_CONN_LOAD       | ``int``                | set        | Load DTLS connection.                                                                      |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_SOL_SECURE  | NRF_SO_SEC_CIPHERSUITE_USED     | ``int``                | get        | Get TLS cipher suite used for the handshake.                                               |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_IPPROTO_ALL | NRF_SO_SILENCE_ALL              | ``int``                | get/set    | Non-zero disables ICMP echo replies on both IPv4 and IPv6.                                 |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_IPPROTO_IP  | NRF_SO_IP_ECHO_REPLY            | ``int``                | get/set    | Non-zero enables ICMP echo replies on IPv4.                                                |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_IPPROTO_IPV6| NRF_SO_IPV6_ECHO_REPLY          | ``int``                | get/set    | Non-zero enables ICMP echo replies on IPv6.                                                |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+
| NRF_IPPROTO_TCP | NRF_SO_TCP_SRV_SESSTIMEO        | ``int``                | get/set    | Non-zero enables TCP server session timeout after a configurable period of inactivity.     |
+-----------------+---------------------------------+------------------------+------------+--------------------------------------------------------------------------------------------+

The details for each socket are described as follows:

NRF_SO_ERROR
   When this option is specified, :c:func:`nrf_getsockopt` returns any pending errors on the socket and clears the error status.
   It returns a ``0`` value if there is no pending error.
   :c:macro:`NRF_SO_ERROR` might be used to check for asynchronous errors on connected connectionless-mode sockets or for other types of asynchronous errors.
   :c:macro:`NRF_SO_ERROR` has no default value.

NRF_SO_REUSEADDR
  Allow for the reuse of local addresses by using the :c:func:`nrf_bind` function.
  The default value for :c:macro:`NRF_SO_REUSEADDR` is ``off``, that is, reuse of local addresses is not permitted.

NRF_SO_RCVTIMEO
   Set a timeout value for the :c:func:`nrf_recv` and :c:func:`nrf_recvfrom` operations.
   This option accepts an :c:struct:`nrf_timeval` structure with a number of seconds and microseconds specifying the limit on how long to wait for an input operation to complete.
   If a receive operation has blocked for this much time without receiving additional data, it returns with a partial count, or ``errno`` is set to ``NRF_EAGAIN`` or ``NRF_EWOULDBLOCK`` if no data were received.
   The default for this option is the value ``0``, which indicates that a receive operation will not time out.

.. note::
   The minimum supported resolution is 1 millisecond.

NRF_SO_SNDTIMEO
   Set a timeout value for the :c:func:`nrf_connect`, :c:func:`nrf_send`, and :c:func:`nrf_sendto` operations.
   The option accepts an :c:struct:`nrf_timeval` structure with a number of seconds and microseconds specifying the limit on how long to wait for an output operation to complete.
   The default for this option is the value ``0``, which indicates that these operations will not time out, or use the maximum timeout available.

.. note::
   The minimum supported resolution is 1 millisecond.

NRF_SO_BINDTOPDN
   Bind this socket to a particular packet data network ID.
   The passed option is an integer specifying the PDN ID.
   If a socket is bound to an interface, only packets received from that particular interface are processed by the socket.

NRF_SO_POLLCB
   Set a callback for events occurring on this socket such as :c:macro:`NRF_POLLIN` and :c:macro:`NRF_POLLOUT`.
   The :c:member:`nrf_modem_pollcb.callback` function is invoked every time any of the events specified by the :c:member:`nrf_modem_pollcb.events` bitmask field occurs.
   In addition, the :c:macro:`NRF_POLLHUP` and :c:macro:`NRF_POLLERR` events will also trigger the callback, regardless of whether they are set in the :c:member:`nrf_modem_pollcb.events` bitmask field.
   The callback receives a :c:struct:`nrf_pollfd` structure, populated in the same way as it would be populated by the :c:func:`nrf_poll` function.
   If the :c:member:`nrf_modem_pollcb.oneshot` field is set to ``true``, the callback will be invoked only once, and it is automatically unset afterwards.

.. important::
   The callback is invoked in an interrupt service routine.

NRF_SO_EXCEPTIONAL_DATA
   Send data on the socket as part of an exceptional event.
   Exceptional events are described in the 3GPP Release 14 specification.
   The feature requires network support.

   Before using this socket option, the PDN associated with the socket must be configured to allow exceptional events by using the ``AT%EXCEPTIONALDATA`` AT command.
   For more information about the ``AT%EXCEPTIONALDATA`` AT command, see the `nRF91x1 AT Commands Reference Guide`_.

   The socket option is supported from modem firmware v2.0.0.

NRF_SO_RAI_LAST
   This is a Release assistance indication (RAI) socket option.
   Enter RRC idle mode after the next output operation on this socket is complete.

NRF_SO_RAI_ONE_RESP
   This is a Release assistance indication (RAI) socket option.
   After the next output operation is complete, wait for one more packet to be received from the network on this socket before entering RRC idle mode.

NRF_SO_RAI_ONGOING
   This is a Release assistance indication (RAI) socket option.
   Keep RRC in connected mode after the next output operation on this socket (client side).

NRF_SO_RAI_WAIT_MORE
   This is a Release assistance indication (RAI) socket option.
   Keep RRC in connected mode after the next output operation on this socket (server side).

NRF_SO_RAI_NO_DATA
   This is a Release assistance indication (RAI) socket option.
   Immediately enter RRC idle mode for this socket.
   Does not require a following output operation.

NRF_SO_SILENCE_ALL
   Disable ICMP echo replies on both IPv4 and IPv6.
   The option value is an integer, a ``1`` value disables echo replies.
   Default value is ``0`` (OFF).

NRF_SO_IP_ECHO_REPLY
   Enable ICMP echo replies on IPv4.
   The option value is an integer, a ``0`` value disables echo replies on IPv4.
   Default value is ``1`` (ON).

NRF_SO_IPV6_ECHO_REPLY
   Enable ICMP echo replies on IPv6.
   The option value is an integer, a ``0`` value disables echo replies on IPv6.
   Default value is ``1`` (ON).

NRF_SO_TCP_SRV_SESSTIMEO
   Configure the TCP server session inactivity timeout for a socket.
   The timeout value is specified in seconds.
   Allowed values for this option range from ``0`` to ``135``, inclusive.
   The default value is ``0`` (no timeout).

.. note::
   This option must be set on the listening socket, but it can be overridden on the accepting socket afterwards.

NRF_SO_SEC_TAG_LIST
   Set an array of security tags to use for credentials when connecting.
   The option length is the size in bytes of the array of security tags.
   Passing ``NULL`` as an option value and ``0`` as an option length removes all security tags associated with a socket.
   By default, no security tags are associated with a socket.
   The maximum number of security tags are given by the :c:macro:`NRF_SOCKET_TLS_MAX_SEC_TAG_LIST_SIZE` macro in :file:`nrf_socket.h`.

NRF_SO_SEC_HOSTNAME
   Set the hostname used for peer verification.
   The option value is a null-terminated string containing the host name to verify against.
   The option length is the size in bytes of the hostname.
   Passing ``NULL`` as an option value and ``0`` as an option length disables peer hostname verification.
   By default, peer hostname verification is disabled.

NRF_SO_SEC_CIPHERSUITE_LIST
   Select which cipher suites are allowed to be used during the TLS handshake.
   The cipher suites are identified by their IANA assigned values.
   By default, all supported cipher suites are allowed.
   For a complete list, see the :ref:`supported cipher_suites <nrf_supported_tls_cipher_suites>` API documentation or refer to the release notes of the modem firmware.
   For more information, see the release notes in the `nRF9160 modem firmware zip file`_ or `nRF91x1 LTE firmware zip file`_ depending on the SiP you are using.

NRF_SO_SEC_PEER_VERIFY
  Set the peer verification level.
  The following values are accepted:

   * :c:macro:`NRF_SO_SEC_PEER_VERIFY_NONE` - No peer verification
   * :c:macro:`NRF_SO_SEC_PEER_VERIFY_OPTIONAL` - Optional peer verification
   * :c:macro:`NRF_SO_SEC_PEER_VERIFY_REQUIRED` - Peer verification is required

   By default, peer verification is required.

NRF_SO_SEC_ROLE
   Set the role for the connection.
   The following values are accepted:

   * :c:macro:`NRF_SO_SEC_ROLE_CLIENT` - Client role
   * :c:macro:`NRF_SO_SEC_ROLE_SERVER` - Server role

   The default role is client.
   For TLS, the choice is implicit from the usage of ``listen()``, ``accept()`` and ``connect()``.

NRF_SO_SEC_SESSION_CACHE
   This option controls TLS session caching.
   The following values are accepted:

   * :c:macro:`NRF_SO_SEC_SESSION_CACHE_DISABLED` - Disable TLS session caching
   * :c:macro:`NRF_SO_SEC_SESSION_CACHE_ENABLED` - Enable TLS session caching

   By default, TLS session caching is disabled.

NRF_SO_SEC_SESSION_CACHE_PURGE
   Delete TLS session cache.
   This option is write-only.

NRF_SO_SEC_DTLS_HANDSHAKE_TIMEO
   Set the DTLS handshake timeout.
   The socket option is supported from modem firmware version 1.3.x.
   The following values are accepted:

   * 0 -  No timeout
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_1S` - 1 second
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_3S` - 3 seconds
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_7S` - 7 seconds
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_15S` - 15 seconds
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_31S` - 31 seconds
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_63S` - 63 seconds
   * :c:macro:`NRF_SO_SEC_DTLS_HANDSHAKE_TIMEOUT_123S` - 123 seconds

   The default is no timeout.

NRF_SO_SEC_HANDSHAKE_STATUS
   Get the latest TLS/DTLS completed handshake procedure end status.
   This option is read-only.
   The socket option is supported from modem firmware v2.x.x.
   The following values are expected:

   * :c:macro:`NRF_SO_SEC_HANDSHAKE_STATUS_FULL` - TLS/DTLS attach/negotiation procedure was made with a full handshake, and session cache data was not used or it was not accepted by the server.
   * :c:macro:`NRF_SO_SEC_HANDSHAKE_STATUS_CACHED` - The latest TLS/DTLS attach negotiation was completed successfully with cached session data.

   The default is a full handshake.

.. _dtls_cid_setting:

NRF_SO_SEC_DTLS_CID
   Set DTLS Connection ID setting.
   This socket option decides whether the modem will request or accept a DTLS connection ID when performing the server handshake.
   The socket option is supported from modem firmware v1.3.x, where x is greater than or equal to 5, and v2.x.x.
   The following values are accepted:

   * :c:macro:`NRF_SO_SEC_DTLS_CID_DISABLED` - The connection ID extension is not included in the client hello, so the DTLS connection ID is not used.
   * :c:macro:`NRF_SO_SEC_DTLS_CID_SUPPORTED` - The connection ID extension with a zero-length CID is included in the client hello, so the modem will accept a DTLS connection ID from the server.
   * :c:macro:`NRF_SO_SEC_DTLS_CID_ENABLED` - The connection ID extension with a valid CID is included in the client hello, so the modem will request DTLS connection ID support.

   The default is disabled.

.. _dtls_cid_status:

NRF_SO_SEC_DTLS_CID_STATUS
   Get the status of DTLS connection ID.
   The status tells whether the connection ID is used in the current connection and in which direction it is used.
   This option is read-only.
   The socket option is supported from modem firmware v1.3.x, where x is greater than or equal to 5, and v2.x.x.
   The following values are expected:

   * :c:macro:`NRF_SO_SEC_DTLS_CID_STATUS_DISABLED` - The DTLS connection ID is not included in DTLS records sent to and from the modem.
     This status is returned before the DTLS handshake is complete.
   * :c:macro:`NRF_SO_SEC_DTLS_CID_STATUS_DOWNLINK` - The DTLS connection ID is included only in DTLS records sent to the modem.
   * :c:macro:`NRF_SO_SEC_DTLS_CID_STATUS_UPLINK` - The DTLS connection ID is included only in DTLS records sent from the modem.
   * :c:macro:`NRF_SO_SEC_DTLS_CID_STATUS_BIDIRECTIONAL` - The DTLS connection ID is included in DTLS records sent to and from the modem.

NRF_SO_SEC_DTLS_CONN_SAVE
   Save DTLS connection.
   This socket option can be used to pause a session that is not frequently used by the application.
   Saving the session will free memory in the modem, so the memory can be used for other connections.
   If the socket is closed, the saved DTLS data is cleaned and the connection with the server is lost.

   This option is write-only.
   This option require a DTLS v1.2 connection with renegotiation disabled.
   The socket option is supported from modem firmware v1.3.x, where x is greater than or equal to 5, and v2.x.x.

   Once the DTLS context is saved, the socket can't be used before the DTLS context is loaded with :c:macro:`NRF_SO_SEC_DTLS_CONN_LOAD`.

   This option fails with nrf_errno ``NRF_EAGAIN`` if an error happened during serialization of the SSL context.
   This can occur, for instance, when the modem cannot allocate enough memory or if the socket is busy sending or receiving data.
   In this case, the SSL context is still present in the socket, so data sending is still possible.
   The option fails with nrf_errno ``NRF_EINVAL`` if the socket option is not supported with the current configuration, for instance because the DTLS handshake is not completed,
   the connection is not an DTLS v1.2 connection with renegotiation disabled, or the connection does not use an AEAD cipher suite (AES-CCM or AES-GCM).
   The option fails with nrf_errno ``NRF_ENOMEM`` if the amount of saved connections exceeds four.

NRF_SO_SEC_DTLS_CONN_LOAD
   Load DTLS connection.
   This option is write-only.
   The socket option is supported from modem firmware v1.3.x, where x is greater than or equal to 5, and v2.x.x.

   This option fails with nrf_errno ``NRF_EAGAIN`` if an error happened during deserialization of the SSL context.
   This can occur, for instance, when the modem cannot allocate enough memory or the connection is not saved.
   The option fails with nrf_errno ``NRF_EINVAL`` if the socket option is not supported with the current configuration.

NRF_SO_SEC_CIPHERSUITE_USED
   Get chosen TLS cipher suite.
   This option is read-only.
   The socket option is supported from modem firmware v2.x.x.


Packet data networks
********************

The Modem library supports selecting which Packet Data Network (PDN) to use on a network socket and for DNS queries.
The configuration of Packet Data Protocol (PDP) contexts, and the activation of PDN connections are not handled by the Modem library.
To configure PDP contexts and activate PDN connections, the application must use the packet domain AT commands.

When performing network operations on any PDN, the application ensures that the PDN connection is available.
For more information about how to configure PDP contexts, activate PDN connections, and determine their state, see the `Packet domain commands`_ section in the nRF9160 AT Commands Reference Guide or the same section in the `nRF91x1 AT Commands Reference Guide`_ depending on the SiP you are using.


Configuring a socket to use a PDN
=================================

The application can select which PDN to use on a specific socket by using the :c:func:`nrf_setsockopt` function with the :c:macro:`NRF_SO_BINDTOPDN` socket option and specifying the PDN ID as an integer.

The following code shows how to create an IPv4 TCP stream socket and configure it to use the PDN with ID 1:

.. code-block:: c

   int pdn_id = 1;

   fd = nrf_socket(NRF_AF_INET, NRF_SOCK_STREAM, NRF_IPPROTO_TCP);
   nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_BINDTOPDN, &pdn_id, sizeof(pdn_id));


Routing a DNS query on a PDN
============================

The application can route a DNS query using the :c:func:`nrf_getaddrinfo` function to a specific PDN.
This can be done by setting the ``NRF_AI_PDNSERV`` flag in the ``ai_flags`` field of the ``nrf_addrinfo`` input structure, and specifying the PDN ID as a string prefixed by ``pdn``, in the ``service`` argument to the :c:func:`nrf_getaddrinfo` function call.

The following code shows how to route a DNS query to the PDN with ID 1:

.. code-block:: c

   struct nrf_addrinfo hints = {
     .ai_flags = NRF_AI_PDNSERV, /* flag to specify PDN ID */
   }

   nrf_getaddrinfo("example.com", "pdn1", &hints, &result);

Handling PDN errors on sockets
==============================

During operation, an active PDN connection may be deactivated due to loss of connectivity or other reasons.
When a socket operation is attempted on a socket that no longer has an active PDN connection, the operation will return ``-1`` and set ``errno`` to ``NRF_ENETDOWN``.
If the socket is being polled, the :c:func:`nrf_poll` function will set the ``POLLERR`` flag and set the socket error to ``NRF_ENETDOWN``.
The socket error can be retrieved using the :c:macro:`NRF_SO_ERROR` socket option.

When the ``NRF_ENETDOWN`` error is detected, the socket is no longer usable and must be closed by the application.
The application is responsible for detecting when the PDN connection is activated again, before re-creating the socket and attempting the failed operation again.

The packet domain AT commands can be used to manage packet data networks.
Alternatively, the :ref:`pdn_readme` library in |NCS| can be used to receive events on PDN connectivity and manage packet data networks.

TLS/DTLS configuration
**********************

The IP stack in the nRF91 Series modem firmware has TLS and DTLS support.

.. _security_tags:

Security tags
=============

To use the cryptographic functions in the modem, the application must provision the security credentials to the modem.
To be able to provision credentials, the modem must be in offline mode.
The credentials are provisioned through AT commands.
For more information, See `Credential storage management %CMNG`_ section in the nRF9160 AT Commands Reference Guide or the same section in the `nRF91x1 AT Commands Reference Guide`_ depending on the SiP you are using.
If you are using the |NCS| to build your application, you can use the :ref:`nrf:modem_key_mgmt` library to manage credentials.
If you prefer a graphical tool, use `Cellular Monitor`_ instead.
To manage credentials with Cellular Monitor, your device must be running an |NCS| application.

The following figure shows how security tags are provisioned using AT commands:

.. figure:: images/security_tags.svg
   :alt: Provisioning credentials with a security tag

   Provisioning credentials with security tag ``1``

As you see, each set of credentials is identified by a security tag (``sec_tag``), which is referenced when a DTLS/TLS socket is created.

The security tag must be attached to a socket using the :c:func:`nrf_setsockopt` function before connecting (through TCP) or transferring data (through UDP).
The following code snippet shows how to set up strict peer verification for a socket and configure the socket to use the security tag ``sec_tag``:

.. code-block:: c

	/* Let 'fd' be a valid UDP or TCP socket descriptor. */

	int err;
	int verify;
	sec_tag_t sec_tag_list[] = { sec_tag };

	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = nrf_setsockopt(fd, NRF_SOL_SECURE, NRF_SO_SEC_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		/* Failed to set up peer verification. */
		return -1;
	}

	err = nrf_setsockopt(fd, NRF_SOL_SECURE, NRF_SO_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list));
	if (err) {
		/* Failed to set up socket security tag. */
		return -1;
	}


It is possible to use multiple security tags.
If a list is provided, one of the matching tags is used when handshaking.
For example, you could define the security tag list as follows::

   sec_tag_t sec_tag_list[] = { 4, 5 };

In this case, either security tag 4 or security tag 5 can be used for operations on the socket.

.. figure:: images/security_tags2.svg
   :alt: Using multiple security tags

   Using multiple security tags

Supported cipher suites
=======================

See the `nRF9160 modem TLS cipher suites`_ or `nRF91x1 modem TLS cipher suites`_ summary page, depending on the SiP you are using, for a full list of TLS/DTLS cipher suites supported by the modem.

Each cipher suite is recognized by an official identification number, which is registered at `IANA`_.
You can narrow down the set of cipher suites that is used for a specific TLS/DTLS connection with :c:func:`nrf_setsockopt`.
For example, see the following code:

.. code-block:: c

	/* TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA */
	nrf_sec_cipher_t cipher_list[] = { 0xC014 };

	err = nrf_setsockopt(fd, NRF_SOL_SECURE, NRF_SO_SEC_CIPHERSUITE_LIST, cipher_list, sizeof(cipher_list));
	if (err) {
		/* Failed to set up cipher suite list. */
		return -1;
	}

Note that as in the case of other TLS/DTLS socket options, you must do this configuration before connecting to the server.
