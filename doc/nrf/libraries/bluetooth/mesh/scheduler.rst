.. _bt_mesh_scheduler_readme:

Scheduler models
################

.. contents::
   :local:
   :depth: 2

The Scheduler models provide the ability to autonomically change the state of a device.
The states are changed based on the notion of the UTC time and the ISO 8601 calendar.
The UTC time is provided from the referenced :ref:`bt_mesh_time_srv_readme` model.

One state change is referred to as an *action*.
The models provide an action scheduler.
The action scheduler uses the Scheduler Register state to schedule up to sixteen action entries.

The Scheduler models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   scheduler_srv.rst
   scheduler_cli.rst

Common types
************

This section lists the types common to the Scheduler mesh models.

| Header file: :file:`include/bluetooth/mesh/scheduler.h`

.. doxygengroup:: bt_mesh_scheduler
   :project: nrf
   :members:
