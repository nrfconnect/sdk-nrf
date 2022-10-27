.. _lib_lwm2m_location_assistance:

LwM2M location assistance
#########################

.. contents::
   :local:
   :depth: 2

The LwM2M location assistance library provides a proprietary mechanism to fetch location assistance data from `nRF Cloud`_ by proxying it through the LwM2M server.

Overview
********

Location Assistance object is a proprietary LwM2M object used to deliver information required by various location services through LwM2M.
This feature is currently under development and considered :ref:`experimental <software_maturity>`.
As of now, only AVSystem's Coiote LwM2M server can be used for utilizing the location assistance data from nRF Cloud.

The library adds support for four objects related to location assistance:

* GNSS Assistance object (ID 33625) for requesting and handling A-GPS and P-GPS assistance data.
* Ground Fix Location object (ID 33626) for requesting and storing estimated cell and Wi-Fi based location.
* Visible Wi-Fi Access Point object (ID 33627) for storing nearby Wi-Fi Access Point information.
* ECID-Signal Measurement Information object (ID 10256) for storing the cell neighborhood information.

The Ground Fix Location object works in co-operation with the Visible Wi-Fi Access Point object and the ECID-Signal Measurement Information object.
When using ground fix, the library always sends as much information about the location to be estimated as it can.

.. note::
   The old Location Assistance object (ID 50001) has been removed.

Supported features
******************

There are three different supported methods of obtaining the location assistance:

* Location based on cell information - The device sends information about the current cell and possibly about the neighboring cells to the  LwM2M server.
  The LwM2M server then sends the location request to nRF Cloud, which responds with the location data.
* Query of A-GPS assistance data - The A-GPS assistance data is queried from nRF Cloud and provided back to the device through the LwM2M server.
  The A-GPS assistance data is then provided to the GNSS module for obtaining the position fix faster.
* Query of P-GPS predictions - The P-GPS predictions are queried from nRF Cloud and provided back to the device through the LwM2M server.
  The predictions are stored in the device flash and injected to the GNSS module when needed.

Configuration
*************

To enable location assistance, configure either or both of the following Kconfig options:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT`
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT`

Following are the other important library options:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS` -  nRF Cloud provides A-GPS assistance data and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS` -  nRF Cloud provides P-GPS predictions and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL` -  nRF Cloud provides estimated location based on currently attached cell and its neighborhood.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS` - Disable this option if you provide your own method of sending the assistance requests to the LwM2M server.

API documentation
*****************

| Header files: :file:`include/net/lwm2m_client_utils_location.h`, :file:`include/net/lwm2m_client_utils_location_events.h`
| Source files: :file:`subsys/net/lib/lwm2m_client_utils/location/location_assistance.c`, :file:`subsys/net/lib/wm2m_client_utils/location/location_events.c`

.. doxygengroup:: lwm2m_client_utils_location
   :project: nrf
   :members:
   :inner:

.. doxygengroup:: lwm2m_client_utils_location_events
   :project: nrf
   :members:
   :inner:
