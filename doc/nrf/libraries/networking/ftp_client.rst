.. _lib_ftp_client:

FTP client
##########

.. contents::
   :local:
   :depth: 2

The FTP client library can be used to download or upload FTP server files.

The file is downloaded in fragments of :c:enumerator:`NET_IPV4_MTU`.
The size of a file can be fetched by LIST command.

The FTP client library reports FTP control message and download data with two separate callback functions (:c:type:`ftp_client_callback_t` and :c:type:`ftp_client_callback_t`).
The library automatically sends KEEPALIVE message to the server through a timer if :kconfig:option:`CONFIG_FTP_CLIENT_KEEPALIVE_TIME` is not zero.
The KEEPALIVE message is sent periodically at the completion of the time interval indicated by the value of :kconfig:option:`CONFIG_FTP_CLIENT_KEEPALIVE_TIME`.

If there is no username or password provided, the library performs a login as an anonymous user.

Protocols
*********

The library is implemented in accordance with the `RFC959 File Transfer Protocol (FTP)`_ specification.

Limitations
***********

The library implements only a minimal set of commands as of now.
However, new command support can be easily added.

The library supports IPv4 protocol only.

Due to the differences in the implementation of FTP servers, the library might need customization to work with a specific server.

API documentation
*****************

| Header file: :file:`include/net/ftp_client.h`
| Source files: :file:`subsys/net/lib/ftp_client/src/`

.. doxygengroup:: ftp_client
   :project: nrf
   :members:
