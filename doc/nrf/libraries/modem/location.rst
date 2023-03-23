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
The supported location methods are as follows:

* GNSS positioning

  * Uses :ref:`gnss_interface` for getting the location.
  * A-GPS and P-GPS are managed with :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps`.
  * The application may also use some other source for the data and use :c:func:`location_agps_data_process` and :c:func:`location_pgps_data_process` to pass the data to the location library.
  * The data format of A-GPS or P-GPS must be as received from :ref:`lib_nrf_cloud_agps`.
  * The data transport method for :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` can be configured to be either MQTT (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) or REST (:kconfig:option:`CONFIG_NRF_CLOUD_REST`).
    If different transport is desired for different location methods, (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) and (:kconfig:option:`CONFIG_NRF_CLOUD_REST`) can be enabled simultaneously. In such a case, MQTT takes
    precedence as the transport method of GNSS assistance data.
  * Note that acquiring GNSS fix only starts when LTE connection, more specifically Radio Resource Control (RRC) connection, is idle.
    Also, if A-GPS is not used and Power Saving Mode (PSM) is enabled, Location library will wait for the modem to enter PSM.
  * Selectable location accuracy (low/normal/high).
  * Obstructed visibility detection enables a fast fallback to another positioning method if the device is detected to be indoors.

* Cellular positioning

  * Uses :ref:`lte_lc_readme` for getting a list of nearby cellular base stations.
  * The ``cloud location`` method handles sending cell information to the selected location service and getting the calculated location back to the device.

* Wi-Fi positioning

  * Uses Zephyr's Network Management API :ref:`zephyr:net_mgmt_interface` for getting a list of nearby Wi-Fi access points.
  * The ``cloud location`` method handles sending access point information to the selected location service and getting the calculated location back to the device.

The ``cloud location`` method handles the location methods (cellular and Wi-Fi positioning)
that scan for technology-specific information and sends it over to the cloud service for location resolution.

The default priority order of location methods is GNSS positioning, Wi-Fi positioning and Cellular positioning.
If any of these methods are disabled, the method is simply omitted from the list.

Here are details related to the services handling cell information for cellular positioning, or access point information for Wi-Fi positioning:

  * Services can be handled by the application by enabling the :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL` Kconfig option, in which case rest of the service configurations are ignored.
  * The service is selected in the :c:struct:`location_method_config` structure when requesting for location.
  * The services available are `nRF Cloud Location Services <nRF Cloud Location Services documentation_>`_ and `HERE Positioning`_.
  * The data transport method for the `nRF Cloud Location Services <nRF Cloud Location Services documentation_>`_ can be configured to either MQTT (:kconfig:option:`CONFIG_NRF_CLOUD_MQTT`) or REST (:kconfig:option:`CONFIG_NRF_CLOUD_REST`).
  * The only data transport method with `HERE Positioning`_ service is REST.

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

* `nRF Cloud Location Services <nRF Cloud Location Services documentation_>`_
* `HERE Positioning`_

You can configure the required credentials for the location services using Kconfig options.

Wi-Fi chip
==========

None of the supported DKs have an integrated Wi-Fi chip.
You can use an external Wi-Fi chip, such as nRF7002 EK, and connect it to the nRF9160 DK.

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

* :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL` - Enables A-GPS and P-GPS data retrieval from an external source, implemented separately by the application.
  If enabled, the library triggers a :c:enum:`LOCATION_EVT_GNSS_ASSISTANCE_REQUEST` or :c:enum:`LOCATION_EVT_GNSS_PREDICTION_REQUEST` event when assistance is needed.
  Once the application has obtained the assistance data, it should call the :c:func:`location_agps_data_process` or the :c:func:`location_pgps_data_process` function to feed it into the library.
* :kconfig:option:`CONFIG_NRF_CLOUD_AGPS` - Enables A-GPS data retrieval from `nRF Cloud`_.
* :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` - Enables P-GPS data retrieval from `nRF Cloud`_.
* :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED` - Reduces assistance size by only downloading ephemerides for visible satellites.
  See :ref:`agps_filtered_ephemerides` for more details.

The following option is useful when setting :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_FILTERED`:

* :kconfig:option:`CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK` - Sets elevation threshold angle.

The obstructed visibility feature is based on the fact that the number of satellites found indoors or in other environments with limited sky-view is severely decreased.
The following options control the sensitivity of obstructed visibility detection:

* :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_VISIBILITY_DETECTION_EXEC_TIME` - Cut-off time for stopping GNSS.
* :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_VISIBILITY_DETECTION_SAT_LIMIT` - Minimum number of satellites that must be found to continue the search beyond :kconfig:option:`CONFIG_LOCATION_METHOD_GNSS_VISIBILITY_DETECTION_EXEC_TIME`.

These options set the threshold for how many satellites need to be found in how long a time period in order to conclude that the device is indoors.
Configuring the obstructed visibility detection is always a tradeoff between power consumption and the accuracy of detection.

The following options control the transport method used with `nRF Cloud`_:

* :kconfig:option:`CONFIG_NRF_CLOUD_REST` - Uses REST APIs to communicate with `nRF Cloud`_ if :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` is not set.
* :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` - Uses MQTT transport to communicate with `nRF Cloud`_.

Both cellular and Wi-Fi location services are handled externally by the application or selected using the runtime configuration, in which case you must first configure the available services.
Use at least one of the following sets of options:

* :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL`
* :kconfig:option:`CONFIG_LOCATION_SERVICE_NRF_CLOUD`
* :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE` and :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_API_KEY`

The following options are related to the HERE service and can usually have the default values:

* :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_HOSTNAME`
* :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE_TLS_SEC_TAG`

The following options control the default location request configurations and are applied
when :c:func:`location_config_defaults_set` function is called:

* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_FIRST` - Choice symbol for first priority location method.
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_SECOND` - Choice symbol for second priority location method.
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_METHOD_THIRD` - Choice symbol for third priority location method.
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_INTERVAL`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_TIMEOUT`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_TIMEOUT`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_ACCURACY`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_NUM_CONSECUTIVE_FIXES`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_VISIBILITY_DETECTION`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_GNSS_PRIORITY_MODE`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_CELLULAR_TIMEOUT`
* :kconfig:option:`CONFIG_LOCATION_REQUEST_DEFAULT_WIFI_TIMEOUT`

Usage
*****

To use the Location library, perform the following steps:

1. Initialize the library with the :c:func:`location_init` function.
#. Create the configuration (:c:struct:`location_config` structure).
#. Set the default values by passing the configuration to the :c:func:`location_config_defaults_set` function together with the list of method types.
#. Set any required non-default values to the structures.
#. Call the :c:func:`location_request` function with the configuration.

You can use the :c:func:`location_request` function in different ways, as in the following examples.

Use default values for location configuration:

.. code-block:: c

   int err;

   err = location_request(NULL);

Use GNSS and cellular and set custom timeout values for them:

.. code-block:: c

   int err;
   struct location_config config;
   enum location_method methods[] = {LOCATION_METHOD_GNSS, LOCATION_METHOD_CELLULAR};

   location_config_defaults_set(&config, ARRAY_SIZE(methods), methods);

   /* Now you have default values set and here you can modify the parameters you want */
   config.timeout = 180 * MSEC_PER_SEC;
   config.methods[0].gnss.timeout = 90 * MSEC_PER_SEC;
   config.methods[1].cellular.timeout = 15 * MSEC_PER_SEC;

   err = location_request(&config);

Use method priority list defined by Kconfig options and set custom timeout values for entire :c:func:`location_request` operation and cellular positioning:

.. code-block:: c

   int err;
   struct location_config config;

   location_config_defaults_set(&config, 0, NULL);

   /* Now you have default values set and you can modify the parameters you want but you
    * need to iterate through the method list as the order is defined by Kconfig options.
    */
   for (int i = 0; i < config.methods_count; i++) {
       if (config.methods[i].method == LOCATION_METHOD_GNSS) {
           config.methods[i].cellular.timeout = 15 * MSEC_PER_SEC;
       }
   }

   err = location_request(&config);

Samples using the library
*************************

The following |NCS| applications and samples use this library:

* :ref:`asset_tracker_v2`
* :ref:`location_sample`
* :ref:`modem_shell_application`
* :ref:`nrf_cloud_mqtt_multi_service`

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
