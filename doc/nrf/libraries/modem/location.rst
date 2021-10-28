.. _lib_location:

Location
########

.. contents::
   :local:
   :depth: 2

Location library provides functionality for retrieving device's location utilizing different positioning methods
such as GNSS satellite positioning including Assisted GPS (A-GPS) and predicted GPS (P-GPS) data, cellular positioning or WiFi positioning.

Overview
********

Location library provides a convenient API for clients to request devices location.
The client can determine the preference order of the positioning methods to be used along with some other configuration information.
Once a method fails to provide location, Location library will perform fallback to the next preferred method.

Supported positioning methods are GNSS, cellular and WiFi positioning.
Both cellular and WiFi positioning take advantage of the base stations seen with corresponding technology, and then utilizing web services for retrieving the location.
GNSS positioning uses satellites to compute the location of the device. Location library can utilize assistance data (A-GPS and P-GPS) to find the satellites faster.

Implementation
==============

Location library has a compact API and then location core that handles location method independent part of the functionality such as fallback to the next preferred method and timeouts.
Each location method has its own implementation for the location retrieval:

* GNSS positioning
   * :ref:`gnss_interface` for getting satellite information
   * A-GPS/P-GPS are managed either with
      * :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps`, or
      * the application utilizes some other source for the data and uses :c:func:`location_agps_data_process` to pass the data to Location library
   * A-GPS/P-GPS data format must be as received from :ref:`lib_nrf_cloud_agps`
   * A-GPS/P-GPS data transport for :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` can be configured to be either MQTT (:kconfig:`CONFIG_NRF_CLOUD_MQTT`) or REST (:kconfig:`CONFIG_NRF_CLOUD_REST`)
* Cellular positioning
   * :ref:`lte_lc_readme` for getting visible cellular base stations
   * :ref:`lib_multicell_location` for sending cell information to the selected location service and getting calculated location back to the device
      * Service is selected in :c:struct:`loc_method_config` when requesting location
      * Data transport for the service is REST
      * Available services are `nRF Cloud Location Services`_, `HERE Positioning`_, `Skyhook Precision Location`_ and `Polte Location API`_
* WiFi positioning
   * Zephyr's Network Management API :ref:`zephyr:net_mgmt_interface` for getting visible WiFi access points
   * Sending access point information to the selected location service and getting calculated location back to the device
      * Location library has implementation for WiFi location services
      * Service is selected in :c:struct:`loc_method_config` when requesting location
      * Data transport for the service is REST
      * Available services are `nRF Cloud Location Services`_, `HERE Positioning`_ and `Skyhook Precision Location`_

Supported features
==================

TODO: Not really sure what to put into this mandatory section given Implementation and Configuration sections has a lot of information and if something is missing, that can be added.

.. note::
   Use this section to describe the features supported by the library.

Requirements
************

nRF Cloud certificates
======================

If you use nRF Cloud for any location data, you must have the certificate provisioned.
See `Updating the nRF Connect for Cloud certificate`_ for more information.

Location service accounts
=========================

To use the location services that provide A-GPS/P-GPS, cellular positioning and WiFi positioning data, see the respective documentation for account setup and for getting the required credentials for authentication:

* `nRF Cloud Location Services`_
* `HERE Positioning`_
* `Skyhook Precision Location`_
* `Polte Location API`_

The required credentials for the location services are configurable using Kconfig options.

WiFi chip
=========

WiFi is not supported by the HW of the supported DKs. External WiFi chips can be used such as ESP32, which you can connect to the DKs.

TODO: any links we should add?

Library files
*************

.. |library path| replace:: :file:`lib/location`

This library is found under |library path| in the |NCS| folder structure.

Configuration
*************

Configure the following Kconfig option to enable this library:

* :kconfig:`CONFIG_LOCATION` - Enables the Location library.

Configure the following options to enable location methods of your choice:

* :kconfig:`CONFIG_LOCATION_METHOD_GNSS` - Enables GNSS location method.
* :kconfig:`CONFIG_LOCATION_METHOD_CELLULAR` - Enables cellular location method.
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI` - Enables WiFi location method.
* :kconfig:`CONFIG_WIFI` - Enable WiFi for Zephyr.

The following options control the use of GNSS assistance data:

* :kconfig:`CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL` - Enables A-GPS data retrieval from an external source which the application implements separately.
* :kconfig:`CONFIG_NRF_CLOUD_AGPS` - Enables A-GPS data retrieval from `nRF Cloud`_.
* :kconfig:`CONFIG_NRF_CLOUD_PGPS` - Enables P-GPS data retrieval from `nRF Cloud`_.

The following options control the transport used with `nRF Cloud`_:

* :kconfig:`CONFIG_NRF_CLOUD_REST` - Uses REST APIs to communicate with `nRF Cloud`_.
* :kconfig:`CONFIG_NRF_CLOUD_MQTT` - Uses MQTT transport to communicate with `nRF Cloud`_.
* :kconfig:`CONFIG_REST_CLIENT` - Enable :ref:`lib_rest_client` library.

Both cellular and WiFi location services are selected utilizing runtime configuration but the available services must be configured first.
For cellular location services, use at least one of the following sets of options and configure corresponding authentication parameters (more details and configuration options can be found from :ref:`lib_multicell_location`):

* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD`
* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_HERE` and :kconfig:`CONFIG_MULTICELL_LOCATION_HERE_API_KEY` (see below other authentication options)
* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_SKYHOOK` and :kconfig:`CONFIG_MULTICELL_LOCATION_SKYHOOK_API_KEY`
* :kconfig:`CONFIG_MULTICELL_LOCATION_SERVICE_POLTE` and :kconfig:`CONFIG_MULTICELL_LOCATION_POLTE_CUSTOMER_ID` and :kconfig:`CONFIG_MULTICELL_LOCATION_POLTE_API_TOKEN`

For WiFi location services, use at least one of the following sets of options and configure corresponding authentication parameters:

* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD`
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE` and :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_API_KEY`
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK` and :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_API_KEY`

Following WiFi service related options can usually have default values:

* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_HOSTNAME`
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_TLS_SEC_TAG`
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_HOSTNAME`
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_SKYHOOK_TLS_SEC_TAG`

Usage
*****

Using Location library is rather easy. First you need to initialize the library with :c:func:`location_init`.

Secondly, you need to set configuration (:c:struct:`loc_config`) including location method configurations (:c:struct:`loc_method_config`).
This is achieved easily by setting first default values by using :c:func:`loc_config_defaults_set` and c:func:`loc_config_method_defaults_set`,
and then setting any required non-default values to the structures.

Once the configuration is set up properly, you just need to call c:func:`location_request` with the configuration.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`location_sample`
* :ref:`modem_shell_application`

Limitations
***********

* Location library can only have one client registered at a time. If there is already a client handler registered, another initialization will override the existing handler.
* Cellular neighbor information used for cellular positioning is more accurate on Modem FW (MFW) 1.3.0 compared to earlier MFW releases which do not have API for scanning the neighboring cells.
  For older than MFW 1.3.0 releases, only serving cell information is provided and that information may be from hours or days ago, or even older, depending on modem sleep states.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`nrf_modem_lib_readme`
* :ref:`lte_lc_readme`
* :ref:`lib_multicell_location`
* :ref:`lib_rest_client`
* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_nrf_cloud_rest`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:gnss_interface`

It uses the following Zephyr libraries:

* :ref:`zephyr:net_mgmt_interface`
* :ref:`zephyr:net_if_interface`

API documentation
*****************

| Header file: :file:`include/modem/location.h`
| Source files: :file:`lib/location`

.. doxygengroup:: location
   :project: nrf
   :members:
