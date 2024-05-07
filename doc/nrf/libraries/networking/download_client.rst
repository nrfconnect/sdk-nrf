.. _lib_download_client:

Download client
###############

.. contents::
   :local:
   :depth: 2

The download client library can be used to download files from an HTTP or a CoAP server.

Overview
********

The download is carried out in a separate thread, and the application receives events such as :c:enumerator:`DOWNLOAD_CLIENT_EVT_FRAGMENT` that contain the data fragments as the download progresses.
The fragment size can be configured independently for HTTP and CoAP (block-wise transfer).
When the download completes, the library sends the :c:enumerator:`DOWNLOAD_CLIENT_EVT_DONE` event to the application.

Protocols
=========

The library supports HTTP, HTTPS (TLS 1.2), CoAP, and CoAPS (DTLS 1.2) over IPv4 and IPv6.

.. _download_client_https:

HTTP and HTTPS (TLS 1.2)
------------------------

In the case of download using HTTP, the library sends only one HTTP request to the server and receives only one HTTP response.

In the case of download using HTTPS, it is carried out through `Content-Range requests (IETF RFC 7233)`_ due to memory constraints that limit the maximum HTTPS message size to 2 kilobytes.
The library thus sends and receives as many requests and responses as the number of fragments that constitutes the download.
For example, to download a file of size 47 kilobytes file with a fragment size of 2 kilobytes, a total of 24 HTTP GET requests are sent.
It is therefore recommended to use the largest fragment size to minimize the network usage.

CoAP and CoAPS (DTLS 1.2)
-------------------------

When downloading from a CoAP server, the library uses the CoAP block-wise transfer.

Configuration
*************

The configuration of the library depends on the protocol you are using.

Configuring HTTP and HTTPS (TLS 1.2)
====================================

Set the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_BUF_SIZE` and :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE` Kconfig options, so that the buffer is large enough to accommodate the entire HTTP header of the request and the response.

Moreover, the application must provision the TLS credentials and pass the security tag to the library when using HTTPS and calling the :c:func:`download_client_set_host` function.
To provision a TLS certificate to the modem, use :c:func:`modem_key_mgmt_write` and other :ref:`modem_key_mgmt` APIs.

Configuring CoAP and CoAPS (DTLS 1.2)
=====================================

Make sure to configure the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_BUF_SIZE` and :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE` Kconfig options, so that the buffer is large enough to accommodate the entire CoAP header and the CoAP block.

The application must provision the TLS credentials and pass the security tag to the library when using CoAPS and calling :c:func:`download_client_set_host`.

When you have modem firmware v1.3.5 or newer, you can use the :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_CID` Kconfig option to enable the DTLS Connection Identifier feature in this library.

Limitations
***********

The library requires the host server to provide a Content-Range field in the HTTP GET header when using HTTPS.
If this header field is missing, the library logs the following error::

   <err> download_client: Server did not send "Content-Range" in response

It is not possible to use a CoAP block size of 1024 bytes, due to internal limitations.

API documentation
*****************

| Header file: :file:`include/download_client.h`
| Source files: :file:`subsys/net/lib/download_client/src/`

.. doxygengroup:: dl_client
   :project: nrf
   :members:
