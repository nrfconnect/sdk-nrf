.. _lib_nrf_cloud_location:
.. _lib_nrf_cloud_cell_pos:

nRF Cloud location
##################

.. contents::
   :local:
   :depth: 2

The nRF Cloud location library enables applications to submit cellular network and/or nearby Wi-Fi network information to `nRF Cloud`_ over MQTT to obtain device location.
This library is an enhancement to the :ref:`lib_nrf_cloud` library.

.. note::
   To use the nRF Cloud location service, you need an nRF Cloud account, and the device needs to be associated with your account.

Configuration
*************

Configure the :kconfig:option:`CONFIG_NRF_CLOUD_LOCATION` Kconfig option to enable or disable the use of this library.

Request and process location data
*********************************

The :c:func:`nrf_cloud_location_request` function is used to submit network information to the cloud.
If specified in the request, nRF Cloud responds with the location data.
If the application provided a callback with the request, the library sends the location data to the application's callback.
Otherwise, the library sends the location data to the application's :ref:`lib_nrf_cloud` event handler as an :c:enum:`NRF_CLOUD_EVT_RX_DATA_LOCATION` event.

The :c:func:`nrf_cloud_location_process` function processes the received location data.
The function parses the data and returns the location if it is found.

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_location.h`, :file:`include/net/wifi_location_common.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/`

.. doxygengroup:: nrf_cloud_location
   :project: nrf
   :members:

.. doxygengroup:: wifi_location_common
   :project: nrf
   :members:
