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

* GNSS method utilizes :ref:`gnss_interface`
* Cellular positioning utilizes :ref:`lte_lc_readme` for getting visible cellular base stations and :ref:`lib_multicell_location` for sending cell information to the selected service and getting calculated location back to the device.
* WiFi positioning uses Zephyr's WiFi API (TODO: link) for getting visible WiFi access points. Location library has then implementation for WiFi services where access point information is sent and calculated location is received back to the device.

Supported features
==================

.. note::
   Use this section to describe the features supported by the library.

Requirements
************

* TODO: certs
* TODO: service accounts
* TODO: nrf9160?
* TODO: WiFi chip

Library files
*************

.. |library path| replace:: :file:`lib/location`

This library is found under |library path| in the |NCS| folder structure.

Configuration
*************

Configure the following Kconfig options when using this library:

* :kconfig:`CONFIG_LOCATION` - Enables the Location library.
* :kconfig:`CONFIG_LOCATION_METHOD_GNSS` - Enables GNSS location method.
* :kconfig:`CONFIG_LOCATION_METHOD_CELLULAR` - Enables cellular location method.
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI` - Enables WiFi location method.
* :kconfig:`CONFIG_NRF_CLOUD_AGPS` - Enables A-GPS data retrieval from `nRF Cloud`_.
* :kconfig:`CONFIG_NRF_CLOUD_PGPS` - Enables P-GPS data retrieval from `nRF Cloud`_.

* TODO: multicell part
* TODO: rest client part
* TODO: nrf cloud rest
* TODO: nrf cloud agps / pgps

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

Application integration*
************************

.. note::
   Use this section to explain how to integrate the library in a custom application.
   This is optional.

Additional information*
***********************

.. note::
   Use this section to describe any additional information relevant to the user.
   This is optional.

Limitations
***********

Location library can only have one client registered at a time. If there is already a client handler registered, another initialization will override the existing handler.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`nrf_modem_lib_readme`
* :ref:`gnss_interface`
* :ref:`lte_lc_readme`
* :ref:`lib_multicell_location`
* :ref:`lib_rest_client`
* :ref:`_lib_nrf_cloud`
* :ref:`_lib_nrf_cloud_agps`
* :ref:`_lib_nrf_cloud_pgps`
* :ref:`_lib_nrf_cloud_rest`

API documentation
*****************

| Header file: :file:`include/modem/location.h`
| Source files: :file:`lib/location`

.. doxygengroup:: location
   :project: nrf
   :members:
