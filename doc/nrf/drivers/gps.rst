.. _gps_api:

GPS
###

The GPS interface exposes an API to access GPS devices.
The API offers functionality to initialize and configure a device, to start and stop GPS search, and to request and provide GPS assistance data (A-GPS).

.. contents::
   :local:
   :depth: 2

.. note::

   The GPS driver is deprecated since v1.7.0. Use the :ref:`nrfxlib:gnss_interface` instead.

API documentation
*****************

| Header file: :file:`include/drivers/gps.h`

.. doxygengroup:: gpsapi
   :project: nrf
   :members:
