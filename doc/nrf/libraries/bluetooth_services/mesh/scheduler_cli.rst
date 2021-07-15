.. _bt_mesh_scheduler_cli_readme:

Scheduler Client
################

.. contents::
   :local:
   :depth: 2

The Scheduler Client model remotely controls the state of a :ref:`bt_mesh_scheduler_srv_readme` model.

Unlike the Scheduler Server model, the Scheduler Client only creates a single model instance in the mesh composition data.
The Scheduler Client can send messages to both the Scheduler Server and the Scheduler Setup Server, as long as it has the right application keys.

Action scheduling example
*************************

This simple example shows how the Scheduler Client can configure the Scheduler Server models managing the light system.
Every year, and every working day throughout the year, the Scheduler Server will turn the light on at 8:00 and turn it off again at 16:00.

.. code-block:: c

   static int schedule_on_weekdays(enum bt_mesh_scheduler_action action,
                                   uint8_t idx, uint8_t hour, uint8_t minute)
   {
      const struct bt_mesh_schedule_entry entry = {
         .action = action,
         .minute = minute,
         .hour = hour,
         .day = BT_MESH_SCHEDULER_ANY_DAY,
         .month = (BT_MESH_SCHEDULER_JAN | BT_MESH_SCHEDULER_FEB |
                   BT_MESH_SCHEDULER_MAR | BT_MESH_SCHEDULER_APR |
                   BT_MESH_SCHEDULER_MAY | BT_MESH_SCHEDULER_JUN |
                   BT_MESH_SCHEDULER_JUL | BT_MESH_SCHEDULER_AUG |
                   BT_MESH_SCHEDULER_SEP | BT_MESH_SCHEDULER_OCT |
                   BT_MESH_SCHEDULER_NOV | BT_MESH_SCHEDULER_DEC),
         .year = BT_MESH_SCHEDULER_ANY_YEAR,
         .day_of_week = (BT_MESH_SCHEDULER_MON | BT_MESH_SCHEDULER_TUE |
                         BT_MESH_SCHEDULER_WED | BT_MESH_SCHEDULER_THU |
                         BT_MESH_SCHEDULER_FRI),
      };

      return bt_mesh_scheduler_cli_action_set_unack(&scheduler_cli, NULL, idx, &entry);
   }

   static void schedule_office_lights(void)
   {
      /* Turn lights on at 08:00 */
      (void)schedule_on_weekdays(BT_MESH_SCHEDULER_TURN_ON, 0, 8, 0);
      /* Turn lights off at 16:00 */
      (void)schedule_on_weekdays(BT_MESH_SCHEDULER_TURN_OFF, 1, 16, 0);
   }

Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/scheduler_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/scheduler_cli.c`

.. doxygengroup:: bt_mesh_scheduler_cli
   :project: nrf
   :members:
