.. _bt_mesh_dtt_srv_readme:

Generic Default Transition Time Server
######################################

The DTT Server provides a common way to specify the state transition time for other models on the same element.
If other generic models on the same element receive state change commands without transition parameters, they will use the default transition time specified by the DTT Server model.
This way, the DTT Server can define a consistent transition time for all states on their elements, without depending on client configurations.

Configuration
==============

The Generic DTT Server has one associated configuration option:

- :option:`CONFIG_BT_MESH_DTT_SRV_PERSISTENT`: Control whether changes to the Generic Default Transition Time are stored persistently.
  Note that this option is only available if :option:`CONFIG_BT_SETTINGS` is enabled.

States
=======

**Generic Default Transition Time**: ``s32_t``

The Default Transition Time can either be 0, a positive number of milliseconds, or ``K_FOREVER`` if the transition is undefined.
On the air, the transition time is encoded into a single byte, and loses some of its granularity:

- Step count: 6 bits (0-0x3e)
- Step resolution: 2 bits
  - Step 0: 100 millisecond resolution
  - Step 1: 1 second resolution
  - Step 2: 10 second resolution
  - Step 3: 10 minutes resolution

The state is encoded with the highest resolution available, and rounded to the nearest representation.
Values lower than 100 milliseconds, but higher than 0 are encoded as 100 milliseconds.
Values higher than the max value of 620 minutes are encoded as "undefined".

The DTT Server holds the memory for this state itself, and optionally notifies the user of any changes through :cpp:member:`bt_mesh_dtt_srv::update_handler`.
If the user changes the transition time manually, the change should be published using :cpp:func:`bt_mesh_dtt_srv_pub`.

Extended models
================

None.

Persistent storage
===================

The Generic Default Transition Time is stored persistently if :option:`CONFIG_BT_MESH_DTT_SRV_PERSISTENT` is enabled.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/gen_dtt_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_dtt_srv.c`

.. doxygengroup:: bt_mesh_dtt_srv
   :project: nrf
   :members:
