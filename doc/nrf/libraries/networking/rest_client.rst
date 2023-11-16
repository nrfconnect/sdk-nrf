.. _lib_rest_client:

REST client
###########

.. contents::
   :local:
   :depth: 2

The REST client library provides means for performing REST requests.

Overview
********

This library creates a socket with TLS when requested.
It uses Zephyr's HTTP client library to send HTTP requests and receive HTTP responses.
When using this library, the :c:struct:`rest_client_req_context` structure is populated and
passed to the :c:func:`rest_client_request` function together with the :c:struct:`rest_client_resp_context` structure,
which will contain the response data.

Configuration
*************

To use the REST client library, enable the :kconfig:option:`CONFIG_REST_CLIENT` Kconfig option.

You can configure the following options to adjust the behavior of the library:

*  :kconfig:option:`CONFIG_REST_CLIENT_REQUEST_TIMEOUT`
*  :kconfig:option:`CONFIG_REST_CLIENT_SCKT_SEND_TIMEOUT`
*  :kconfig:option:`CONFIG_REST_CLIENT_SCKT_RECV_TIMEOUT`
*  :kconfig:option:`CONFIG_REST_CLIENT_SCKT_TLS_SESSION_CACHE_IN_USE`

Limitations
***********

* Executing REST request is a blocking operation. The calling thread is blocked until the request has completed.
* REST client only works in the default PDP context.
* REST client do not allow selection of IPV4 or IPV6 but it works on what DNS returns for name query.

API documentation
*****************

| Header file: :file:`include/net/rest_client.h`
| Source files: :file:`subsys/net/lib/rest_client`

.. doxygengroup:: rest_client
   :project: nrf
   :members:
