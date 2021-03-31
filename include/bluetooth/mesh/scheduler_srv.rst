.. _bt_mesh_scheduler_srv_readme:

Scheduler Server
################

.. contents::
   :local:
   :depth: 2

The Scheduler Server model holds the current state of the Scheduler Register, and performs scheduling of the configured actions.
The model notifies appropriate :ref:`bt_mesh_scene_srv_readme` and :ref:`bt_mesh_onoff_srv_readme` instances about actions in progress.

The Scheduler Server relies on the :ref:`bt_mesh_time_srv_readme` that is provided during initialization.
However, it is the application that should handle the Time Server events and notify the Scheduler Server if the UTC time or the Time Zone have changed.

The Scheduler Server model adds two model instances in the composition data:

* Scheduler Server
* Scheduler Setup Server

The two model instances share the states of the Scheduler Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Scheduler Server states, as the two model instances can be bound to different application keys.

* Scheduler Server manages the scheduled actions.
* Scheduler Setup Server provides functionality for configuration and recalling of actions in the Scheduler Server's Scheduler Register state.

Operation
*********

The Scheduler models perform conversion of the configuration parameters from incoming client messages into :ref:`international atomic time (TAI) <bt_mesh_time_tai_readme>`.
The configuration parameters with calculated time closest to the current time are scheduled as actions.
If an action requires rescheduling when the scheduled time has expired, the Scheduler Server calculates new time and repeats the scheduling procedure.
However, the Scheduler Server skips configuration parameters not allowing to calculate the exact time of the action.
Such actions will never be executed.

When the scheduled action is executed, the Scheduler Server sends a notification about the action in progress to appropriate Scene or OnOff Server instances, starting with the same element on which the Scheduler Server is present.
The Scene or OnOff Server instances are notified until the Scheduler Server reaches another element with a Scheduler Server instance, or there are just no elements left.
The Scene or OnOff Servers publish their states if the states have changed.

The application should notify the Scheduler Server if the Time Zone or the UTC time has changed, using the Scheduler Server API:

* :c:func:`bt_mesh_scheduler_srv_time_update`: Notify server about UTC time or Time Zone changes

The Scheduler Server will recalculate time for the actions.

States
******

Schedule Register: :c:type:`bt_mesh_schedule_entry`
   The Scheduler Server model offers a Schedule Register that contains up to sixteen registered action entries.

Each entry has the following fields:

* Year: 2 least significant digits of the year

  +---------------------------------------+----------------------------------------+
  | Value                                 | Description                            |
  +---------------------------------------+----------------------------------------+
  | 0x00-0x63                             | 2 least significant digits of the year |
  +---------------------------------------+----------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ANY_YEAR` | Any year                               |
  +---------------------------------------+----------------------------------------+
  | All other values                      | Prohibited                             |
  +---------------------------------------+----------------------------------------+

* Month: Bitmask of the months in the year in which the scheduled event is enabled :c:enum:`bt_mesh_scheduler_month`

* Day: The day of the month the scheduled event occurs

  +--------------------------------------+------------------+
  | Value                                | Description      |
  +--------------------------------------+------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ANY_DAY` | Any day          |
  +--------------------------------------+------------------+
  | 0x01-0x1F                            | Day of the month |
  +--------------------------------------+------------------+

* Hour: The hour when the scheduled event occurs

  +-----------------------------------------+----------------------------------+
  | Value                                   | Description                      |
  +-----------------------------------------+----------------------------------+
  | 0x00-0x17                               | Hour of the day (00 to 23 hours) |
  +-----------------------------------------+----------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ANY_HOUR`   | Any hour of the day              |
  +-----------------------------------------+----------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ONCE_A_DAY` | Once a day (at a random hour)    |
  +-----------------------------------------+----------------------------------+
  | All other values                        | Prohibited                       |
  +-----------------------------------------+----------------------------------+

* Minute: The minute when the scheduled event occurs

  +-----------------------------------------------+----------------------------------------------------------+
  | Value                                         | Description                                              |
  +-----------------------------------------------+----------------------------------------------------------+
  | 0x00-0x3B                                     | Minute of the hour (00 to 59)                            |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ANY_MINUTE`       | Any minute of the hour                                   |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_EVERY_15_MINUTES` | Every 15 minutes (minute modulo 15 is 0) (0, 15, 30, 45) |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_EVERY_20_MINUTES` | Every 20 minutes (minute modulo 20 is 0) (0, 20, 40)     |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ONCE_AN_HOUR`     | Once an hour (at a random minute)                        |
  +-----------------------------------------------+----------------------------------------------------------+

* Second: The second when the scheduled event occurs

  +-----------------------------------------------+----------------------------------------------------------+
  | Value                                         | Description                                              |
  +-----------------------------------------------+----------------------------------------------------------+
  | 0x00-0x3B                                     | Second of the minute (00 to 59)                          |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ANY_SECOND`       | Any second of the minute                                 |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_EVERY_15_SECONDS` | Every 15 seconds (second modulo 15 is 0) (0, 15, 30, 45) |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_EVERY_20_SECONDS` | Every 20 seconds (second modulo 20 is 0) (0, 20, 40)     |
  +-----------------------------------------------+----------------------------------------------------------+
  | :c:macro:`BT_MESH_SCHEDULER_ONCE_A_MINUTE`    | Once a minute (at a random second)                       |
  +-----------------------------------------------+----------------------------------------------------------+

* DayOfWeek: Bitmask of the days of the week when the scheduled event is enabled :c:enum:`bt_mesh_scheduler_wday`

* Action: Action to be executed for a scheduled event :c:enum:`bt_mesh_scheduler_action`

* Transition Time: Transition time for the action

  * Step count: 6 bits (range `0x00` to `0x3e`)
  * Step resolution: 2 bits

* Scene Number: Scene number to be used for the action

  +------------------+--------------+
  | Value            | Description  |
  +------------------+--------------+
  | 0x0000           | No scene     |
  +------------------+--------------+
  | All other values | Scene number |
  +------------------+--------------+

Extended models
***************

The Scheduler Server is implemented as a root model.
When a Scheduler Server model is present on an element, the Scene Server model (see the :ref:`bt_mesh_scene_srv_readme` documentation) shall also be present on the same element.

Persistent storage
******************

The Scheduler Server stores the following information:

* Any changes to the Schedule Register state

This information is used to restore previously configured register entries when the device powers up.

The scheduler operation depends on the availability of the updated current time provided by the Time Server.
It is the application's responsibility to call :c:func:`bt_mesh_scheduler_srv_time_update` after the current local
time has been updated to schedule available entries correctly.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/scheduler_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/scheduler_srv.c`

.. doxygengroup:: bt_mesh_scheduler_srv
   :project: nrf
   :members:
