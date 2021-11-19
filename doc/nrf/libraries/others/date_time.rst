.. _lib_date_time:

Date-Time
#########

.. contents::
   :local:
   :depth: 2

The date-time library maintains the current date-time information in UTC format and stores it as Zephyr time and modem time.
The option :kconfig:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS` determines the frequency with which the library updates the date-time information.

The information is fetched in the following prioritized order:

1. The library checks if the current date-time information is valid and relatively new.
   If the date-time information currently set in the library was obtained sometime during the interval set by CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS, the library does not fetch new date-time information.
   In this way, unnecessary update cycles are avoided.
#. If the aforementioned check fails, the library requests time from the onboard modem of nRF9160.
#. If the time information obtained from the onboard modem of nRF9160 is not valid, the library requests time from NTP servers.
#. If the NTP time request does not succeed, the library tries to request time information from several other NTP servers, before it fails.

The :c:func:`date_time_set` function can be used to obtain the current date-time information from external sources independent of the internal date-time update routine.
Time from GPS can be such an external source.

The option :kconfig:`CONFIG_DATE_TIME_AUTO_UPDATE` determines whether date-time update is triggered automatically when the LTE network becomes available.
Libraries that require date-time information can just enable this library to get updated time information.
Applications do not need to do anything to trigger time update when they start because this library handles it automatically.

Current date-time information is stored as Zephyr time when it has been retrieved and hence applications can also get the time using the POSIX function ``clock_gettime``.
It is also stored as modem time if the modem does not have valid time.

To get date-time information from the library, either call the :c:func:`date_time_uptime_to_unix_time_ms` function or the :c:func:`date_time_now` function.
See the API documentation for more information on these functions.

.. note::

   It is recommended to set the :kconfig:`CONFIG_DATE_TIME_AUTO_UPDATE` option to trigger a time update when the device has connected to LTE.
   If an application has time-dependent operations immediately after connecting to LTE network, it should wait for a confirmation telling that time has been updated.
   If the :kconfig:`CONFIG_DATE_TIME_AUTO_UPDATE` option is not set, the first date-time update cycle (after boot) does not occur until the time set by the :kconfig:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS` option has elapsed.

Configuration
*************

:kconfig:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS`

   Configure this option to control the frequency with which the library fetches the time information.

:kconfig:`CONFIG_DATE_TIME_AUTO_UPDATE`

CONFIG_DATE_TIME_AUTO_UPDATE - Trigger date-time update when LTE is connected
   Configure this option to determine whether date-time update is triggered automatically when the device has connected to LTE.

API documentation
*****************

| Header file: :file:`include/date_time.h`
| Source files: :file:`lib/date_time/src/`

.. doxygengroup:: date_time
   :project: nrf
   :members:
