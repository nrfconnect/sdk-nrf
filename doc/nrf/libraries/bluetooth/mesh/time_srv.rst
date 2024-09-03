.. _bt_mesh_time_srv_readme:

Time Server
###########

.. contents::
   :local:
   :depth: 2

The Time Server model allows mesh nodes to synchronize the current time and date by publishing and subscribing to Time Status messages.

The Time Server model relies on an external clock source to provide the current time and date, but takes care of distributing this information through the network.

The Time Server model adds the following new model instances in the composition data:

* Time Server
* Time Setup Server

Operation
=========

The Time Server model builds an ad-hoc time synchronization hierarchy through the mesh network that propagates the current wall-clock time as TAI timestamps.

To ensure consistent synchronization, the Time Server model should be configured to publish its Time Status with regular intervals using a :ref:`zephyr:bluetooth_mesh_models_cfg_cli` model.

.. note::

   To ensure a high level of accuracy, Time Status messages are always transmitted one hop at a time if it is sent as an unsolicited message.
   Every mesh node without access to the Time Authority that needs access to the common time reference must be within the radio range of at least one other node with the Time Server model.
   Time Server nodes that have no neighboring Time Servers need to get this information in some other way (for example, getting time from the Time Authority).

Roles
*****

The Time Server model operates in 3 different roles:

Time Authority
   A Time Server that has access to an external clock source, and can propagate the current time to other nodes in the network.
   The Time Authority will publish Time Status messages, but will not receive them.

Time Relay
   A Time Server that both publishes and receives Time Status messages.
   The Time Relay relies on other mesh nodes to tell time.

Time Client
   A Time Server that receives Time Status messages, but does not relay them.
   The Time Client relies on other mesh nodes to tell time.

Whenever a Time Server model that is not a Time Authority receives a Time Status message, it will update its internal clock to match the new Time Status - if it improves its clock uncertainty.

The Time Authority role should not be confused with the Authority state, which denotes whether the device is actively monitoring a reliable external time source.

Clock uncertainty
*****************

The Time Server's time synchronization mechanism is not perfect, and every timestamp it provides has some uncertainty.
The clock uncertainty is measured in milliseconds, and can come from various sources, like inaccuracies in the original clock source, internal clock drift or uncertainty in the mesh stack itself.
The Time Server inherits the uncertainty of the node it got its Time Status from, and the clock uncertainty of each mesh node increases for every hop the Time Status had to make from the Time Authority.

The expected uncertainty in the mesh stack comes from the fluctuation in time to encrypt a message and broadcast it, and can be configured through the :kconfig:option:`CONFIG_BT_MESH_TIME_MESH_HOP_UNCERTAINTY`.

In addition to the uncertainty added by the mesh stack itself, the internal clock of each device will increase the uncertainty of the timestamp over time.
As crystal hardware is not perfectly accurate, it will gradually drift away from the correct time.
The potential range of this clock drift is generally a known property of the onboard hardware, even if the actual drift fluctuates.
As it is impossible to measure the actual clock drift at any given moment, the Time Server gradually increases the uncertainty of its timestamp based on the known clock accuracy.

The Time Server's notion of the local clock accuracy can be configured with :kconfig:option:`CONFIG_BT_MESH_TIME_SRV_CLOCK_ACCURACY`.
By default, it uses the configured kernel clock control accuracy.

Leap seconds and time zones
***************************

As described in the section on :ref:`bt_mesh_time_tai_readme`, the Time Server measures time as TAI seconds, and adds the UTC leap second offset and local time zone as additional information.

Both leap seconds and time zone changes are irregular events that are scheduled by governing boards ahead of time.
Because of this, it is impossible to algorithmically determine the time zone and UTC offset for future dates, as there is no way to tell when and how many changes will be scheduled.

.. note::
   The time zone of the device not only changes with its location on the globe, but also with Daylight Saving Time.
   For instance, Central Europe is in the UTC+2 (CEST) time zone during the summer, but the UTC+1 (CET) time zone during the winter.

Time Servers' Time Status messages include the current time zone and UTC offset, but Time models may also distribute information about known future changes to these states.
For instance, if a Time Authority node learns through its time source that the device will change to Daylight Saving Time on March 29th, it can broadcast a Time Zone change message, which includes the new time zone offset as well as the TAI timestamp of the change.
All Time Server models that receive this message will automatically store this change and notify the application.
The application can then reschedule any timeouts that happen after the change to reflect the new offset.

Timestamp conversion
********************

To convert between human-readable time and device time, the Time Server model API includes three functions with signatures similar to the C standard library's :file:`time.h` API:

* :cpp:func:`bt_mesh_time_srv_mktime`: Get the uptime at a specific date/time.
* :cpp:func:`bt_mesh_time_srv_localtime`: Get the local date/time at a specific uptime.
* :cpp:func:`bt_mesh_time_srv_localtime_r`: A thread safe version of ``localtime``.

For example, if you want to schedule your mesh device to send up fireworks exactly at midnight on New Year's Eve, you can use ``mktime`` to find the device uptime at this exact timestamp:

.. code-block:: c

   void schedule_fireworks(void)
   {
      struct tm new_years_eve = {
         .tm_year = 2021 - 1900, /* struct tm measures years since 1900 */
         /* January 1st: */
         .tm_mon = 0,
         .tm_mday = 1,
         /* Midnight: */
         .tm_hour = 0,
         .tm_min = 0,
         .tm_sec = 0,
      };

      int64_t uptime = bt_mesh_time_srv_mktime(&time_srv, &new_years_eve);
      if (uptime < 0) {
         /* Time Server does not know */
         return;
      }

      k_timer_start(&start_fireworks, uptime - k_uptime_get(), 0);
   }

And, to print the current date and time, you can use ``localtime``:

.. code-block:: c

   void print_datetime(void)
   {
      struct tm *today = bt_mesh_time_srv_localtime(&time_srv, k_uptime_get());
      if (!today) {
         /* Time Server does not know */
         return;
      }

      const char *weekdays[] = {
         "Sunday",
         "Monday",
         "Tuesday",
         "Wednesday",
         "Thursday",
         "Friday",
         "Saturday",
      };

      printk("Today is %s %04u-%02u-%02u\n", weekdays[today->tm_wday],
            today->tm_year + 1900, today->tm_mon + 1, today->tm_mday);
      printk("The time is %02u:%02u\n", today->tm_hour, today->tm_min);
   }

Additionally, the Time Server API includes :cpp:func:`bt_mesh_time_srv_uncertainty_get`, which allows the application to determine the current uncertainty of a specific uptime.
This function can be used in combination with the three others to determine the accuracy of a provided timestamp.

.. note::
   All time and uncertainty conversion is based on the Time Server's current data, and assumes that no corrections are made between the call and the provided timestamp.
   Timestamps that are weeks or months into the future may have an uncertainty of several hours, due to clock drift.
   The application can subscribe to changes in the Time Server state through the :cpp:type:`bt_mesh_time_srv_cb` callback structure.

   Any time zone or UTC delta changes are taken into account.

Configuration
=============

The clock uncertainty of the Time Server model can be configured with the following configuration options:

* :kconfig:option:`CONFIG_BT_MESH_TIME_MESH_HOP_UNCERTAINTY`: The amount of uncertainty introduced in the mesh stack through sending a single message, in milliseconds.
* :kconfig:option:`CONFIG_BT_MESH_TIME_SRV_CLOCK_ACCURACY`: The largest possible clock drift introduced by the kernel clock's hardware, in parts per million.

States
======

The Time Server model contains the following states:

TAI time: :cpp:type:`bt_mesh_time_tai`
   The TAI time is a composite state, with members ``sec`` and an 8-bit ``subsec``.
   If the current time is known, the TAI time changes continuously.

Uncertainty: ``uint64_t``
   Current clock uncertainty in milliseconds.
   Without new data, clock uncertainty increases gradually due to clock drift.

UTC delta: ``int16_t``
   Number of seconds between the TAI and UTC timestamps due to UTC leap seconds.

Time zone offset: ``int16_t``
   Local time zone offset in 15-minute increments.

Authority: ``bool``
   Whether this device has continuous access to a reliable TAI source, such as a GPS receiver or an NTP-synchronized clock.
   The Authority state does not transfer to other devices.

Role: :cpp:enum:`bt_mesh_time_role`
   The Time Server's current role in the Time Status propagation.

Time zone change: :cpp:type:`bt_mesh_time_zone_change`
   The Time zone change state determines the next scheduled change in time zones, and includes both the new time zone offset and the timestamp of the scheduled change.
   If no change is known, the timestamp is 0.

UTC delta change: :cpp:type:`bt_mesh_tai_utc_change`
   The UTC delta change state determines the next scheduled leap second, and includes both the new UTC offset and the timestamp of the scheduled change.
   If no change is known, the timestamp is 0.

Extended models
===============

None.

Persistent storage
==================

The Timer Server stores the following states persistently:

* Role
* Time zone change
* UTC delta change

All other states change with time, and are not stored.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/time_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/time_srv.c`

.. doxygengroup:: bt_mesh_time_srv
   :project: nrf
   :members:
