.. _lib_date_time:

Date-Time
#########

.. contents::
   :local:
   :depth: 2

The date-time library retrieves the date-time information and stores it as Zephyr time and modem time.


Overview
********

Date-time information update can be triggered to this library by the following means:

* Periodic date-time updates with the frequency determined by the option :kconfig:option:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS`.
* By the client using the :c:func:`date_time_update_async` function.
* External date-time source from which the date-time information is set to this library with the :c:func:`date_time_set` function.
* Automatic update when connecting to LTE network if the option :kconfig:option:`CONFIG_DATE_TIME_AUTO_UPDATE` is set.
* Notification about date-time information coming from the modem through AT%XTIME notification.

When this library retrieves the date-time information, it is fetched in the following prioritized order:

1. The library checks if the current date-time information is valid and relatively new.
   If the date-time information currently set in the library was obtained sometime during the interval set by the :kconfig:option:`CONFIG_DATE_TIME_TOO_OLD_SECONDS` option, the library does not fetch new date-time information.
   In this way, unnecessary update cycles are avoided.
#. If the check fails and the :kconfig:option:`CONFIG_DATE_TIME_MODEM` option is set, the library requests time from the onboard modem of nRF9160.
#. If the time information obtained from the onboard modem of nRF9160 is not valid and the :kconfig:option:`CONFIG_DATE_TIME_NTP` option is set, the library requests time from an NTP server.
#. If the NTP time request does not succeed, the library tries to request time information from a different NTP server, before it fails.

The current date-time information is stored as Zephyr time when it has been retrieved and hence, applications can also get the time using the POSIX function ``clock_gettime``.
It is also stored as modem time if the modem does not have valid time.

Implementation
==============

The :c:func:`date_time_set` function can be used to obtain the current date-time information from external sources independent of the internal date-time update routine.
Time from GNSS can be such an external source.

The option :kconfig:option:`CONFIG_DATE_TIME_AUTO_UPDATE` determines whether date-time update is triggered automatically when the LTE network becomes available.
Libraries that require date-time information can just enable this library to get updated time information.
Applications do not need to do anything to trigger time update when they start because this library handles it automatically.

Retrieving date-time information using the POSIX function ``clock_gettime`` is encouraged when feasible.
You can obtain the information also from the library by calling either the :c:func:`date_time_uptime_to_unix_time_ms` function or the :c:func:`date_time_now` function.
See the API documentation for more information on these functions.

.. note::

   It is recommended to set the :kconfig:option:`CONFIG_DATE_TIME_AUTO_UPDATE` option to trigger a time update when the device has connected to LTE.
   If an application has time-dependent operations immediately after connecting to LTE network, it should wait for a confirmation telling that time has been updated.
   If the :kconfig:option:`CONFIG_DATE_TIME_AUTO_UPDATE` option is not set, the first date-time update cycle (after boot) does not occur until the time set by the :kconfig:option:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS` option has elapsed.

Configuration
*************

Configure the following Kconfig options to enable this library and its main functionalities:

* :kconfig:option:`CONFIG_DATE_TIME` - Enables this library.
* :kconfig:option:`CONFIG_DATE_TIME_MODEM` - Enables use of modem time.
* :kconfig:option:`CONFIG_DATE_TIME_NTP` - Enables use of NTP (Network Time Protocol) time.
* :kconfig:option:`CONFIG_DATE_TIME_AUTO_UPDATE` - Trigger date-time update automatically when LTE is connected.

Configure the following options to fine-tune the behavior of the library:

* :kconfig:option:`CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS` - Control the frequency with which the library fetches the time information.
* :kconfig:option:`CONFIG_DATE_TIME_TOO_OLD_SECONDS` - Control the time when date-time update is applied if previous update was done earlier.
* :kconfig:option:`CONFIG_DATE_TIME_NTP_QUERY_TIME_SECONDS` - Timeout for a single NTP query.
* :kconfig:option:`CONFIG_DATE_TIME_THREAD_STACK_SIZE` - Configure the stack size of the date-time update thread.

Samples using the library
*************************

The following |NCS| samples and applications use this library:

* :ref:`asset_tracker_v2`
* :ref:`serial_lte_modem`
* :ref:`location_sample`
* :ref:`gnss_sample`
* :ref:`modem_shell_application`
* :ref:`lwm2m_client`

Limitations
***********

The date-time library can only have one application registered at a time.
If there is already an application handler registered, another registration will override the existing handler.
Also, using the :c:func:`date_time_update_async` function will override the existing handler.

Dependencies
************

* :ref:`nrf_modem_lib_readme`
* :ref:`lte_lc_readme`
* :ref:`sntp_interface`

API documentation
*****************

| Header file: :file:`include/date_time.h`
| Source files: :file:`lib/date_time/src/`

.. doxygengroup:: date_time
   :project: nrf
   :members:
