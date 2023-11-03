.. _gnss_interface:

GNSS interface
##############

.. contents::
   :local:
   :depth: 2

`Global Navigation Satellite System (GNSS)`_ interface in the Modem library is used to control the GNSS module.
The interface configures and fetches data from the GNSS module and writes `A-GNSS`_ data to the GNSS module.

.. note::

   You need to enable GNSS in modem system mode and activate it in modem functional mode before starting or configuring the GNSS module.

Handling events and reading data from GNSS
******************************************

To handle events from the GNSS interface, the application needs to implement an event handler function.
This function is then called by the interface whenever there is a new event and possibly associated data available.

.. code-block:: c

   static struct nrf_modem_gnss_pvt_data_frame pvt_data;

   static void gnss_event_handler(int event_id)
   {
       int err;

       /* Process event */
       switch (event_id) {
       case NRF_MODEM_GNSS_EVT_PVT:
           /* Read PVT data */
           err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
       ...
   }

The event handler function is set by calling the :c:func:`nrf_modem_gnss_event_handler_set` function.

.. code-block:: c

   err = nrf_modem_gnss_event_handler_set(gnss_event_handler);

The event handler is called in interrupt service routine (ISR) context, so processing in the handler should be kept to a minimum.
Data can be read using function :c:func:`nrf_modem_gnss_read` at any time after receiving the event.
Because there is no buffering for the data, it is overwritten as soon as an event with the same data type is received.
This is not an issue with PVT data, which is updated only once a second, but to receive all NMEA strings, the application needs to read the data inside the event handler.

Starting GNSS
*************

GNSS is started by calling the :c:func:`nrf_modem_gnss_start` function.
When GNSS is started, the application starts receiving events in the registered event handler function.

.. code-block:: c

   err = nrf_modem_gnss_start();

Stopping GNSS
*************

GNSS is stopped by calling the :c:func:`nrf_modem_gnss_stop` function.

.. code-block:: c

   err = nrf_modem_gnss_stop();

Deleting GNSS data stored in NV memory
**************************************

When GNSS is running, it stores information into non-volatile (NV) memory.
GNSS uses this information when GNSS is restarted and also when GNSS starts after a device reboot.
It is possible to delete stored data to simulate for example GNSS warm or cold starts.
The data to be deleted is selected using a bitmap.

.. code-block:: c

   uint32_t delete_mask;

   delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES |
                 NRF_MODEM_GNSS_DELETE_ALMANACS |
                 NRF_MODEM_GNSS_DELETE_IONO_CORRECTION_DATA |
                 NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX |
                 NRF_MODEM_GNSS_DELETE_GPS_TOW |
                 NRF_MODEM_GNSS_DELETE_GPS_WEEK |
                 NRF_MODEM_GNSS_DELETE_UTC_DATA |
                 NRF_MODEM_GNSS_DELETE_GPS_TOW_PRECISION;

   err = nrf_modem_gnss_nv_data_delete(delete_mask);

.. note::

   TCXO offset is a slowly changing characteristic of each device.
   It should typically not be deleted when simulating a cold start.

   This is considered a debug feature, and is not supposed to be used in production code.

Configuring GNSS
****************

GNSS has various parameters you can use to configure the GNSS behavior.
You can only set the configuration options when GNSS is not running.

Operation mode
==============

GNSS supports different operation modes.
The operation mode is configured using functions :c:func:`nrf_modem_gnss_fix_interval_set` and :c:func:`nrf_modem_gnss_fix_retry_set`.

The default operation mode is continuous navigation.

Single fix
----------

In single fix mode, the GNSS receiver is on until it has produced a valid PVT estimate.
After that, it is automatically switched off.

Even though the GNSS receiver is switched off after producing a fix, the :c:func:`nrf_modem_gnss_stop` function still needs to be called before GNSS can be started again.

To enable single fix navigation, set the fix interval to 0.
If the fix retry parameter is non-zero, GNSS stops after the fix retry time is up if a valid PVT estimate has not been produced.
If the fix retry parameter is set to zero, GNSS is allowed to run indefinitely until a valid PVT estimate is produced.

.. code-block:: c

   err = nrf_modem_gnss_fix_interval_set(0);
   ...
   err = nrf_modem_gnss_fix_retry_set(180);

Continuous navigation
---------------------

In continuous navigation mode, GNSS receiver is on continuously and produces PVT estimates at 1 Hz rate.

To enable continuous navigation, set the fix interval to 1.
The fix retry parameter has no effect in this mode even if it is set to a non-zero value.

.. code-block:: c

   err = nrf_modem_gnss_fix_interval_set(1);
   ...
   err = nrf_modem_gnss_fix_retry_set(0);

Periodic navigation
-------------------

In periodic navigation mode, the fix interval indicates how often GNSS tries to produce a valid PVT estimate.
In this mode, the GNSS receiver is turned off after each valid PVT estimate, and turned back on periodically after each fix interval has passed.

To enable periodic navigation, set the fix interval to 10...65535.
If the fix retry parameter is non-zero, GNSS stops after the fix retry time is up if a valid PVT estimate has not been produced.
If the fix retry parameter is set to zero, GNSS is allowed to run indefinitely until a valid PVT estimate is produced.

.. code-block:: c

   err = nrf_modem_gnss_fix_interval_set(600);
   ...
   err = nrf_modem_gnss_fix_retry_set(180);

.. note::

   Unless disabled using the :c:func:`nrf_modem_gnss_use_case_set` function, GNSS performs :term:`Scheduled downloads` in periodic navigation mode.
   During a scheduled download, the fix interval and fix retry parameters are temporarily ignored.
   After GNSS has downloaded the data it needs, normal operation is resumed.

System mask
===========

System mask controls which GNSSs are enabled.
The system mask is set using the :c:func:`nrf_modem_gnss_system_mask_set` function by providing a bitmap of the selected systems.

By default, all supported GNSSs are enabled.

GPS cannot be disabled and it remains enabled even if the corresponding bit is not set.

.. code-block:: c

   uint8_t system_mask;

   system_mask = NRF_MODEM_GNSS_SYSTEM_GPS_MASK | NRF_MODEM_GNSS_SYSTEM_QZSS_MASK;

   err = nrf_modem_gnss_system_mask_set(system_mask);

Satellite elevation threshold
=============================

Satellite elevation threshold controls below which elevation angle (degrees above the horizon) GNSS stops tracking a satellite.
The elevation threshold is set using the :c:func:`nrf_modem_gnss_elevation_threshold_set` function.

The default value is 5 degrees.

.. code-block:: c

   err = nrf_modem_gnss_elevation_threshold_set(5);

Use case
========

The use case configuration is used to enable use case specific features.
The use case bitmask is set using the :c:func:`nrf_modem_gnss_use_case_set` function.

Start mode
----------

Currently, the only supported start mode is optimized for multiple hot starts.
This is enabled by default and does not need to be set using a function.
However, whenever the :c:func:`nrf_modem_gnss_use_case_set` function is called, the bit :c:data:`NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START` should be set.

.. _gnss_int_low_accuracy_mode:

Low accuracy mode
-----------------

If low accuracy mode is enabled, GNSS demonstrates a looser acceptance criterion for a fix.
The error in position calculation, when compared to the actual position, can be larger than in normal accuracy mode.
In addition, GNSS might use only three satellites to determine a fix.
In normal accuracy mode, four or more satellites are used.

For a possible position fix using only three satellites, GNSS must have a reference altitude that has low enough uncertainty.
The reference altitude is obtained from one of the following sources:

* A GNSS fix using five or more satellites.
* A-GNSS assistance data - The assistance data is injected to GNSS using the :c:type:`nrf_modem_gnss_agnss_data_location` A-GNSS data location struct, as shown in the following example code:

  .. code-block:: c

     struct nrf_modem_gnss_agnss_data_location location;

     location.latitude          = latitude; /* Best estimate within maximum limit of 1800 km. */
     location.longitude         = longitude;/* Best estimate within maximum limit of 1800 km. */
     location.altitude          = altitude; /* Actual altitude of the device in meters. */
     location.unc_semimajor     = 127;      /* Uncertainty, semi-major. Range 0...127 or 255. */
     location.unc_semiminor     = 127;      /* Uncertainty, semi-minor. Range 0...127 or 255. */
     location.orientation_major = 0;        /* Set to 0 if unc_semimajor and unc_semiminor are identical values. */
     location.unc_altitude      = 0;        /* Uncertainty, altitude. Range 0...127 or 255. */
     location.confidence        = 100;      /* Set to 100 for maximum confidence. */

     err = nrf_modem_gnss_agnss_write(&location, sizeof(location), NRF_MODEM_GNSS_AGNSS_LOCATION);

 The struct contains the geodetic latitude, longitude (WGS-84 format), and altitude (in meters) parameters.
 The uncertainties for the latitude, longitude (unc_semimajor and unc_semiminor), and for the altitude (unc_altitude) are given as an index from ``0`` to ``127``, see :file:`nrf_modem_gnss.h` for the encoding of the uncertainty fields.

 The altitude uncertainty must be less than 100 meters (index less than ``48``) for it to be valid as a reference altitude.
 The accuracy of the latitude and longitude are less important, but it must be within 1800 kilometers of the actual location if the coordinates are given.
 It is also possible to inject only the altitude without a known latitude and longitude.
 In this case, unc_semimajor and unc_semiminor are set to ``255`` to indicate that latitude and longitude are not valid.

If both verified GNSS fix (five or more satellites used in earlier fix) and A-GNSS assistance data are available, the altitude from the verified GNSS fix is used.

Thus, if GNSS has started in the low accuracy mode, it will not be able to produce fixes using three satellites until it has a reference altitude from one of the mentioned sources.
Over time, the uncertainty of the reference altitude increases unless a GNSS fix is obtained using five or more satellites, or altitude assistance is injected to GNSS.
See :ref:`ref_alt_exp_evt` for GNSS indication of reference altitude expiry.

.. note::

   Calling the :c:func:`nrf_modem_gnss_nv_data_delete` function with :c:data:`NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX` bit set clears the reference altitude value.

.. important::

   The altitude must be accurate to a value within ±10 meters of the actual altitude of the device.
   An erroneous altitude will result in a severe error in the PVT estimation using three satellites.

If the actual altitude of the device changes with respect to the altitude stored in GNSS (for example, when the device moves around), the accuracy of the position fix using three satellites will be degraded.

All fixes, including the low accuracy fixes, are reported as 3D fixes.
See the `NMEA report sample`_ and number of IDs of SVs used in the position fix to get information of the number of satellites that are used for the position fix.

The low accuracy mode can be enabled as shown in the following example:

.. code-block:: c

   uint8_t use_case;

   use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START | NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;

   err = nrf_modem_gnss_use_case_set(use_case);

.. _ref_alt_exp_evt:

Reference altitude expiration event
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

GNSS sends the event :c:data:`NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED` when the reference altitude expires.
This event can be used to trigger a reference altitude update whenever it is needed.

NMEA mask
=========

NMEA mask is used to enable different NMEA string.
Multiple NMEA strings can be enabled at the same time.

By default, all NMEA strings are disabled.

All NMEA strings can be enabled as shown in the following example:

.. code-block:: c

   uint16_t nmea_mask;

   nmea_mask = NRF_MODEM_GNSS_NMEA_GGA_MASK |
               NRF_MODEM_GNSS_NMEA_GLL_MASK |
               NRF_MODEM_GNSS_NMEA_GSA_MASK |
               NRF_MODEM_GNSS_NMEA_GSV_MASK |
               NRF_MODEM_GNSS_NMEA_RMC_MASK;

   err = nrf_modem_gnss_nmea_mask_set(nmea_mask);

Power saving mode
=================

In continuous navigation, two different power saving modes are available to lower the power consumption.
Power saving is implemented as duty-cycling.
When GNSS engages duty-cycled tracking, it only tracks for 20% of time and spends the rest of the time in sleep.
The different modes control how aggressively GNSS engages duty-cycled tracking, but the duty-cycling itself is the same with both modes.

In the duty-cycling performance mode, duty-cycled tracking is engaged when it can be done without significant performance degradation.
In the duty-cycling power mode, duty-cycled tracking is engaged more aggressively with acceptable performance degradation.

The default value is :c:data:`NRF_MODEM_GNSS_PSM_DISABLED`.

.. code-block:: c

   err = nrf_modem_gnss_power_mode_set(NRF_MODEM_GNSS_PSM_DUTY_CYCLING_POWER);

.. _sleep_timing_source:

Sleep timing source
===================

Timing source used during GNSS sleep periods can be selected between RTC and TCXO.
Using TCXO instead of RTC during GNSS sleep periods might be beneficial when used with 1PPS.
When GNSS is not running all the time (periodic navigation or duty-cycling is used), 1PPS accuracy can be improved by using TCXO.
It may also improve sensitivity for periodic navigation when the fix interval is short.

The default value is :c:data:`NRF_MODEM_GNSS_TIMING_SOURCE_RTC`.

.. code-block:: c

   err = nrf_modem_gnss_timing_source_set(NRF_MODEM_GNSS_TIMING_SOURCE_TCXO);

.. note::

   Use of TCXO significantly raises the idle current consumption.

QZSS configuration
==================

GNSS has configuration options that can be used to change the QZSS-related behavior.

NMEA mode
---------

QZSS NMEA mode controls whether QZSS satellites are reported in NMEA strings or not.
The NMEA 4.10 standard does not support QZSS satellites, so in the standard NMEA mode, QZSS satellites are not reported in GPGSA and GPGSV sentences.
In custom NMEA mode, satellite IDs 193...202 are used for QZSS satellites.

The default value is :c:data:`NRF_MODEM_GNSS_QZSS_NMEA_MODE_STANDARD`.

.. code-block:: c

   err = nrf_modem_gnss_qzss_nmea_mode_set(NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM);

PRN mask
--------

QZSS satellite acquisition and tracking can be configured for each satellite using QZSS PRN mask.
Bits 0...9 correspond to QZSS PRNs 193...202 respectively.
When a bit is set, using the corresponding QZSS satellite is enabled.
Bits 10...15 are reserved and their value is ignored.

The default PRN mask follows the anticipated development of the QZSS constellation and may differ between modem firmware versions.

QZSS PRNs 194, 195, 196 and 199 can be enabled (and others disabled) as shown in the following example:

.. code-block:: c

   err = nrf_modem_gnss_qzss_prn_mask_set(0x4e);

Enabling GNSS priority mode
***************************

GNSS can be given priority over LTE idle mode procedures to help getting a fix.
Usually, this is not necessary when either eDRX or PSM (or both) is used, but if that is not possible, the GNSS priority mode may be used.

Priority for GNSS should be used only when a fix has been blocked by LTE idle mode operations, which can be detected by :c:data:`NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME` bit being set in the PVT data frame flags member.
The application should not make the decision based on a single PVT event, but should enable priority only in case this flag has been set in several consecutive PVT events.

Priority mode is disabled automatically after the first fix or after 40 seconds.
It can also be disabled by the application by calling the :c:func:`nrf_modem_gnss_prio_mode_disable` function.

.. note::

   GNSS priority may interfere with LTE operations.
   If possible, it would be good to time the use of priority to moments where data transfer is not anticipated.
   In general, eDRX cycles that are long enough, or PSM, ensure better functionality for both GNSS and LTE.

.. code-block:: c

   err = nrf_modem_gnss_prio_mode_enable();

Changing the dynamics mode
**************************

The dynamics mode describes the dynamics model for the receiver.
General purpose mode is suitable for a wide range of applications, but using a dynamics mode tuned for a specific use case improves the positioning performance.
The :ref:`nrf_modem_gnss_api` lists the maximum receiver speed used for predicting satellite visibility and Doppler frequencies in each mode, and for limiting receiver movement between fixes.
However, these are not hard limits and GNSS will continue working at higher speeds, but with sub-optimal predictions and sub-optimal position computation and filtering.

The dynamics mode can be changed without disruption in positioning.
The selected dynamics mode is stored into the non-volatile memory.

The default value is :c:data:`NRF_MODEM_GNSS_DYNAMICS_GENERAL_PURPOSE`.

.. code-block:: c

   err = nrf_modem_gnss_dyn_mode_change(NRF_MODEM_GNSS_DYNAMICS_AUTOMOTIVE);

1PPS
****

GNSS can provide time synchronized electrical pulses to the COEX1 pin.
The rising edge of the pulse is aligned as closely as possible to the GPS time second.

The pulse interval and width are configurable.
It is also possible to configure the pulses to start at a specific date and time.
Instead of repeating pulses, 1PPS can also be used in a one-time pulse mode, where only a single pulse is given at the specified time or as soon as GNSS gets a fix.

GNSS only starts giving pulses after it has got at least one fix.
After this, the pulses will continue also when GNSS is no longer running, but the precision will start degrading.

In cases where GNSS is not running continuously, it may be beneficial to change the timing source used by GNSS during sleep periods, see :ref:`sleep_timing_source`.

1PPS can be enabled or disabled only when GNSS is not running.
1PPS can be enabled with a 1 s pulse interval and 100 ms pulse width as shown in the following example:

.. code-block:: c

   struct nrf_modem_gnss_1pps_config config = {
       .pulse_interval = 1,
       .pulse_width = 100,
       .apply_start_time = false
   };

   err = nrf_modem_gnss_1pps_enable(&config);

.. note::

   The 1PPS feature must not be used when LTE is enabled.

   The application must make sure that LTE is not enabled in functional mode when 1PPS is enabled.
   The application needs to call the :c:func:`nrf_modem_gnss_1pps_disable` function before activating LTE by setting the functional mode to ``1`` (full functionality), ``2`` (receive only) or ``21`` (activate LTE).
   However, LTE can remain enabled in system mode configuration to avoid the need to change between system modes when a switch between LTE connectivity and time pulse output is desired.

Resolving the UTC time of 1PPS pulse occurrence
===============================================

As the time of the pulse (aligned to the top of an UTC second) is calculated from the previous valid PVT fix, the latest PVT fix notification needs to be used when resolving the UTC time of the 1PPS pulse.
While the 1PPS pulse does not have any time delay, the PVT fix notification will always have some delay, both from the PVT solution calculation (approximately 100 ms) and the notification message delivery.
Therefore, the 1PPS pulse may come before the PVT notification that was used to calculate the exact pulse time.

The UTC time of the 1PPS pulse can be calculated using the following formula:

.. math::

   round\ to\ nearest\ second\ (t_{(PVT,GPST)}+∆t+100 ms)

t\ :sub:`(PVT,GPST)` \ is the GPS time stamp in the previous PVT notification and |delta| t is the time difference between the 1PPS pulse and the reception of previous PVT notification:

.. |delta| unicode:: 0x394 .. capital delta sign
   :rtrim:

.. math::

   ∆t=t_P-t_{PVT}

Thus, |delta| t is always positive.

.. _gnss_int_agps_data:

A-GNSS data
***********

You can use GNSS assistance data to shorten TTFF and decrease GNSS power consumption.
See :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` for information how to obtain assistance data from :ref:`lib_nrf_cloud` to be used with the nRF91 Series SiPs.

.. _gnss_int_assistance_need:

Determining the assistance data needed by GNSS
==============================================

The GNSS interface has two methods for getting the current GNSS assistance data need.

A-GNSS data need event
----------------------

GNSS requests A-GNSS data when GNSS is started for the first time or it determines that the existing data will expire soon.
Whenever A-GNSS data is needed, GNSS sends the :c:data:`NRF_MODEM_GNSS_EVT_AGNSS_REQ` event.
The payload for this event contains information about what kind of data is needed.

When the event is received, the associated payload can be read like this:

.. code-block:: c

   struct nrf_modem_gnss_agnss_data_frame agnss_data;

   err = nrf_modem_gnss_read(&agnss_data, sizeof(agnss_data), NRF_MODEM_GNSS_DATA_AGNSS_REQ);

After reading the data successfully, the ``system`` array in the struct contains bitmasks ``sv_mask_ephe`` and ``sv_mask_alm`` for each supported system, which indicate the need for ephemerides and almanacs for each satellite in the system.
The ``data_flags`` member is a bitmask for other A-GNSS data.

To prevent triggering assistance data download too often, GNSS sends the :c:data:`NRF_MODEM_GNSS_EVT_AGNSS_REQ` event at most once an hour.

A-GNSS data expiry query
------------------------

You can use the :c:func:`nrf_modem_gnss_agnss_expiry_get` function to query assistance data need at any time.

The assistance data need can be read like this:

.. code-block:: c

   struct nrf_modem_gnss_agnss_expiry agnss_expiry;

   err = nrf_modem_gnss_agnss_expiry_get(&agnss_expiry);

After reading the data successfully, the struct contains expiration times for different types of assistance data.
The ``sv`` array contains ephemeris and almanac expiration times for each GNSS satellite.
The number of satellites in the array depends on the modem firmware.
For older modem firmwares, the array only contains GPS satellites, but from modem firmware v2.0.0 onwards, it also contains QZSS satellites.
The struct also contains expiration times, for example, for Klobuchar ionospheric corrections and satellite integrity assistance.
The ``data_flags`` member contains a bitmask for certain data types, indicating whether the data is currently needed.

Injecting assistance data
=========================

A-GNSS data is injected into GNSS using the :c:func:`nrf_modem_gnss_agnss_write` function.
Each data type has its own struct that is used when A-GNSS data is written to GNSS.

For example, GPS UTC parameters can be written to GNSS as shown in the following example:

.. code-block:: c

   struct nrf_modem_gnss_agnss_gps_data_utc utc_data;

   /* Populate struct with data */
   utc_data.a1 = ...

   err = nrf_modem_gnss_agnss_write(&utc_data, sizeof(utc_data), NRF_MODEM_GNSS_AGNSS_GPS_UTC_PARAMETERS);
