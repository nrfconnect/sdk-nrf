.. _lib_download_client:

Download client
###############

The download client library provides functions to download resources from a remote server.
The resource could, for example, be a firmware image.
Currently, the only supported protocol for the download is HTTP.

The library is designed to download large objects like firmware images.
However, it does not impose any requirements on the object that is being downloaded, which means that you can use the library for any kind of object, not only firmware images.
There are also no constraints on the object if it is a firmware image.
The image can be of any form, for example, a delta image or a full image with many merged HEX files, and there is no revision management or specification of the flash address where the image is downloaded.

The size of the object to download is obtained from the server.
The object is then downloaded in fragments of a maximum fragment size (:option:`CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE`).
For example, if the size of the object to be downloaded is 60478 bytes and the maximum fragment size is 1024 bytes, the module downloads 60 fragments of the object; 59 fragments with a size of 1024 bytes and one fragment with a size of 62 bytes.
If any of the download requests fail, they can be repeated.

After every fragment download, a :cpp:enum:`download_client_evt` is returned, indicating if the download was successful (:cpp:member:`DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG`), failed (:cpp:member:`DOWNLOAD_CLIENT_EVT_ERROR`), or is completed (:cpp:member:`DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE`).

Make sure to configure :option:`CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE` in a way that suits your application.
A low value causes many download requests, which can result in too much protocol overhead, while a large value requires a lot of RAM.


Protocols
*********

The download protocol is determined from the download URI.
Currently, only HTTP is supported.

HTTP
====

For HTTP, the following requirements must be met:

* The address family is IPv4.
* TCP transport is used to communicate with the server.
* The application protocol to communicate with the server is HTTP 1.1.
* IETF RFC 7233 is supported by the HTTP Server.
* :option:`CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE` is configured so that it can contain the entire HTTP response.

HTTPS is not supported.

The library uses the "Range" header to request firmware fragments of size :option:`CONFIG_NRF_DOWNLOAD_MAX_FRAGMENT_SIZE`.
The firmware size is obtained from the "Content-Length" header in the response.
To request the server to keep the TCP connection after a partial content response, the "Connection: keep-alive" header is included in the request.
If the server response contains "Connection: close", the library automatically reconnects to the server and resumes the download.


API documentation
*****************

| Header file: :file:`include/download_client.h`
| Source files: :file:`subsys/net/lib/download_client/src/`

.. doxygengroup:: dl_client
   :project: nrf
   :members:
