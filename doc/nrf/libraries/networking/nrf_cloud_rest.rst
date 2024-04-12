.. _lib_nrf_cloud_rest:

nRF Cloud REST
##############

.. contents::
   :local:
   :depth: 2

The nRF Cloud REST library enables applications to use `nRF Cloud's REST-based device API <nRF Cloud REST API_>`_.
This library is an enhancement to the :ref:`lib_nrf_cloud` library.

Overview
********

This library exposes a number of functions that provide access to nRF Cloud REST API endpoints.

When called, these functions initiate an HTTP request to the nRF Cloud REST backend.
Various `nRF Cloud REST API endpoints <nRF Cloud REST API_>`_ are provided for tasks such as:

* Updating a device's shadow.
* Sending and receiving device messages.
* Interacting with location services.

All of these functions operate on an :c:struct:`nrf_cloud_rest_context` structure that you must initialize and provide with your desired parameters.
When each function returns, the context contains the status of the API call.

Authentication
==============

Each HTTP/REST request must include a valid JSON Web Token (JWT) that serves as a proof of identity.
Usually, these tokens are generated based on the `device credentials <nRF Cloud Security_>`_, and must be passed to the ``auth`` parameter of the :c:struct:`nrf_cloud_rest_context` structure.
To generate a token, call the :c:func:`nrf_cloud_jwt_generate` function, or use the :ref:`lib_modem_jwt` library.
Alternatively, if you have enabled the :kconfig:option:`CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT` option (along with its dependencies), the nRF Cloud REST library generates a JWT automatically if one is not provided.

Socket management and reuse
===========================

The socket used for each HTTP request is determined by the ``connect_socket`` parameter of the :c:struct:`nrf_cloud_rest_context` structure.
If a valid socket file descriptor is passed, this socket is used.
If ``-1`` is passed, a socket will be created automatically.

The selected socket can be reused depending on the passed-in value of the ``keep_alive`` property of the :c:struct:`nrf_cloud_rest_context` structure:

* If you set ``keep_alive`` to false, the selected socket is closed after each request and the ``connect_socket`` property is reset to ``-1``.
* If you set ``keep_alive`` to true, the socket remains open and ``connect_socket`` remains unaltered, meaning the socket is reused on the next API call.

However, if you set ``keep_alive`` to true, make sure that the socket has not been closed externally (for example, due to inactivity) before sending further requests.
Otherwise, the request will be dropped.

Timeouts in the socket can happen in two ways:

* The socket may be closed immediately on timeout, causing ``-ENOTCONN`` to be returned by the next REST request function call, without reaching nRF Cloud.
* The socket may be left open, allowing the next REST request to reach nRF Cloud, but the request is ignored and the socket is closed gracefully (FIN,ACK) with null HTTP response.
  In this case, the called request function returns ``-ESHUTDOWN``.

See :ref:`nrf_cloud_rest_failure` for more details.

Request success
===============

If the API call is successful, the context also contains the response data and length.
Some functions also have an optional ``result`` parameter.
If this parameter is provided, the response data is parsed into ``result``.

A number of functions automatically handle response data themselves:

* :c:func:`nrf_cloud_rest_pgps_data_get` - Pass response data to the P-GPS library :ref:`lib_nrf_cloud_pgps`.
* :c:func:`nrf_cloud_rest_agnss_data_get` - Pass response data to the A-GNSS library :ref:`lib_nrf_cloud_agnss`.
* :c:func:`nrf_cloud_rest_fota_job_get` - If a FOTA job exists, :ref:`lib_fota_download` can perform the firmware download and installation.
  Call the :c:func:`nrf_cloud_rest_fota_job_update` function to report the status of the job.

.. _nrf_cloud_rest_failure:

Request failure
===============

If an API call is unsuccessful, the called request function may return a variety of outputs:

* If the error occurred at the socket level, the exact socket errno is returned.
  For instance, ``-ENOTCONN`` if the socket was closed, or was never opened before the request was made.
* If the remote endpoint closes the connection gracefully without giving a response (a null HTTP response), ``-ESHUTDOWN`` is returned.
* If the remote endpoint responds with an unexpected HTTP status code (indicating request rejection), ``-EBADMSG`` is returned.
  Possible causes include, but are not limited to: bad endpoint, invalid request data, and invalid JWT.
* If the response body is empty and the request expects response data, ``-ENODATA`` is returned.
* If a heap allocation fails, ``-ENOMEM`` is returned.
* Request formatting errors return ``-ETXTBSY``.

Some functions may have additional return values.
These are documented on the function itself.

Configuration
*************

Configure the :kconfig:option:`CONFIG_NRF_CLOUD_REST` option to enable or disable the use of this library.

Additionally, configure the following options for the needs of your application:

* :kconfig:option:`CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE`
* :kconfig:option:`CONFIG_NRF_CLOUD_REST_HOST_NAME`
* :kconfig:option:`CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT`
* :kconfig:option:`CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT_VALID_TIME_S`
* :kconfig:option:`CONFIG_NRF_CLOUD_SEC_TAG`

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_rest.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_rest
   :project: nrf
   :members:
