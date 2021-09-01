.. _lib_multicell_location:

Multicell location
##################

.. contents::
   :local:
   :depth: 2

The Multicell location library provides a way to acquire the location of the device based on LTE cell measurements.
The library uses HTTP requests to get the location from a configurable location service.


Overview
********

The library uses LTE cell information such as :ref:`lte_lc_readme` library to generate HTTP requests.
The HTTP request is then sent using TLS to the configured location service.

When the location service has resolved the location based on the cell measurements provided in the request, it sends back an HTTP response to the device.
After receiving the HTTP response, the Multicell location library parses the response and returns the location to the caller.

The library supports location services from `nRF Cloud Location Services`_, `HERE Positioning`_ (v1 and v2) and `Skyhook Precision Location`_.
To use the location services, see the respective documentation for account setup and for getting the required credentials for authentication.

.. reprovision_cert_note_start

.. note::

   nRF9160 DK and Thingy:91 devices are shipped with RSA256 certificates.
   To start using the Multicell location library with nRF Cloud, you must perform either of the following actions:

      * Delete the device from nRF Cloud and reprovision it with a new ES256 device certificate. See `Updating the nRF Connect for Cloud certificate`_ for more information.
      * Register a separate key for JWT signing as described in `Securely Generating Credentials on the nRF9160`_ and set :kconfig:`CONFIG_MULTICELL_LOCATION_NRF_CLOUD_JWT_SEC_TAG` accordingly.

.. reprovision_cert_note_end

The required credentials for the location services are configurable using Kconfig options.
There is no difference in the API calls when using the services.

The library has an API to handle provisioning of the required TLS certificates for the location services, :c:func:`multicell_location_provision_certificate`.

.. note::
   Certificates must be provisioned while the modem's functional mode is offline, or it is powered off.
   The simplest way to achieve this is to call :c:func:`multicell_location_provision_certificate` after booting the application, before connecting to the LTE network.


Configuration
*************

To enable the multicell location library, enable the :kconfig:`CONFIG_MULTICELL_LOCATION` Kconfig option.

The user must select nRF Cloud, HERE or Skyhook location services using one of the following options:

*  :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_HERE`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK`.


The next required step is to configure the authentication method.
By default, API key is used for HERE and Skyhook.
A JSON Web Token (JWT) signed by the device's private key is used for nRF Cloud.
Depending on the selected service, one of the three options below must be configured:

*  :kconfig:`CONFIG_MULTICELL_LOCATION_HERE_API_KEY`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_SKYHOOK_API_KEY`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_NRF_CLOUD_JWT_SEC_TAG`

Following are the options that can usually have default values:

*  :kconfig:`CONFIG_MULTICELL_LOCATION_HOSTNAME`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_TLS_SEC_TAG`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_SEND_BUF_SIZE`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_RECV_BUF_SIZE`
*  :kconfig:`CONFIG_MULTICELL_LOCATION_HTTPS_PORT`

Limitations
***********

*  Retrieving the device's location is a blocking operation.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`

API documentation
*****************

| Header file: :file:`include/net/multicell_location.h`
| Source files: :file:`lib/multicell_location/`

.. doxygengroup:: multicell_location
   :project: nrf
   :members:
