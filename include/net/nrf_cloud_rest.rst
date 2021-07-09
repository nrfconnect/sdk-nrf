.. _lib_nrf_cloud_rest:

nRF Cloud REST
###############

.. contents::
   :local:
   :depth: 2

The nRF Cloud REST library enables applications to use nRF Cloud's REST-based device API.
This library is an enhancement to the :ref:`lib_nrf_cloud`.

To make a REST call, configure a :c:struct:`nrf_cloud_rest_context` structure with your desired parameters.
Pass the REST context structure, along with any other required parameter(s), to the desired function.
When the function returns, the context will contain the status of the API call.  On success, the context will also contain the response data and length.
Some functions also have an optional `result` parameter.  If provided, the response data will be parsed into `result`.

Configuration
*************

Configure the following option to enable or disable the use of this library:

* :option:`CONFIG_NRF_CLOUD_REST`

Additionally, configure the following options for the needs of your application:

* :option:`CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE`
* :option:`CONFIG_NRF_CLOUD_REST_HOST_NAME`
* :option:`CONFIG_NRF_CLOUD_SEC_TAG`

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_rest.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_rest
   :project: nrf
   :members:
