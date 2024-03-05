.. _lib_lwm2m_location_assistance:

LwM2M location assistance
#########################

.. contents::
   :local:
   :depth: 2

The LwM2M location assistance library provides a proprietary mechanism to fetch location assistance data from `nRF Cloud`_ by proxying it through the LwM2M Server.

Overview
********

Location Assistance object is a proprietary LwM2M object used to deliver information required by various location services through LwM2M.
This feature is currently under development and considered :ref:`experimental <software_maturity>`.
As of now, only AVSystem's Coiote LwM2M Server can be used for utilizing the location assistance data from nRF Cloud.
To know more about the AVSystem integration with |NCS|, see :ref:`ug_avsystem`.

The library adds support for four objects related to location assistance:

* GNSS Assistance object (ID 33625) for requesting and handling A-GNSS and P-GPS assistance data.
* Ground Fix Location object (ID 33626) for requesting and storing estimated cell and Wi-Fi based location.
* Visible Wi-Fi Access Point object (ID 33627) for storing nearby Wi-Fi Access Point information.
* ECID-Signal Measurement Information object (ID 10256) for storing the cell neighborhood information.

The Ground Fix Location object works in co-operation with the Visible Wi-Fi Access Point object and the ECID-Signal Measurement Information object.
When using ground fix, the library always sends as much information about the location to be estimated as it can.

.. note::
   The old Location Assistance object (ID 50001) has been removed.

Supported features
******************

There are four different supported methods of obtaining the location assistance:

* Location based on cell information - The device sends information about the current cell and possibly about the neighboring cells to the LwM2M Server.
  The LwM2M Server then sends the location request to nRF Cloud, which responds with the location data.
* Location based on Wi-Fi access points - The device sends information about the nearby Wi-Fi access points to the LwM2M Server.
  The LwM2M Server then sends the location request to nRF Cloud, which responds with the location data.
* Query of A-GNSS assistance data - The A-GNSS assistance data is queried from nRF Cloud and provided back to the device through the LwM2M Server.
  The A-GNSS assistance data is then provided to the GNSS module for obtaining the position fix faster.
* Query of P-GPS predictions - The P-GPS predictions are queried from nRF Cloud and provided back to the device through the LwM2M Server.
  The predictions are stored in the device flash and injected to the GNSS module when needed.

API usage
*********

This section describes API usage in different scenarios.

.. _location_assistance_cell:

Cell-based location
===================

Cell-based location uses only current cell or current and neighboring cells.
To get information about the current cell, use the Connectivity Monitor object (ID 4).
When you enable the Kconfig option :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT`, the connectivity monitor is populated with data about the current cell.

To get information about the neighboring cells, use a collection of ECID-Signal Measurement Information objects (ID 10256).
To populate the objects, call the :c:func:`lwm2m_ncell_handler_register` function to register the listener for the neighborhood measurements and :c:func:`lwm2m_ncell_schedule_measurement` to schedule a measurement.

The Ground Fix Location object needs to address the ``report_back`` resource before sending a location request.
Back reporting tells the server whether it needs to send the acquired location back to the device.
If the location is sent back to the device, the location is stored in the Ground Fix Location object and copied in the LwM2M Location object.

To send the location request for the cell-based location, call the :c:func:`location_assistance_ground_fix_request_send` function.

.. _location_assistance_wifi:

Wi-Fi based location
====================

Wi-Fi based location uses the nearby Wi-Fi access points for estimating location.
The library uses a collection of Visible Wi-Fi Access Point objects (ID 33627) for storing the information about nearby Wi-Fi access points.
To populate the objects, first enable the Kconfig option for the access point scanner :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_WIFI_AP_SCANNER` and call the :c:func:`lwm2m_wifi_request_scan` function to request the access point scan.

The Ground Fix Location object is used in the same manner as it is used in the cell-based location when sending the location request.

.. note::
   Cell-based location and Wi-Fi based location can be combined.
   When combined, the ground fix assistance request contains data from both, the nearby cells and nearby Wi-Fi access points.

.. _location_assistance_agnss_lwm2m:

A-GNSS assistance
=================

When using A-GNSS assistance, the device requests A-GNSS assistance data from the server.
You can query the GNSS module for the data needed.
A device can request for all data at once or split the request to reduce the memory usage.
The request also contains information about the current cell the device is connected to and the information is similarly available on Connectivity Monitor object as in the cell-based location.

When requesting for A-GNSS assistance data, the device must first set the mask for the data it is requesting by calling the :c:func:`location_assistance_agnss_set_mask` function.
When the mask has been set, the :c:func:`location_assistance_agnss_request_send` function sends the request with all necessary data to the server and responds with the A-GNSS assistance data.
The assistance data is written to the GNSS module automatically by the library.

Filtered A-GNSS
---------------

With filtered A-GNSS, the satellites below the given angle above the ground are filtered out.
You can set the angle to a degree `[0 - 90]` using the :c:func:`location_assist_agnss_set_elevation_mask` function.
Setting the degree to `-1` disables filtering, which is the default setting.

.. _location_assistance_pgps_lwm2m:

P-GPS assistance
================

When using P-GPS assistance, the device requests predictions for the satellites for a near future.
P-GPS does not use information about current cell at all.
It stores the information about satellites and injects the data to the GNSS module when needed.
When using P-GPS, external flash is necessary as each prediction needs 2 kB of memory.

When requesting for P-GPS assistance data, the device can set the P-GPS resources.
If default values are used in the resources, predictions are requested for one week (42 predictions, 7 days, 4 hours between predictions).
When the resources have been set, the :c:func:`location_assistance_pgps_request_send` function sends the request to the server.

Result codes and automatic resend
=================================

The location assistance objects have a resource called ``result_code``.
This resource contains information about the request handling in the server side.
It can have three different values:

* ``0``  - The request was handled successfully.
* ``-1`` - A permanent error in the server needs fixing.
  The library will reject further requests and the device must be rebooted after the issue has been resolved in the server.
* ``1``  - Due to a temporary error in the server, the device needs to retry sending the request after a while.
* ``2`` - When no response has been received from the server in LOCATION_ASSISTANT_RESULT_TIMEOUT seconds.

The library has a resend handler for the temporary error code.
You can initialize it with the :c:func:`location_assistance_retry_init` function.
It uses an exponential backoff for scheduling the resends.

The library has a callback handler for the result code.
You can set your own callback with the :c:func:`location_assistance_set_result_code_cb` function.
It is called whenever the request has been handled.

Using A-GNSS and P-GPS simultaneously
=====================================

A-GNSS and P-GPS can be used simultaneously.
However, only one active request at a time for the object is allowed.
The functions :c:func:`location_assistance_agnss_set_mask`, :c:func:`location_assistance_agnss_request_send` and :c:func:`location_assistance_pgps_request_send` return ``-EAGAIN`` if there is an active request.
In such case, the device must resend the request after the previous request has been handled.


Configuration
*************

To enable location assistance, configure either or both of the following Kconfig options:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT`
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT`

Following are the other important library options:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS` -  nRF Cloud provides A-GNSS assistance data and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M Server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS` -  nRF Cloud provides P-GPS predictions and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M Server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL` -  nRF Cloud provides estimated location based on currently attached cell and its neighborhood.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT` - Enable support for connectivity monitoring utilities.
  Provides data about the current cell and network the device has connected to.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_OBJ_SUPPORT` - Enable support for the Visible Wi-Fi Access Point objects (ID 33627).
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_WIFI_AP_SCANNER` - Enable support for scanning Wi-Fi access points and populating Visible Wi-Fi Access Point objects.

API documentation
*****************

| Header file: :file:`include/net/lwm2m_client_utils_location.h`
| Source file: :file:`subsys/net/lib/lwm2m_client_utils/location/location_assistance.c`

.. doxygengroup:: lwm2m_client_utils_location
   :project: nrf
   :members:
   :inner:
