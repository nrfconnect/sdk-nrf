.. _lib_download_client:

Download client
###############

The download client library can be used to download files from an HTTP or
HTTPS server. It supports the IPv4 and IPv6 protocols.

The file is downloaded in fragments of configurable size
(:option:`CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE`), that are returned to the
application via events (:cpp:member:`DOWNLOAD_CLIENT_EVT_FRAGMENT`).

The library can detect the size of the file being downloaded and sends an
event (:cpp:member:`DOWNLOAD_CLIENT_EVT_DONE`) to the application when the
download has completed.

The library can detect when the server has closed the connection and returns
an event to the application with an appropriate error code when that happens.
The application can then resume the download by calling the
:cpp:func:`download_client_connect` and :cpp:func:`download_client_start`
functions again.

The download happens in a separate thread, which can be paused and resumed.

Make sure to configure :option:`CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE` in a way
that suits your application. A small fragment size results in more download
requests, and thus a higher protocol overhead, while large fragments require
more RAM.


Protocols
*********

The library supports HTTP and HTTPS (TLS 1.2) over IPv4 and IPv6.


HTTP
====

For HTTP, the following requirements must be met:

* The application protocol to communicate with the server is HTTP 1.1.
* IETF RFC 7233 is supported by the HTTP Server.
* :option:`CONFIG_NRF_DOWNLOAD_MAX_RESPONSE_SIZE` is configured so that it
  can contain the entire HTTP response.

HTTPS
=====

The library uses TLS version 1.2.
When using HTTPS, the application must provision the TLS credentials and pass
the security tag to the library when calling
:cpp:func:`download_client_connect`.


API documentation
*****************

| Header file: :file:`include/download_client.h`
| Source files: :file:`subsys/net/lib/download_client/src/`

.. doxygengroup:: dl_client
   :project: nrf
   :members:
