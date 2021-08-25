.. _lib_nrf_cloud_rest:

nRF Cloud REST
##############

.. contents::
   :local:
   :depth: 2

The nRF Cloud REST library enables applications to use `nRF Cloud's REST-based device API <nRF Cloud REST API_>`_.
This library is an enhancement to the :ref:`lib_nrf_cloud` library.

To make a REST call, configure a :c:struct:`nrf_cloud_rest_context` structure with your desired parameters.

* ``auth`` - Use :ref:`lib_modem_jwt` to generate a JWT string. The ``sec_tag`` used for JWT generation must contain a credential that has been registered with nRF Cloud.

Pass the REST context structure, along with any other required parameters, to the desired function.
When the function returns, the context contains the status of the API call.
If the API call is successful, the context also contains the response data and length.
Some functions also have an optional ``result`` parameter.
If this parameter is provided, the response data is parsed into ``result``.

Response is handled in the following way:

* :c:func:`nrf_cloud_rest_pgps_data_get` - Pass response data to the P-GPS library :ref:`lib_nrf_cloud_pgps`.
* :c:func:`nrf_cloud_rest_agps_data_get` - Pass response data to the A-GPS library :ref:`lib_nrf_cloud_agps`.
* :c:func:`nrf_cloud_rest_fota_job_get` - If a FOTA job exists, :ref:`lib_fota_download` can perform the firmware download and installation. Call the :c:func:`nrf_cloud_rest_fota_job_update` function to report the status of the job.

Configuration
*************

Configure the :kconfig:`CONFIG_NRF_CLOUD_REST` option to enable or disable the use of this library.

Additionally, configure the following options for the needs of your application:

* :kconfig:`CONFIG_NRF_CLOUD_REST_FRAGMENT_SIZE`
* :kconfig:`CONFIG_NRF_CLOUD_REST_HOST_NAME`
* :kconfig:`CONFIG_NRF_CLOUD_SEC_TAG`

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_rest.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_rest
   :project: nrf
   :members:
