.. _lib_location:

Location
########

.. contents::
   :local:
   :depth: 2

The location library provides functionality for retrieving the location of a device using different positioning methods such as:

* GNSS satellite positioning including Assisted GPS (A-GPS) and Predicted GPS (P-GPS) data.
* Cellular positioning.
* Wi-Fi positioning.

Overview
********

This library provides an API for applications to request the location of a device.
The application can determine the preferred order of the location methods to be used along with other configuration information.
If a method fails to provide the location, the library performs a fallback to the next preferred method.

Both cellular and Wi-Fi positioning detect the base stations and use web services for retrieving the location.
GNSS positioning uses satellites to compute the location of the device.
This library can use the assistance data (A-GPS and P-GPS) to find the satellites faster.

Implementation
==============

The location library has a compact API and a location core that handles the functionality that is independent of the location method, such as fallback to the next preferred method and timeouts.
Each location method has its own implementation for the location retrieval:

* GNSS positioning

  * :ref:`gnss_interface` for getting the location.
  * A-GPS and P-GPS are managed with :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps`.
  * The application may also use some other source for the data and use :c:func:`location_agps_data_process` to pass the data to the location library.
  * The data format of A-GPS or P-GPS must be as received from :ref:`lib_nrf_cloud_agps`.
  * The data transport method for :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` can be configured to be either MQTT (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) or REST (:kconfig:option:`CONFIG_NRF_CLOUD_REST`).

    If different transport is desired for different location methods, (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) and (:kconfig:option:`CONFIG_NRF_CLOUD_REST`) can be enabled simultaneously. In such a case, MQTT takes
    precedence as the transport method of GNSS assistance data.
  * Note that acquiring GNSS fix only starts when LTE connection, more specifically Radio Resource Control (RRC) connection, is idle.

    Also, if A-GPS is not used and Power Saving Mode (PSM) is enabled, Location library will wait for the modem to enter PSM.
  * Selectable location accuracy (low/normal/high).

* Cellular positioning

  * :ref:`lte_lc_readme` for getting visible cellular base stations.
  * :ref:`lib_multicell_location` for sending cell information to the selected location service and getting the calculated location back to the device.

    * The service is selected in the :c:struct:`location_method_config` structure when requesting for location.
    * The services available are `nRF Cloud Location Services`_ and `HERE Positioning`_.
    * The data transport method for the service is mainly REST. However, either MQTT (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) or REST (:kconfig:option:`CONFIG_NRF_CLOUD_REST`) can be configured for `nRF Cloud Location Services`_.

* Wi-Fi positioning

  * Zephyr's Network Management API :ref:`zephyr:net_mgmt_interface` for getting the visible Wi-Fi access points.
  * Sending access point information to the selected location service and getting the calculated location back to the device:

    * The location library has an implementation for the Wi-Fi location services.
    * The service is selected in the :c:struct:`location_method_config` structure when requesting for location.
    * The services available are `nRF Cloud Location Services`_ and `HERE Positioning`_.
    * The data transport method for the service is REST.

Requirements
************

nRF Cloud certificates
======================

When using nRF Cloud for any location data, you must have the certificate provisioned.
See :ref:`nrf9160_ug_updating_cloud_certificate` for more information.
nRF9160 DK comes pre-provisioned with certificates for nRF Cloud.

Location service accounts
=========================

To use the location services that provide A-GPS or P-GPS, cellular or Wi-Fi positioning data, see the respective documentation for setting up your account and getting the required credentials for authentication:

* `nRF Cloud Location Services`_
* `HERE Positioning`_

You can configure the required credentials for the location services using Kconfig options.

Wi-Fi chip
==========

None of the supported DKs have a Wi-Fi chip. You can use external Wi-Fi chip, such as ESP8266, and connect it to the nRF9160 DK.
You can see :ref:`location_sample` and its DTC overlay for some more information on ESP8266 integration.

Library files
*************

.. |library path| replace:: :file:`lib/location`

This library is found under |library path| in the |NCS| folder structure.

Configuration
*************

Configure the following Kconfig options to enable this library:

* :kconfig:option:`CONFIG_LOCATION` - Enables the Location library.
* :kconfig:option:`CONFIG_NRF_MODEM_LIB` - Enable modem library.
* :kconfig:option:`CONFIG_LTE_LINK_CONTROL` - Enable LTE link control.

Configure the following Kconfig options to enable Wi-Fi interface:

* :kconfig:option:`CONFIG_WIFI` - Enable Wi-Fi for Zephyr.

The chosen Wi-Fi device needs to be set in Devicetree:

.. code-block:: devicetree

    chosen {
      ncs,location-wifi = &mywifi;
    };

Configure the following options to enable location methods of your choice:

* :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS` - Enables GNSS location method.
* :kconfig:option:`CONFIG_LOCATION_METHOD_CELLULAR` - Enables cellular location method.
* :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI` - Enables Wi-Fi location method.

The following options control the use of GNSS assistance data:

* :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL` - Enables A-GPS data retrieval from an external source, implemented separately by the application. If enabled, the library triggers a :c:enum:`LOCATION_EVT_GNSS_ASSISTANCE_REQUEST` event when assistance is needed. Once the application has obtained the assistance data, it should call the :c:func:`location_agps_data_process` function to feed it into the library.
* :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL` - Enables P-GPS data retrieval from an external source, implemented separately by the application. If enabled, the library triggers a :c:enum:`LOCATION_EVT_GNSS_PREDICTION_REQUEST` event when assistance is needed. Once the application has obtained the assistance data, it should call the :c:func:`location_pgps_data_process` function to feed it into the library.
* :kconfig:option:`CONFIG_NRF_CLOUD_AGPS` - Enables A-GPS data retrieval from `nRF Cloud`_.
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` - Enables P-GPS data retrieval from `nRF Cloud`_.
* :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED` - Reduces assistance size by only downloading ephemerides for visible satellites.

The following option is useful when setting :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED`:

* :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK` - Sets elevation threshold angle.

The following options control the transport method used with `nRF Cloud`_:

* :kconfig:option:`CONFIG_NRF_CLOUD_REST` - Uses REST APIs to communicate with `nRF Cloud`_ if :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` is not set.
* :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` - Uses MQTT transport to communicate with `nRF Cloud`_.
* :kconfig:option:`CONFIG_REST_CLIENT` - Enable :ref:`lib_rest_client` library.

Both cellular and Wi-Fi location services are selected using the runtime configuration but the available services must be configured first.
For cellular location services, use at least one of the following sets of options and configure corresponding authentication parameters (for more details and configuration options, see :ref:`lib_multicell_location`):

* :kconfig:option:`CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD`
* :kconfig:option:`CONFIG_MULTICELL_LOCATION_SERVICE_HERE` and :kconfig:option:`CONFIG_MULTICELL_LOCATION_HERE_API_KEY`

For Wi-Fi location services, use at least one of the following sets of options and configure the corresponding authentication parameters:

* :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_NRF_CLOUD`
* :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE` and :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_API_KEY`

The following options are related to the Wi-Fi service and can usually have the default values:

* :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_HOSTNAME`
* :kconfig:option:`CONFIG_LOCATION_METHOD_WIFI_SERVICE_HERE_TLS_SEC_TAG`

Usage
*****

To use the Location library, perform the following steps:

1. Initialize the library with the :c:func:`location_init` function.
#. Create the configuration (:c:struct:`location_config` structure).
#. Set the default values by passing the configuration to the :c:func:`location_config_defaults_set` function together with the list of method types.
#. Set any required non-default values to the structures.
#. Call the :c:func:`location_request` function with the configuration.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`location_sample`
* :ref:`modem_shell_application`

Limitations
***********

* The Location library can only have one application registered at a time. If there is already an application handler registered, another initialization will override the existing handler.
* Cellular neighbor information used for cellular positioning is more accurate on modem firmware (MFW) 1.3.0 compared to earlier MFW releases that do not have an API for scanning the neighboring cells.
  For MFW releases older than 1.3.0, only serving cell information is provided and it can be hours or days old, or even older, depending on the modem sleep states.

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
* :ref:`lib_modem_jwt`

It uses the following `sdk-nrfxlib`_ library:

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
