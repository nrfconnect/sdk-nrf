.. _app_behavior_and_functionality:

Application behavior and functionality
######################################

.. contents::
   :local:
   :depth: 2

This section describes the general functioning of the Asset Tracker v2 application.

Data types
**********

Data from multiple sensor sources are collected to construct information about the location, environment, and the health of the nRF9160-based device.
The data types that are collected by the application are listed in the following table:

.. _app_data_types:

+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Data type      | Description                | Identifiers                                   | String identifier for NOD list |
+================+============================+===============================================+================================+
| Location       | Position coordinates       | APP_DATA_LOCATION                             |``gnss``, ``ncell``, ``wifi``   |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Environmental  | Temperature, humidity      | APP_DATA_ENVIRONMENTAL                        | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Movement       | Acceleration               | APP_DATA_MOVEMENT                             | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Modem          | LTE link data, device data | APP_DATA_MODEM_DYNAMIC, APP_DATA_MODEM_STATIC | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Battery        | Battery level              | APP_DATA_BATTERY                              | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+

Additionally, the following data types are supported that provide some asynchronous data:

+----------------+-----------------------------------------------------+
| Data type      | Description                                         |
+================+=====================================================+
| Button         | ID of pressed Button                                |
+----------------+-----------------------------------------------------+
| Impact         | Magnitude of impact in gravitational constant (G)   |
+----------------+-----------------------------------------------------+

.. _real_time_configs:

Real-time configurations
************************

You can alter the behavior of the application at run time by updating the application's real-time configurations through the cloud service.
The real-time configurations supported by the application are listed in the following table:

+------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Real-time Configurations           | Description                                                                                                                          | Default values |
+====================================+======================================================================================================================================+================+
| Device mode                        | Either in active or passive mode.                                                                                                    | Active         |
+----------+-------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|  Active  |                         | Cloud updates occur at regular intervals.                                                                                            |                |
|          +-------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Active wait time        | Number of seconds between each cloud update in active mode.                                                                          | 300 seconds    |
+----------+-------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|  Passive |                         | Cloud updates occur upon movement.                                                                                                   |                |
|          +-------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Movement resolution     | Number of seconds between each cloud update in passive mode, given that the device is moving.                                        | 120 seconds    |
|          +-------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Movement timeout        | Number of seconds between each cloud update in passive mode, regardless of movement.                                                 | 3600 seconds   |
+----------+-------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Location timeout                   | Timeout for location retrieval during data sampling.                                                                                 | 300 seconds    |
|                                    | This value should be large enough so that the location can be retrieved in different conditions.                                     |                |
|                                    | This can be considered more of a safeguard rather than the deadline when the operation must be completed.                            |                |
|                                    | Hence, this value can be larger than the sampling interval.                                                                          |                |
+------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Accelerometer activity threshold   | Accelerometer activity threshold in m/s². Minimal absolute value for accelerometer readings to be considered valid movement.         | 4 m/s²         |
+------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Accelerometer inactivity threshold | Accelerometer inactivity threshold in m/s². Maximal absolute value for accelerometer readings to be considered stillness.            | 4 m/s²         |
+------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Accelerometer inactivity timeout   | Accelerometer inactivity timeout in seconds. Minimum time for lack of movement to be considered stillness.                           | 1 second       |
+------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| No Data List (NOD)                 | A list of strings that references :ref:`data types <app_data_types>`, which will not be sampled by the application.                  | No entries     |
|                                    | Used to disable sampling from sensor sources.                                                                                        | (Request all)  |
|                                    | For instance, when GNSS should be disabled in favor of location based on neighbor cell measurements,                                 |                |
|                                    | the string identifier ``GNSS`` must be added to this list.                                                                           |                |
|                                    | The supported string identifiers for each data type can be found in the :ref:`data types <app_data_types>` table.                    |                |
+------------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+

You can alter the *default* values of the real-time configurations at compile time by setting the options listed in :ref:`Default device configuration options <default_config_values>`.
However, note that these are only the default values.
If a different value set has been set through the cloud service, it takes precedence.
The application also stores its configuration values to non-volatile memory.


The application receives new configurations in one of the following three ways:

* Upon every established connection to the cloud service.
* When the device sends an update to cloud.
* From non-volatile flash memory after boot.

The following flow charts show the operation of the application in the active and passive modes.
The charts show the relationship between data sampling, sending of data, and the real-time configurations.
All configurations that are not essential to this relationship are not included.

.. figure:: /images/asset_tracker_v2_active_state.svg
    :alt: Active mode flow chart

    Active mode flow chart

In the active mode, the application samples and sends data at regular intervals that are set by the ``Active wait timeout`` configuration.

.. figure:: /images/asset_tracker_v2_passive_state.svg
    :alt: Passive mode flow chart

    Passive mode flow chart

In the passive mode, the application samples and sends data upon two occurrences:

* When the timer controlled by the ``Movement resolution`` configuration expires and movement is detected.
* When the timer controlled by the ``Movement timeout`` configuration expires.
  This timer acts as failsafe if no movement is detected for extended periods of time.
  Essentially, it makes sure that the application always sends data at some rate, regardless of movement.

User interface
**************

The application supports basic UI elements to visualize its operating state and to notify the cloud using button presses.
This functionality is implemented in the :ref:`UI module <asset_tracker_v2_ui_module>` and the supported LED patterns are documented in the :ref:`UI module LED indication <led_indication>` section.

A-GPS and P-GPS
***************

The application supports processing of incoming A-GPS and P-GPS data to reduce the GNSS Time-To-First-Fix (`TTFF`_).
Requesting and processing of A-GPS data is a default feature of the application.
See :ref:`nRF Cloud A-GPS and P-GPS <nrfcloud_agps_pgps>` for further details.
To enable support for P-GPS, add the parameter ``-DOVERLAY_CONFIG=overlay-pgps.conf`` to your build command.

.. note::
   Enabling support for P-GPS creates a new flash partition in the image for storing P-GPS data.
   To ensure that the resulting binary can be deployed using FOTA, you must make sure that the new partition layout is compatible with layout of the old image.
   See :ref:`static partitioning <ug_pm_static_providing>` for more details.

   |gps_tradeoffs|
