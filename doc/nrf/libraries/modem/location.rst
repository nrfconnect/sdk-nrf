.. _lib_location:

Location
########

.. contents::
   :local:
   :depth: 2

Location library provides functionality for retriving device's location  utilizing different positioning methods such as GNSS, cellular positioning or WiFi positioning.
The client can determine the preference order of the methods to be used along with some other configuration information.

Available location methods are GNSS satellite positioning, cellular and WiFi positioning.
Both cellular and WiFi positioning take advantage of the base stations seen with corresponding technology, and then utilizing web services for retrieving the location.

Location library can be configured to utilize Assisted GPS (A-GPS) and predicted GPS (P-GPS) data.

Overview*
*********

.. note::
   Use this section to give a general overview of the library.
   This is optional if the library is so small the the initial short description also gives an overview of the library.

Implementation
==============

.. note::
   Use this section to describe the library architecture and implementation.

Supported features
==================

.. note::
   Use this section to describe the features supported by the library.

Supported backends*
===================

.. note::
   Use this section to describe the backends supported by the library, if applicable.
   This is optional.

Requirements*
*************

.. note::
   Use this section to list the requirements needed to use the library.
   This is not required if there are no specific requirements to use the library. It is a mandatory section if there are specific requirements that have to be met to use the library.

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

TODO: multicell part
TODO: rest client part

Usage*
******

.. note::
   Use this section to explain how to use the library.
   This is optional if the library is so small the the initial short description also gives information about how to use the library.

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

* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`
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
