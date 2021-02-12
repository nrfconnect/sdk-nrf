.. _lib_nrf_cloud_agps:

nRF Cloud A-GPS
###############

.. contents::
   :local:
   :depth: 2

The nRF Cloud A-GPS library enables applications to request and process Assisted GPS (`A-GPS`_) data from `nRF Connect for Cloud`_ to be used with the nRF9160 SiP.
This library is an enhancement to the :ref:`lib_nrf_cloud`.

The use of A-GPS reduces the time for a GPS device to estimate its position, which is also called Time to First Fix (TTFF).
To get a position fix, a GPS needs information such as the satellite orbital data, exact timing data, and a rough position estimate.
GPS satellites broadcast this information in a pattern, which repeats every 12.5 minutes.
If nRF Connect for Cloud A-GPS service is used, the broadcasted information can be downloaded at a faster rate from nRF Connect for Cloud.

.. note::
   To use the nRF Connect for Cloud A-GPS service, an nRF Connect for Cloud account is needed, and the device needs to be associated with a user's account.

Configuration
*************

Configure the following options to enable or disable the use of this library:

* :option:`CONFIG_NRF_CLOUD`
* :option:`CONFIG_NRF_CLOUD_AGPS`

See :ref:`configure_application` for information on how to change configuration options.

Request and process A-GPS data
******************************

A-GPS data can be requested using one of the following methods:

* By specifying an array of A-GPS types
* By requesting all the available assistance data

The :c:func:`nrf_cloud_agps_request` function is used to request by type, and the :c:func:`nrf_cloud_agps_request_all` function is used to return all available assistance data.

When nRF Connect for Cloud responds with the requested A-GPS data, the :c:func:`nrf_cloud_agps_process` function processes the received data.
The function parses the data and passes it on to the modem.

Practical considerations
************************

The type of assistance data needed at a certain point of time may vary.
Since the library requests only partial data, data traffic reduction and battery conservation might be observed.
The duration for which a particular type of assistance data is valid is different for each type of assistance data.
As an example, `Almanac`_ data has a far longer validity (several months) than `Ephemeris`_ data (2 to 4 hours).

Since the library receives a partial assistance data set, it may cause GPS to download the missing data from satellites.

When A-GPS data is downloaded using LTE network, the LTE link is in `RRC connected mode <RRC idle mode_>`_.
The GPS can only operate only when the device is in RRC idle mode.
The time to go from RRC connected mode to RRC idle mode is network-dependent.
This time is usually not controlled by the device and is typically in the range of 5 to 70 seconds.
If the GPS service has already started before the device enters the RRC idle mode, this RRC inactivity may make TTFF appear longer than the actual time spent searching for the GPS satellite signals.

Limitations
***********

.. include:: ../../samples/nrf9160/agps/README.rst
   :start-after: agpslimitation_start
   :end-before: agpslimitation_end

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_agps.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_agps
   :project: nrf
   :members:
