.. _lib_date_time:

Date-Time Library
#################

The date-time library maintains the current date-time information in UTC format.
The option :option:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS` determines the frequency with which the library updates the date-time information.

The information is fetched in the following prioritized order:

1. The library checks if the current date-time information is valid and relatively new.
   If the date-time information currently set in the library was obtained sometime during the interval set by CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS, the library does not fetch new date-time information.
   In this way, unnecessary update cycles are avoided.
#. If the aforementioned check fails, the library requests time from the onboard modem of nRF9160.
#. If the time information obtained from the onboard modem of nRF9160 is not valid, the library requests time from NTP servers.
#. If the NTP time request does not succeed, the library tries to request time information from several other NTP servers, before it fails.

The :cpp:func:`date_time_set` function can be used to obtain the current date-time information from external sources independent of the internal date-time update routine.
Time from GPS can be such an external source.

To get date-time information from the library, either call the :cpp:func:`date_time_uptime_to_unix_time_ms` function or the :cpp:func:`date_time_now` function.
See the API documentation for more information on these functions.

.. note::

   The first date-time update cycle (after boot) does not occur until the time set by the :option:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS` has elapsed.
   It is recommended to call the :cpp:func:`date_time_update` function after the device has connected to LTE, to get the initial date-time information.

Configuration
*************

:option:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS`

   Configure this option to control the frequency with which the library fetches the time information.

API documentation
*****************

| Header file: :file:`include/date_time.h`
| Source files: :file:`lib/date_time/src/`

.. doxygengroup:: date_time
   :project: nrf
   :members:
