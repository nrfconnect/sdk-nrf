.. _lib_multicell_location:

Multicell location
##################

.. contents::
   :local:
   :depth: 2

The Multicell location library provides a way to acquire the location of the device based on LTE cell measurements.


Overview
********

The library uses LTE cell information such as :ref:`lte_lc_readme` library to generate requests through
:ref:`lib_nrf_cloud_cell_pos`, :ref:`lib_nrf_cloud_rest` or :ref:`lib_rest_client` depending on the selected service and transport (REST or MQTT).

When the location service has resolved the location based on the cell measurements provided in the request, it sends back a response to the device.
After receiving the response, the Multicell location library parses the response and returns the location to the caller.

The library supports the following location services:

*  `nRF Cloud Location Services`_
*  `HERE Positioning`_ (v1 and v2)

The data transport method for the request to the service is mainly REST. However, either MQTT (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) or REST (:kconfig:option:`CONFIG_NRF_CLOUD_REST`) can be configured for `nRF Cloud Location Services`_.

To use the location services, see the respective documentation for account setup and for getting the required credentials for authentication.

.. reprovision_cert_note_start

.. note::

   nRF9160 DK and Thingy:91 devices are shipped with RSA256 certificates.
   To start using the Multicell location library with nRF Cloud, you must perform either of the following actions:

      * Delete the device from nRF Cloud and reprovision it with a new ES256 device certificate. See :ref:`nrf9160_ug_updating_cloud_certificate` for more information.
      * Register a separate key for JWT signing as described in `Securely Generating Credentials on the nRF9160`_.

.. reprovision_cert_note_end

The required credentials for the location services are configurable using Kconfig options.
There is no difference in the API calls when using the services.

The library has an API to handle provisioning of the required TLS certificates for the location services, :c:func:`multicell_location_provision_certificate`.

.. note::
   Certificates must be provisioned while the modem's functional mode is offline, or it is powered off.
   The simplest way to achieve this is to call :c:func:`multicell_location_provision_certificate` after booting the application, before connecting to the LTE network.


Configuration
*************

To use the multicell location library, enable the :kconfig:option:`CONFIG_MULTICELL_LOCATION` Kconfig option.

Select nRF Cloud and HERE location services using at least one of the following sets of options and configure corresponding authentication parameters:

*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_SERVICE_HERE` and :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_API_KEY` (see below other authentication options)

API key is used for HERE as default authentication method.
A JSON Web Token (JWT) signed by the device's private key is used for nRF Cloud.

The following options offer different version and authentication method for HERE location service:

*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_V1`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_V2`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_USE_API_KEY`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_USE_APP_CODE_ID`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_APP_CODE`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_APP_ID`

The following options control the transport method used with `nRF Cloud`_:

* :kconfig:option:`CONFIG_NRF_CLOUD_REST` - Uses REST APIs to communicate with `nRF Cloud`_ if :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` is not set.
* :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` - Uses MQTT transport to communicate with `nRF Cloud`_.

Following are the options that can usually have default values:

*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_HTTPS_PORT`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_HOSTNAME`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_TLS_SEC_TAG`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS`
*  :kconfig:option:`CONFIG_MULTICELL_LOCATION_RECV_BUF_SIZE`

Other relevant options for configuring location retrieval can be found from :ref:`lib_nrf_cloud_rest` and :ref:`lib_rest_client`.

Limitations
***********

*  Retrieving the device's location is a blocking operation.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`
* :ref:`lib_rest_client`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_nrf_cloud_cell_pos`

API documentation
*****************

| Header file: :file:`include/net/multicell_location.h`
| Source files: :file:`lib/multicell_location/`

.. doxygengroup:: multicell_location
   :project: nrf
   :members:
