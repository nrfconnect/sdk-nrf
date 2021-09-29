.. _location_readme:

Location
########

.. contents::
   :local:
   :depth: 2

Location library provides functions for getting location information utilizing different positioning methods available in the system.
The client can determine the preference order of the methods to be used along with some other configuration information.

Available location methods are GNSS satellite positioning, cellular and WiFi positioning.
Both cellular and WiFi positioning take advantage of the base stations seen with corresponding technology, and then utilizing web services for retrieving the location.

Location library can be configured to utilize Assisted GPS (A-GPS) and predicted GPS (P-GPS) data.

Configuration
*************

Configure the following Kconfig options when using this library:

* :kconfig:`CONFIG_LOCATION` - Enables the Location library.
* :kconfig:`CONFIG_LOCATION_METHOD_GNSS` - Enables GNSS location method.
* :kconfig:`CONFIG_LOCATION_METHOD_CELLULAR` - Enables cellular location method.
* :kconfig:`CONFIG_LOCATION_METHOD_WIFI` - Enables WiFi location method.
* :kconfig:`CONFIG_NRF_CLOUD_AGPS` - Enables A-GPS data retrieval from `nRF Cloud`_.
* :kconfig:`CONFIG_NRF_CLOUD_PGPS` - Enables P-GPS data retrieval from `nRF Cloud`_.

Limitations
***********

Location library can only have one client registered at a time. If there is already a client handler registered, another initialization will override the existing handler.

API documentation
*****************

| Header file: :file:`include/modem/location.h`
| Source file: :file:`lib/location/location.c`

.. doxygengroup:: location
   :project: nrf
   :members:
